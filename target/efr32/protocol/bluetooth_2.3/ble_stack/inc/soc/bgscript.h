#ifndef BGSCRIPT_H
#define BGSCRIPT_H

#include <stdint.h>

void bgscript_init(const uint8_t * script,uint32_t variable_sz,uint32_t stack_sz,uint8_t *variable_ptr,uint8_t *stack_ptr);
void bgscript_run_event(uint8_t len1,const uint8_t *stack1,uint16_t len2,const uint8_t*stack2);


#endif
