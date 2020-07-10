#ifndef _NTP_H_
#define _NTP_H_

#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "esp_log.h"
#include "esp_sntp.h"

typedef struct ntp_config
{
    const char *uri;
    bool autosync;
} ntp_config_t;

void ntp_init();
void ntp_deinit();
void ntp_sync();
void ntp_set_config(ntp_config_t config);
ntp_config_t ntp_get_config();

#endif