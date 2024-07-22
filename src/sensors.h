#ifndef __SENSORS_H_
#define __SENSORS_H_

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/kernel.h>

void sensors_init(uint16_t quantity, struct k_msgq * sensors_cmd_msgq, struct k_msgq * sensors_data_msgq);
void sensors_data_update_task(void);
void sensors_handle_cmd_task(void);

#endif /* __SENSORS_H_ */