#ifndef _LED_H_
#define _LED_H_

#include "driver/gpio.h"

void led_init(gpio_num_t gpio_pin);

void led_enable(gpio_num_t gpio_pin);

void led_disable(gpio_num_t gpio_pin);

#endif