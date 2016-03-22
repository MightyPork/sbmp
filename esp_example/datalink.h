#ifndef DATALINK_H
#define DATALINK_H

#include <esp8266.h>

void datalinkInit(void);

void datalink_receive(uint8_t byte);

#endif // DATALINK_H
