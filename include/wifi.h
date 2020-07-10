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

#include "lwip/err.h"
#include "lwip/sys.h"

#include "esp_wps.h"

void wifi_init();

void wifi_connect(wifi_config_t config);

void wifi_disconnect();

wifi_config_t wifi_get_config();

void wifi_wps_start();

#endif