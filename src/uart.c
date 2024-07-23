#include "uart.h"
#include <zephyr/drivers/uart.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(logger);

#define RX_BUFFER_SIZE     512
#define TX_BUFFER_SIZE     1024

static const struct device *uart_dev = DEVICE_DT_GET(DT_NODELABEL(usart2));

static struct ring_buf uart_rx_buffer = {0};
static uint8_t uart_rx_buffer_data[RX_BUFFER_SIZE] = {0};

static struct ring_buf uart_tx_buffer = {0};
static uint8_t uart_tx_buffer_data[TX_BUFFER_SIZE] = {0};

const struct uart_config uart_cfg = {
    .baudrate = 115200,
    .parity = UART_CFG_PARITY_NONE,
    .stop_bits = UART_CFG_STOP_BITS_1,
    .data_bits = UART_CFG_DATA_BITS_8,
    .flow_ctrl = UART_CFG_FLOW_CTRL_NONE
};

static void uart_cb(const struct device *dev, void *user_data)
{
	if(!uart_irq_update(dev)) return;

    if(uart_irq_rx_ready(dev))
    {
        uint8_t rx_byte = 0;
        uart_fifo_read(dev, &rx_byte, 1);
        ring_buf_put(&uart_rx_buffer, &rx_byte, 1);
    }
    if(uart_irq_tx_ready(dev))
    {
        uint8_t tx_byte = 0;
        if(ring_buf_get(&uart_tx_buffer, &tx_byte, 1) == 0)
            uart_irq_tx_disable(dev);
        else
            uart_fifo_fill(dev, &tx_byte, 1);
    }
    LOG_DBG("Uart callback\r\n");
}

void uart_init(void)
{
    uart_configure(uart_dev, &uart_cfg);

    if (!device_is_ready(uart_dev)) {
        LOG_ERR("Cannot find UART device!\n");
        return;
    }
    uart_irq_callback_set(uart_dev, uart_cb);

    ring_buf_init(&uart_rx_buffer, RX_BUFFER_SIZE, uart_rx_buffer_data);
    ring_buf_init(&uart_tx_buffer, TX_BUFFER_SIZE, uart_tx_buffer_data);

    uart_irq_rx_enable(uart_dev);
}

bool uart_is_rx_data(void)
{
    unsigned int key = irq_lock();
    bool res = !ring_buf_is_empty(&uart_rx_buffer);
    irq_unlock(key);
    return res;
}

uint8_t uart_get_byte(void)
{
    uint8_t rx_byte = 0;
    unsigned int key = irq_lock();
    ring_buf_get(&uart_rx_buffer, &rx_byte, 1);
    irq_unlock(key);
    return rx_byte;
}

void uart_send_byte(uint8_t byte)
{
    unsigned int key = irq_lock();
    ring_buf_put(&uart_tx_buffer, &byte, 1);
    irq_unlock(key);
    uart_irq_tx_enable(uart_dev);
    k_sleep(K_USEC(10));
}