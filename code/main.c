/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hardware/gpio.h"

#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>

#include "port_common.h"

#include "wizchip_conf.h"
#include "socket.h"
#include "w5x00_spi.h"
#include "w5x00_gpio_irq.h"

#include "timer.h"
#include "NetworkServices.h"

//#include "mbedtls/x509_crt.h"
//#include "mbedtls/error.h"
//#include "mbedtls/ssl.h"
//#include "mbedtls/ctr_drbg.h"
//#include "mbedtls/compat-1.3.h"

/* Clock */
#define PLL_SYS_KHZ (133 * 1000)
static void set_clock_khz(void);
/* Timer  */
static volatile uint32_t g_msec_cnt = 0;
static void repeating_timer_callback(void);

static xSemaphoreHandle dataready;

void test_task(void *argument)
{
    static uint8_t testDataBlock1[512];
    static uint16_t tdb1Len = 512;
    static uint8_t testDataBlock2[512];
    static uint16_t tdb2Len = 512;
    dataready = xSemaphoreCreateCounting((unsigned portBASE_TYPE)0x7fffffff, (unsigned portBASE_TYPE)0);
    socketConfig * netSocket = NULL;

    uint8_t remoteAddr[] = {0,0,0,0};
    uint16_t remotePort = 55601;

    while(netSocket == NULL)
    { 
        //vTaskDelay(100);
        //if(g_dhcp_get_ip_flag==1)
        xSemaphoreTake(DHCP_sem, portMAX_DELAY);//wait for DHCP
        xSemaphoreGive(DHCP_sem);//release once avalible
        vTaskDelay(100);
        netSocket = NetworkService_getSoc(2,UDP, 55600, testDataBlock1,tdb1Len,dataready);

        //TODO: timeout and error handling
    }

    while(1)
    {
        xSemaphoreTake(dataready, portMAX_DELAY);//wait for activity
        if(netSocket->RXdataReady == READY)
        {
            for(int i = 0; i<(netSocket->RXbuffUsed) ;i++)
            {
                testDataBlock2[i] = testDataBlock1[i];
            }
            remoteAddr[0] = netSocket->sourceAddr[0];
            remoteAddr[1] = netSocket->sourceAddr[1];
            remoteAddr[2] = netSocket->sourceAddr[2];
            remoteAddr[3] = netSocket->sourceAddr[3];
            netSocket->RXdataReady = WAITING;
            errorStatus error = NetworkService_TrySendUDP(netSocket, remoteAddr, remotePort, testDataBlock2, netSocket->RXbuffUsed);
        }

        

    }
}

void *pvPortCalloc(size_t sNb, size_t sSize)
{
    void *vPtr = NULL;

    if (sSize > 0)
    {
        vPtr = pvPortMalloc(sSize * sNb); // Call FreeRTOS or other standard API

        if (vPtr)
        {
            memset(vPtr, 0, (sSize * sNb)); // Must required
        }
    }

    return vPtr;
}

void pvPortFree(void *vPtr)
{
    if (vPtr)
    {
        vPortFree(vPtr); // Call FreeRTOS or other standard API
    }
}

/* GPIO */
static void gpio_callback(uint gpio, uint32_t events)
{

    NetworkService_AwakeTask_ISR();
}

int main() {
    /* Initialize */
    set_clock_khz();

    stdio_init_all();

    //mbedtls_platform_set_calloc_free(pvPortCalloc, pvPortFree);
    
    wizchip_spi_initialize();
    wizchip_cris_initialize();

    wizchip_reset();
    wizchip_initialize();
    wizchip_check();

    wizchip_1ms_timer_initialize(repeating_timer_callback);

    NetworkService_init();
    //start tasks
    xTaskCreate(dhcp_task, "DHCP_Task", DHCP_TASK_STACK_SIZE, NULL, DHCP_TASK_PRIORITY, NULL);
    //xTaskCreate(dns_task, "DNS_Task", DNS_TASK_STACK_SIZE, NULL, DNS_TASK_PRIORITY, NULL);
    xTaskCreate(NetworkService_task, "NetworkService_task", NETSERV_TASK_STACK_SIZE, NULL, NETSERV_TASK_PRIORITY, NULL);
    xTaskCreate(test_task, "test_task", NETSERV_TASK_STACK_SIZE, NULL, 8, NULL);
    NetworkService_SetIntMask(0b00000001);
    
    gpio_set_irq_enabled_with_callback(21,GPIO_IRQ_EDGE_FALL,true,gpio_callback);
    //dns_sem = xSemaphoreCreateCounting((unsigned portBASE_TYPE)0x7fffffff, (unsigned portBASE_TYPE)0);

    vTaskStartScheduler();

    while (1)
    {
        ;
    }
}
/* Clock */
static void set_clock_khz(void)
{
    // set a system clock frequency in khz
    set_sys_clock_khz(PLL_SYS_KHZ, true);

    // configure the specified clock
    clock_configure(
        clk_peri,
        0,                                                // No glitchless mux
        CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLKSRC_PLL_SYS, // System PLL on AUX mux
        PLL_SYS_KHZ * 1000,                               // Input frequency
        PLL_SYS_KHZ * 1000                                // Output (must be same as no divider)
    );
}
/* Timer */
static void repeating_timer_callback(void)
{
    g_msec_cnt++;

    if (g_msec_cnt >= 1000 - 1)
    {
        g_msec_cnt = 0;

        DHCP_time_handler();
        DNS_time_handler();
    }
}