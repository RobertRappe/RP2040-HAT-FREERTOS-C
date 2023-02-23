#ifndef SYS_MANAGER_C
#define SYS_MANAGER_C
#include "SystemManager.h"
#include "NetworkServices.h"
#include "GPIO_isrMplex.h"



void SM_init()
{
    //set up GPIO monitoring and start up here
}

//call periodicly to monitor sensors and switches, place in periodic timer call
void SM_MonitorFunction()
{
    //poll GPIOS for changes
}
static char sendBuff[255];

void SM_TerminalServ(int(*reply)(uint8_t*,uint16_t), uint8_t* newString, uint16_t newStringLength)
{
    char *  pointer = strstr((char*)newString,"test");
    if(pointer != NULL){

        reply("Tested!\n", 8);
    }
    pointer = strstr((char*)newString,"getStatus");
    if(pointer != NULL){
        sprintf(sendBuff,"RX: addr:%d.%d.%d.%d", g_net_info.ip[0],
                    g_net_info.ip[1], g_net_info.ip[2], g_net_info.ip[3]);

        reply(sendBuff, sizeof(sendBuff));
    }
}
#endif