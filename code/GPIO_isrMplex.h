#ifndef GPIO_MPLEX_H
#define GPIO_MPLEX_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include "hardware/gpio.h"


void GPIO_isrMplex_init(void);
void GPIO_isrMplex_AddGPIOwatch(uint, uint32_t, gpio_irq_callback_t);//void (void*)(uint, uint32_t)
void GPIO_isrMplex_ClearGPIOwatch(uint);
void GPIO_isrMplex_Action(void);

#endif
