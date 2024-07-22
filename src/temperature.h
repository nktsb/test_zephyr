#ifndef __TEMPERATURE_H_
#define __TEMPERATURE_H_

#include <stdint.h>
#include <stdlib.h>

void temp_sensor_init(void);
int16_t temp_sensor_read(void);

#endif /* __TEMPERATURE_H_ */