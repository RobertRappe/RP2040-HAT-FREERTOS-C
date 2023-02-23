#ifndef NETWORK_SERV_H
#define NETWORK_SERV_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>

#include "port_common.h"

#include "socket.h"
#include "wizchip_conf.h"
#include "w5x00_spi.h"

#include "dhcp.h"
#include "dns.h"

#include "timer.h"

/* Task */
#define DHCP_TASK_STACK_SIZE 2048
#define DHCP_TASK_PRIORITY 8

#define DNS_TASK_STACK_SIZE 512
#define DNS_TASK_PRIORITY 10

/* Socket */
#define SOCKET_DHCP 0
#define SOCKET_DNS 3

/* Retry count */
#define DHCP_RETRY_COUNT 5
#define DNS_RETRY_COUNT 5

extern wiz_NetInfo g_net_info;


/* DHCP */
extern uint8_t g_dhcp_get_ip_flag;
static uint8_t g_DHCP_ethernet_buf[1024 * 2] = {
    0,
};

/* DNS */
static uint8_t g_dns_target_domain[] = "www.wiznet.io";
static uint8_t g_dns_target_ip[4] = {
    0,
};

/* Semaphore */
extern xSemaphoreHandle dns_sem;
static uint8_t g_dns_get_ip_flag = 0;

static xSemaphoreHandle recv_sem = NULL;
/* Task */
void dhcp_task(void *argument);
void dns_task(void *argument);



/* Timer  */
static void repeating_timer_callback(void);
#endif



/*  Shaired Listener Service Task   */

void shairedListener_task(void *argument);

typedef enum
{
    WizDataRec,
    WizStandby
} WizIntFlag;

typedef enum 
{
    UDP,
    TCPc,
    TCPs
}socketType;

typedef enum
{
    Connected,
    NotConnected
}connectState;
typedef enum 
{
    inactive,
    isUDP,
    isTCPcli,
    isTCPserv
}netSocState;
typedef enum 
{
    DataReady, //data in buffer for handling
    DataRW, //data is beingv read or writen to, wait
    DataClear //buffer is empty, ready to recive data
}socBuffStat;

typedef enum
{
    TXSuccess, //msg sent without issue
    TXFail_config, //error, send doesnot match socket config
    TXFail_send,  //error, fail to send to wiznet hardware
    TXFail_closed //error, conection not estabished
}TXreturnStatus;

typedef struct 
{
    netSocState socState;//is inactive/is UDP/is TCP client/is TCP serv
    connectState isConnected;//is connected to client/server or not
    uint8_t sourceAddr[4];//source address from UDP packet
    uint16_t sourcePort;//socket port
    uint8_t * socRXbuff;//active write buffer address
    uint16_t buffLen;//buffer size
    uint16_t buffUsed;//buffer used
    uint16_t dataRemaining;//data still in socket
    xSemaphoreHandle bufferSemi;//semiphore for buffer in use
    socBuffStat buffReady;//semiphore for data to handle
}socSet;

socSet* setListener(uint8_t socket, socketType stype, uint16_t buffSize);
void disableListener(uint8_t socket);
void WizDataReady(void);
connectState connectToClient(uint8_t socket,uint8_t* destAddr, uint16_t destPort);
TXreturnStatus sendData(uint8_t socket,uint8_t* destAddr, uint16_t destPort, uint8_t * TXbuff, uint16_t TXlength);
