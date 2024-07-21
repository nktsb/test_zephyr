#include "interface.h"
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/printk.h>
#include <zephyr/spinlock.h>

#include "sensors.h"

#define RX_BUFFER_SIZE     512
#define CMD_LAST_SYMBOL    '\n'

static const struct device *uart_dev = DEVICE_DT_GET(DT_NODELABEL(usart2));

struct k_spinlock rx_buffer_spinlock;
static struct ring_buf uart_rx_buffer = {0};
static uint8_t uart_rx_buffer_data[RX_BUFFER_SIZE] = {0};

static const char *read_cmd_preamble = "read";
static const char *toggle_cmd_preamble = "toggle";
static const char *quantity_cmd_preamble = "quantity ";
static const char *period_cmd_preamble = "period ";

static const char *sensor_pckt_preamble = "SENS";

typedef enum {
    CMD_UNDEF,      // NO_CMD
    CMD_READ,       // read sensors data "read\n"
    CMD_TOGGLE,     // switch data type "toggle\n"
    CMD_QUANTITY,   // change sensors quantity (up to 256) "quantity <q>\n", q - quantity
    CMD_PERIOD      // update sensor's period  "period <n> <p>\n" n - sensor, p - period
} cmd_t;

typedef struct command_parser {

    char buffer[16];
    char *cmd_args;
    uint8_t counter;
    cmd_t cmd;
    
    bool new_cmd_flg;

} command_parser_st;

static command_parser_st cmd_parser = {
    .buffer = {0},
    .counter = 0,
    .cmd = CMD_UNDEF,
    .cmd_args = cmd_parser.buffer,
    .new_cmd_flg = false
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

    // printk("Uart callback!\r\n");
}

static void interface_reset_parser(void)
{
    memset((void*)cmd_parser.buffer, 0, sizeof(cmd_parser.buffer));
    cmd_parser.cmd_args = cmd_parser.buffer;
    cmd_parser.cmd = CMD_UNDEF;
    cmd_parser.new_cmd_flg = false;
    cmd_parser.counter = 0;
}

void interface_parse_task(void)
{
    k_spinlock_key_t key = k_spin_lock(&rx_buffer_spinlock);
    if(!ring_buf_is_empty(&uart_rx_buffer) && !cmd_parser.new_cmd_flg)
    {
        uint8_t rx_byte = 0;
        ring_buf_get(&uart_rx_buffer, &rx_byte, 1);

        if(rx_byte == CMD_LAST_SYMBOL)
        {
            if(strncmp(cmd_parser.buffer, read_cmd_preamble, strlen(read_cmd_preamble)) == 0)
            {
                cmd_parser.cmd = CMD_READ;
                cmd_parser.new_cmd_flg = true;
            }
            else
            if(strncmp(cmd_parser.buffer, toggle_cmd_preamble, strlen(toggle_cmd_preamble)) == 0)
            {
                cmd_parser.cmd = CMD_TOGGLE;
                cmd_parser.new_cmd_flg = true;
            }
            else
            if(strncmp(cmd_parser.buffer, quantity_cmd_preamble, strlen(quantity_cmd_preamble)) == 0)
            {
                cmd_parser.cmd_args = &cmd_parser.buffer[strlen(quantity_cmd_preamble)];

                cmd_parser.cmd = CMD_QUANTITY;
                cmd_parser.new_cmd_flg = true;
            }
            else
            if(strncmp(cmd_parser.buffer, period_cmd_preamble, strlen(period_cmd_preamble)) == 0)
            {
                cmd_parser.cmd_args = &cmd_parser.buffer[strlen(period_cmd_preamble)];

                cmd_parser.cmd = CMD_PERIOD;
                cmd_parser.new_cmd_flg = true;
            }
            else
            {
                printk("Wrong cmd\r\n");
                interface_reset_parser();
            }
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

void interface_cmd_apply_task(void)
{
    if(cmd_parser.new_cmd_flg)
    {
        char *endptr = NULL;
        switch(cmd_parser.cmd)
        {
            case CMD_READ:
                printk("Read cmd received\r\n");
                break;
            case CMD_TOGGLE:
                printk("Data format toggled\r\n");
                break;
            case CMD_QUANTITY:
                uint16_t qty = strtol(cmd_parser.cmd_args, &endptr, 10);
                sensors_change_quantity(qty);
                break;
            case CMD_PERIOD:
                uint16_t sensor = strtoul(cmd_parser.cmd_args, &endptr, 10);

                if (*endptr == ' ')
                {
                    cmd_parser.cmd_args = endptr + 1;
                    uint16_t period = strtoul(cmd_parser.cmd_args, NULL, 10);
                    sensors_change_period((uint16_t)sensor, (uint16_t)period);
                }
                else
                    printk("Invalid period\r\n");

                break;
            default:
                break;
        }

        interface_reset_parser();
    }
}

void interface_init(void)
{
    if (!device_is_ready(uart_dev)) {
        printk("Cannot find UART device!\n");
        return;
    }

    uart_irq_callback_set(uart_dev, uart_cb);
    uart_irq_rx_enable(uart_dev);

    ring_buf_init(&uart_rx_buffer, RX_BUFFER_SIZE, uart_rx_buffer_data);
}