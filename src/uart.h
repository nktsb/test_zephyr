#ifndef __UART_H_
#define __UART_H_

#include <stdbool.h>
#include <stdint.h>

void uart_init(void);
bool uart_is_rx_data(void);
uint8_t uart_get_byte(void);
void uart_send_byte(uint8_t byte);

#endif /* __UART_H_ */