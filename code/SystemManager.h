#ifndef SYS_MANAGER_H
#define SYS_MANAGER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>

#include "NetworkServices.h"

/* ENUMs */

typedef enum
{
    NETWORK,
    SERIAL
}TerminalCallTyple;//what connection is calling terminal

//initialise
void SM_init();
//call periodicly to monitor sensors and switches
void SM_MonitorFunction(void);
//terminal interface handler
void SM_TerminalServ(int(*reply)(uint8_t*,uint16_t), uint8_t* newString, uint16_t newStringLength);
#endif