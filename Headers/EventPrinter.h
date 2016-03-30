#ifndef EVENT_PRINTER_H
#define EVENT_PRINTER_H

#include "ES_Configure.h"
#include "ES_Types.h"


bool InitEventPrinter(uint8_t Priority);
bool PostEventPrinter(ES_Event ThisEvent);
ES_Event RunEventPrinter(ES_Event ThisEvent);

#endif // EVENT_PRINTER_H
