#include "SNTP_Driver.h"

#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_attr.h"
#include "esp_sleep.h"
#include "nvs_flash.h"
#include "esp_netif_sntp.h"
#include "lwip/ip_addr.h"
#include "esp_sntp.h"

#define INET6_ADDRSTRLEN 48

static const char *TAG = "example";

void print_servers(void)
{
    ESP_LOGI(TAG, "List of configured NTP servers:");

    for (uint8_t i = 0; i < SNTP_MAX_SERVERS; ++i)
    {
        if (esp_sntp_getservername(i))
        {
            ESP_LOGI(TAG, "server %d: %s", i, esp_sntp_getservername(i));
        }
        else
        {
            // we have either IPv4 or IPv6 address, let's print it
            char buff[INET6_ADDRSTRLEN];
            ip_addr_t const *ip = esp_sntp_getserver(i);
            if (ipaddr_ntoa_r(ip, buff, INET6_ADDRSTRLEN) != NULL)
                ESP_LOGI(TAG, "server %d: %s", i, buff);
        }
    }
}

/// 从时钟服务器获取当前时间，更新esp32的rtc
void obtain_time(void)
{
    // pool.ntp.org国家授时中心
    esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
    esp_netif_sntp_init(&config);

    // 打印时间服务器
    print_servers();

    // wait for time to be set
    time_t now = 0;
    struct tm timeinfo = {0};
    int retry = 0;
    const int retry_count = 15;
    while (esp_netif_sntp_sync_wait(2000 / portTICK_PERIOD_MS) == ESP_ERR_TIMEOUT && ++retry < retry_count)
    {
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
    }
    time(&now);
    localtime_r(&now, &timeinfo);

    esp_netif_sntp_deinit();
}

/// @brief 和网络时钟同步
/// @param  
void SNTP_Init(void)
{
    // 初始化时间变量
    time_t now;
    // 初始化一个保存时间信息的结构体
    struct tm timeinfo;
    // 获取RTC当前时间(以秒为单位)
    time(&now);
    // 将now转换成本地时间并存储到timeinfo结构体中。
    localtime_r(&now, &timeinfo);
    // Is time set? If not, tm_year will be (1970 - 1900).
    // 时间同步好了吗？如果没有同步好，则从网上下载时间进行同步
    if (timeinfo.tm_year < (2016 - 1900))
    {
        ESP_LOGI(TAG, "Time is not set yet. Connecting to WiFi and getting time over NTP.");
        /// 从网络上的ntp服务器获取时间
        obtain_time();
        // update 'now' variable with current time
        // 更新实时时钟
        time(&now);
    }

    printf("time_t now : %lld\r\n", now);

    char strftime_buf[64];

    // Set timezone to China Standard Time
    setenv("TZ", "CST-8", 1);
    tzset();
    localtime_r(&now, &timeinfo);
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    /// example: The current date/time in Shanghai is: Tue Jul  2 16:16:09 2024
    ESP_LOGI(TAG, "The current date/time in Shanghai is: %s", strftime_buf);
}