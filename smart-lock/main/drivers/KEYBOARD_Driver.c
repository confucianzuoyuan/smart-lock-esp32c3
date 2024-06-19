#include "KEYBOARD_Driver.h"

// 毫秒级延时
void Delay_ms(uint8_t time)
{
    vTaskDelay(time / portTICK_PERIOD_MS);
}

void I2C_Start(void)
{
    I2C_SDA_OUT;
    I2C_SDA_HIGH;
    I2C_SCL_HIGH;
    Delay_ms(1);
    I2C_SDA_LOW;
    Delay_ms(1);
    I2C_SCL_LOW;
    Delay_ms(1);
}

void I2C_Stop(void)
{
    I2C_SCL_LOW;
    I2C_SDA_OUT;
    I2C_SDA_LOW;
    Delay_ms(1);
    I2C_SCL_HIGH;
    Delay_ms(1);
    I2C_SDA_HIGH;
}

/// 主机 ---> 从机：发送应答信号
void I2C_Ack(uint8_t ack)
{
    I2C_SCL_LOW;
    I2C_SDA_OUT;
    if (ack)
    {
        I2C_SDA_HIGH;
    }
    else
    {
        I2C_SDA_LOW;
    }
    Delay_ms(1);
    I2C_SCL_HIGH;
    Delay_ms(1);
    I2C_SCL_LOW;
}

/// 主机 <---- 从机: 等待应答信号
void I2C_Wait_Ack(void)
{
    I2C_SCL_LOW;
    I2C_SDA_IN;
    Delay_ms(1);
    I2C_SCL_HIGH;
    Delay_ms(1);
    while (I2C_READ_SDA)
    {
    }
    I2C_SCL_LOW;
}

// 发送1个字节
void I2C_SendByte(uint8_t byte)
{
    I2C_SDA_OUT;
    for (int i = 0; i < 8; i++)
    {
        I2C_SCL_LOW;
        Delay_ms(1);
        if (byte & 0b10000000)
        {
            I2C_SDA_HIGH;
        }
        else
        {
            I2C_SDA_LOW;
        }
        Delay_ms(1);
        I2C_SCL_HIGH;
        Delay_ms(1);
        byte <<= 1;
    }
}

uint8_t I2C_ReadByte(uint8_t ack)
{
    uint8_t received_byte = 0;
    I2C_SDA_IN;
    for (int i = 0; i < 8; i++)
    {
        I2C_SCL_LOW;
        Delay_ms(1);
        I2C_SCL_HIGH;
        received_byte <<= 1;
        if (I2C_READ_SDA)
        {
            received_byte += 1;
        }
        Delay_ms(1);
    }
    I2C_Ack(ack);
    return received_byte;
}

/// 从电容键盘的寄存器中读取按键数值
uint8_t I2C_Read_Key(void)
{
    uint8_t i = 0;
    uint8_t key = 0;
    I2C_Start();
    // 从机地址是0x40, 0x01表示读取
    I2C_SendByte((0x40 << 1) | 0x01);
    // 等待应答
    I2C_Wait_Ack();
    I2C_SDA_IN;
    for (i = 1; i < 9; i++)
    {
        I2C_SCL_LOW;
        Delay_ms(1);
        I2C_SCL_HIGH;
        /// 如果还没有读取到按键值，且SDA为高电平
        if (!key && I2C_READ_SDA)
        {
            /// i就是读取到的按键值
            key = i;
        }
        Delay_ms(1);
    }
    if (key)
    {
        I2C_Ack(1);
        I2C_Stop();
        return key;
    }
    I2C_Ack(0);
    I2C_SDA_IN;
    for (i = 9; i < 17; i++)
    {
        I2C_SCL_LOW;
        Delay_ms(1);
        I2C_SCL_HIGH;
        /// 如果还没有读取到按键值，且SDA为高电平
        if (!key && I2C_READ_SDA)
        {
            /// i就是读取到的按键值
            key = i;
        }
        Delay_ms(1);
    }
    I2C_Ack(1);
    I2C_Stop();
    return key;
}

uint8_t KEYBOARD_read_key(void)
{
    uint16_t key = I2C_Read_Key();
    if (key == 2)
        return 1;
    else if (key == 3)
        return 2;
    else if (key == 4)
        return 3;
    else if (key == 8)
        return 4;
    else if (key == 6)
        return 5;
    else if (key == 7)
        return 6;
    else if (key == 5)
        return 7;
    else if (key == 10)
        return 8;
    else if (key == 11)
        return 9;
    else if (key == 1)
        return 0;
    else if (key == 12)
        return '#';
    else if (key == 9)
        return 'M';
    return 255;
}

void KEYBOARD_Init(void)
{
    /// 初始化GPIO引脚
    gpio_config_t io_conf;
    /// 禁用中断
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 1;
    // 1ULL ---> unsigned long long 1
    // pin_bit_mask: 0b0000
    // 0b0110
    io_conf.pin_bit_mask = ((1ULL << I2C_SCL) | (1ULL << I2C_SDA));
    gpio_config(&io_conf);

    /// 中断类型：上升沿触发
    io_conf.intr_type = GPIO_INTR_POSEDGE;
    io_conf.mode = GPIO_MODE_INPUT;
    // 0b0001
    io_conf.pin_bit_mask = (1ULL << KEYBOARD_INT);
    gpio_config(&io_conf);
}