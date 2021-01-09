//For upload data to ESP filesystem run in Terminal:
//platformio run --target uploadfs
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "sdkconfig.h"

#include "servo.h"
#include "ntp.h"
#include "led.h"
#include "button.h"
#include "wifi.h"
#include "wifi_connection/esptouch.h"
#include "wifi_connection/wps.h"
#include "flash.h"
#include "server.h"
#include "schedule.h"

#include <driver/adc.h>

#define PIN_SERVO GPIO_NUM_4
#define PIN_LED GPIO_NUM_3
#define PIN_WIFI_LED GPIO_NUM_2
#define PIN_BUTTON GPIO_NUM_14
#define PIN_MOTOR_DC GPIO_NUM_12
#define PIN_DISTANCE_SENSOR GPIO_NUM_13
#define PIN_ROTATION_SENSOR GPIO_NUM_27
#define PIN_LINE_SENSOR GPIO_NUM_32

#define FLASH_LIGHT_NOTIFY "LIGHT_NOTIFY"
#define FLASH_LOCK_STATE "LOCK"
#define FLASH_NTP_URI "NTP_URI"
#define FLASH_NTP_AUTOSYNC "NTP_AUTOSYNC"
#define FLASH_WIFI_SSID "WIFI_SSID"
#define FLASH_WIFI_PASS "WIFI_PASS"
#define FLASH_AUTH_USER "AUTH_USER"
#define FLASH_AUTH_PASS "AUTH_PASS"
#define FLASH_SCHEDULE "SCHEDULE"

typedef struct config
{
    bool light_notification;
    bool lock_state;
} config_t;

#define CONFIG_DEFAULT() {      \
    .light_notification = true, \
    .lock_state = false,        \
};

config_t current_config = CONFIG_DEFAULT();

bool killTask = 1;

void timer_handler()
{
    //Distance
    led_enable(25);
    //Line
    led_enable(33);
    int output = gpio_get_level(PIN_DISTANCE_SENSOR);
    int current_value;
    int count_distance = 0;
    bool is_white_sector = false, is_sector_complete = false;
    led_enable(PIN_MOTOR_DC);
    while (count_distance < 12 && !is_sector_complete)
    {
        current_value = gpio_get_level(PIN_LINE_SENSOR);
        if (is_white_sector)
        {
            if (current_value == 0) is_sector_complete = true;
        }
        else if (current_value == 1)
        {
            is_white_sector = true;
        }

        current_value = gpio_get_level(PIN_DISTANCE_SENSOR);
        if (output != current_value && current_value == 0)
        {
            count_distance++;
        }
        output = current_value;
    }
    led_disable(PIN_MOTOR_DC);
    led_disable(25);
    led_disable(33);
    //Enable light
    if (current_config.light_notification)
    {
        led_enable(PIN_LED);
    }
}

void sync_time()
{
    ESP_LOGI("SCHEDULE", "Sync");
    schedule_reset();
}

void blink_start()
{
    //gpio_get_level(PIN_BUTTON);
    bool state = 1;
    while (!killTask)
    {
        state = !state;
        gpio_set_level(PIN_WIFI_LED, state);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    //servo_set_angle(0);
    vTaskDelete(NULL);
}

void rotation_sensor()
{
    led_disable(PIN_LED);
}

void isr_wps_start(void *arg)
{
    led_disable(PIN_LED);
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    if (!gpio_get_level(PIN_BUTTON))
    {
        led_enable(PIN_LED);
        wps_start();
    }
}

void load_configuration()
{
    current_config.light_notification = flash_get_int(FLASH_LIGHT_NOTIFY, current_config.light_notification);
    current_config.lock_state = flash_get_int(FLASH_LOCK_STATE, current_config.lock_state);

    ntp_config_t ntp_setting = {
        .uri = flash_get_str(FLASH_NTP_URI, "pool.ntp.org"),
        .autosync = (bool)flash_get_int(FLASH_NTP_AUTOSYNC, false),
    };
    ntp_set_config(ntp_setting);

    size_t tasks_data_length;
    schedule_task_t *tasks = flash_get_blob(FLASH_SCHEDULE, &tasks_data_length);

    if (tasks_data_length > 0)
    {
        schedule_init(tasks, tasks_data_length / sizeof(schedule_task_t), timer_handler);
        free(tasks);
    }

    const char *ssid = flash_get_str(FLASH_WIFI_SSID, "WiFi");
    const char *pass = flash_get_str(FLASH_WIFI_PASS, "Password");
    wifi_sta_config_t wifi_sta = {0};
    strncpy((char *)wifi_sta.ssid, ssid, 32);
    strncpy((char *)wifi_sta.password, pass, 64);
    wifi_config_t wifi_config = {
        .sta = wifi_sta};
    wifi_connect(wifi_config);
    digest_config_t auth_setting = {
        .username = flash_get_str(FLASH_AUTH_USER, "Admin"),
        .password = flash_get_str(FLASH_AUTH_PASS, "Password"),
    };
    digest_set_config(auth_setting);
}

const char *get_handler(const char uri[])
{
    cJSON *root = cJSON_CreateObject();
    if (strcmp(uri, "/api/info") == 0)
    {
        esp_chip_info_t chip_info;
        esp_chip_info(&chip_info);

        cJSON_AddStringToObject(root, "version", "4.0.0");
        cJSON_AddNumberToObject(root, "cores", chip_info.cores);
    }
    else if (strcmp(uri, "/api/info/battery") == 0)
    {
        cJSON_AddNumberToObject(root, "status", 2);
        cJSON_AddNumberToObject(root, "percent", 0);
    }
    else if (strcmp(uri, "/api/light") == 0)
    {
        cJSON_AddBoolToObject(root, "enable", current_config.light_notification);
    }
    else if (strcmp(uri, "/api/lock") == 0)
    {
        cJSON_AddBoolToObject(root, "enable", current_config.lock_state);
    }
    else if (strcmp(uri, "/api/time") == 0)
    {
        struct timeval tv_now;
        gettimeofday(&tv_now, NULL);
        cJSON_AddNumberToObject(root, "time_sec", (unsigned long)tv_now.tv_sec);
    }
    else if (strcmp(uri, "/api/time/ntp") == 0)
    {
        cJSON_AddStringToObject(root, "uri", ntp_get_config().uri);
    }
    else if (strcmp(uri, "/api/time/ntp/auto") == 0)
    {
        cJSON_AddNumberToObject(root, "enable", ntp_get_config().autosync);
    }
    else if (strcmp(uri, "/api/schedule") == 0)
    {
        cJSON_Delete(root);
        size_t count_distance = 0;
        schedule_task_t *schedules = schedule_get_tasks(&count_distance);
        root = cJSON_CreateArray();
        for (size_t i = 0; i < count_distance; i++)
        {
            cJSON *item = cJSON_CreateObject();
            cJSON_AddNumberToObject(item, "time_sec", schedules[i].time);
            cJSON_AddNumberToObject(item, "repeat", schedules[i].repeat);
            cJSON_AddItemToArray(root, item);
        }
        free(schedules);
    }
    else if (strcmp(uri, "/api/wifi") == 0)
    {
        const char *const ssid = (char *)wifi_get_config().sta.ssid;
        const char *const pass = (char *)wifi_get_config().sta.password;
        cJSON_AddStringToObject(root, "ssid", ssid);
        cJSON_AddStringToObject(root, "pass", pass);
    }
    else
    {
        cJSON_Delete(root);
        return NULL;
    }

    const char *json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return json;
}

httpd_err_code_t put_handler(const char uri[], char *json)
{
    int status = HTTPD_404_NOT_FOUND;
    cJSON *const root = cJSON_Parse(json);
    if (strcmp(uri, "/api/light") == 0)
    {
        int enable = cJSON_GetObjectItem(root, "enable")->valueint;
        if (enable != current_config.light_notification)
        {
            flash_set_int(FLASH_LIGHT_NOTIFY, enable);
            flash_commit();
        }
        current_config.light_notification = enable;
        if (enable)
            led_enable(PIN_LED);
        else
            led_disable(PIN_LED);
        status = -1;
    }
    else if (strcmp(uri, "/api/lock") == 0)
    {
        int enable = cJSON_GetObjectItem(root, "enable")->valueint;
        if (enable != current_config.lock_state)
        {
            flash_set_int(FLASH_LOCK_STATE, enable);
            flash_commit();
        }
        current_config.lock_state = enable;
        if (enable)
            servo_set_angle(0);
        else
            servo_set_angle(180);
        status = -1;
    }
    else if (strcmp(uri, "/api/time") == 0)
    {
        long sec = (long)cJSON_GetObjectItem(root, "time_sec")->valuedouble;
        struct timeval tv_now;
        tv_now.tv_sec = sec - 1073;
        settimeofday(&tv_now, NULL);
        //int correct = schedule_time_correct();
        //printf("Skip: %d\n", correct);
        status = -1;
    }
    else if (strcmp(uri, "/api/time/ntp") == 0)
    {
        ntp_config_t config = ntp_get_config();
        config.uri = cJSON_GetObjectItem(root, "uri")->valuestring;
        flash_set_str(FLASH_NTP_URI, config.uri);
        flash_commit();
        ntp_set_config(config);
        status = -1;
    }
    else if (strcmp(uri, "/api/time/ntp/auto") == 0)
    {
        ntp_config_t config = ntp_get_config();
        int enable = cJSON_GetObjectItem(root, "enable")->valueint;
        if (enable != config.autosync)
        {
            flash_set_int(FLASH_NTP_AUTOSYNC, enable);
            flash_commit();
        }
        config.autosync = enable;
        ntp_deinit();
        ntp_set_config(config);
        ntp_init();
        status = -1;
    }
    else if (strcmp(uri, "/api/schedule") == 0)
    {
        schedule_deinit();
        int count_distance = cJSON_GetArraySize(root);
        schedule_task_t *tasks = (schedule_task_t *)malloc(count_distance * sizeof(schedule_task_t));
        for (size_t i = 0; i < count_distance; i++)
        {
            cJSON *const item = cJSON_GetArrayItem(root, i);
            tasks[i].time = (long)cJSON_GetObjectItem(item, "time_sec")->valuedouble;
            tasks[i].repeat = cJSON_GetObjectItem(item, "repeat")->valueint;
        }
        flash_set_blob(FLASH_SCHEDULE, tasks, count_distance * sizeof(schedule_task_t));
        schedule_init(tasks, count_distance, timer_handler);
        free(tasks);
        status = -1;
    }
    else if (strcmp(uri, "/api/wifi") == 0)
    {
        wifi_disconnect();
        wifi_sta_config_t wifi_sta = {0};
        const char *ssid = cJSON_GetObjectItem(root, "ssid")->valuestring;
        const char *pass = cJSON_GetObjectItem(root, "password")->valuestring;
        flash_set_str(FLASH_WIFI_SSID, ssid);
        flash_set_str(FLASH_WIFI_PASS, pass);
        flash_commit();
        strncpy((char *)wifi_sta.ssid, ssid, 32);
        strncpy((char *)wifi_sta.password, pass, 64);
        wifi_config_t wifi_config = {.sta = wifi_sta};
        wifi_connect(wifi_config);
        status = -1;
    }
    else if (strcmp(uri, "/api/auth") == 0)
    {
        const char *user = cJSON_GetObjectItem(root, "login")->valuestring;
        const char *old_pass = cJSON_GetObjectItem(root, "old_password")->valuestring;
        const char *new_pass = cJSON_GetObjectItem(root, "new_password")->valuestring;
        if (strcmp(old_pass, digest_get_config().password) == 0)
        {
            flash_set_str(FLASH_AUTH_USER, user);
            flash_set_str(FLASH_AUTH_PASS, new_pass);
            digest_config_t auth_setting = {
                .username = user,
                .password = new_pass,
            };
            digest_deinit();
            digest_set_config(auth_setting);
            digest_init();
            status = -1;
        }
    }
    cJSON_Delete(root);
    return status;
}

httpd_err_code_t post_handler(const char uri[], char *json)
{
    int status = HTTPD_404_NOT_FOUND;
    cJSON *const root = cJSON_Parse(json);
    if (strcmp(uri, "/api/time/ntp/sync") == 0)
    {
        ntp_sync();
        status = -1;
    }
    else if (strcmp(uri, "/api/next") == 0)
    {
        timer_handler();
        status = -1;
    }
    cJSON_Delete(root);
    return status;
}

static void wifi_events(wifi_event_connection_t status)
{
    switch (status)
    {
    case WIFI_CONNECTING:
        killTask = 0;
        xTaskCreate(blink_start, "blink", 1000, NULL, 1, NULL);
        break;
    case WIFI_CONNECTED:
        killTask = 1;
        led_enable(PIN_WIFI_LED);
        break;
    case WIFI_WPS_START:
        killTask = 0;
        xTaskCreate(blink_start, "blink", 1000, NULL, 1, NULL);
        break;
    default:
        killTask = 1;
        led_disable(PIN_WIFI_LED);
        break;
    }
}

void init()
{
    ESP_ERROR_CHECK(flash_init());
    servo_init(PIN_SERVO);
    led_init(PIN_WIFI_LED);
    led_init(PIN_LED);
    led_init(PIN_MOTOR_DC);
    button_init_global();
    button_init(PIN_BUTTON);
    button_init(PIN_LINE_SENSOR);
    button_init(PIN_DISTANCE_SENSOR);
    button_init(PIN_ROTATION_SENSOR);
    struct timeval tv_now;
    tv_now.tv_sec = 604800 * 3;
    settimeofday(&tv_now, NULL);
    //Sensor power
    //Line Sensor
    led_init(33);
    //Distance
    led_init(25);
    led_init(26);
    led_enable(26);

    wifi_init();
    wifi_handler_register(wifi_events);

    flash_open_directory();
    load_configuration();
    ntp_set_sync_handler(sync_time);
    ntp_init();
}

void app_main(void)
{
    init();
    //timer_handler();
    /*
    int current_value;
    //led_enable(PIN_MOTOR_DC);
    while (true)
    {
        current_value = gpio_get_level(32);
        ESP_LOGI("MAIN", "%d", current_value);
    }
*/
    button_handler_add(PIN_BUTTON, isr_wps_start, NULL);
    button_handler_add(PIN_ROTATION_SENSOR, rotation_sensor, NULL);
    server_register_get_handler(get_handler);
    server_register_put_handler(put_handler);
    server_register_post_handler(post_handler);
    ESP_ERROR_CHECK(start_server());
}