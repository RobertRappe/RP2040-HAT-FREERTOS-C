#ifndef NETWORK_SERV_C
#define NETWORK_SERV_C
#include "NetworkServices.h"

uint8_t interuptmask;
void NetworkService_SetIntMask(uint8_t newMask)
{
    interuptmask = newMask;
}
uint8_t NetworkService_GetIntMask()
{
    return interuptmask;
}

xSemaphoreHandle dns_sem = NULL;
xSemaphoreHandle DHCP_sem = NULL;
uint8_t g_dhcp_get_ip_flag=0;

wiz_NetInfo g_net_info =
    {
        .mac = {0x00, 0x08, 0xDC, 0x12, 0x34, 0x56}, // MAC address
        .ip = {192, 168, 11, 2},                     // IP address
        .sn = {255, 255, 255, 0},                    // Subnet Mask
        .gw = {192, 168, 11, 1},                     // Gateway
        .dns = {8, 8, 8, 8},                         // DNS server
        .dhcp = NETINFO_DHCP                       // DHCP enable/disable
};

/* DHCP */
void wizchip_dhcp_init(void)
{
    printf(" DHCP client running\n");

    DHCP_init(SOCKET_DHCP, g_DHCP_ethernet_buf);

    reg_dhcp_cbfunc(wizchip_dhcp_assign, wizchip_dhcp_assign, wizchip_dhcp_conflict);

    g_dhcp_get_ip_flag = 0;
}

void wizchip_dhcp_assign(void)
{
    getIPfromDHCP(g_net_info.ip);
    getGWfromDHCP(g_net_info.gw);
    getSNfromDHCP(g_net_info.sn);
    getDNSfromDHCP(g_net_info.dns);

    g_net_info.dhcp = NETINFO_DHCP;

    /* Network initialize */
    network_initialize(g_net_info); // apply from DHCP

    print_network_information(g_net_info);
    printf(" DHCP leased time : %ld seconds\n", getDHCPLeasetime());
}

void wizchip_dhcp_conflict(void)
{
    printf(" Conflict IP from DHCP\n");

    // halt or reset or any...
    while (1)
    {
        vTaskDelay(1000 * 1000);
    }
}
/* Task */
void dhcp_task(void *argument)
{
    int retval = 0;
    uint8_t link;
    uint16_t len = 0;
    uint32_t dhcp_retry = 0;

    if (g_net_info.dhcp == NETINFO_DHCP) // DHCP
    {
        wizchip_dhcp_init();
    }
    else // static
    {
        network_initialize(g_net_info);

        /* Get network information */
        print_network_information(g_net_info);

        while (1)
        {
            vTaskDelay(1000 * 1000);
        }
    }

    while (1)
    {
        link = wizphy_getphylink();

        if (link == PHY_LINK_OFF)
        {
            printf("PHY_LINK_OFF\n");

            DHCP_stop();

            while (1)
            {
                link = wizphy_getphylink();

                if (link == PHY_LINK_ON)
                {
                    wizchip_dhcp_init();

                    dhcp_retry = 0;

                    break;
                }

                vTaskDelay(1000);
            }
        }

        retval = DHCP_run();

        if (retval == DHCP_IP_LEASED)
        {
            if (g_dhcp_get_ip_flag == 0)
            {
                dhcp_retry = 0;

                printf(" DHCP success\n");

                g_dhcp_get_ip_flag = 1;
                xSemaphoreGive(DHCP_sem);

                xSemaphoreGive(dns_sem);
            }
            vTaskDelay(100);
        }
        else if (retval == DHCP_FAILED)
        {
            g_dhcp_get_ip_flag = 0;
            dhcp_retry++;

            if (dhcp_retry <= DHCP_RETRY_COUNT)
            {
                printf(" DHCP timeout occurred and retry %d\n", dhcp_retry);
            }
        }

        if (dhcp_retry > DHCP_RETRY_COUNT)
        {
            printf(" DHCP failed\n");

            DHCP_stop();

            while (1)
            {
                vTaskDelay(1000 * 1000);
            }
        }

        vTaskDelay(10);
    }
}

void dns_task(void *argument)
{
    
    uint8_t dns_retry;

    while (1)
    {
        xSemaphoreTake(dns_sem, portMAX_DELAY);
        DNS_init(SOCKET_DNS, g_DHCP_ethernet_buf);

        dns_retry = 0;

        while (1)
        {
            if (DNS_run(g_net_info.dns, g_dns_target_domain, g_dns_target_ip) > 0)
            {
                printf(" DNS success\n");
                printf(" Target domain : %s\n", g_dns_target_domain);
                printf(" IP of target domain : %d.%d.%d.%d\n", g_dns_target_ip[0], g_dns_target_ip[1], g_dns_target_ip[2], g_dns_target_ip[3]);

                break;
            }
            else
            {
                dns_retry++;

                if (dns_retry <= DNS_RETRY_COUNT)
                {
                    printf(" DNS timeout occurred and retry %d\n", dns_retry);
                }
            }

            if (dns_retry > DNS_RETRY_COUNT)
            {
                printf(" DNS failed\n");

                break;
            }

            vTaskDelay(10);
        }
        
    }
}


void NetworkService_init()
{
    dns_sem = xSemaphoreCreateCounting((unsigned portBASE_TYPE)0x7fffffff, (unsigned portBASE_TYPE)0);
    DHCP_sem= xSemaphoreCreateCounting((unsigned portBASE_TYPE)0x7fffffff, (unsigned portBASE_TYPE)0);
    NetworkDataReady = xSemaphoreCreateCounting((unsigned portBASE_TYPE)0x7fffffff, (unsigned portBASE_TYPE)0);

    uint16_t Nreg_val = (SIK_CONNECTED | SIK_DISCONNECTED | SIK_RECEIVED | SIK_TIMEOUT); // except SendOK
    for(int i = 0; i<8;i++)
    {
        socketSet[i].socketNumber = i;
        socketSet[i].connectStatus = NotConnected;
        socketSet[i].connectType = NOT_SET;
        socketSet[i].RXdataReady = WAITING;
        socketSet[i].TXdataReady = WAITING;
        ctlsocket(i, CS_SET_INTMASK, (void *)&Nreg_val);
    }
    

}

void NetworkService_AwakeTask_ISR()
{
    signed portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;

    xSemaphoreGiveFromISR(NetworkDataReady, &xHigherPriorityTaskWoken);
    portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
}

bool dataSillWaiting()
{
    bool finished = true;

    for(int i = 0; i<8; i++)
    {
        if(socketSet[i].connectType != NOT_SET)
        {
            if((socketSet[i].RXdataReady == WAITING)&&(socketSet[i].RXdataRemaining>0))
                finished = false;
            if(socketSet[i].TXdataReady == READY)
                finished = false;
        }
    }
    return finished;
}

void NetworkService_task(void *argument)
{
    network_initialize(g_net_info);
    while(1)
    {
        if(!g_dhcp_get_ip_flag)
        {
            xSemaphoreTake(DHCP_sem, portMAX_DELAY);//wait for DHCP
            xSemaphoreGive(DHCP_sem);//release once avalible
            network_initialize(g_net_info);
        }
        uint16_t reg_val;
        ctlwizchip(CW_GET_INTERRUPT, (void *)&reg_val);
        reg_val = (reg_val >> 8) & 0x00FF & (~interuptmask);
        if((reg_val == 0)&&(dataSillWaiting()))//if wiznet has nothing else and there are no more TX
        {
            xSemaphoreTake(NetworkDataReady, portMAX_DELAY);//wait for activity
        }else
        {
            for(int i = 0; i<8; i++)
            {
                if((reg_val & (1 << i))>0)
                {
                    uint16_t recv_len =0;
                    switch(socketSet[i].connectType )
                    {
                        case NOT_SET:
                        //not in use todo?: flush socket?
                        break;
                        uint16_t Oreg_val = 0;
                        case UDP:
                            if(socketSet[i].RXdataReady == WAITING)
                            {
                                Oreg_val = (SIK_CONNECTED | SIK_DISCONNECTED | SIK_RECEIVED | SIK_TIMEOUT) & 0x00FF; // except SIK_SENT(send OK) interrupt

                                ctlsocket(i, CS_CLR_INTERRUPT, (void *)&Oreg_val);
                                getsockopt(i, SO_RECVBUF, (void *)&recv_len);
                                
                                if(recv_len > socketSet[i].RXmsgLen)
                                {
                                    socketSet[i].RXbuffUsed = socketSet[i].RXmsgLen;
                                    socketSet[i].RXdataRemaining = recv_len - socketSet[i].RXmsgLen;
                                }
                                else
                                {
                                    socketSet[i].RXbuffUsed = recv_len;
                                    socketSet[i].RXdataRemaining = 0;
                                }
                                recvfrom(i,socketSet[i].RXmsgbuff,socketSet[i].RXbuffUsed,socketSet[i].sourceAddr,&socketSet[i].sourcePort);
                                socketSet[i].RXdataReady = READY;
                                if(socketSet[i].RXdataSema != NULL)//if exists, realease
                                {
                                    xSemaphoreGive(socketSet[i].RXdataSema);
                                }
                            }
                        break;
                        case TCPc:
                        //if connected!
                            if(socketSet[i].RXdataReady == WAITING)
                            {
                                Oreg_val = (SIK_CONNECTED | SIK_DISCONNECTED | SIK_RECEIVED | SIK_TIMEOUT) & 0x00FF; // except SIK_SENT(send OK) interrupt

                                ctlsocket(i, CS_CLR_INTERRUPT, (void *)&Oreg_val);
                                getsockopt(i, SO_RECVBUF, (void *)&recv_len);

                                if(recv_len > socketSet[i].RXmsgLen)
                                {
                                    socketSet[i].RXbuffUsed = socketSet[i].RXmsgLen;
                                    socketSet[i].RXdataRemaining = recv_len - socketSet[i].RXmsgLen;
                                }
                                else
                                {
                                    socketSet[i].RXbuffUsed = recv_len;
                                    socketSet[i].RXdataRemaining = 0;
                                }

                                recv(i, socketSet[i].RXmsgbuff, socketSet[i].RXbuffUsed );
                                socketSet[i].RXdataReady = READY;
                                if(socketSet[i].RXdataSema != NULL)//if exists, realease
                                {
                                    xSemaphoreGive(socketSet[i].RXdataSema);
                                }
                            }
                        break;
                        case TCPs:
                            //connection control goes here
                            
                            switch(getSn_SR(i))
                            {
		                        case SOCK_ESTABLISHED://if connected!
                                
                                    if(socketSet[i].RXdataReady == WAITING)
                                    {
                                        socketSet[i].connectStatus = Connected;
                                        Oreg_val = (SIK_CONNECTED | SIK_DISCONNECTED | SIK_RECEIVED | SIK_TIMEOUT) & 0x00FF; // except SIK_SENT(send OK) interrupt

                                        ctlsocket(i, CS_CLR_INTERRUPT, (void *)&Oreg_val);
                                        getsockopt(i, SO_RECVBUF, (void *)&recv_len);

                                        if(recv_len > socketSet[i].RXmsgLen)
                                        {
                                            socketSet[i].RXbuffUsed = socketSet[i].RXmsgLen;
                                            socketSet[i].RXdataRemaining = recv_len - socketSet[i].RXmsgLen;
                                        }
                                        else
                                        {
                                            socketSet[i].RXbuffUsed = recv_len;
                                            socketSet[i].RXdataRemaining = 0;
                                        }

                                        recv(i, socketSet[i].RXmsgbuff, socketSet[i].RXbuffUsed );
                                        socketSet[i].RXdataReady = READY;

                                        if(socketSet[i].RXdataSema != NULL)//if exists, realease
                                        {
                                            xSemaphoreGive(socketSet[i].RXdataSema);
                                        }
                                    }
                                break;
		                        case SOCK_INIT:
                                    socketSet[i].connectStatus = NotConnected;
                                    listen(i);
			                    break;

		                        case SOCK_LISTEN:
			                    break;

		                        default :
			                    break;

                            }
                        break;
                    }
                }
                if((socketSet[i].connectType != NOT_SET)&&(socketSet[i].TXdataReady == READY))
                {
                    int retval = 0;
                    switch(socketSet[i].connectType )
                    {
                        case TCPc:
                        case TCPs:
                            retval = send(i, socketSet[i].TXmsgbuff, socketSet[i].TXmsgLen);
                        break;
                        case UDP:
                            retval = sendto(i, socketSet[i].TXmsgbuff, socketSet[i].TXmsgLen, socketSet[i].destAddr, socketSet[i].destPort);
                        break;
                    }
                    //TODO:error handling?
                    socketSet[i].TXdataReady = WAITING;
                }else
                {
                    ctlsocket(i, CS_CLR_INTERRUPT, (void *)&reg_val);//if interupt is pulled but no
                }


            }
        }


    }
}
socketConfig* NetworkService_getSoc(int socNum, socketType conTyp, uint16_t port, uint8_t * RXbuff, uint16_t buffLen, xSemaphoreHandle RXdSema)
{
    int num = -1;
    switch(conTyp)
    {
        case TCPc:
           num = socket(socNum, Sn_MR_TCP, port, SF_TCP_NODELAY);
        break;
        case TCPs:
           num = socket(socNum, Sn_MR_TCP, port, 0);
        break;
        case UDP:
           num = socket(socNum, Sn_MR_UDP, port, 0);
        break;
    }
    if(num != socNum)
    {
        socketSet[socNum].connectType = NOT_SET;
    }else
    {
        switch(conTyp)
        {
            case TCPc:
                socketSet[socNum].connectType = TCPc;
                socketSet[socNum].connectStatus = NotConnected;
                socketSet[socNum].RXdataReady = WAITING;
                socketSet[socNum].TXdataReady = WAITING;
                socketSet[socNum].RXmsgbuff = RXbuff;
                socketSet[socNum].RXmsgLen = buffLen;
                socketSet[socNum].RXdataSema = RXdSema;
            break;
            case TCPs:
                socketSet[socNum].connectType = TCPs;
                socketSet[socNum].connectStatus = NotConnected;
                socketSet[socNum].RXdataReady = WAITING;
                socketSet[socNum].TXdataReady = WAITING;
                socketSet[socNum].RXmsgbuff = RXbuff;
                socketSet[socNum].RXmsgLen = buffLen;
                socketSet[socNum].RXdataSema = RXdSema;
            break;
            case UDP:
                socketSet[socNum].connectType = UDP;
                socketSet[socNum].connectStatus = NotConnected;
                socketSet[socNum].RXdataReady = WAITING;
                socketSet[socNum].TXdataReady = WAITING;
                socketSet[socNum].RXmsgbuff = RXbuff;
                socketSet[socNum].RXmsgLen = buffLen;
                socketSet[socNum].RXdataSema = RXdSema;
            break;
        
        }
    }
    uint16_t mask;
    ctlwizchip(CW_GET_INTRMASK, (void *)&mask);
    mask |= ((1 << socNum) << 8);
    ctlwizchip(CW_SET_INTRMASK, (void *)&mask);
    return &socketSet[socNum];
}

void NetworkService_releaseSoc(socketConfig *netSocket)
{
    uint16_t mask;
    ctlwizchip(CW_GET_INTRMASK, (void *)&mask);
    mask &= ~((1 << netSocket->socketNumber) << 8);
    ctlwizchip(CW_SET_INTRMASK, (void *)&mask);
    close(netSocket->socketNumber);
    netSocket->connectStatus = NotConnected;
    netSocket->connectType = NOT_SET;
}

errorStatus NetworkService_TryConnectTCPclient(socketConfig *netSocket, uint8_t *hostAddr, uint16_t hostPort)
{
    if(netSocket->connectType == TCPc)
    {
        return Fail_config;
    }
    int num = -1;
    num = connect(netSocket->socketNumber, hostAddr, hostPort);
    if(num == SOCK_OK)
    {
        netSocket->connectStatus = Connected;
        return Success;
    }
    else
    {
        netSocket->connectStatus = NotConnected;
        return Fail_closed;
    }
}

errorStatus NetworkService_TrySendTCP(socketConfig *netSocket, uint8_t * newTXmsg,uint16_t newTXmsgLen)
{
    if((netSocket->connectType != TCPc)&&(netSocket->connectType != TCPs))
    {
        return Fail_config;
    }
    netSocket->TXmsgbuff = newTXmsg;
    netSocket->TXmsgLen = newTXmsgLen;
    netSocket->TXdataReady = READY;
    xSemaphoreGive(NetworkDataReady);
}


errorStatus NetworkService_TrySendUDP(socketConfig *netSocket, uint8_t *destinAddr, uint16_t destinPort, uint8_t * newTXmsg,uint16_t newTXmsgLen)
{
    if(netSocket->connectType != UDP)
    {
        return Fail_config;
    }
    netSocket->destAddr = destinAddr;
    netSocket->destPort = destinPort;
    netSocket->TXmsgbuff = newTXmsg;
    netSocket->TXmsgLen = newTXmsgLen;
    netSocket->TXdataReady = READY;
    xSemaphoreGive(NetworkDataReady);
}

//call after handling recived data
void NetworkService_RXhandled(socketConfig *netSocket)
{
    
    netSocket->RXdataReady = WAITING;
    xSemaphoreGive(NetworkDataReady);
}

/*formating tool functions*/

int setIPnum(uint8_t*addr,uint8_t block0,uint8_t block1,uint8_t block2,uint8_t block3)
{
    addr[0] = block0;
    addr[1] = block1;
    addr[2] = block2;
    addr[3] = block3;
    return 0;
}
#endif