#include "AUDIO_Driver.h"

void AUDIO_Send(uint8_t DDATA)
{
    uint8_t S_DATA;
    uint8_t B_DATA;
    S_DATA = DDATA;
    AUDIO_SDA_LOW;
    // 关键延时
    DELAY_MS(10);
    B_DATA = S_DATA & 0x01;
    /// 发送1个字节的数据
    for (int i = 0; i < 8; i++)
    {
        if (B_DATA == 1)
        {
            AUDIO_SDA_HIGH;
            DELAY_US(600);
            AUDIO_SDA_LOW;
            DELAY_US(200);
        }
        else
        {
            AUDIO_SDA_HIGH;
            DELAY_US(200);
            AUDIO_SDA_LOW;
            DELAY_US(600);
        }
        S_DATA = S_DATA >> 1;
        B_DATA = S_DATA & 0x01;
    }
    /// 比较关键
    AUDIO_SDA_HIGH;
    DELAY_MS(2);
}

void AUDIO_Test(void)
{
    AUDIO_Send(85);
    DELAY_MS(2000);
    AUDIO_Send(86);
    DELAY_MS(2000);
    AUDIO_Send(69);
    DELAY_MS(2000);
}

void AUDIO_Init(void)
{
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << AUDIO_SDA);
    gpio_config(&io_conf);
}