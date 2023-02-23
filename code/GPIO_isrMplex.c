#ifndef GPIO_MPLEX_C
#define GPIO_MPLEX_C

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include "hardware/gpio.h"
#include "hardware/sync.h"
#include "hardware/irq.h"
#include "hardware/structs/iobank0.h"
#include "GPIO_isrMplex.h"

static gpio_irq_callback_t isrHandlerList[NUM_BANK0_GPIOS];

void GPIO_isrMplex_init(void)
{
    #ifndef IO_IRQ_BANK0
    #define IO_IRQ_BANK0 13
    #endif
    irq_set_exclusive_handler(IO_IRQ_BANK0, GPIO_isrMplex_Action);
    irq_set_enabled(IO_IRQ_BANK0, true);
}

void GPIO_isrMplex_AddGPIOwatch(uint gpio, uint32_t events, gpio_irq_callback_t newCaller)
{
    // Clear stale events which might cause immediate spurious handler entry
    gpio_acknowledge_irq(gpio, events);
    isrHandlerList[gpio] = newCaller;

    // Separate mask/force/status per-core, so check which core called, and
    // set the relevant IRQ controls.
    io_irq_ctrl_hw_t *irq_ctrl_base = get_core_num() ?
                                           &iobank0_hw->proc1_irq_ctrl : &iobank0_hw->proc0_irq_ctrl;

    io_rw_32 *en_reg = &irq_ctrl_base->inte[gpio / 8];
    events <<= 4 * (gpio % 8);
    uint32_t oldevents = irq_ctrl_base->inte[gpio / 8];//hw_get_bits(en_reg);
    events  |= oldevents;
    hw_set_bits(en_reg, events);

}

void GPIO_isrMplex_ClearGPIOwatch(uint gpio)
{
    // Clear stale events which might cause immediate spurious handler entry
    //gpio_acknowledge_irq(gpio, events);
    isrHandlerList[gpio] = NULL;

        // Separate mask/force/status per-core, so check which core called, and
    // set the relevant IRQ controls.
    io_irq_ctrl_hw_t *irq_ctrl_base = get_core_num() ?
                                           &iobank0_hw->proc1_irq_ctrl : &iobank0_hw->proc0_irq_ctrl;

    uint32_t events = 0xF;
    io_rw_32 *en_reg = &irq_ctrl_base->inte[gpio / 8];
    events <<= 4 * (gpio % 8);
    events = ~events; //create reverse mask to clear selected bits
    uint32_t oldevents = irq_ctrl_base->inte[gpio / 8];//hw_get_bits(en_reg);
    events  &= oldevents;
    hw_set_bits(en_reg, events);
}

void GPIO_isrMplex_Action()
{
    // if(isrHandlerList[gpio] != NULL)
    // {
    //     isrHandlerList[gpio](gpio,events);
    // }
    io_irq_ctrl_hw_t *irq_ctrl_base = get_core_num() ?
            &iobank0_hw->proc1_irq_ctrl : &iobank0_hw->proc0_irq_ctrl;

    for (uint gpio = 0; gpio < NUM_BANK0_GPIOS; gpio++) {
        io_ro_32 *status_reg = &irq_ctrl_base->ints[gpio / 8];
        uint events = (*status_reg >> 4 * (gpio % 8)) & 0xf;
        if (events) {
            // TODO: core agnostic?
            gpio_acknowledge_irq(gpio, events);
            gpio_irq_callback_t callback = isrHandlerList[gpio];
            if (callback) {
                callback(gpio, events);
            }
        }
    }
}

#endif