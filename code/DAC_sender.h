#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pico/stdlib.h"
#include "hardware/dma.h"
#include "hardware/pio.h"
#include "hardware/structs/pio.h"
#include "pico/multicore.h"

#include "DACloader.pio.h"
#include "port_common.h"

#ifndef DAC_SENDER_H
#define DAC_SENDER_H


#define BUFFER_SIZE 20
#define SUB_BUFFER_SIZE 4


static const uint8_t RXpins[] = { 0, 1, 2, 3, 4 }; 

//variables
    uint16_t *buffer;
    uint16_t **sub_buffers;

    uint16_t BufferLength;
    bool *isSendDone; 

    uint16_t dma_chan;//DMA transfer channel for main buffer load
    uint16_t dma_chans[5];//DMA transfer channel for sub buffer loads

//functions
void DAC_sender_init(void);
void DAC_sender_dataReady(uint16_t *,uint16_t,bool*);//send buffer address, length, and pointer to conferm buffer cleared


#endif
