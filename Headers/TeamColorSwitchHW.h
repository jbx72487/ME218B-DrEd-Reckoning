
#ifndef TeamColorSwitchHW_H
#define TeamColorSwitchHW_H

#include <stdint.h>

typedef enum {RED, BLUE} TeamColor_t;

void InitTeamColorSwitchHW (void);
TeamColor_t ReadTeamColor( void); 

#endif // TeamColorSwitchHW_H
