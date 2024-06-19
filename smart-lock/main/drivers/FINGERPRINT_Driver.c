/// 校验和计算方式：文档第18页
/// 校验和是从包标识至校验和之间所有字节之和,包含包标识不包含校验和

#include "FINGERPRINT_Driver.h"
#include "AUDIO_Driver.h"

void FINGERPRINT_Init(void)
{
    /// 串口配置
    uart_config_t uart_config = {
        // 波特率是57.6Kbps
        .baud_rate = 57600,
        /// 数据位是8位
        .data_bits = UART_DATA_8_BITS,
        // 奇偶校验
        .parity = UART_PARITY_DISABLE,
        // 停止位
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    /// 使用的是串口1
    uart_driver_install(UART_NUM_1, BUFFER_SIZE * 2, 0, 0, NULL, 0);
    /// 让串口配置生效
    uart_param_config(UART_NUM_1, &uart_config);
    /// 配置串口1使用的引脚
    uart_set_pin(UART_NUM_1, FINGERPRINT_UART_TX, FINGERPRINT_UART_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    /// 触摸中断
    /// 默认情况下，指纹模块应该处于休眠模式
    /// 当有手指按上，触发中断，唤醒模块
    /// 中断类型：上升沿触发
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_POSEDGE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << FINGERPRINT_TOUCH_OUT);
    gpio_config(&io_conf);
}

/// 见文档：3.4.3 获取芯片唯一序列号PS_GetChipSN
void FINGERPRINT_GetChipSerialNumber(void)
{
    /// 上电之后等待200ms，指纹模块才能正常工作
    vTaskDelay(200 / portTICK_PERIOD_MS);
    /// 获取序列号的指令
    uint8_t command[13] = {
        0xEF, 0x01,             // 包头
        0xFF, 0xFF, 0xFF, 0xFF, // 默认设备地址
        0x01,                   // 包标识
        0x00, 0x04,             // 包长度
        0x34,                   // 指令码
        0x00,                   // 参数
        0x00, 0x39              // 校验和
    };

    /// 发送指令
    uart_write_bytes(UART_NUM_1, (const char *)command, 13);

    /// 读取串口发送过来的数据
    uint8_t data[64];
    int len = uart_read_bytes(UART_NUM_1, data, 64 - 1, 2000 / portTICK_PERIOD_MS);

    if (len)
    {
        /// 返回的响应的索引为6：包标识
        /// 索引为9：确认码
        ///     - 确认码 == 0x00: OK
        ///     - 确认码 == 0x01: 收包错误
        if (data[6] == 0x07 && data[9] == 0x00)
        {
            printf("finger module serial number: %.32s\r\n", &data[10]);
        }
    }
}

/// 进入休眠的指令
uint8_t FINGERPRINT_Sleep(void)
{
    /// 休眠指令
    uint8_t command[12] = {0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x03, 0x33, 0x00, 0x37};
    /// 发送休眠指令
    uart_write_bytes(UART_NUM_1, (const char *)command, 12);
    /// 读取串口发送过来的数据
    uint8_t data[64];
    int len = uart_read_bytes(UART_NUM_1, data, 64 - 1, 2000 / portTICK_PERIOD_MS);

    if (len)
    {
        if (data[6] == 0x07 && data[9] == 0x00)
        {
            printf("finger module sleep success\r\n");
            return 0;
        }
        else if (data[6] == 0x07 && data[9] == 0x01)
        {
            printf("finger module sleep failed\r\n");
            return 1;
        }
        else
        {
            return 1;
        }
    }
    else
    {
        return 1;
    }
}

void FINGERPRINT_ControlLED(uint8_t buffer[])
{
    /// 发送指纹模块LED指令
    uart_write_bytes(UART_NUM_1, (const char *)buffer, 16);
}

/// 文档3.5.7 LED控制灯指令PS_ControlBLN
void FINGERPRINT_ControlLEDC(uint8_t fun, uint8_t start, uint8_t end, uint8_t cycle)
{
    uint8_t command[16] = {
        0xEF, 0x01,             // 包头
        0xFF, 0xFF, 0xFF, 0xFF, // 默认地址
        0x01,                   // 包标识
        0x00, 0x07,             // 包长度
        0x3C,                   // 指令码
        '\0',                   // 功能码占位符
        '\0',                   // 起始颜色占位符
        '\0',                   // 结束颜色占位符
        '\0',                   // 循环次数占位符
        '\0', '\0'              // 校验和占位符
    };
    /// 校验和初始值 = 0x01 + 0x00 + 0x07 + 0x3C == 0x44
    uint16_t checksum = 0x44;
    command[10] = (fun); // 对指令码占位符进行赋值
    checksum += fun;
    command[11] = (start); // 对起始颜色占位符进行赋值
    checksum += start;
    command[12] = (end); // 对结束颜色占位符进行赋值
    checksum += end;
    command[13] = (cycle); // 对循环次数占位符进行赋值
    checksum += cycle;
    /// uint8_t array[2];
    /// 0x1234 ===> {0x12, 0x34}
    /// array[0] = 0x1234 >> 8;
    /// array[1] = 0x1234
    command[14] = checksum >> 8;
    command[15] = checksum;
    uart_write_bytes(UART_NUM_1, (const char *)command, 16);
}

/// 1. 获取图像
/// 2. 生成特征
/// 3. 合并模板
/// 4. 存储指纹模板
/// 文档3.3.2.2 自动注册模板PS_AutoEnroll
uint8_t *FINGERPRINT_AutoEnroll(uint16_t PageID, uint8_t entriesCount)
{
    uint8_t PS_AutoEnrollBuffer[17] = {0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x08, 0x31, '\0', '\0', '\0', 0x00, 0x17, '\0', '\0'};
    // 校验和预处理
    uint8_t PS_AutoEnrollBuffer_Check = PS_AutoEnrollBuffer[6] + PS_AutoEnrollBuffer[7] + PS_AutoEnrollBuffer[8] + PS_AutoEnrollBuffer[9] +
                                        PS_AutoEnrollBuffer[12] + PS_AutoEnrollBuffer[13] + PS_AutoEnrollBuffer[14];
    static uint8_t backData[3] = {0xFF, 0xFF, 0xFF};
    uint8_t eC = entriesCount > 12 ? 12 : entriesCount;
    PS_AutoEnrollBuffer[10] = (PageID >> 8);
    PS_AutoEnrollBuffer[11] = (PageID);
    PS_AutoEnrollBuffer[12] = (eC);
    PS_AutoEnrollBuffer[15] = (PS_AutoEnrollBuffer_Check + PS_AutoEnrollBuffer[10] + PS_AutoEnrollBuffer[11] + PS_AutoEnrollBuffer[12]) >> 8;
    PS_AutoEnrollBuffer[16] = (PS_AutoEnrollBuffer_Check + PS_AutoEnrollBuffer[10] + PS_AutoEnrollBuffer[11] + PS_AutoEnrollBuffer[12]);
    uart_write_bytes(UART_NUM_1, PS_AutoEnrollBuffer, 17);
    /// 读取串口发送过来的数据
    uint8_t data[64];
    /// 录入指纹需要等待10秒钟，10秒钟之类需要按压指纹模板entriesCount次
    int len = uart_read_bytes(UART_NUM_1, data, 64 - 1, 10000 / portTICK_PERIOD_MS);

    if (len)
    {
        if (data[6] == 0x07)
        {
            backData[0] = data[9];
            backData[1] = data[10];
            backData[2] = data[11];
            printf("error message: 0x%x, 0x%x, 0x%x\r\n", data[9], data[10], data[11]);
        }
    }
    return backData;
}

uint8_t FINGERPRINT_Enroll(uint16_t PageID, uint8_t entriesCount)
{
    uint8_t PS_BlueLEDBuffer[16] = {0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x07, 0x3C, 0x03, 0x01, 0x01, 0x00, 0x00, 0x49};    // 常开 蓝色
    uint8_t PS_GreenLEDBuffer[16] = {0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x07, 0x3C, 0x02, 0x02, 0x02, 0x02, 0x00, 0x4C};   // 闪烁2次 绿色
    uint8_t PS_RedLEDLOOPBuffer[16] = {0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x07, 0x3C, 0x02, 0x04, 0x04, 0x00, 0x00, 0x4E}; // 循环闪烁闪烁 红色
    uint8_t PS_OFFLEDBuffer[16] = {0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x07, 0x3C, 0x04, 0x00, 0x00, 0x00, 0x00, 0x48};     // 全灭
    // 点亮蓝灯，注册开始
    FINGERPRINT_ControlLED(PS_BlueLEDBuffer);
    vTaskDelay(10 / portTICK_PERIOD_MS);
    uint8_t *confirmationCode = FINGERPRINT_AutoEnroll(PageID, entriesCount);
    if (confirmationCode[0] == 0x00)
    {
        FINGERPRINT_ControlLED(PS_GreenLEDBuffer); // 绿灯闪烁，注册成功
        printf("finger registered success\r\n");
        return 0x00;
    }
    else
    {
        FINGERPRINT_ControlLED(PS_OFFLEDBuffer);
        return 0xFF;
    }
}

/// 由于自动指纹录入需要安全级别是0或者1，所以我们设置为0
void FINGERPRINT_SetSecurityLevel(void)
{
    /// 校验和是从包标识至校验和之间所有字节之和,包含包标识不包含校验和,超出 2 字节的进位忽略。
    uint8_t command[14] = {0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x05, 0x0E, 0x07, 0x00, 0x00, 0x1B};
    /// 发送休眠指令
    uart_write_bytes(UART_NUM_1, (const char *)command, 14);
    /// 读取串口发送过来的数据
    uint8_t data[64];
    int len = uart_read_bytes(UART_NUM_1, data, 64 - 1, 2000 / portTICK_PERIOD_MS);

    if (len)
    {
        if (data[6] == 0x07 && data[9] == 0x00)
        {
            printf("finger module set security success\r\n");
        }
    }
}

/**
 * @brief   读取有效模板个数，查询当前已注册指纹数量
 * @param   None
 * @return  应答包第11位有效数量或者无效值0xFF
 */
uint8_t FINGERPRINT_Inquiry(void)
{
    uint8_t PS_ValidTempleteNumBuffer[12] = {0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x03, 0x1D, 0x00, 0x21};
    uart_write_bytes(UART_NUM_1, PS_ValidTempleteNumBuffer, 12);
    /// 读取串口发送过来的数据
    uint8_t data[64];
    int len = uart_read_bytes(UART_NUM_1, data, 64 - 1, 2000 / portTICK_PERIOD_MS);

    if (len)
    {
        if (data[6] == 0x07 && data[9] == 0x00)
        {
            printf("registered finger templates number: %d\r\n", data[11]);
            return data[11];
        }
    }

    return 0;
}

/**
 * @brief   模块获取搜索指纹用的图像函数
 * @param   None
 * @return  应答包第9位确认码或者无效值0xFF
 */
static uint8_t getImage()
{
    uint8_t PS_GetImageBuffer[12] = {0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x03, 0x01, 0x00, 0x05};
    uart_write_bytes(UART_NUM_1, PS_GetImageBuffer, 12);
    /// 读取串口发送过来的数据
    uint8_t data[64];
    uart_read_bytes(UART_NUM_1, data, 64 - 1, 2000 / portTICK_PERIOD_MS);
    return data[6] == 0x07 ? data[9] : 0xFF;
}

/**
 * @brief   模块获取图像后生成特征，存储到缓冲区1
 * @param   None
 * @return  应答包第9位确认码或者无效值0xFF
 */
static uint8_t getChar()
{
    uint8_t PS_GetCharBuffer[13] = {0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x04, 0x02, 0x01, 0x00, 0x08};
    uart_write_bytes(UART_NUM_1, PS_GetCharBuffer, 13);
    /// 读取串口发送过来的数据
    uint8_t data[64];
    uart_read_bytes(UART_NUM_1, data, 64 - 1, 2000 / portTICK_PERIOD_MS);
    return data[6] == 0x07 ? data[9] : 0xFF;
}

/**
 * @brief   搜索指纹模板函数
 * @param   None
 * @return  应答包第9位确认码或者无效值0xFF
 */
static uint8_t searchMB()
{
    uint8_t PS_SearchMBBuffer[17] = {0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x08, 0x04, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0x02, 0x0C};
    uart_write_bytes(UART_NUM_1, PS_SearchMBBuffer, 17);
    /// 读取串口发送过来的数据
    uint8_t data[64];
    uart_read_bytes(UART_NUM_1, data, 64 - 1, 2000 / portTICK_PERIOD_MS);
    return data[6] == 0x07 ? data[9] : 0xFF;
}

/**
 * @brief   分步式命令搜索指纹函数
 * @param   None
 * @return  应答包第9位确认码或者无效值0xFF
 */
uint8_t FINGERPRINT_Identify(void)
{
    if (getImage() == 0x00)
    {
        if (getChar() == 0x00)
        {
            uint8_t sMB = searchMB();
            // Serial.println(sMB);
            if (sMB == 0x00)
            {
                printf("finger identify success\r\n");
                return 0;
            }
            else
            {
                printf("finger identify failed！\r\n");
                return 1;
            }
        }
        else
        {
            return 1;
        }
    }
    else if (getImage() == 0x02)
    {
        printf("get image failed！\r\n");
        return 1;
    }
    else
    {
        return 1;
    }
}

void FINGERPRINT_deleteAllTemplates(void)
{
    uint8_t PS_ValidTempleteNumBuffer[12] = {0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x03, 0x0D, 0x00, 0x11};
    uart_write_bytes(UART_NUM_1, PS_ValidTempleteNumBuffer, 12);
    /// 读取串口发送过来的数据
    uint8_t data[64];
    int len = uart_read_bytes(UART_NUM_1, data, 64 - 1, 2000 / portTICK_PERIOD_MS);

    if (len)
    {
        if (data[6] == 0x07 && data[9] == 0x00)
        {
            printf("delete all templates success.\r\n");
        }
    }
}

uint8_t FINGERPRINT_HandShake(void)
{
    uint8_t PS_ValidTempleteNumBuffer[12] = {0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x03, 0x35, 0x00, 0x39};
    uart_write_bytes(UART_NUM_1, PS_ValidTempleteNumBuffer, 12);
    /// 读取串口发送过来的数据
    uint8_t data[64];
    int len = uart_read_bytes(UART_NUM_1, data, 64 - 1, 2000 / portTICK_PERIOD_MS);

    if (len)
    {
        if (data[6] == 0x07 && data[9] == 0x00)
        {
            return 0;
        }
        else
        {
            return 1;
        }
    }
    else
    {
        return 1;
    }
}

/// 取消注册任务
void FINGERPRINT_Cancel(void)
{
    uint8_t PS_ValidTempleteNumBuffer[12] = {0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x03, 0x30, 0x00, 0x34};
    uart_write_bytes(UART_NUM_1, PS_ValidTempleteNumBuffer, 12);
    /// 读取串口发送过来的数据
    uint8_t data[64];
    int len = uart_read_bytes(UART_NUM_1, data, 64 - 1, 2000 / portTICK_PERIOD_MS);

    if (len)
    {
        if (data[6] == 0x07 && data[9] == 0x00)
        {
            printf("cancel success\r\n");
        }
        else
        {
            printf("cancel failed\r\n");
        }
    }
    else
    {
        printf("cancel failed\r\n");
    }
}