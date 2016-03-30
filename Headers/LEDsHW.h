
#ifndef LEDsHW_H
#define LEDsHW_H

#include <stdint.h>

typedef enum {RED_LED, BLUE_LED, CAMPAIGN_LED} WhichLED_t;

void InitLEDsHW (void);
void SetLEDOn(WhichLED_t led); 
void SetLEDOff(WhichLED_t led);

#endif // LEDsHW_H
