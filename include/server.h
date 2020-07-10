#ifndef _SERVER_H_
#define _SERVER_H_

#include <string.h>
#include <fcntl.h>
#include "esp_http_server.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_vfs.h"
#include "cJSON.h"

#include "server_auth_digest.h"

#define BASE_PATH "/spiffs"
#define FILE_PATH_MAX (ESP_VFS_PATH_MAX + 128)
#define SCRATCH_BUFSIZE (10240)

#define CHECK_FILE_EXTENSION(filename, ext) (strcasecmp(&filename[strlen(filename) - strlen(ext)], ext) == 0)

#define REST_CHECK(a, str, goto_tag, ...)                                         \
    do                                                                            \
    {                                                                             \
        if (!(a))                                                                 \
        {                                                                         \
            ESP_LOGE(TAG, "%s(%d): " str, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
            goto goto_tag;                                                        \
        }                                                                         \
    } while (0)

typedef struct http_server_context
{
    char base_path[ESP_VFS_PATH_MAX + 1];
    char scratch[SCRATCH_BUFSIZE];
} http_server_context_t;

esp_err_t start_server();

void server_register_get_handler(const char* (*get_handler)(const char uri[HTTPD_MAX_URI_LEN + 1]));
void server_register_post_handler(httpd_err_code_t (*post_handler)(const char uri[HTTPD_MAX_URI_LEN + 1], char *json));
void server_register_put_handler(httpd_err_code_t (*put_handler)(const char uri[HTTPD_MAX_URI_LEN + 1], char *json));

#endif