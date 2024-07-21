#include "interface.h"
#include <zephyr/drivers/uart.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/printk.h>

#include "ring_buffer.h"

#define RX_BUFFER_SIZE     512
#define CMD_LAST_SYMBOL    '\n'

static const struct device *uart_dev = DEVICE_DT_GET(DT_NODELABEL(usart2));
static ring_buffer_st *uart_rx_buffer = NULL;

static const char *read_cmd_preamble = "read";
static const char *toggle_cmd_preamble = "toggle";
static const char *number_cmd_preamble = "number ";

typedef enum {
    CMD_UNDEF,  // NO_CMD
    CMD_READ,   // read sensors data
    CMD_TOGGLE, // switch data type
    CMD_NUMBER  // change sensors number (up to 256)
} cmd_t;

typedef struct command_parser {

    char buffer[16];
    uint8_t counter;
    cmd_t cmd;
    int cmd_arg;
    
    bool new_cmd_flg;

} command_parser_st;

static command_parser_st cmd_parser = {
    .buffer = {0},
    .counter = 0,
    .cmd = CMD_UNDEF,
    .new_cmd_flg = false
};

static void uart_cb(const struct device *dev, void *user_data)
{
	if (!uart_irq_update(uart_dev)) return;
	if (!uart_irq_rx_ready(uart_dev)) return;

    uint8_t rx_byte = 0;

    uart_fifo_read(uart_dev, &rx_byte, 1);
    ringBufferPutSymbol(uart_rx_buffer, &rx_byte);

    // printk("Uart callback!\r\n");
}

void interface_parse_task(void)
{
    if(ringBufferGetAvail(uart_rx_buffer) && !cmd_parser.new_cmd_flg)
    {
        uint8_t rx_byte = 0;
        ringBufferGetSymbol(uart_rx_buffer, &rx_byte);

        if(rx_byte == CMD_LAST_SYMBOL)
        {
            if(strcmp(cmd_parser.buffer, read_cmd_preamble) == 0)
            {
                cmd_parser.cmd = CMD_READ;
                cmd_parser.new_cmd_flg = true;
            }
            else
            if(strcmp(cmd_parser.buffer, toggle_cmd_preamble) == 0)
            {
                cmd_parser.cmd = CMD_TOGGLE;
                cmd_parser.new_cmd_flg = true;
            }
            else
            if(memcmp(cmd_parser.buffer, number_cmd_preamble, strlen(number_cmd_preamble)) == 0)
            {
                cmd_parser.cmd_arg = atoi(&cmd_parser.buffer[strlen(number_cmd_preamble)]);

                cmd_parser.cmd = CMD_NUMBER;
                cmd_parser.new_cmd_flg = true;
            }
            else
            {
                cmd_parser.cmd = CMD_UNDEF;
                cmd_parser.new_cmd_flg = false;
            }

            memset((void*)cmd_parser.buffer, 0, sizeof(cmd_parser.buffer));
            cmd_parser.counter = 0;
        }
        else
            cmd_parser.buffer[cmd_parser.counter++] = rx_byte;

    }

}

void interface_cmd_apply_task(void)
{
    if(cmd_parser.new_cmd_flg)
    {
        switch(cmd_parser.cmd)
        {
            case CMD_READ:
                printk("Read cmd received\r\n");
                break;
            case CMD_TOGGLE:
                printk("Data format toggled\r\n");
                break;
            case CMD_NUMBER:
                int number = cmd_parser.cmd_arg;
                printk("Changed sensor number to %d\r\n", number);
                break;
            default:
                break;
        }

        cmd_parser.new_cmd_flg = false;
        cmd_parser.cmd = CMD_UNDEF;
        cmd_parser.cmd_arg = 0;
    }
}

void interface_init(void)
{
    if (!uart_dev) {
        printk("Cannot find UART device!\n");
        return;
    }

    uart_irq_callback_set(uart_dev, uart_cb);
    uart_irq_rx_enable(uart_dev);

    uart_rx_buffer = ringBufferInit(sizeof(uint8_t), RX_BUFFER_SIZE);
}