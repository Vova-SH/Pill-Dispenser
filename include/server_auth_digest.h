#ifndef _SERVER_AUTH_DIGEST_H_
#define _SERVER_AUTH_DIGEST_H_
#include <string.h>
#include <fcntl.h>
#include "esp_log.h"
#include "esp_http_server.h"

#include "mbedtls/base64.h"
#include "mbedtls/md5.h"

typedef struct digest_config
{
    const char *username;
    const char *password;
} digest_config_t;

esp_err_t digest_auth(httpd_req_t *req, const char *method);

void digest_init();
void digest_deinit();

void digest_set_config(digest_config_t config);
digest_config_t digest_get_config();

#endif