#include "server.h"
static const char *TAG = "server";

const char *(*user_rest_get_handler)(const char uri[HTTPD_MAX_URI_LEN + 1]) = NULL;
httpd_err_code_t (*user_rest_post_handler)(const char uri[HTTPD_MAX_URI_LEN + 1], char *json) = NULL;
httpd_err_code_t (*user_rest_put_handler)(const char uri[HTTPD_MAX_URI_LEN + 1], char *json) = NULL;
/* Set HTTP response content type according to file extension */
static esp_err_t set_content_type_from_file(httpd_req_t *req, const char *filepath)
{
    const char *type = "text/plain";
    if (CHECK_FILE_EXTENSION(filepath, ".html"))
    {
        type = "text/html";
    }
    else if (CHECK_FILE_EXTENSION(filepath, ".js"))
    {
        type = "application/javascript";
    }
    else if (CHECK_FILE_EXTENSION(filepath, ".css"))
    {
        type = "text/css";
    }
    else if (CHECK_FILE_EXTENSION(filepath, ".png"))
    {
        type = "image/png";
    }
    else if (CHECK_FILE_EXTENSION(filepath, ".ico"))
    {
        type = "image/x-icon";
    }
    else if (CHECK_FILE_EXTENSION(filepath, ".svg"))
    {
        type = "text/xml";
    }
    return httpd_resp_set_type(req, type);
}

/* Send HTTP response with the contents of the requested file */
static esp_err_t common_get_handler(httpd_req_t *req)
{
    if(digest_auth(req, "GET") != ESP_OK) return ESP_OK;
    char filepath[FILE_PATH_MAX];

    http_server_context_t *rest_context = (http_server_context_t *)req->user_ctx;
    strlcpy(filepath, rest_context->base_path, sizeof(filepath));
    if (req->uri[strlen(req->uri) - 1] == '/')
    {
        strlcat(filepath, "/index.html", sizeof(filepath));
    }
    else
    {
        strlcat(filepath, req->uri, sizeof(filepath));
    }
    int fd = open(filepath, O_RDONLY, 0);
    if (fd == -1)
    {
        ESP_LOGE(TAG, "Failed to open file : %s", filepath);
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to read existing file");
        return ESP_FAIL;
    }

    set_content_type_from_file(req, filepath);

    char *chunk = rest_context->scratch;
    ssize_t read_bytes;
    do
    {
        /* Read file in chunks into the scratch buffer */
        read_bytes = read(fd, chunk, SCRATCH_BUFSIZE);
        if (read_bytes == -1)
        {
            ESP_LOGE(TAG, "Failed to read file : %s", filepath);
        }
        else if (read_bytes > 0)
        {
            /* Send the buffer contents as HTTP response chunk */
            if (httpd_resp_send_chunk(req, chunk, read_bytes) != ESP_OK)
            {
                close(fd);
                ESP_LOGE(TAG, "File sending failed!");
                /* Abort sending file */
                httpd_resp_sendstr_chunk(req, NULL);
                /* Respond with 500 Internal Server Error */
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to send file");
                return ESP_FAIL;
            }
        }
    } while (read_bytes > 0);
    /* Close file after sending complete */
    close(fd);
    ESP_LOGI(TAG, "File sending complete");
    /* Respond with an empty chunk to signal HTTP response completion */
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

/* Handler for restful post request */
static esp_err_t rest_put_handler(httpd_req_t *req)
{
    if(digest_auth(req, "PUT") != ESP_OK) return ESP_OK;
    int total_len = req->content_len;
    int cur_len = 0;
    char *buf = ((http_server_context_t *)(req->user_ctx))->scratch;
    int received = 0;
    if (total_len >= SCRATCH_BUFSIZE)
    {
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "content too long");
        return ESP_FAIL;
    }
    while (cur_len < total_len)
    {
        received = httpd_req_recv(req, buf + cur_len, total_len);
        if (received <= 0)
        {
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
            return ESP_FAIL;
        }
        cur_len += received;
    }
    buf[total_len] = '\0';

    if (user_rest_put_handler == NULL)
        return ESP_OK;

    httpd_resp_set_type(req, HTTPD_TYPE_JSON);

    ESP_LOGI(TAG, "PUT: %s", req->uri);
    httpd_err_code_t err_code = user_rest_put_handler(req->uri, buf);
    if (err_code == -1)
    {
        const char *result = user_rest_get_handler(req->uri);
        httpd_resp_sendstr(req, result);
        httpd_resp_sendstr(req, HTTPD_200);
    }
    else
    {
        httpd_resp_send_err(req, err_code, "Error");
    }
    return ESP_OK;
}

/* Handler for restful post request */
static esp_err_t rest_post_handler(httpd_req_t *req)
{
    if(digest_auth(req, "POST") != ESP_OK) return ESP_OK;
    int total_len = req->content_len;
    int cur_len = 0;
    char *buf = ((http_server_context_t *)(req->user_ctx))->scratch;
    int received = 0;
    if (total_len >= SCRATCH_BUFSIZE)
    {
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "content too long");
        return ESP_FAIL;
    }
    while (cur_len < total_len)
    {
        received = httpd_req_recv(req, buf + cur_len, total_len);
        if (received <= 0)
        {
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
            return ESP_FAIL;
        }
        cur_len += received;
    }
    buf[total_len] = '\0';

    if (user_rest_post_handler == NULL)
        return ESP_OK;

    httpd_resp_set_type(req, "text/plane");

    ESP_LOGI(TAG, "POST: %s", req->uri);
    httpd_err_code_t err_code = user_rest_post_handler(req->uri, buf);
    if (err_code == -1)
    {
        httpd_resp_sendstr(req, HTTPD_200);
    }
    else
    {
        httpd_resp_send_err(req, err_code, "Error");
    }
    return ESP_OK;
}

/* Handler for restful get request */
static esp_err_t rest_get_handler(httpd_req_t *req)
{
    if(digest_auth(req, "GET") != ESP_OK) return ESP_OK;
    httpd_resp_set_type(req, "application/json");

    if (user_rest_get_handler != NULL)
    {
        const char *result = user_rest_get_handler(req->uri);
        if (result == NULL)
        {
            httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Not found");
            return ESP_OK;
        }
        httpd_resp_sendstr(req, result);
        free((void *)result);
    }
    return ESP_OK;
}

void server_register_get_handler(const char *(*get_handler)(const char uri[HTTPD_MAX_URI_LEN + 1]))
{
    user_rest_get_handler = get_handler;
}
void server_register_put_handler(httpd_err_code_t (*put_handler)(const char uri[HTTPD_MAX_URI_LEN + 1], char *json))
{
    user_rest_put_handler = put_handler;
}
void server_register_post_handler(httpd_err_code_t (*post_handler)(const char uri[HTTPD_MAX_URI_LEN + 1], char *json))
{
    user_rest_post_handler = post_handler;
}

esp_err_t start_server()
{

    http_server_context_t *rest_context = calloc(1, sizeof(http_server_context_t));
    REST_CHECK(rest_context, "No memory for rest context", err);
    digest_init();
    strlcpy(rest_context->base_path, BASE_PATH, sizeof(rest_context->base_path));

    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.uri_match_fn = httpd_uri_match_wildcard;

    ESP_LOGI(TAG, "Starting HTTP Server");
    REST_CHECK(httpd_start(&server, &config) == ESP_OK, "Start server failed", err_start);

    httpd_uri_t rest_get_uri = {
        .uri = "/api/*",
        .method = HTTP_GET,
        .handler = rest_get_handler,
        .user_ctx = rest_context,
    };
    httpd_register_uri_handler(server, &rest_get_uri);

    httpd_uri_t rest_put_uri = {
        .uri = "/api/*",
        .method = HTTP_PUT,
        .handler = rest_put_handler,
        .user_ctx = rest_context,
    };
    httpd_register_uri_handler(server, &rest_put_uri);

    httpd_uri_t rest_post_uri = {
        .uri = "/api/*",
        .method = HTTP_POST,
        .handler = rest_post_handler,
        .user_ctx = rest_context,
    };
    httpd_register_uri_handler(server, &rest_post_uri);

    httpd_uri_t common_get_uri = {
        .uri = "/*",
        .method = HTTP_GET,
        .handler = common_get_handler,
        .user_ctx = rest_context,
    };
    httpd_register_uri_handler(server, &common_get_uri);

    return ESP_OK;
err_start:
    free(rest_context);
err:
    return ESP_FAIL;
}