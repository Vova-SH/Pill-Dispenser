#ifndef _WIFI_H_
#define _WIFI_H_

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_smartconfig.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "esp_wps.h"

typedef enum {
    WIFI_DISCONNECTED = 0,               /**< WiFi connection failed */
    WIFI_CONNECTING,                      /**< WiFi connection succesful */
    WIFI_CONNECTED,                      /**< WiFi connection succesful */
    WIFI_WPS_START,                      /**< WiFi start WPS scan */
    WIFI_SMARTCONFIG_START,              /**< WiFi start Smart Config */
    WIFI_WPS_STOP,                       /**< WiFi stop WPS scan */
    WIFI_SMARTCONFIG_STOP,               /**< WiFi stop Smart Config */
} wifi_event_connection_t;

void wifi_init();

void wifi_handler_register(void (*handler)(wifi_event_connection_t));

void wifi_handler_unregister(void (*handler)(wifi_event_connection_t));

void wifi_connect(wifi_config_t config);

void wifi_disconnect();

wifi_config_t wifi_get_config();

#endif