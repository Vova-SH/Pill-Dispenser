#include "wifi.h"

#define RETRY_COUNT 10

void (*wifi_handler)(wifi_event_connection_t) = NULL;

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

static const char *TAG = "WIFI";

/*set wps mode via project configuration */
#define WPS_MODE WPS_TYPE_PBC //WPS_TYPE_PIN

#ifndef PIN2STR
#define PIN2STR(a) (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5], (a)[6], (a)[7]
#define PINSTR "%c%c%c%c%c%c%c%c"
#endif

static esp_wps_config_t config = WPS_CONFIG_INIT_DEFAULT(WPS_MODE);

static int s_retry_num = 0;

static void event_handler_ip(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    ESP_LOGI(TAG, "Your IPv4:" IPSTR, IP2STR(&event->ip_info.ip));
    xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
}

static void event_handler_wifi(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    switch (event_id)
    {
    case WIFI_EVENT_STA_START:
        esp_wifi_connect();
        break;

    case WIFI_EVENT_STA_DISCONNECTED:
        if (s_retry_num < RETRY_COUNT)
        {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "Retry %d connecting to AP", s_retry_num);
        }
        else
        {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG, "Connect to the AP fail");
        break;
    case WIFI_EVENT_STA_WPS_ER_SUCCESS:
        ESP_LOGI(TAG, "WPS connection successful");
        /* esp_wifi_wps_start() only gets ssid & password, so call esp_wifi_connect() here. */
        ESP_ERROR_CHECK(esp_wifi_wps_disable());
        ESP_ERROR_CHECK(esp_wifi_connect());
        break;
    case WIFI_EVENT_STA_WPS_ER_FAILED:
        ESP_LOGI(TAG, "WPS connection failed");
        if (s_retry_num < RETRY_COUNT)
        {
            ESP_ERROR_CHECK(esp_wifi_wps_disable());
            ESP_ERROR_CHECK(esp_wifi_wps_enable(&config));
            ESP_ERROR_CHECK(esp_wifi_wps_start(0));
            s_retry_num++;
        }
        else
        {
            if (wifi_handler != NULL) wifi_handler(WIFI_WPS_STOP);
            s_retry_num = 0;
        }
        break;
    case WIFI_EVENT_STA_WPS_ER_TIMEOUT:
        ESP_LOGI(TAG, "WPS timeout");
        if (s_retry_num < RETRY_COUNT)
        {
            ESP_ERROR_CHECK(esp_wifi_wps_disable());
            ESP_ERROR_CHECK(esp_wifi_wps_enable(&config));
            ESP_ERROR_CHECK(esp_wifi_wps_start(0));
            s_retry_num++;
        }
        else
        {
            if (wifi_handler != NULL) wifi_handler(WIFI_WPS_STOP);
            s_retry_num = 0;
        }
        break;
    case WIFI_EVENT_STA_WPS_ER_PIN:
        ESP_LOGI(TAG, "WIFI_EVENT_STA_WPS_ER_PIN");
        /* display the PIN code */
        wifi_event_sta_wps_er_pin_t *event = (wifi_event_sta_wps_er_pin_t *)event_data;
        ESP_LOGI(TAG, "WPS_PIN = " PINSTR, PIN2STR(event->pin_code));
        break;
    }
}

void wifi_wps_start()
{
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Start WPS...");
    if (wifi_handler != NULL)
        wifi_handler(WIFI_WPS_START);

    ESP_ERROR_CHECK(esp_wifi_wps_enable(&config));
    ESP_ERROR_CHECK(esp_wifi_wps_start(0));
}

void wifi_init()
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
    //tcpip_adapter_ip_info_t ipInfo;
    //IP4_ADDR(&ipInfo.ip, 192, 168, 1, 218);
    //tcpip_adapter_set_ip_info(TCPIP_ADAPTER_IF_STA, &ipInfo);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler_wifi, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler_ip, NULL));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    ESP_LOGI(TAG, "Initializing complete");
}

void wifi_connect(wifi_config_t config)
{
    s_wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &config));
    ESP_ERROR_CHECK(esp_wifi_start());
    if (wifi_handler != NULL) wifi_handler(WIFI_CONNECTING);
    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                           WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdFALSE,
                                           pdFALSE,
                                           portMAX_DELAY);
    s_retry_num = 0;
    //TODO: make async wait time
    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT)
    {
        if (wifi_handler != NULL) wifi_handler(WIFI_CONNECTED);
        ESP_LOGI(TAG, "Connected to ap SSID:%s password:%s", config.sta.ssid, config.sta.password);
    }
    else if (bits & WIFI_FAIL_BIT)
    {
        if (wifi_handler != NULL) wifi_handler(WIFI_DISCONNECTED);
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s", config.sta.ssid, config.sta.password);
    }
    else
    {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }

    vEventGroupDelete(s_wifi_event_group);
}

void wifi_disconnect()
{
    //TODO: add sleep mode wifi
    ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler_ip));
    ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler_wifi));

    ESP_ERROR_CHECK(esp_wifi_stop());
}

void wifi_handler_register(void (*handler)(wifi_event_connection_t))
{
    wifi_handler = handler;
}

void wifi_handler_unregister()
{
    wifi_handler = NULL;
}

wifi_config_t wifi_get_config()
{
    wifi_config_t wifi_config;
    ESP_ERROR_CHECK(esp_wifi_get_config(ESP_IF_WIFI_STA, &wifi_config));
    return wifi_config;
}