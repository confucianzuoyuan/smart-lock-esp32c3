#include "MQTT_Driver.h"

static const char *TAG = "MQTT_EXAMPLE";

static void mqtt_app_start(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = "mqtt://39.98.123.211:1883",
        .credentials.client_id = "n8l85e60993pphp",
        .credentials.username = "r47feu3x",
        .credentials.authentication.password = "111111",
    };

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    esp_mqtt_client_start(client);

    vTaskDelay(5000 / portTICK_PERIOD_MS);

    esp_mqtt_client_subscribe(client, "sys/oyhju8e3h8/n8l85e60993pphp/property/set", 0);

    vTaskDelay(5000 / portTICK_PERIOD_MS);

    esp_mqtt_client_publish(client, "sys/oyhju8e3h8/n8l85e60993pphp/property/set", "{\"id\":\"zuoyuan\",\"params\":{\"Clarity\":\"1\"}}", 0, 0, 0);
}

void MQTT_Init(void)
{
    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %" PRIu32 " bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("mqtt_client", ESP_LOG_VERBOSE);
    esp_log_level_set("MQTT_EXAMPLE", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_BASE", ESP_LOG_VERBOSE);
    esp_log_level_set("esp-tls", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT", ESP_LOG_VERBOSE);
    esp_log_level_set("outbox", ESP_LOG_VERBOSE);

    mqtt_app_start();
}