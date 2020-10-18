#include "server_auth_digest.h"
#define DEBUG false
static const char *TAG = "DIGEST AUTH";

typedef enum
{
    DIGEST_NO_ARG = 0,
    DIGEST_INVALID_USER,
    DIGEST_HASH_FAILED,
    DIGEST_NC_FAILED,
    DIGEST_HASH_A2_FAILED,
    DIGEST_SUCCESS
} digest_auth_err_t;

size_t nonce_size = 0;
unsigned char *nonce;
unsigned long last_update_hash = 0;

digest_config_t configs = {
    .username = "Admin",
    .password = "Password",
};

void convert_md5(const unsigned char input[16], unsigned char output[33])
{
    sprintf((char *)output, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
            input[0],
            input[1],
            input[2],
            input[3],
            input[4],
            input[5],
            input[6],
            input[7],
            input[8],
            input[9],
            input[10],
            input[11],
            input[12],
            input[13],
            input[14],
            input[15]);
}

char *get_header_param(const char *name, size_t name_size, const char *source)
{
    char *param = strstr(source, name);
    if (param != NULL)
    {
        param += name_size;
    }
    return param;
}

char *clean_value(char *value)
{
    //Remove brackets
    if (value[0] == '\"')
    {
        char *end = strchr(value + 1, '\"');
        if (end != NULL)
        {
            end[0] = '\0';
            value++;
        }
    }
    char *end = strchr(value, ',');
    if (end != NULL)
    {
        end[0] = '\0';
    }
    return value;
}

static digest_auth_err_t authentification_login(char *source, const char *method)
{
    char *hasDigest = strstr(source, "Digest");
    if (hasDigest == NULL)
    {
        return DIGEST_NO_ARG;
    }
    char *username = get_header_param("username=", 9, source);
    char *response = get_header_param("response=", 9, source);
    char *uri = get_header_param("uri=", 4, source);
    char *realm = get_header_param("realm=", 6, source);
    char *cnonce = get_header_param("cnonce=", 7, source);
    char *qop = get_header_param("qop=", 4, source);
    char *nc = get_header_param("nc=", 3, source);

    if (username == NULL || response == NULL || uri == NULL || realm == NULL || cnonce == NULL || qop == NULL || nc == NULL)
    {
        return DIGEST_NO_ARG;
    }

    username = clean_value(username);
    uri = clean_value(uri);
    response = clean_value(response);
    realm = clean_value(realm);
    cnonce = clean_value(cnonce);
    qop = clean_value(qop);
    nc = clean_value(nc);
    if (DEBUG)
    {
        ESP_LOGI(TAG, "Response: %s", response);
    }

    if (strcmp(username, configs.username) != 0)
    {
        return DIGEST_INVALID_USER;
    }

    unsigned char a1[64], ha1[16], str_ha1[33];
    sprintf((char *)a1, "%s:%s:%s", username, realm, configs.password);
    mbedtls_md5_ret(a1, strlen((char *)a1), ha1);
    convert_md5(ha1, str_ha1);
    if (DEBUG)
    {
        ESP_LOGI(TAG, "%s", a1);
        ESP_LOGI(TAG, "%s", str_ha1);
    }

    unsigned char a2[64], ha2[16], str_ha2[33];
    sprintf((char *)a2, "%s:%s", method, uri);
    mbedtls_md5_ret(a2, strlen((char *)a2), ha2);
    convert_md5(ha2, str_ha2);
    if (DEBUG)
    {
        ESP_LOGI(TAG, "%s", a2);
        ESP_LOGI(TAG, "%s", str_ha2);
    }

    unsigned char result[256], hres[16], str_hres[33];
    sprintf((char *)result, "%s:%s:%s:%s:%s:%s", str_ha1, nonce, nc, cnonce, qop, str_ha2);
    //add counter to nc
    mbedtls_md5(result, strlen((char *)result), hres);
    convert_md5(hres, str_hres);

    if (DEBUG)
    {
        ESP_LOGI(TAG, "%s", result);
        ESP_LOGI(TAG, "%s", str_hres);
    }
    if (strcmp(response, (char *)str_hres) == 0)
    {
        return DIGEST_SUCCESS;
    }
    else
    {
        return DIGEST_HASH_FAILED;
    }
}

esp_err_t digest_auth(httpd_req_t *req, const char *method)
{
    if (last_update_hash == 0 || last_update_hash + 3600 * CLOCKS_PER_SEC < clock())
    {
        last_update_hash = clock();
        char result[21];
        sprintf(result, "%lu:AutoPills", last_update_hash);
        mbedtls_base64_encode(nonce, 32, &nonce_size, (unsigned char *)result, 21);
    }
    size_t length = httpd_req_get_hdr_value_len(req, "Authorization");
    digest_auth_err_t auth_err = DIGEST_HASH_FAILED;
    if (length > 0)
    {
        if (length > 512)
            length = 512;
        char *source = (char *)malloc(length + 1);
        httpd_req_get_hdr_value_str(req, "Authorization", source, length + 1);
        auth_err = authentification_login(source, method);
    }
    if (auth_err != DIGEST_SUCCESS)
    {
        char result[128];
        sprintf(result, "Digest realm=\"Admin panel\", nonce=\"%s\", algorithm=MD5, qop=auth", nonce);
        httpd_resp_set_hdr(req, "WWW-Authenticate", result);
        httpd_resp_set_status(req, "401 Unauthorized");
        httpd_resp_send(req, "401 Unauthorized", HTTPD_RESP_USE_STRLEN);
        //httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "401 Unauthorized");
        return ESP_ERR_HTTPD_RESP_SEND;
    }
    return ESP_OK;
}

void digest_init()
{
    nonce = (unsigned char *)malloc(64 * sizeof(char));
}
void digest_deinit()
{
    free((void *)nonce);
    nonce_size = 0;
    last_update_hash = 0;
}

void digest_set_config(digest_config_t config)
{
    configs = config;
}

digest_config_t digest_get_config()
{
    return configs;
}