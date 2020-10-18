#include "schedule.h"

schedule_task_t *s_times = NULL;
size_t times_count;
time_t triggered_last = 0;

void (*schedule_timer_handler)() = NULL;
esp_timer_handle_t schedule_timer = NULL;

time_t task_update(size_t task_num, time_t current_time)
{
    time_t time = s_times[task_num].time;
    int repeat = s_times[task_num].repeat;

    if (current_time > time)
    {
        if (repeat == SCHEDULE_REPEAT_DISABLE)
        {
            repeat = -1;
            return -1;
        }
        do
        {
            time += repeat;
        } while (current_time > time);
    }
    s_times[task_num].time = time;
    return time;
}

void timer_stop()
{
    if (schedule_timer != NULL)
    {
        esp_timer_stop(schedule_timer);
        esp_timer_delete(schedule_timer);
        schedule_timer = NULL;
    }
}

time_t get_current_time()
{
    struct timeval tv_now;
    gettimeofday(&tv_now, NULL);
    return tv_now.tv_sec;
}

static void schedule_timer_callback(void *arg)
{
    int64_t time_since_boot = esp_timer_get_time();
    ESP_LOGI("Timer", "Periodic timer called, time since boot: %lld us", time_since_boot);
    schedule_timer_handler();
    printf("Current time: %ld\n", triggered_last);
    schedule_reset();
}

int schedule_repeat_to_interval(schedule_repeat_t repeat)
{
    switch (repeat)
    {
    case 0:
        return 0;
    case 1:
        return 604800;
    case 2:
        return 86400;
    default:
        return -1;
    }
}

void schedule_set_timer()
{
    //Find nearest time
    size_t i;
    time_t min_time;
    for (i = 0; i < times_count; i++)
    {
        if (s_times[i].repeat != -1)
        {
            min_time = s_times[i].time;
            break;
        }
    }
    if (i >= times_count)
        return;
    for (i++; i < times_count; i++)
    {
        if (s_times[i].repeat != -1 && min_time > s_times[i].time)
        {
            min_time = s_times[i].time;
        }
    }
    //Setup timer
    const esp_timer_create_args_t timer_args = {
        .callback = &schedule_timer_callback,
        /* name is optional, but may help identify the timer when debugging */
        .name = "once"};
    min_time -= get_current_time();
    printf("Start timer on: %lu\n", min_time);
    ESP_ERROR_CHECK(esp_timer_create(&timer_args, &schedule_timer));
    ESP_ERROR_CHECK(esp_timer_start_once(schedule_timer, min_time * 1e6));
}

void schedule_correct()
{
    timer_stop();
    time_t now = get_current_time();
    if (triggered_last > now)
    {
        now = triggered_last;
    }

    for (size_t i = 0; i < times_count; i++)
    {
        time_t time = s_times[i].time;
        int repeat = s_times[i].repeat;
        if (repeat < 1)
        {
            s_times[i].repeat = 0;
        }
        else
        {
            while (now < time)
            {
                time -= repeat;
            }
            s_times[i].time = time;
        }
        task_update(i, now);
    }
    schedule_set_timer();
}

void schedule_reset()
{
    timer_stop();
    time_t now = get_current_time();
    triggered_last = now;

    for (size_t i = 0; i < times_count; i++)
    {
        ESP_LOGI("TIMER", "%d", s_times[i].repeat);
        task_update(i, now);
    }
    schedule_set_timer();
}

schedule_task_t *schedule_get_tasks(size_t *length)
{
    *length = times_count;
    if (times_count == 0)
        return NULL;
    schedule_task_t *result = (schedule_task_t *)malloc(times_count * sizeof(schedule_task_t));
    for (size_t i = 0; i < times_count; i++)
    {
        result[i] = s_times[i];
        if (result[i].repeat == -1)
        {
            result[i].repeat = 0;
        }
        else if (result[i].repeat == 86400)
        {
            result[i].repeat = 2;
        }
        else if (result[i].repeat == 604800)
        {
            result[i].repeat = 1;
        }
    }
    return result;
}

void schedule_init(const schedule_task_t tasks[], size_t size, void (*handler)())
{
    if (size == 0)
        return;
    schedule_timer_handler = handler;
    time_t now = get_current_time();
    times_count = size;
    s_times = (schedule_task_t *)malloc(size * sizeof(schedule_task_t));
    for (size_t i = 0; i < size; i++)
    {
        s_times[i].time = tasks[i].time;
        s_times[i].repeat = schedule_repeat_to_interval(tasks[i].repeat);
        task_update(i, now);
    }
    schedule_set_timer();
}

void schedule_deinit()
{
    timer_stop();
    free(s_times);
}