#include "ntp.h"

static const char *TAG = "NTP";

ntp_config_t ntp_configs = {
    .uri = "pool.ntp.org",
    .autosync = 0,
};

void ntp_init_();

void ntp_set_time(struct timeval *tv)
{
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    //Debug
    char strftime_buf[64];
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    ESP_LOGI(TAG, "The current date/time is: %s", strftime_buf);
}

void time_sync_notification_cb(struct timeval *tv)
{
    ESP_LOGI(TAG, "Notification of a time synchronization event");
    ntp_set_time(tv);
}

void ntp_sync()
{
    if (!ntp_configs.autosync)
        ntp_init_();
    int retry = 0;
    const int retry_count = 10;
    sntp_set_sync_status(SNTP_SYNC_STATUS_RESET);
    while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry < retry_count)
    {
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }

    if (!ntp_configs.autosync)
        ntp_deinit();
}

void ntp_init_()
{
    ESP_LOGI(TAG, "Initializing SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, ntp_configs.uri);
    sntp_set_time_sync_notification_cb(time_sync_notification_cb);
    sntp_init();
}

void ntp_init()
{
    ESP_LOGI(TAG, "Initializing SNTP");
    if (ntp_configs.autosync)
        ntp_init_();
}

void ntp_deinit()
{
    sntp_stop();
}

void ntp_set_config(ntp_config_t config)
{
    ntp_configs = config;
}

ntp_config_t ntp_get_config()
{
    return ntp_configs;
}