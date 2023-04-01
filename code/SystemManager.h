#ifndef SYS_MANAGER_H
#define SYS_MANAGER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>

#include "NetworkServices.h"

#include "bsp/board.h"
#include "tusb.h"

#include "pico/bootrom.h"
#include "hardware/flash.h"
#include "hardware/gpio.h"
#include "GPIO_isrMplex.h"
#include "ff.h"
//
#include "sd_card.h" /* Declarations of disk functions */
#include "spi.h"

/* ENUMs */
//extern tusb_desc_device_t usbd_desc_device;

typedef enum
{
    NETWORK,
    SERIAL
}TerminalCallType;//what connection is calling terminal

//initialise
void SM_init();
//call periodicly to monitor sensors and switches
//void SM_MonitorFunction(void);
//terminal interface handler
void SM_TerminalServ(int(*reply)(uint8_t*,uint16_t), uint8_t* newString, uint16_t newStringLength, TerminalCallType conType);

void usb_device_task(void*);
void cdc_task(void*);

//USB input handler
void USBwatcher_task(void*);


static xSemaphoreHandle dataready;
//network:UDP terminal handler
void NetworkUDPwatcher_task(void*);

//GPIO events
static xSemaphoreHandle GPIO_hndlr_UIhardwareEvent;
//static uint8_t GPIO_hndlr_SWTC_flag;
static uint GPIO_hndlr_SWTC_pin;
static uint32_t GPIO_hndlr_SWTC_event;
static uint32_t GPIO_hndlr_lastState;



//GPIO handler

void GPIO_hndlr_task(void*);
void GPIO_hndlr_set_LEDs(uint8_t);

//GPIO Pins
#define LED_BASE    7

#define LED_YEL     0
#define LED_GRN     1
#define LED_RED     2
#define LED_WHT     3
#define LED_BLU     4
#define LED_SWC     5

#define SWTC_ESTP     2
#define SWTC_BOXW     3
#define SWTC_BLSB     4
#define SWTC_BLSG     5
#define SWTC_TOGL     6

//flash storage
#define FLASHBLOCK_OFFSET   (256*1024)
static const char *flashBlock = (const char *) (XIP_BASE + FLASHBLOCK_OFFSET);

//SD card read/write/file system
// Hardware Configuration of SPI "objects"
// Note: multiple SD cards can be driven by one SPI if they use different slave
// selects.
static spi_t spis[] = {  // One for each SPI.
    {
        .hw_inst = spi1,  // SPI component
        .miso_gpio = 12,  // GPIO number (not Pico pin number)
        .mosi_gpio = 15,
        .sck_gpio = 14,

        // .baud_rate = 1000 * 1000
        .baud_rate = 12500 * 1000
        // .baud_rate = 25 * 1000 * 1000 // Actual frequency: 20833333.
    }};

// Hardware Configuration of the SD Card "objects"
static sd_card_t sd_cards[] = {  // One for each SD card
    {
        .pcName = "0:",   // Name used to mount device
        .spi = &spis[0],  // Pointer to the SPI driving this card
        .ss_gpio = 13,    // The SPI slave select GPIO for this SD card
        .use_card_detect = false,
        .card_detect_gpio = 13,  // Card detect
        .card_detected_true = 1  // What the GPIO read returns when a card is
                                 // present.
    }};

#endif