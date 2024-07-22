#include "sensors.h"
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include "temperature.h"
#include "interface.h"

#define SENSORS_MAX_QTY     256
#define SENSORS_MIN_PER_MS  100
#define SENSORS_MAX_PER_MS  2000

#define SENSORS_DEF_PER_MS  2000

static int16_t (*def_get_val_func)() = temp_sensor_read;

static const char *sensor_pckt_preamble = "SENS";

static struct k_msgq * sensors_cmd_queue = NULL;
static struct k_msgq * sensors_data_queue = NULL;

typedef enum {
    FORMAT_STRING,
    FORMAT_BINARY,

    FORMATS_QTY
} data_format_t;

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

static data_format_t sensors_data_format = FORMAT_STRING;

static inline void sensors_array_init(uint16_t quantity, 
        int64_t def_timeout, int16_t (*get_val_func)() )
{
    sensors_array.quantity = quantity;
    sensors_array.sensors = calloc(quantity, sizeof(sensor_st));
    if(sensors_array.sensors == NULL)
    {
        printk("Calloc failed: sensors array");
        return;
    }
    int64_t current_time_ms = k_uptime_get();

    for(sensor_st *sensor = sensors_array.sensors; 
            sensor < (sensors_array.sensors + quantity); sensor++)
    {
        sensor->get_val_func = get_val_func;
        sensor->period_ms = def_timeout;
        sensor->last_value = 0;
        sensor->last_read_stamp_ms = current_time_ms;
    }
}

static void sensors_send_string_packet(void)
{
    for(uint16_t idx = 0; idx < sensors_array.quantity; idx++)
    {
        sensor_st *sensor = &sensors_array.sensors[idx];
        char buffer[64] = {0};
        snprintk(buffer, sizeof(buffer), "%s%d:%d\r\n", sensor_pckt_preamble, idx, sensor->last_value);
        for(char *symbol = buffer; symbol < buffer + strlen(buffer); symbol++)
            k_msgq_put(sensors_data_queue, symbol, K_FOREVER);
    }
}

static void sensors_send_bin_packet(void)
{
    for(sensor_st *sensor = sensors_array.sensors; 
            sensor < (sensors_array.sensors + sensors_array.quantity); sensor++)
    {
        int16_t sensor_data = sensor->last_value;
        k_msgq_put(sensors_data_queue, (uint8_t*)&sensor_data, K_FOREVER);
        k_msgq_put(sensors_data_queue, (uint8_t*)&sensor_data + 1, K_FOREVER);
    }
}

static void sensors_change_period(uint16_t sensor_idx, uint16_t period)
{
    if(sensor_idx >= sensors_array.quantity)
    {
        printk("There is no sensor %d\r\n", sensor_idx);
        return;
    }

    if(period < SENSORS_MIN_PER_MS) period = SENSORS_MIN_PER_MS;
    if(period > SENSORS_MAX_PER_MS) period = SENSORS_MAX_PER_MS;

    sensors_array.sensors[sensor_idx].period_ms = period;

    printk("Changed sensor %d period to %d\r\n", sensor_idx, period);
}

static void sensors_change_quantity(uint16_t new_qty)
{
    if(new_qty > SENSORS_MAX_QTY) new_qty = SENSORS_MAX_QTY;
    if(new_qty == sensors_array.quantity)
    {
        printk("It's actual quantity\r\n");
        return;
    }

    sensor_st *new_sensors = calloc(new_qty, sizeof(sensor_st));
    if(new_sensors == NULL)
    {
        printk("Calloc failed: updating sensors qty");
        return;
    }

    if(new_qty > sensors_array.quantity)
    {
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
    }
    else
        memcpy(new_sensors, sensors_array.sensors, (new_qty * sizeof(sensor_st)));

    free(sensors_array.sensors);
    sensors_array.sensors = new_sensors;
    sensors_array.quantity = new_qty;

    printk("Changed sensor qty to %d\r\n", new_qty);
}

void sensors_init(uint16_t quantity, struct k_msgq * sensors_cmd_msgq, struct k_msgq * sensors_data_msgq)
{
    sensors_cmd_queue = sensors_cmd_msgq;
    sensors_data_queue = sensors_data_msgq;

    temp_sensor_init();
    sensors_array_init(quantity, SENSORS_DEF_PER_MS, def_get_val_func);
}

void sensors_data_update_task(void)
{
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
}

void sensors_handle_cmd_task(void)
{
    sensor_cmd_t cmd_buff = {0};
    if (k_msgq_get(sensors_cmd_queue, &cmd_buff, K_NO_WAIT) == 0)
    {
        switch(cmd_buff.cmd_type)
        {
            case CMD_PERIOD:
                sensors_change_period(cmd_buff.idx, cmd_buff.period);
                break;
            case CMD_QUANTITY:
                sensors_change_quantity(cmd_buff.idx);
                break;
            case CMD_READ:
                switch(sensors_data_format)
                {
                    case FORMAT_STRING:
                        sensors_send_string_packet();
                        break;
                    case FORMAT_BINARY:
                        sensors_send_bin_packet();
                        break;
                    default:
                        break;
                }
                break;
            case CMD_TOGGLE:
                sensors_data_format++;
                sensors_data_format %= FORMATS_QTY;
                switch(sensors_data_format)
                {
                    case FORMAT_STRING:
                        printk("Format toggled to string\r\n");
                        break;
                    case FORMAT_BINARY:
                        printk("Format toggled to bin\r\n");
                        break;
                    default:
                        break;
                }
                break;
            default:
                break;
        }
    }
}