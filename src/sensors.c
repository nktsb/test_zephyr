#include "sensors.h"
#include "temperature.h"
#include <zephyr/kernel.h>
#include <zephyr/sys/mutex.h>

#define SENSORS_MAX_QTY     256
#define SENSORS_MIN_PER_MS  100
#define SENSORS_MAX_PER_MS  2000

#define SENSORS_DEF_PER_MS  1000

static int16_t (*def_get_val_func)() = temp_sensor_read;

K_MUTEX_DEFINE(sensors_mtx);

typedef struct sensor {

    int64_t period_ms;
    int64_t last_read_stamp_ms;
    int16_t last_value;

    int16_t (*get_val_func)();

} sensor_st;

typedef struct sensors_arr {
    sensor_st *sensors;
    uint16_t quantity;
} sensors_arr_st;

static sensors_arr_st sensors_array = {0};

static inline void sensors_array_init(uint16_t quantity, 
        int64_t def_timeout, int16_t (*get_val_func)() )
{
    k_mutex_lock(&sensors_mtx, K_FOREVER);
    sensors_array.quantity = quantity;
    sensors_array.sensors = calloc(quantity, sizeof(sensor_st));
    int64_t current_time_ms = k_uptime_get();

    for(sensor_st *sensor = sensors_array.sensors; 
            sensor < (sensors_array.sensors + quantity); sensor++)
    {
        sensor->get_val_func = get_val_func;
        sensor->period_ms = def_timeout;
        sensor->last_value = 0;
        sensor->last_read_stamp_ms = current_time_ms;
    }
    k_mutex_unlock(&sensors_mtx);
}

void sensors_change_period(uint16_t sensor_idx, uint16_t period)
{
    k_mutex_lock(&sensors_mtx, K_FOREVER);

    if(sensor_idx >= sensors_array.quantity)
    {
        printk("Wrong sensor idx %d\r\n", sensor_idx);
        k_mutex_unlock(&sensors_mtx);
        return;
    }

    if(period < SENSORS_MIN_PER_MS) period = SENSORS_MIN_PER_MS;
    if(period > SENSORS_MAX_PER_MS) period = SENSORS_MAX_PER_MS;

    sensors_array.sensors[sensor_idx].period_ms = period;

    printk("Changed sensor %d period to %d\r\n", sensor_idx, period);

    k_mutex_unlock(&sensors_mtx);
}

void sensors_change_quantity(uint16_t new_qty)
{
    if(new_qty > SENSORS_MAX_QTY) new_qty = SENSORS_MAX_QTY;

    k_mutex_lock(&sensors_mtx, K_FOREVER);
    sensor_st *new_sensors = calloc(new_qty, sizeof(sensor_st));
    memcpy(new_sensors, sensors_array.sensors, (sensors_array.quantity * sizeof(sensor_st)));
    int64_t current_time_ms = k_uptime_get();

    for(sensor_st *sensor = &new_sensors[sensors_array.quantity]; 
            sensor < (&new_sensors[sensors_array.quantity] + new_qty); sensor++)
    {
        sensor->get_val_func = def_get_val_func;
        sensor->period_ms = SENSORS_DEF_PER_MS;
        sensor->last_value = 0;
        sensor->last_read_stamp_ms = current_time_ms;
    }

    free(sensors_array.sensors);
    sensors_array.sensors = new_sensors;
    sensors_array.quantity = new_qty;
    k_mutex_unlock(&sensors_mtx);

    printk("Changed sensor qty to %d\r\n", new_qty);
}

void sensors_init(uint16_t quantity)
{
    temp_sensor_init();
    sensors_array_init(quantity, SENSORS_DEF_PER_MS, def_get_val_func);
    k_mutex_init(&sensors_mtx);
}

void sensors_data_update_task(void)
{
    k_mutex_lock(&sensors_mtx, K_FOREVER);
    int64_t current_time_ms = k_uptime_get();

    for(sensor_st *sensor = sensors_array.sensors; 
            sensor < (sensors_array.sensors + sensors_array.quantity); sensor++)
    {
        if(current_time_ms - sensor->last_read_stamp_ms >= sensor->period_ms)
        {
            sensor->last_value = sensor->get_val_func();
            sensor->last_read_stamp_ms = current_time_ms;
        }
    }
    k_mutex_unlock(&sensors_mtx);
}