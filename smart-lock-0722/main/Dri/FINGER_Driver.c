#include "FINGER_Driver.h"
#include "Utils.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "hal/gpio_types.h"
#include "hal/uart_types.h"
#include "portmacro.h"
#include <stdint.h>

#define FINGER_TX 21
#define FINGER_RX 20

void FINGER_Init(void) {
  uart_config_t conf = {};
  conf.baud_rate = 57600;
  conf.data_bits = UART_DATA_8_BITS;
  conf.stop_bits = UART_STOP_BITS_1;
  conf.parity = UART_PARITY_DISABLE;
  conf.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
  conf.source_clk = UART_SCLK_DEFAULT;

  uart_driver_install(UART_NUM_1, 2048, 0, 0, NULL, 0);
  uart_param_config(UART_NUM_1, &conf);
  uart_set_pin(UART_NUM_1, FINGER_TX, FINGER_RX, UART_PIN_NO_CHANGE,
               UART_PIN_NO_CHANGE);

  gpio_config_t io_conf = {};
  io_conf.intr_type = GPIO_INTR_POSEDGE;
  io_conf.mode = GPIO_MODE_INPUT;
  io_conf.pin_bit_mask = (1ULL << FINGER_INT);
  gpio_config(&io_conf);

  // 上电后200ms启动时间
  DelayMs(300);

  // 读取指纹模块的序列号
  FINGER_ReadSerialNumber();
  // 获取指纹模块中保存的指纹数量
  FINGER_GetTemplatesNumber();
  // 将指纹模块安全等级设置为0
  FINGER_SetSecurityToZero();
  // 指纹模块进入休眠
  FINGER_Sleep();
}

void FINGER_ReadSerialNumber(void) {
  // 获取芯片唯一序列号指令
  uint8_t cmd[13] = {
      0xEF, 0x01,             // 包头
      0xFF, 0xFF, 0xFF, 0xFF, // 设备地址
      0x01,                   // 包标识
      0x00, 0x04,             // 包长度
      0x34,                   // 指令码
      0x00,                   // 参数
      0x00, 0x39,             // 校验和
  };

  uart_write_bytes(UART_NUM_1, cmd, 13);

  uint8_t recv_data[64] = {0};

  // 不管接收到多少数据，都要执行100ms
  int len =
      uart_read_bytes(UART_NUM_1, recv_data, 2048, 100 / portTICK_PERIOD_MS);

  if (len > 0 && recv_data[6] == 0x07) {
    if (recv_data[9] == 0x00) {
      printf("指纹模块序列号：%.32s\r\n", &recv_data[10]);
    }
  } else {
    printf("获取指纹序列号失败\r\n");
  }
}

uint8_t FINGER_Sleep(void) {
  // 指纹休眠指令
  uint8_t cmd[12] = {
      0xEF, 0x01,             // 包头
      0xFF, 0xFF, 0xFF, 0xFF, // 设备地址
      0x01,                   // 包标识
      0x00, 0x03,             // 包长度
      0x33,                   // 指令码
      0x00, 0x37,             // 校验和
  };

  uart_write_bytes(UART_NUM_1, cmd, 12);

  uint8_t recv_data[64] = {0};

  int len =
      uart_read_bytes(UART_NUM_1, recv_data, 2048, 100 / portTICK_PERIOD_MS);

  if (len > 0 && recv_data[6] == 0x07) {
    if (recv_data[9] == 0x00) {
      printf("指纹模块休眠成功。\r\n");
      return 0;
    }
  }

  printf("指纹模块休眠失败。\r\n");
  return 1;
}

uint8_t FINGER_GetTemplatesNumber(void) {
  // 获取已经录入的指纹数量
  uint8_t cmd[12] = {
      0xEF, 0x01,             // 包头
      0xFF, 0xFF, 0xFF, 0xFF, // 设备地址
      0x01,                   // 包标识
      0x00, 0x03,             // 包长度
      0x1D,                   // 指令码
      0x00, 0x21,             // 校验和
  };

  uart_write_bytes(UART_NUM_1, cmd, 12);

  uint8_t recv_data[64] = {0};

  int len =
      uart_read_bytes(UART_NUM_1, recv_data, 2048, 100 / portTICK_PERIOD_MS);

  if (len > 0 && recv_data[6] == 0x07) {
    if (recv_data[9] == 0x00) {
      printf("指纹数量：%d\r\n", recv_data[11]);
      return recv_data[11];
    }
  }

  return 0xFF;
}

uint8_t FINGER_GetImage(void) {
  // 获取指纹图像
  uint8_t cmd[12] = {
      0xEF, 0x01,             // 包头
      0xFF, 0xFF, 0xFF, 0xFF, // 设备地址
      0x01,                   // 包标识
      0x00, 0x03,             // 包长度
      0x01,                   // 指令码
      0x00, 0x05,             // 校验和
  };

  uart_write_bytes(UART_NUM_1, cmd, 12);

  uint8_t recv_data[64] = {0};

  int len =
      uart_read_bytes(UART_NUM_1, recv_data, 2048, 100 / portTICK_PERIOD_MS);

  if (len > 0 && recv_data[6] == 0x07) {
    if (recv_data[9] == 0x00) {
      printf("获取指纹图像成功\r\n");
      return 0;
    } else if (recv_data[9] == 0x01) {
      printf("收包错误。\r\n");
      return 1;
    } else if (recv_data[9] == 0x02) {
      printf("传感器上无手指。\r\n");
      return 1;
    }
  }

  return 1;
}

uint8_t FINGER_GenChar(uint8_t BufferID) {
  // 生成特征
  uint8_t cmd[13] = {
      0xEF, 0x01,             // 包头
      0xFF, 0xFF, 0xFF, 0xFF, // 设备地址
      0x01,                   // 包标识
      0x00, 0x04,             // 包长度
      0x02,                   // 指令码
      '\0',                   // 缓冲区号BufferID
      '\0', '\0',             // 校验和
  };

  cmd[10] = BufferID;
  uint16_t sum = cmd[6] + cmd[7] + cmd[8] + cmd[9] + cmd[10];
  cmd[11] = sum >> 8;
  cmd[12] = sum;

  uart_write_bytes(UART_NUM_1, cmd, 13);

  uint8_t recv_data[64] = {0};

  // 延时设置为500ms，为指纹模块提取特征提供充足的时间
  int len =
      uart_read_bytes(UART_NUM_1, recv_data, 2048, 500 / portTICK_PERIOD_MS);

  if (len > 0 && recv_data[6] == 0x07) {
    if (recv_data[9] == 0x00) {
      printf("生成特征成功\r\n");
      return 0;
    } else if (recv_data[9] == 0x01) {
      printf("收包错误。\r\n");
      return 1;
    } else if (recv_data[9] == 0x06) {
      printf("指纹图像太乱而生不成特征。\r\n");
      return 1;
    } else if (recv_data[9] == 0x07) {
      printf("指纹图像正常，但特征点太少而生不成特征\r\n");
      return 1;
    }
  }

  return 1;
}

uint8_t FINGER_Search(void) {
  // 全局搜索
  uint8_t cmd[17] = {
      0xEF, 0x01,             // 包头
      0xFF, 0xFF, 0xFF, 0xFF, // 设备地址
      0x01,                   // 包标识
      0x00, 0x08,             // 包长度
      0x04,                   // 指令码
      0x01,                   // 缓冲区号BufferID，默认是1
      0x00, 0x00,             // StartPage
      0xFF, 0xFF,             // PageNum
      0x02, 0x0c,             // 校验和
  };

  uart_write_bytes(UART_NUM_1, cmd, 17);

  uint8_t recv_data[64] = {0};

  // 设置延时为500ms，为搜索指纹提供充足的时间
  int len =
      uart_read_bytes(UART_NUM_1, recv_data, 2048, 500 / portTICK_PERIOD_MS);

  if (len > 0 && recv_data[6] == 0x07) {
    if (recv_data[9] == 0x00) {
      printf("搜索成功。\r\n");
      return 0;
    } else if (recv_data[9] == 0x01) {
      printf("收包错误。\r\n");
      return 1;
    } else if (recv_data[9] == 0x09) {
      printf("没搜索到；此时页码与得分为 0。\r\n");
      return 1;
    } else if (recv_data[9] == 0x17) {
      printf("残留指纹或两次采集之间手指没有移动过\r\n");
      return 1;
    }
  }

  return 1;
}

uint8_t FINGER_RegModel(void) {
  // 合并特征
  uint8_t cmd[12] = {
      0xEF, 0x01,             // 包头
      0xFF, 0xFF, 0xFF, 0xFF, // 设备地址
      0x01,                   // 包标识
      0x00, 0x03,             // 包长度
      0x05,                   // 指令码
      0x00, 0x09,             // 校验和
  };

  uart_write_bytes(UART_NUM_1, cmd, 12);

  uint8_t recv_data[64] = {0};

  // 设置延时为500ms，为合并特征提供时间
  int len =
      uart_read_bytes(UART_NUM_1, recv_data, 2048, 500 / portTICK_PERIOD_MS);

  if (len > 0 && recv_data[6] == 0x07) {
    if (recv_data[9] == 0x00) {
      printf("合并成功。\r\n");
      return 0;
    } else if (recv_data[9] == 0x01) {
      printf("收包错误。\r\n");
      return 1;
    } else if (recv_data[9] == 0x0A) {
      printf("合并失败。\r\n");
      return 1;
    }
  }

  return 1;
}

uint8_t FINGER_StoreChar(uint8_t PageID) {
  // 储存模板
  uint8_t cmd[15] = {
      0xEF, 0x01,             // 包头
      0xFF, 0xFF, 0xFF, 0xFF, // 设备地址
      0x01,                   // 包标识
      0x00, 0x06,             // 包长度
      0x06,                   // 指令码
      0x01,                   // BufferID
      '\0', '\0',             // PageID
      '\0', '\0',             // 校验和
  };

  cmd[11] = 0;
  cmd[12] = PageID;
  uint16_t sum =
      cmd[6] + cmd[7] + cmd[8] + cmd[9] + cmd[10] + cmd[11] + cmd[12];

  cmd[13] = sum >> 8;
  cmd[14] = sum;

  uart_write_bytes(UART_NUM_1, cmd, 15);

  uint8_t recv_data[64] = {0};

  // 设置延时为500ms，为存储特征提供时间
  int len =
      uart_read_bytes(UART_NUM_1, recv_data, 2048, 500 / portTICK_PERIOD_MS);

  if (len > 0 && recv_data[6] == 0x07) {
    if (recv_data[9] == 0x00) {
      printf("储存成功。\r\n");
      return 0;
    } else if (recv_data[9] == 0x01) {
      printf("收包错误。\r\n");
    } else if (recv_data[9] == 0x0B) {
      printf("PageID 超出指纹库范围。\r\n");
    } else if (recv_data[9] == 0x18) {
      printf("写 FLASH 出错；。\r\n");
    } else if (recv_data[9] == 0x31) {
      printf("功能与加密等级不匹配。\r\n");
    }
  }

  return 1;
}

void FINGER_SetSecurityToZero(void) {
  // 储存模板
  uint8_t cmd[14] = {
      0xEF, 0x01,             // 包头
      0xFF, 0xFF, 0xFF, 0xFF, // 设备地址
      0x01,                   // 包标识
      0x00, 0x05,             // 包长度
      0x0E,                   // 指令码
      0x07,                   // 寄存器序号
      0x00,                   // 内容
      0x00, 0x1B,             // 校验和
  };

  uart_write_bytes(UART_NUM_1, cmd, 14);

  uint8_t recv_data[64] = {0};

  int len =
      uart_read_bytes(UART_NUM_1, recv_data, 2048, 100 / portTICK_PERIOD_MS);

  if (len > 0 && recv_data[6] == 0x07) {
    if (recv_data[9] == 0x00) {
      printf("设置加密等级为0成功。\r\n");
    } else if (recv_data[9] == 0x01) {
      printf("收包错误。\r\n");
    }
  }
}

uint8_t FINGER_Enroll(uint8_t PageID) {
  int n = 1;
SendGetImageCmd:
  if (FINGER_GetImage())
    goto SendGetImageCmd;
  printf("第 %d 次获取图像成功\r\n", n);
  // 注意：这里使用的BufferID是n。
  if (FINGER_GenChar(n))
    goto SendGetImageCmd;
  printf("第 %d 次生成特征成功\r\n", n);

  if (n < 4) {
    n++;
    goto SendGetImageCmd;
  }

  if (FINGER_RegModel())
    goto ERROR;
  if (FINGER_StoreChar(PageID))
    goto ERROR;

  return 0;

ERROR:
  return 1;
}

uint8_t FINGER_Identify(void) {
SendGetImageCmd:
  if (FINGER_GetImage())
    goto SendGetImageCmd;
  if (FINGER_GenChar(1))
    goto SendGetImageCmd;
  if (FINGER_Search())
    return 1;
  return 0;
}