#ifndef __FINGERPRINT_H_
#define __FINGERPRINT_H_

#include "driver/uart.h"
#include "driver/gpio.h"

#define FINGERPRINT_UART_TX GPIO_NUM_21
#define FINGERPRINT_UART_RX GPIO_NUM_20

#define FINGERPRINT_TOUCH_OUT GPIO_NUM_10

#define BUFFER_SIZE 1024

void FINGERPRINT_Init(void);
/// 获取指纹模块唯一序列号的函数
void FINGERPRINT_GetChipSerialNumber(void);
/// 休眠指令
uint8_t FINGERPRINT_Sleep(void);
/// 模块LED灯控制函数
void FINGERPRINT_ControlLED(uint8_t buffer[]);
/**
 * @brief   模块LED灯用户自由控制控制函数
 * @param   fun：LED 灯模式控制位，1-普通呼吸灯，2-闪烁灯，3-常开灯，4-常闭灯，5-渐开灯，6-渐闭灯，其他功能码不适用于此指令包格式；
 * @param   start：设置为普通呼吸灯时，由灭到亮的颜色，只限于普通呼吸灯（功能码 01）功能，其他功能时，与结束颜色保持一致。
 *                 其中，bit0 是蓝灯控制位；bit1 是绿灯控制位；bit2 是红灯控制位。置 1 灯亮，置 0 灯灭。
 *                 例如 0x01_蓝灯亮，0x02_绿灯亮，0x04_红灯亮，0x06_红绿灯亮，0x05_红蓝灯亮，0x03_绿蓝灯亮，0x07_红绿蓝灯亮，0x00_全灭；
 * @param   end：结束颜色：设置为普通呼吸灯时，由亮到灭的颜色，只限于普通呼吸灯（功能码 0x01），其他功能时，与起始颜色保持一致。设置方式与起始颜色一样；
 * @param   cycle：循环次数：表示呼吸或者闪烁灯的次数。当设为 0 时，表示无限循环，当设为其他值时，表示呼吸有限次数。
 *                 循环次数适用于呼吸、闪烁功能，其他功能中无效，例如在常开、常闭、渐开和渐闭中是无效的；
 * @return  None
 */
void FINGERPRINT_ControlLEDC(uint8_t fun, uint8_t start, uint8_t end, uint8_t cycle);

/**
 * @brief   自动注册指纹模板函数, 默认采集4次
 * @param   PageID：注册指纹的ID号，取值0 - 49（FPM383F）
 * @param   entriesCount：录入（拼接）次数，取值1~12，推荐4~6
 * @return  应答包第9位确认码或者无效值0xFF
 */
uint8_t *FINGERPRINT_AutoEnroll(uint16_t PageID, uint8_t entriesCount);

/**
 * @brief   二次封装自动注册指纹函数，实现注册成功闪烁两次绿灯，失败闪烁两次红灯
 * @param   PageID：注册指纹的ID号，取值0 - 49（FPM383F）
 * @return  应答包第9位确认码或者无效值0xFF
 */
uint8_t FINGERPRINT_Enroll(uint16_t PageID, uint8_t entriesCount);

void FINGERPRINT_SetSecurityLevel(void);

/// 查询已存在模板数量
uint8_t FINGERPRINT_Inquiry(void);

uint8_t FINGERPRINT_Identify(void);

/// 删除所有指纹模板
void FINGERPRINT_deleteAllTemplates(void);

uint8_t FINGERPRINT_HandShake(void);

void FINGERPRINT_Cancel(void);

#endif