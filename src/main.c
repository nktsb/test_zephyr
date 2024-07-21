#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include "interface.h"
#include "sensors.h"

#define STACK_SIZE  2048U
#define PRIORITY    7

#define DEFAULT_SENSORS_QTY 10

#define CMD_QUEUE_SIZE  16

K_MSGQ_DEFINE(sensors_cmd_queue, sizeof(sensor_cmd_t), CMD_QUEUE_SIZE, 4);

static void interface_thread(void)
{
    interface_init(&sensors_cmd_queue);

    for(;;)
    {
        interface_parse_task();
        // printk("Interface thread\r\n");
        k_sleep(K_MSEC(1));
    }

}

static void sensors_thread(void)
{
	sensors_init(DEFAULT_SENSORS_QTY, &sensors_cmd_queue);

    for(;;)
    {
        sensors_data_update_task();
        sensors_handle_cmd_task();
        // printk("Sensors thread\r\n");
        k_sleep(K_MSEC(1));
    }
}

int main(void)
{
	return 0;
}

K_THREAD_DEFINE(interface_thread_id, STACK_SIZE, interface_thread, NULL, NULL, NULL,
		PRIORITY, 0, 0);
K_THREAD_DEFINE(sensors_thread_id, STACK_SIZE, sensors_thread, NULL, NULL, NULL,
		PRIORITY, 0, 0);
