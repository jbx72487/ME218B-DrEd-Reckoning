#ifndef Project_PWMlab
#define Project_PWMlab
#include <stdint.h>

void PWMInit(uint32_t freq);
void setDuty(int8_t dutyL, int8_t dutyR);

#endif
