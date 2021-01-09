#include "wifi.h"
#include "list.h"

#define RETRY_COUNT 10

list_t *wifi_handlers = NULL;

void (*wifi_handler)(wifi_event_connection_t) = NULL;

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1
#define WIFI_CANCEL_BIT BIT2

static const char *TAG = "WIFI";

static void wifi_task(void *parm);
static void wifi_event_status(wifi_event_connection_t event);

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
        xTaskCreate(wifi_task, "wifi_task", 4096, NULL, 3, NULL);
        break;

    case WIFI_EVENT_STA_DISCONNECTED:
        xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        ESP_LOGI(TAG, "Connect to the AP fail");
        break;
    }
}

void wifi_init()
{
    wifi_handlers = list_new();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
    //tcpip_adapter_ip_info_t ipInfo;
    //IP4_ADDR(&ipInfo.ip, 192, 168, 1, 218);
    //tcpip_adapter_set_ip_info(TCPIP_ADAPTER_IF_STA, &ipInfo);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    ESP_LOGI(TAG, "Initializing complete");
}

void wifi_connect(wifi_config_t config)
{

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler_wifi, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler_ip, NULL));

    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &config));
    ESP_ERROR_CHECK(esp_wifi_start());
}

void wifi_disconnect()
{
    //TODO: add sleep mode wifi
    ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler_ip));
    ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler_wifi));

    xEventGroupSetBits(s_wifi_event_group, WIFI_CANCEL_BIT);
    ESP_ERROR_CHECK(esp_wifi_stop());
}

void wifi_handler_register(void (*handler)(wifi_event_connection_t))
{
    list_rpush(wifi_handlers, list_node_new(handler));
    wifi_handler = handler;
}

void wifi_handler_unregister(void (*handler)(wifi_event_connection_t))
{
    list_node_t *node = list_find(wifi_handlers, handler);
    if (node != NULL) {
        list_remove(wifi_handlers, node);
    }
    wifi_handler = NULL;
}

wifi_config_t wifi_get_config()
{
    wifi_config_t wifi_config;
    ESP_ERROR_CHECK(esp_wifi_get_config(ESP_IF_WIFI_STA, &wifi_config));
    return wifi_config;
}

static void wifi_task(void *parm)
{
    s_wifi_event_group = xEventGroupCreate();
    EventBits_t bits;
    int s_retry_num = 0;

    wifi_event_status(WIFI_CONNECTING);
    while (s_retry_num < RETRY_COUNT)
    {
        ESP_LOGI(TAG, "Retry %d connecting to AP", s_retry_num);
        esp_wifi_connect();
        bits = xEventGroupWaitBits(s_wifi_event_group,
                                   WIFI_CONNECTED_BIT | WIFI_FAIL_BIT | WIFI_CANCEL_BIT,
                                   pdTRUE,
                                   pdFALSE,
                                   portMAX_DELAY);
        if (bits & WIFI_CONNECTED_BIT)
        {
            wifi_event_status(WIFI_CONNECTED);
            ESP_LOGI(TAG, "Connected to ap");
            break;
        }
        else if (bits & WIFI_CANCEL_BIT)
        {
            wifi_event_status(WIFI_DISCONNECTED);
            ESP_LOGI(TAG, "Connecting to ap canceled");
            break;
        }
        s_retry_num++;
    }
    if (bits & WIFI_FAIL_BIT)
    {
        wifi_event_status(WIFI_DISCONNECTED);
        ESP_LOGI(TAG, "Failed to connect to ap");
        wifi_disconnect();
    }
    vTaskDelete(NULL);
}

static void wifi_event_status(wifi_event_connection_t event)
{
    list_node_t *node;
    list_iterator_t *it = list_iterator_new(wifi_handlers, LIST_HEAD);
    while ((node = list_iterator_next(it)))
    {
        ((void(*)(wifi_event_connection_t))(node->val))(event);
    }
    list_iterator_destroy(it);
}