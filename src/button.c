#include "button.h"

static const char *TAG = "button";

typedef struct handler_data {
    gpio_num_t gpio_pin;
    int gpio_level;
    void (*handle)(void*);
    void *arg;
} handler_data_t;

static TaskHandle_t handle = NULL;

void button_debounce_contact(void *arg)
{
    handler_data_t *data = ((handler_data_t*)arg);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    if (gpio_get_level(data->gpio_pin) == data->gpio_level)
    {
        ESP_LOGI(TAG, "%d %d", data->gpio_pin, data->gpio_level);
        (data->handle)(data->arg);
    }
    handle = NULL;
    vTaskDelete(NULL);
}

static void IRAM_ATTR button_handler(void *arg)
{
    if(handle == NULL)
    {
        xTaskCreate(button_debounce_contact, "debounce", 2048, arg, 1, &handle);
    }    
}

void button_init_global()
{
    gpio_install_isr_service(0);
}
/**
 * pull_up - triggered on +V
 * pull_down - triggered on GND
 * GPIO_PIN_INTR_ANYEDGE - triggered on any change state
 * GPIO_PIN_INTR_POSEDGE - triggered on up state
 * 
 */
void button_init(gpio_num_t gpio_pin)
{
    gpio_config_t io_conf;
    //disable pull-down mode
    io_conf.pull_down_en = 0;

    //interrupt of rising edge
    io_conf.intr_type = GPIO_PIN_INTR_NEGEDGE;
    //bit mask of the pins, use GPIO4/5 here
    io_conf.pin_bit_mask = 1ULL << gpio_pin;
    //set as input mode
    io_conf.mode = GPIO_MODE_INPUT;
    //enable pull-up mode
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);
}

void button_handler_add(gpio_num_t gpio_pin, void (*handler)(void*), void* args)
{
    //remove this
    handler_data_t *data = malloc(sizeof(handler_data_t));
    data->gpio_pin = gpio_pin;
    data->gpio_level = 0;
    data->handle = handler;
    data->arg = args;
    gpio_isr_handler_add(gpio_pin, button_handler, (void *) data);
}

void button_handler_remove(gpio_num_t gpio_pin)
{
    gpio_isr_handler_remove(gpio_pin);
}