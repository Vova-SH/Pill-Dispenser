#ifndef _ESPTOUCH_H_
#define _ESPTOUCH_H_
#include <string.h>
#include <stdlib.h>
#include "esp_smartconfig.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "wifi.h"
#include "freertos/event_groups.h"

void esptouch_start();

void esptouch_stop();

#endif