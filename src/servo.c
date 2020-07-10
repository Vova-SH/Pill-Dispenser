#include "servo.h"

void servo_init(gpio_num_t gpio_pin)
{
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, gpio_pin);
    mcpwm_config_t pwm_config = SERVO_CONFIG_DEFAULT();
    mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &pwm_config);
}

uint32_t servo_angle_to_pulsewidth(uint32_t angle)
{
    uint32_t pulsewidth = 0;
    pulsewidth = (SERVO_MAX_PULSEWIDTH - SERVO_MIN_PULSEWIDTH) * angle / SERVO_MAX_DEGREE + SERVO_MIN_PULSEWIDTH;
    return pulsewidth;
}

void servo_set_angle(uint32_t angle)
{
    mcpwm_set_duty_in_us(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, servo_angle_to_pulsewidth(angle));
}