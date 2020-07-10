#ifndef _BUTTON_H_
#define _BUTTON_H_

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

void button_init_global();

void button_init(gpio_num_t gpio_pin);

/**
 * @brief This function add handler on interrupt event. Do not use delay in isr_handler
 */
void button_handler_add(gpio_num_t gpio_pin, void (*handler)(void*), void* args);

void button_handler_remove(gpio_num_t gpio_pin);

#endif