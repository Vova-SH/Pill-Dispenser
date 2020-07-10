#ifndef _SERVO_H_
#define _SERVO_H_

#include "driver/mcpwm.h"
//This default for SG90 servo
/**
 * @brief Default MCPWM configuration
 */
#define SERVO_CONFIG_DEFAULT() {    \
    .frequency = 50,                \
    .cmpr_a = 0,                    \
    .cmpr_b = 0,                    \
    .duty_mode = MCPWM_DUTY_MODE_0, \
    .counter_mode = MCPWM_UP_COUNTER};

/**
 * @brief Minimum pulse width in microsecond
 */
#define SERVO_MIN_PULSEWIDTH 500
/**
 * @brief Maximum pulse width in microsecond
 */
#define SERVO_MAX_PULSEWIDTH 2500
/**
 * @brief Maximum angle in degree upto which servo can rotate
 */
#define SERVO_MAX_DEGREE 180

void servo_init(gpio_num_t gpio_pin);

void servo_set_angle(uint32_t angle);

#endif