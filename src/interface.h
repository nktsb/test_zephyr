#ifndef __INTERFACE_H_
#define __INTERFACE_H_

#include <stdint.h>
#include <stdbool.h>

void interface_init(void);
void interface_parse_task(void);
void interface_cmd_apply_task(void);

#endif /* __INTERFACE_H_ */