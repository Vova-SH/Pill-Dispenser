#ifndef _SCHEDULE_H_
#define _SCHEDULE_H_

#include "stdint.h"


#include <stdio.h>
#include <sys/time.h>
#include <malloc.h>
#include "esp_types.h"
#include "esp_log.h"
#include "time.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/periph_ctrl.h"
#include "driver/timer.h"

/**
 * @brief Select repeat time
 */
typedef enum {
    SCHEDULE_REPEAT_DISABLE = 0,
    SCHEDULE_REPEAT_WEEK,
    SCHEDULE_REPEAT_DAY
} schedule_repeat_t;

typedef struct schedule_task {
    time_t time;
    schedule_repeat_t repeat;
} schedule_task_t;

void schedule_init(const schedule_task_t tasks[], size_t size, void (*handler)());

/**
 * @brief Correct timers after update time
 * 
 * @return Количество таймеров, которые сработали ложно. Положительное значение, если сработали раньше, отрицательное, если позже
 */
int schedule_time_correct();
/**
 * @brief Return all task in schedule
 * 
 * @return Count of tasks
 */
schedule_task_t *schedule_get_tasks(size_t *length);

void schedule_deinit();

#endif