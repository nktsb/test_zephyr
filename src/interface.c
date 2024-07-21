#include "interface.h"
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/printk.h>
#include <zephyr/spinlock.h>
#include <zephyr/kernel.h>

#include "sensors.h"

#define RX_BUFFER_SIZE     512
#define CMD_LAST_SYMBOL    '\n'

static const struct device *uart_dev = DEVICE_DT_GET(DT_NODELABEL(usart2));

static struct k_msgq * sensors_cmd_queue = NULL;

struct k_spinlock rx_buffer_spinlock;
static struct ring_buf uart_rx_buffer = {0};
static uint8_t uart_rx_buffer_data[RX_BUFFER_SIZE] = {0};

static const char *read_cmd_preamble = "read";
static const char *toggle_cmd_preamble = "toggle";
static const char *quantity_cmd_preamble = "quantity ";
static const char *period_cmd_preamble = "period ";

struct command_parser {

    char buffer[16];
    uint8_t counter;

    sensor_cmd_t cmd;
};

static struct command_parser cmd_parser = {
    .buffer = {0},
    .counter = 0,
    .cmd = {
        .idx = 0,
        .period = 0,
        .cmd_type = CMD_UNDEF
    }
};

static void uart_cb(const struct device *dev, void *user_data)
{
	if (!uart_irq_update(uart_dev)) return;
	if (!uart_irq_rx_ready(uart_dev)) return;

    uint8_t rx_byte = 0;

    uart_fifo_read(uart_dev, &rx_byte, 1);

    k_spinlock_key_t key = k_spin_lock(&rx_buffer_spinlock);
    ring_buf_put(&uart_rx_buffer, &rx_byte, 1);
    k_spin_unlock(&rx_buffer_spinlock, key);
}

static void interface_reset_parser(void)
{
    memset((void*)cmd_parser.buffer, 0, sizeof(cmd_parser.buffer));
    cmd_parser.cmd.idx = 0;
    cmd_parser.cmd.period = 0;
    cmd_parser.cmd.cmd_type = CMD_UNDEF;
    cmd_parser.counter = 0;
}

void interface_parse_task(void)
{
    k_spinlock_key_t key = k_spin_lock(&rx_buffer_spinlock);
    if(!ring_buf_is_empty(&uart_rx_buffer))
    {
        uint8_t rx_byte = 0;
        ring_buf_get(&uart_rx_buffer, &rx_byte, 1);

        if(rx_byte == CMD_LAST_SYMBOL)
        {
            if(strncmp(cmd_parser.buffer, read_cmd_preamble, strlen(read_cmd_preamble)) == 0)
            {
                cmd_parser.cmd.cmd_type = CMD_READ;
            }
            else
            if(strncmp(cmd_parser.buffer, toggle_cmd_preamble, strlen(toggle_cmd_preamble)) == 0)
            {
                cmd_parser.cmd.cmd_type = CMD_TOGGLE;
            }
            else
            if(strncmp(cmd_parser.buffer, quantity_cmd_preamble, strlen(quantity_cmd_preamble)) == 0)
            {
                char *cmd_args = &cmd_parser.buffer[strlen(quantity_cmd_preamble)];
                char *endptr = NULL;
                cmd_parser.cmd.idx = strtol(cmd_args, &endptr, 10);
                cmd_parser.cmd.cmd_type = CMD_QUANTITY;
            }
            else
            if(strncmp(cmd_parser.buffer, period_cmd_preamble, strlen(period_cmd_preamble)) == 0)
            {
                char *cmd_args = &cmd_parser.buffer[strlen(period_cmd_preamble)];
                char *endptr = NULL;
                cmd_parser.cmd.idx = strtoul(cmd_args, &endptr, 10);

                if (*endptr == ' ')
                {
                    cmd_args = endptr + 1;
                    cmd_parser.cmd.period = strtoul(cmd_args, NULL, 10);
                }
                else
                    printk("Invalid period\r\n");

                cmd_parser.cmd.cmd_type = CMD_PERIOD;
            }
            else
            {
                printk("Wrong cmd\r\n");
                interface_reset_parser();
                return;
            }

            if (k_msgq_put(sensors_cmd_queue, &cmd_parser.cmd, K_NO_WAIT) != 0)
                printk("Failed to put cmd in queue\n");

            interface_reset_parser();
        }
        else if(cmd_parser.counter < sizeof(cmd_parser.buffer) - 1)
        {
            cmd_parser.buffer[cmd_parser.counter++] = rx_byte;
        }
        else
        {
            printk("Command buffer overflow!\r\n");
            interface_reset_parser();
        }

    }
    k_spin_unlock(&rx_buffer_spinlock, key);
}

void interface_init(struct k_msgq * sensors_cmd_msgq)
{
    if (!device_is_ready(uart_dev)) {
        printk("Cannot find UART device!\n");
        return;
    }

    sensors_cmd_queue = sensors_cmd_msgq;

    uart_irq_callback_set(uart_dev, uart_cb);
    uart_irq_rx_enable(uart_dev);

    ring_buf_init(&uart_rx_buffer, RX_BUFFER_SIZE, uart_rx_buffer_data);
}