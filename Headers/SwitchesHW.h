
#ifndef SwitchesHW_H
#define SwitchesHW_H

#include <stdint.h>

typedef enum {SWITCH_0 = 0,
				SWITCH_2 = 1,
				SWITCH_4 = 2,
				SWITCH_6 = 3,
				SWITCH_8 = 4,
				SWITCH_10 = 5,
				SWITCH_CAM = 6
			} WhichSwitch_t;
			
typedef enum {SWITCH_OFF, SWITCH_ON} SwitchStatus_t;

void InitSwitchesHW (void);
SwitchStatus_t ReadSwitch(uint8_t whichSwitch);
#endif // SwitchesHW_H
