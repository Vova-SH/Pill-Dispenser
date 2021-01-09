#include "wifi_connection/esptouch.h"

#define RETRY_COUNT 10
/* FreeRTOS event group to signal when we are connected and ready to make a request */
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event,
   but we only care about one event - are we connected
   to the AP with an IP? */
static const int CONNECTED_BIT = BIT0;
static const int ESPTOUCH_DONE_BIT = BIT1;
static const char *TAG = "ESP-Touch";

static void esptouch_task(void *parm);

static void event_handler_wifi(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    switch (event_id)
    {
    case WIFI_EVENT_STA_START:
        ESP_LOGI(TAG, "STA_START");
        xTaskCreate(esptouch_task, "esptouch_task", 4096, NULL, 3, NULL);
        break;
    case WIFI_EVENT_STA_DISCONNECTED:
        ESP_LOGI(TAG, "STA_DISCONNECTED");
        break;
    case WIFI_EVENT_STA_STOP:
        ESP_LOGI(TAG, "STA_STOP");
        xEventGroupSetBits(s_wifi_event_group, ESPTOUCH_DONE_BIT);
        break;
    }
}

static void event_handler_smartconfig(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    switch (event_id)
    {
    case SC_EVENT_SCAN_DONE:
        ESP_LOGI(TAG, "Scan done");
        break;
    case SC_EVENT_FOUND_CHANNEL:
        ESP_LOGI(TAG, "Found channel");
        break;
    case SC_EVENT_GOT_SSID_PSWD:
        ESP_LOGI(TAG, "Got SSID and password");

        smartconfig_event_got_ssid_pswd_t *evt = (smartconfig_event_got_ssid_pswd_t *)event_data;
        wifi_config_t wifi_config;
        uint8_t ssid[33] = {0};
        uint8_t password[65] = {0};

        bzero(&wifi_config, sizeof(wifi_config_t));
        memcpy(wifi_config.sta.ssid, evt->ssid, sizeof(wifi_config.sta.ssid));
        memcpy(wifi_config.sta.password, evt->password, sizeof(wifi_config.sta.password));
        wifi_config.sta.bssid_set = evt->bssid_set;
        if (wifi_config.sta.bssid_set == true)
        {
            memcpy(wifi_config.sta.bssid, evt->bssid, sizeof(wifi_config.sta.bssid));
        }

        memcpy(ssid, evt->ssid, sizeof(evt->ssid));
        memcpy(password, evt->password, sizeof(evt->password));
        ESP_LOGI(TAG, "SSID:%s", ssid);
        ESP_LOGI(TAG, "PASSWORD:%s", password);

        ESP_ERROR_CHECK(esp_wifi_disconnect());
        ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
        xEventGroupSetBits(s_wifi_event_group, CONNECTED_BIT);
        break;
    case SC_EVENT_SEND_ACK_DONE:
        xEventGroupSetBits(s_wifi_event_group, ESPTOUCH_DONE_BIT);
        break;
    }
}

static void event_handler_wifi_connection(wifi_event_connection_t status)
{
    if (status == WIFI_DISCONNECTED)
    {
        xEventGroupSetBits(s_wifi_event_group, ESPTOUCH_DONE_BIT);
    }
}

void esptouch_start()
{
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler_wifi, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(SC_EVENT, ESP_EVENT_ANY_ID, &event_handler_smartconfig, NULL));
    ESP_ERROR_CHECK(esp_wifi_start());
}

void esptouch_stop()
{
    ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler_wifi));
    ESP_ERROR_CHECK(esp_event_handler_unregister(SC_EVENT, ESP_EVENT_ANY_ID, &event_handler_smartconfig));
    xEventGroupSetBits(s_wifi_event_group, ESPTOUCH_DONE_BIT);
    ESP_ERROR_CHECK(esp_wifi_stop());
}

static void esptouch_task(void *parm)
{
    s_wifi_event_group = xEventGroupCreate();
    EventBits_t bits;
    ESP_ERROR_CHECK(esp_smartconfig_set_type(SC_TYPE_ESPTOUCH));
    smartconfig_start_config_t cfg = SMARTCONFIG_START_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_smartconfig_start(&cfg));
    while (1)
    {
        bits = xEventGroupWaitBits(s_wifi_event_group,
                                   CONNECTED_BIT | ESPTOUCH_DONE_BIT,
                                   pdTRUE,
                                   pdFALSE,
                                   portMAX_DELAY);
        
        if (bits & ESPTOUCH_DONE_BIT)
        {
            ESP_LOGI(TAG, "smartconfig over");
            esp_smartconfig_stop();
            wifi_handler_unregister(event_handler_wifi_connection);
            vTaskDelete(NULL);
        }
        else if (bits & CONNECTED_BIT)
        {
            ESP_LOGI(TAG, "WiFi Try Connecting to ap");
            ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler_wifi));
            esp_wifi_stop();
            wifi_handler_register(event_handler_wifi_connection);
            wifi_connect(wifi_get_config());
        }
    }
}