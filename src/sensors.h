#ifndef __SENSORS_H_
#define __SENSORS_H_

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

void sensors_init(uint16_t quantity);

void sensors_change_quantity(uint16_t new_qty);
void sensors_change_period(uint16_t sensor_idx, uint16_t period);

void sensors_data_update_task(void);
void sensors_change_quantity(uint16_t new_qty);

#endif /* __SENSORS_H_ */