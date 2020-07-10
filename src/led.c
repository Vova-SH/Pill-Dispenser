#include "led.h"

void led_init(gpio_num_t gpio_pin)
{
    gpio_pad_select_gpio(gpio_pin);
    gpio_set_direction(gpio_pin, GPIO_MODE_OUTPUT);
}

void led_enable(gpio_num_t gpio_pin)
{
    gpio_set_level(gpio_pin, 1);
}

void led_disable(gpio_num_t gpio_pin)
{
    gpio_set_level(gpio_pin, 0);
}