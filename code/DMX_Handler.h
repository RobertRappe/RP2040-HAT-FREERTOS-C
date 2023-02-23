#ifndef DMX_HANDLER_H
#define DMX_HANDLER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include "hardware/pio.h"
#include "DMX_Handler_pio.pio.h"



/* functions */
//init DMX pio and handler, send start commands to config DMX responsive hardware
void DMX_Handler_init(void);
//set starting address to monitor
void DMX_Handler_SetAdder(uint16_t);

#endif