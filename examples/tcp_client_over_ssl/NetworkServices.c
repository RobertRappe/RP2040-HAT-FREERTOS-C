#ifndef NETWORK_SERV_C
#define NETWORK_SERV_C
#include "NetworkServices.h"


xSemaphoreHandle dns_sem = NULL;
uint8_t g_dhcp_get_ip_flag = 0;

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
static void wizchip_dhcp_init(void);
static void wizchip_dhcp_assign(void);
static void wizchip_dhcp_conflict(void);

/* DHCP */
static void wizchip_dhcp_init(void)
{
    printf(" DHCP client running\n");

    DHCP_init(SOCKET_DHCP, g_DHCP_ethernet_buf);

    reg_dhcp_cbfunc(wizchip_dhcp_assign, wizchip_dhcp_assign, wizchip_dhcp_conflict);

    g_dhcp_get_ip_flag = 0;
}

static void wizchip_dhcp_assign(void)
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

static void wizchip_dhcp_conflict(void)
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

                xSemaphoreGive(dns_sem);
            }
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

/* Shaired Listener Task */
static WizIntFlag RXintSat;
static socSet socketSet[8];

void shairedListener_task(void *argument)
{
    
    uint16_t reg_val;
    RXintSat = WizStandby;

    for(int i = 0; i<8;i++)
    {
        socketSet[i].socState = inactive;
        socketSet[i].buffReady = DataClear;
    }



    while(1)
    {

        uint16_t reg_val;
        ctlwizchip(CW_GET_INTERRUPT, (void *)&reg_val);
        reg_val = (reg_val >> 8) & 0x00FF;
        if(reg_val == 0)
        {
            xSemaphoreTake(recv_sem, portMAX_DELAY);
            
        }else
        {
            uint8_t i = 0;
            while(i<8)
            {
                if((reg_val & (1 << i))>0)
                {
                    uint16_t recv_len =0;
                    switch(socketSet[i].socState )
                    {
                        case inactive:
                        //not in use todo?: flush socket?
                        break;
                        case isUDP:
                            reg_val = (SIK_CONNECTED | SIK_DISCONNECTED | SIK_RECEIVED | SIK_TIMEOUT) & 0x00FF; // except SIK_SENT(send OK) interrupt

                            ctlsocket(i, CS_CLR_INTERRUPT, (void *)&reg_val);
                            getsockopt(i, SO_RECVBUF, (void *)&recv_len);
                            
                            if(recv_len > socketSet[i].buffLen)
                            {
                                socketSet[i].buffUsed = socketSet[i].buffLen;
                                socketSet[i].dataRemaining = recv_len > socketSet[i].buffLen;
                            }
                            else
                            {
                                socketSet[i].buffUsed = recv_len;
                                socketSet[i].dataRemaining = 0;
                            }
                            recvfrom(i,socketSet[i].socRXbuff,socketSet[i].buffUsed,socketSet[i].sourceAddr,&socketSet[i].sourcePort);
                            socketSet[i].buffReady = DataReady;
                        break;
                        case isTCPcli:
                        //if connected!
                            reg_val = (SIK_CONNECTED | SIK_DISCONNECTED | SIK_RECEIVED | SIK_TIMEOUT) & 0x00FF; // except SIK_SENT(send OK) interrupt

                            ctlsocket(i, CS_CLR_INTERRUPT, (void *)&reg_val);
                            getsockopt(i, SO_RECVBUF, (void *)&recv_len);

                            if(recv_len > socketSet[i].buffLen)
                            {
                                socketSet[i].buffUsed = socketSet[i].buffLen;
                                socketSet[i].dataRemaining = recv_len > socketSet[i].buffLen;
                            }
                            else
                            {
                                socketSet[i].buffUsed = recv_len;
                                socketSet[i].dataRemaining = 0;
                            }

                            recv(i, socketSet[i].socRXbuff, socketSet[i].buffUsed );
                            socketSet[i].buffReady = DataReady;
                        break;
                        case isTCPserv:
                            //connection control goes here
                            switch(getSn_SR(i))
                            {
		                        case SOCK_ESTABLISHED://if connected!
                                    socketSet[i].isConnected = Connected;
                                    reg_val = (SIK_CONNECTED | SIK_DISCONNECTED | SIK_RECEIVED | SIK_TIMEOUT) & 0x00FF; // except SIK_SENT(send OK) interrupt

                                    ctlsocket(i, CS_CLR_INTERRUPT, (void *)&reg_val);
                                    getsockopt(i, SO_RECVBUF, (void *)&recv_len);

                                    if(recv_len > socketSet[i].buffLen)
                                    {
                                    socketSet[i].buffUsed = socketSet[i].buffLen;
                                    socketSet[i].dataRemaining = recv_len > socketSet[i].buffLen;
                                    }
                                    else
                                    {
                                        socketSet[i].buffUsed = recv_len;
                                       socketSet[i].dataRemaining = 0;
                                    }
                                    recv(i, socketSet[i].socRXbuff, socketSet[i].buffUsed );
                                    socketSet[i].buffReady = DataReady;
                                break;
		                        case SOCK_INIT:
                                socketSet[i].isConnected = NotConnected;
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
            }
        }


    }
}

connectState connectToClient(uint8_t socket,uint8_t* destAddr, uint16_t destPort)
{
    int retval = 0;
    retval = connect(socket, destAddr, destPort);
    if (retval == SOCK_OK)
    {

        socketSet[socket].isConnected = Connected;
        return Connected;
    }
    else 
    {

        socketSet[socket].isConnected = NotConnected;
        return NotConnected;
    }
}

TXreturnStatus sendData(uint8_t socket,uint8_t* destAddr, uint16_t destPort, uint8_t * TXbuff, uint16_t TXlength)
{
    int retval = 0;
    switch(socketSet[socket].socState )
    {
        case inactive:
            return TXFail_config;
        break;//yeah i know, its good coding practice
        case isUDP:
            retval = sendto(socket, TXbuff, TXlength,destAddr,destPort);
            if(retval <0)
            {
                return TXFail_send;
            }
            return TXSuccess;
        break;
        case isTCPcli:
        case isTCPserv:
            if(socketSet[socket].isConnected != Connected)
                return TXFail_closed;
            retval = send(socket, TXbuff, TXlength);
            if(retval <0)
            {
                return TXFail_send;
            }
            return TXSuccess;      
        break;

    }

}

socSet* setListener(uint8_t socket, socketType stype, uint16_t buffSize)
{
    socketSet[socket].buffLen = buffSize;
    socketSet[socket].socState = stype;
    socketSet[socket].buffUsed = 0;

    return socketSet;
}

void disableListener(uint8_t socket)
{
    socketSet[socket].socState = inactive;
    socketSet[socket].buffReady = DataClear;
}

//call from wiznet interupt handler
void WizDataReady()
{
    RXintSat = WizDataRec;
}
#endif