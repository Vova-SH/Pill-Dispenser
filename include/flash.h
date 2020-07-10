#ifndef _FLASH_H_
#define _FLASH_H_

#include "esp_spiffs.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_log.h"

esp_err_t flash_init();

void flash_set_int(const char *key, int8_t value);

void flash_set_str(const char *key, const char *value);

void flash_set_blob(const char *key, const void *value, size_t length);

int8_t flash_get_int(const char *key, int8_t value_default);

const char *flash_get_str(const char *key, const char *value_default);

void *flash_get_blob(const char *key, size_t *length);

void flash_commit();
void flash_open_directory();
void flash_close_directory();

#endif