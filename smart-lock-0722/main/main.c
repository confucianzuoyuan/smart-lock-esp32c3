#include <stdint.h>
#include <stdio.h>

#include "Dri/AUDIO_Driver.h"
#include "Dri/BT_Driver.h"
#include "Dri/FINGER_Driver.h"
#include "Dri/KEYBOARD_Driver.h"
#include "Dri/LED_Driver.h"
#include "Dri/MOTOR_Driver.h"
#include "Dri/OTA_Driver.h"
#include "Dri/PASSWORD_Driver.h"
#include "Dri/WIFI_Driver.h"

#include "nvs_flash.h"

#include "Dri/Utils.h"
#include "Utils.h"
#include "driver/gpio.h"

/// RTOS通用API
#include "freertos/FreeRTOS.h"
/// RTOS任务相关API
#include "freertos/task.h"
/// RTOS队列相关API
#include "freertos/queue.h"

/// 声明一个队列
static QueueHandle_t gpio_event_queue = NULL;

/// 是否进入了录入指纹的任务
static uint8_t is_enroll_finger = 0;
/// 是否进入ota任务
static uint8_t is_ota_start = 0;
/// 是否有手指按在指纹模块上
static uint8_t is_finger_on = 0;
/// 按键次数
static uint8_t key_count = 0;
/// 保存按键值的数组
static uint8_t key_array[6] = {0};
/// 处理GPIO中断的回调函数
static void xxxxyyyy(void *arg) {
  uint32_t gpio_num = (uint32_t)arg;
  // 如果是指纹模块产生的中断
  // 说明有手指放在了指纹模块上面，
  // 那么不是录入指纹的任务就是验证指纹的任务
  // 为了防止整个过程被打断，将中断全部删除
  if (gpio_num == FINGER_INT) {
    is_finger_on = 1;
    gpio_isr_handler_remove(KEYBOARD_INT);
    gpio_isr_handler_remove(FINGER_INT);
  }
  xQueueSendFromISR(gpio_event_queue, &gpio_num, NULL);
}

/// freertos任务，轮询队列，处理gpio事件
static void gpio_isr_handler(void *arg) {
  uint32_t gpio_num;
  while (1) {
    if (xQueueReceive(gpio_event_queue, &gpio_num, portMAX_DELAY)) {
      if (gpio_num == KEYBOARD_INT) {
        uint16_t key = KEYBOARD_ReadKey();
        printf("key: %d\r\n", key);
        if (key < 10) {
          LED_Light(key);
        } else if (key == '#') {
          LED_Light(10);
        } else if (key == 'M') {
          LED_Light(11);
        }
        AUDIO_Play(11);

        key_array[key_count++] = key;
        /// 录入指纹任务
        if (key_count >= 3 && key_array[key_count - 1] == '#' &&
            key_array[key_count - 2] == '#' &&
            key_array[key_count - 3] == '#') {
          is_enroll_finger = 1;
          key_count = 0;
        }

        /// ota任务
        if (key_count >= 3 && key_array[key_count - 1] == 'M' &&
            key_array[key_count - 2] == 'M' &&
            key_array[key_count - 3] == 'M') {
          is_ota_start = 1;
          key_count = 0;
        }

        // 验证永久性密码
        if (key_count == 6) {
          int password = key_array[0] * 100000 + key_array[1] * 10000 +
                         key_array[2] * 1000 + key_array[3] * 100 +
                         key_array[4] * 10 + key_array[5] * 1;
          // 将密码存储到nvs flash中
          nvs_handle_t my_handle;
          // 创建一个叫做`smart-lock`的命名空间
          nvs_open("smart-lock", NVS_READWRITE, &my_handle);
          // 通过key：password查询flash中保存的密码
          int32_t password_from_flash;
          nvs_get_i32(my_handle, "password", &password_from_flash);
          // 关闭命名空间
          nvs_close(my_handle);

          if (password == password_from_flash) {
            MOTOR_OpenLock();
          }
        }

        // 验证临时密码
        if (key_count == 6) {
          int temp_password = key_array[0] * 100000 + key_array[1] * 10000 +
                              key_array[2] * 1000 + key_array[3] * 100 +
                              key_array[4] * 10 + key_array[5] * 1;
          if (!PASSWORD_ValidateTempPassword(temp_password)) {
            MOTOR_OpenLock();
          }
        }

        // 防御式编程，防止数组越界访问。
        if (key_count == 6) {
          key_count = 0;
        }
      } else if (gpio_num == FINGER_INT) {
        printf("指纹模块中断触发了。\r\n");
      }
    }
    DelayMs(100);
  }
}

/// 重置指纹模块
void reset_finger_module(void) {
  is_enroll_finger = 0;
  is_finger_on = 0;
  while (FINGER_Sleep()) {
    DelayMs(1);
  }
  gpio_isr_handler_add(KEYBOARD_INT, xxxxyyyy, (void *)KEYBOARD_INT);
  gpio_isr_handler_add(FINGER_INT, xxxxyyyy, (void *)FINGER_INT);
}

/// 录入指纹的任务
void enroll_finger_task(void *arg) {
  while (1) {
    /// 按了3次#键
    if (is_enroll_finger) {
      printf("请放置手指。\r\n");
      while (!is_finger_on) {
        DelayMs(1);
      }
      printf("开始录入指纹。\r\n");
      uint8_t num = FINGER_GetTemplatesNumber();
      if (FINGER_Enroll(num + 1)) {
        printf("录入指纹失败。\r\n");
      } else {
        printf("录入指纹成功。\r\n");
      }

      // 清理现场
      reset_finger_module();
    }
    DelayMs(1000);
  }
}

/// 识别指纹的任务
void identify_finger_task(void *arg) {
  while (1) {
    /// 不是录入指纹任务，且有手指按在指纹模块上。
    if (!is_enroll_finger && is_finger_on) {
      printf("开始识别指纹。\r\n");
      if (FINGER_Identify()) {
        printf("识别指纹失败。\r\n");
      } else {
        printf("识别指纹成功。\r\n");
        MOTOR_OpenLock();
      }

      reset_finger_module();
    }
    DelayMs(100);
  }
}

/// ota任务
void ota(void *arg) {
  while (1) {
    if (is_ota_start) {
      is_ota_start = 0;
      ota_task();
    }
    DelayMs(5000);
  }
}

void app_main(void) {
  // 初始化键盘相关GPIO
  KEYBOARD_Init();
  // 初始化电机
  MOTOR_Init();
  // 初始化语音模块
  AUDIO_Init();
  // 初始化LED灯模块
  LED_Init();
  // 初始化指纹模块
  FINGER_Init();

  esp_err_t ret;

  // Initialize NVS.
  // 初始化esp32自带的flash的nvs分区
  ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }

  BT_Init();
  WIFI_Init();

  // 创建10个元素的队列
  gpio_event_queue = xQueueCreate(10, sizeof(uint32_t));

  // 按照默认配置启用中断服务
  gpio_install_isr_service(0);
  // KEYBOARD_INT中断的处理函数是：xxxxyyyy
  gpio_isr_handler_add(KEYBOARD_INT, xxxxyyyy, (void *)KEYBOARD_INT);
  // FINGER_INT中断的处理函数是：xxxxyyyy
  gpio_isr_handler_add(FINGER_INT, xxxxyyyy, (void *)FINGER_INT);

  // 处理gpio中断，轮询gpio_event_queue队列，然后进行处理
  xTaskCreate(gpio_isr_handler, "gpio_isr_handler", 2048, NULL, 10, NULL);
  // 验证指纹的任务
  xTaskCreate(identify_finger_task, "identify_finger_task", 2048, NULL, 10,
              NULL);
  // 注册指纹的任务
  xTaskCreate(enroll_finger_task, "enroll_finger_task", 2048, NULL, 10, NULL);
  xTaskCreate(ota, "ota_task", 8192, NULL, 10, NULL);
}
