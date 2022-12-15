
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

#ifndef NETWORK_SERV_H
#define NETWORK_SERV_H

/* Task */
#define DHCP_TASK_STACK_SIZE 2048
#define DHCP_TASK_PRIORITY 7

#define DNS_TASK_STACK_SIZE 512
#define DNS_TASK_PRIORITY 9

/* Socket */
#define SOCKET_DHCP 0
#define SOCKET_DNS 3

/* Retry count */
#define DHCP_RETRY_COUNT 5
#define DNS_RETRY_COUNT 5

/* DHCP */
static uint8_t g_DHCP_ethernet_buf[1024 * 2] = {
    0,
};

/* DNS */
static uint8_t g_dns_target_domain[] = "www.wiznet.io";
static uint8_t g_dns_target_ip[4] = {
    0,
};

#define NETSERV_TASK_STACK_SIZE 2048
#define NETSERV_TASK_PRIORITY 9
/* ENUMs */

typedef enum
{
    READY,
    WAITING
}dataReady;

typedef enum{
    NOT_SET,
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
    Success, //msg sent without issue
    Fail_config, //error, send doesnot match socket config
    Fail_send,  //error, fail to send to wiznet hardware
    Fail_closed //error, conection not estabished
}errorStatus;

/* Structs */

typedef struct
{
    int8_t socketNumber;
    socketType connectType; //closed or set to packet type
    connectState connectStatus; //is TCP client/host connected

    uint8_t sourceAddr[4];//source address from UDP packet
    uint16_t sourcePort;//socket port
    
    uint8_t * RXmsgbuff;//active write buffer address
    uint16_t RXmsgLen;//buffer size
    uint16_t RXbuffUsed;//buffer used
    uint16_t RXdataRemaining;//data still in socket
    dataReady RXdataReady; // data ready for App
    xSemaphoreHandle RXdataSema; //(optional for waking app for new data)

    uint8_t *destAddr;//destination address for UDP
    uint16_t destPort;//destination port for UDP
    uint8_t * TXmsgbuff;// message for transmition
    uint16_t TXmsgLen;//message size
    dataReady TXdataReady; // data ready from App

}socketConfig;

/* common variables */

static xSemaphoreHandle NetworkDataReady;
static socketConfig socketSet[8];


/* functions */


void NetworkService_init();
void NetworkService_task(void *argument);
void NetworkService_AwakeTask_ISR();
socketConfig* NetworkService_getSoc(int socNum, socketType conTyp, uint16_t port, uint8_t * RXbuff, uint16_t buffLen, xSemaphoreHandle RXdSema);
void NetworkService_releaseSoc(socketConfig *netSocket);
errorStatus NetworkService_TryConnectTCPclient(socketConfig *netSocket, uint8_t *hostAddr, uint16_t hostPort);
errorStatus NetworkService_TrySendTCP(socketConfig *netSocket, uint8_t * newTXmsg,uint16_t newTXmsgLen);
errorStatus NetworkService_TrySendUDP(socketConfig *netSocket, uint8_t *destAddr, uint16_t destPort, uint8_t * newTXmsg,uint16_t newTXmsgLen);
void NetworkService_SetIntMask(uint8_t);
uint8_t NetworkService_GetIntMask(void);




/* DNS and DHCP */

extern xSemaphoreHandle dns_sem;
extern xSemaphoreHandle DHCP_sem;
extern uint8_t g_dhcp_get_ip_flag;

extern wiz_NetInfo g_net_info;

/* DHCP */
void wizchip_dhcp_init(void);
void wizchip_dhcp_assign(void);
void wizchip_dhcp_conflict(void);
void dhcp_task(void *argument);
void dns_task(void *argument);
#endif