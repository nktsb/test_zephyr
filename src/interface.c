#include "interface.h"
#include <zephyr/drivers/uart.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/printk.h>

static const struct device *uart_dev = DEVICE_DT_GET(DT_NODELABEL(usart2));


static void uart_cb(const struct device *dev, void *user_data)
{
	if (!uart_irq_update(uart_dev)) return;
	if (!uart_irq_rx_ready(uart_dev)) return;

    uint8_t rx_byte = 0;

    uart_fifo_read(uart_dev, &rx_byte, 1);

    printk("Uart callback!\r\n");
}

void interface_init(void)
{
    if (!uart_dev) {
        printk("Cannot find UART device!\n");
        return;
    }

    uart_irq_callback_set(uart_dev, uart_cb);
    uart_irq_rx_enable(uart_dev);
}