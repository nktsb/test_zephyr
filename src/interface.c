#include "interface.h"
#include <zephyr/sys/printk.h>
#include <zephyr/kernel.h>

#include "uart.h"
#include "sensors.h"

#define CMD_LAST_SYMBOL    '\n'

static struct k_msgq * sensors_cmd_queue = NULL;
static struct k_msgq * sensors_data_queue = NULL;

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
    if(uart_is_rx_data())
    {
        uint8_t rx_byte = uart_get_byte();

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
}

void interface_transmit_task(void)
{
    uint8_t tx_byte = 0;
    if (k_msgq_get(sensors_data_queue, &tx_byte, K_NO_WAIT) == 0)
        uart_send_byte(tx_byte);

}

void interface_init(struct k_msgq * sensors_cmd_msgq, struct k_msgq * sensors_data_msgq)
{
    uart_init();

    sensors_cmd_queue = sensors_cmd_msgq;
    sensors_data_queue = sensors_data_msgq;
}