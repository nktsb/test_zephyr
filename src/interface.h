#ifndef __INTERFACE_H_
#define __INTERFACE_H_

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <zephyr/kernel.h>

typedef enum {
    CMD_UNDEF,      // NO_CMD
    CMD_READ,       // read sensors data "read\n"
    CMD_TOGGLE,     // switch data type "toggle\n"
    CMD_QUANTITY,   // change sensors quantity (up to 256) "quantity <q>\n", q - quantity
    CMD_PERIOD      // update sensor's period  "period <n> <p>\n" n - sensor, p - period
} cmd_t;

typedef struct sensor_cmd {
    cmd_t cmd_type;
    uint16_t idx;        // quantity of idx for period changing
    uint16_t period;
} sensor_cmd_t;

void interface_init(struct k_msgq * sensors_cmd_msgq);
void interface_parse_task(void);

#endif /* __INTERFACE_H_ */