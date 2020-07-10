#include "flash.h"

static const char *TAG = "flash";

static nvs_handle handle;

void flash_open_directory()
{
    ESP_ERROR_CHECK(nvs_open("storage", NVS_READWRITE, &handle));
}

void flash_close_directory()
{
    nvs_close(handle);
}

esp_err_t flash_init()
{
    ESP_LOGI(TAG, "Initializing NVS");
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "Initializing SPIFFS");

    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 5, // This decides the maximum number of files that can be created on the storage
        .format_if_mount_failed = true};

    ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK)
    {
        if (ret == ESP_FAIL)
        {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        }
        else if (ret == ESP_ERR_NOT_FOUND)
        {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        }
        else
        {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return ESP_FAIL;
    }
    size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    return ESP_OK;
}

void flash_set_int(const char *key, int8_t value)
{
    nvs_set_i8(handle, key, value);
}

void flash_set_str(const char *key, const char *value)
{
    nvs_set_str(handle, key, value);
}

void flash_set_blob(const char *key, const void *value, size_t length)
{
    printf("Status: %d\n", nvs_set_blob(handle, key, value, length));
}

int8_t flash_get_int(const char *key, int8_t value_default)
{
    int8_t value;
    if (nvs_get_i8(handle, key, &value) == ESP_OK)
    {
        return value;
    }
    return value_default;
}

const char *flash_get_str(const char *key, const char *value_default)
{
    size_t length;
    if(nvs_get_str(handle, key, NULL, &length) != ESP_OK) {
        return value_default;
    }
    char* value = (char*) malloc(length);
    nvs_get_str(handle, key, value, &length);
    return value;
}

void *flash_get_blob(const char *key, size_t *length)
{
    if(nvs_get_blob(handle, key, NULL, length) != ESP_OK) {
        *length = 0;
        return NULL;
    }
    void *data = malloc(*length);
    nvs_get_blob(handle, key, data, length);
    return data;
}

void flash_commit()
{
    nvs_commit(handle);
}