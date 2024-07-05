// 整数类型
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include <inttypes.h>
// esp32的GPIO驱动
#include "driver/gpio.h"
/// 引入键盘驱动
#include "drivers/AUDIO_Driver.h"
#include "drivers/BLUETOOTH_Driver.c"
#include "drivers/FINGERPRINT_Driver.h"
#include "drivers/KEYBOARD_Driver.h"
#include "drivers/LED_Driver.h"
#include "drivers/MOTOR_Driver.h"
#include "WIFI_Driver.h"
#include "OTA_Driver.h"
#include "MQTT_Driver.h"
#include "SNTP_Driver.h"
#include "PASSWORD_Driver.h"

/// 智能门锁序列号
static char *serial_number = "AtguiguSmartLock001";

/// 是否开始ota任务
static uint8_t start_ota = 0;

/// 录入指纹标志位
uint8_t fingerprint_enroll_flag = 0xFF;
uint8_t fingerprint_enroll_success = 0xFF;
/// 是否有手指放在指纹模块上
static uint8_t fingerprint_is_up = 0;
/// 已存在指纹模板数量
static uint8_t fingerprint_templates_number = 0;

/// 存储密码
static uint8_t password_array[6] = {0};
static uint8_t password_input_count = 0;

/// wifi相关标志位
uint8_t wifi_init_flag = 0xFF;
uint8_t wifi_ssid[32] = {0};
uint8_t wifi_password[64] = {0};
uint8_t wifi_init_success_flag = 0xFF;

// 用来保存中断事件的队列
static QueueHandle_t gpio_event_queue = NULL;

/// 处理GPIO引脚中断的回调函数
static void IRAM_ATTR gpio_interrupt_handler(void *arg)
{
  /// 读取产生中断的gpio引脚号
  uint32_t gpio_num = (uint32_t)arg;
  /// 如果中断来自指纹模块，删除指纹模块GPIO中断的处理
  if (gpio_num == FINGERPRINT_TOUCH_OUT)
  {
    gpio_isr_handler_remove(FINGERPRINT_TOUCH_OUT);
    gpio_isr_handler_remove(KEYBOARD_INT);
    fingerprint_is_up = 1;
  }
  else
  {
    /// 将产生中断的GPIO引脚号发送到gpio_event_queue队列中
    /// 为什么不在这里读取按键值？而是要将引脚号发送到队列中呢？
    xQueueSendFromISR(gpio_event_queue, &gpio_num, NULL);
  }
}

/// rtos任务：ota
static void ota_task_helper(void)
{
  while (1)
  {
    if (start_ota == 1)
    {
      start_ota = 0;
      get_sha256_of_partitions();
      ota_task();
    }
    vTaskDelay(5000 / portTICK_PERIOD_MS);
  }
}

/// 指纹进入休眠
static void fingerprint_sleep(void)
{
  FINGERPRINT_Cancel();
  FINGERPRINT_Sleep();
  fingerprint_enroll_flag = 0xFF; // 需要注册，请重新进入
  fingerprint_enroll_success = 0xFF;
  fingerprint_is_up = 0;
  /// 将处理指纹模块中断的方法重新注册回来
  gpio_isr_handler_add(FINGERPRINT_TOUCH_OUT, gpio_interrupt_handler,
                       (void *)FINGERPRINT_TOUCH_OUT);
  gpio_isr_handler_add(KEYBOARD_INT, gpio_interrupt_handler,
                       (void *)KEYBOARD_INT);
}

/// 录入指纹的RTOS任务
static void enroll_fingerprint_task(void)
{
  while (1)
  {
    if (fingerprint_enroll_flag == 0xFF - 1)
    {
      vTaskDelay(50 / portTICK_PERIOD_MS);
      AUDIO_Send(80);
      if (fingerprint_is_up)
      {
        /// 10秒内按下两次手指
        uint8_t res = FINGERPRINT_Enroll(fingerprint_templates_number + 1, 2);
        if (res == 0)
        {
          printf("指纹注册成功\r\n");
          fingerprint_templates_number += 1;
          fingerprint_enroll_success = 1;
          AUDIO_Send(83);
        }
        else
        {
          printf("指纹注册失败\r\n");
          fingerprint_enroll_success = 0;
          AUDIO_Send(84);
        }
        fingerprint_sleep();
      }
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

/// 识别指纹的任务
static void identify_fingerprint_task(void)
{
  while (1)
  {
    if (fingerprint_is_up && fingerprint_enroll_flag == 0xFF)
    {
      printf("进入指纹识别任务\r\n");
      vTaskDelay(100 / portTICK_PERIOD_MS);
      uint8_t res = FINGERPRINT_Identify();
      if (res == 0)
      {
        /// 播报指纹识别成功语音
        AUDIO_Send(85);
        /// 开锁
        MOTOR_Open_lock();
        vTaskDelay(20 / portTICK_PERIOD_MS);
        AUDIO_Send(25);
      }
      else
      {
        /// 播报指纹识别失败语音
        vTaskDelay(20 / portTICK_PERIOD_MS);
        AUDIO_Send(86);
      }
      fingerprint_sleep();
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

// wifi启动任务
static void wifi_init_task(void)
{
  while (1)
  {
    if (wifi_init_flag == 1)
    {
      printf("启动wifi");
      WIFI_Init(wifi_ssid, wifi_password);
      vTaskDelay(1000 / portTICK_PERIOD_MS);
      wifi_init_flag = 0xFF;
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

// freertos任务：轮询gpio_event_queue队列，如果队列中有gpio引脚号，则处理
static void process_gpio_interrupt(void *arg)
{
  uint32_t gpio_number;
  while (1)
  {
    if (xQueueReceive(gpio_event_queue, &gpio_number, portMAX_DELAY))
    {
      /// 如果产生中断的引脚号是键盘中断的引脚号
      /// 则读取按键值
      if (gpio_number == KEYBOARD_INT)
      {
        uint8_t key_read = KEYBOARD_read_key();
        printf("press key: %d\r\n", key_read);
        light_led(key_read);
        password_array[password_input_count] = key_read;
        password_input_count += 1;
        // 重置键盘和指纹
        if (password_array[password_input_count - 1] == 'M' && password_array[password_input_count - 2] == 'M' &&
            password_array[password_input_count - 3] == 'M')
        {
          password_input_count = 0;
          memset(password_array, 0, sizeof(password_array));
          fingerprint_sleep();
          printf("reset success\r\n");
        }
        /// 启动OTA
        if (password_array[0] == '#' && password_array[1] == '#' &&
            password_array[2] == '#')
        {
          password_input_count = 0;
          memset(password_array, 0, sizeof(password_array));
          start_ota = 1;
        }

        /// 判断临时密码和永久性密码
        char password[7] = {0};
        if (password_input_count == 6)
        {
          /// 先判断永久性密码
          nvs_handle password_handler;
          int err = nvs_open("password", NVS_READWRITE, &password_handler);
          if (err != ESP_OK)
          {
            printf("Error (%s) opening NVS handle!\r\n", esp_err_to_name(err));
          }
          else
          {
            printf("Done\r\n");
            size_t len = 7;
            err = nvs_get_str(password_handler, "lock_password", password, &len);
            if (err == ESP_OK)
            {
              printf("lock password get ok: %s\r\n", password);
            }
            else
            {
              printf("lock password get error\r\n");
            }
            err = nvs_commit(password_handler);
          }
          nvs_close(password_handler);

          uint32_t pswd = password_array[0] * 100000 + password_array[1] * 10000 + password_array[2] * 1000 + password_array[3] * 100 + password_array[4] * 10 + password_array[5];

          uint32_t eternal_pswd = (password[0] - '0') * 100000 + (password[1] - '0') * 10000 + (password[2] - '0') * 1000 + (password[3] - '0') * 100 + (password[4] - '0') * 10 + (password[5] - '0');

          if (pswd == eternal_pswd)
          {
            password_input_count = 0;
            memset(password_array, 0, sizeof(password_array));
            /// 播报密码验证成功语音
            AUDIO_Send(68);
            MOTOR_Open_lock();
            AUDIO_Send(25);
          }
          else
          {
            // 验证临时密码
            int res = validate_temppassword(pswd);
            if (res == 1)
            {
              password_input_count = 0;
              memset(password_array, 0, sizeof(password_array));
              /// 播报密码验证成功语音
              AUDIO_Send(68);
              MOTOR_Open_lock();
              AUDIO_Send(25);
            }
            else
            {
              password_input_count = 0;
              memset(password_array, 0, sizeof(password_array));
              /// 播报密码验证失败语音
              AUDIO_Send(69);
            }
          }
        }
      }
    }

    /// 保证rtos任务可以让出CPU的时间片
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

/// app_main这个rtos任务的优先级是2
void app_main(void)
{
  /// 初始化键盘用到的GPIO引脚
  KEYBOARD_Init();
  /// 初始化电机用到的GPIO引脚
  MOTOR_Init();
  /// 初始化RMT模块
  LED_Init();
  /// 初始化语音模块
  AUDIO_Init();
  /// 初始化指纹模块
  FINGERPRINT_Init();

  /// 创建队列
  gpio_event_queue = xQueueCreate(10, sizeof(uint32_t));

  /// 这里的0是一个标志位，按照默认配置打开中断服务
  gpio_install_isr_service(0);
  /// 注册中断处理函数
  gpio_isr_handler_add(KEYBOARD_INT, gpio_interrupt_handler,
                       (void *)KEYBOARD_INT);
  gpio_isr_handler_add(FINGERPRINT_TOUCH_OUT, gpio_interrupt_handler,
                       (void *)FINGERPRINT_TOUCH_OUT);

  /// 获取指纹信息
  FINGERPRINT_GetChipSerialNumber();
  /// 设置安全级别为0
  /// 因为只有在安全级别为0或者1的情况下，才支持一站式注册指纹
  FINGERPRINT_SetSecurityLevel();
  /// 取消指纹芯片的所有操作
  FINGERPRINT_Cancel();
  /// 进入休眠，如果不取消指纹芯片的所有操作，指纹芯片很大可能会休眠失败
  FINGERPRINT_Sleep();
  /// 读取模板的数量
  fingerprint_templates_number = FINGERPRINT_Inquiry();

  esp_err_t ret;

  // 初始化flash
  ret = nvs_flash_init();
  // 如果初始化flash失败，则擦除flash并重新初始化
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
  {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }

  /// 蓝牙初始化
  BLUETOOTH_Init();

  // SNTP_Init();

  // MQTT_Init();

  /// 将process_gpio_interrupt注册为一个rtos任务
  xTaskCreate(process_gpio_interrupt, "process_int", 2048, NULL, 10, NULL);
  // 录入指纹的任务
  xTaskCreate(enroll_fingerprint_task, "enroll_fingerprint_task", 2048, NULL, 10, NULL);
  // 识别指纹的任务
  xTaskCreate(identify_fingerprint_task, "identify_fingerprint_task", 2048, NULL, 10, NULL);
  // 注册wifi启动任务
  xTaskCreate(wifi_init_task, "wifi_init_task", 4096, NULL, 10, NULL);
  /// 注册ota任务
  xTaskCreate(ota_task_helper, "ota_task", 8192, NULL, 10, NULL);
}
