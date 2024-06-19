#ifndef __OTA_H_
#define __OTA_H_

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "string.h"
#include "esp_crt_bundle.h"
#include "esp_wifi.h"
#include "esp_netif.h"


#define HASH_LEN 32
#define OTA_URL_SIZE 256
#define EXAMPLE_NETIF_DESC_STA "example_netif_sta"

void get_sha256_of_partitions(void);
/// ota任务
void ota_task(void);

#endif