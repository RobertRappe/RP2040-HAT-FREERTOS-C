#ifndef SYS_MANAGER_C
#define SYS_MANAGER_C
#include "SystemManager.h"
#include "NetworkServices.h"
#include "GPIO_isrMplex.h"

static int USB_reply(uint8_t*,uint16_t);

/* GPIO */
static void SM_gpio_callback(uint gpio, uint32_t events)
{
  
    signed portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
  //uint8_t flagScratch = 0;
    //check Estop and apply rapid safety shutdown if needed
    if((gpio == SWTC_ESTP)&&((events&GPIO_IRQ_EDGE_FALL)>0))
    {
        //rapid stop code here
        
    }
    GPIO_hndlr_SWTC_pin = gpio;
    GPIO_hndlr_SWTC_event = events;
    
    xSemaphoreGiveFromISR(GPIO_hndlr_UIhardwareEvent, &xHigherPriorityTaskWoken);
}

void SM_init()
{
    GPIO_hndlr_lastState = 0;


    //set up GPIO monitoring and start up here
    GPIO_hndlr_SWTC_pin = 0;
    GPIO_hndlr_SWTC_event = 0;
    GPIO_hndlr_UIhardwareEvent = xSemaphoreCreateCounting((unsigned portBASE_TYPE)0x7fffffff, (unsigned portBASE_TYPE)0);

//config LED outputs
    gpio_init(LED_BASE + LED_YEL);
    gpio_set_dir(LED_BASE + LED_YEL, GPIO_OUT);
	  gpio_put(LED_BASE + LED_YEL, 0);
    
    gpio_init(LED_BASE + LED_GRN);
    gpio_set_dir(LED_BASE + LED_GRN, GPIO_OUT);
	  gpio_put(LED_BASE + LED_GRN, 0);

    gpio_init(LED_BASE + LED_RED);
    gpio_set_dir(LED_BASE + LED_RED, GPIO_OUT);
	  gpio_put(LED_BASE + LED_RED, 0);

    gpio_init(LED_BASE + LED_WHT);
    gpio_set_dir(LED_BASE + LED_WHT, GPIO_OUT);
	  gpio_put(LED_BASE + LED_WHT, 0);
    
    gpio_init(LED_BASE + LED_BLU);
    gpio_set_dir(LED_BASE + LED_BLU, GPIO_OUT);
	  gpio_put(LED_BASE + LED_BLU, 0);
    
    gpio_init(LED_BASE + LED_SWC);
    gpio_set_dir(LED_BASE + LED_SWC, GPIO_OUT);
	  gpio_put(LED_BASE + LED_SWC, 0);

    //config button inputs

    
    gpio_init(SWTC_ESTP);
    gpio_pull_up(SWTC_ESTP);
    gpio_set_dir(SWTC_ESTP, GPIO_IN);
    GPIO_isrMplex_AddGPIOwatch(SWTC_ESTP,GPIO_IRQ_EDGE_FALL|GPIO_IRQ_EDGE_RISE,&SM_gpio_callback);
    GPIO_hndlr_lastState |= (gpio_get(SWTC_ESTP)<<(SWTC_ESTP));

    gpio_init(SWTC_BOXW);
    gpio_pull_up(SWTC_BOXW);
    gpio_set_dir(SWTC_BOXW, GPIO_IN);
    GPIO_isrMplex_AddGPIOwatch(SWTC_BOXW,GPIO_IRQ_EDGE_FALL|GPIO_IRQ_EDGE_RISE,&SM_gpio_callback);
    GPIO_hndlr_lastState |= (gpio_get(SWTC_BOXW)<<(SWTC_BOXW));
    
    gpio_init(SWTC_BLSB);
    gpio_pull_up(SWTC_BLSB);
    gpio_set_dir(SWTC_BLSB, GPIO_IN);
    GPIO_isrMplex_AddGPIOwatch(SWTC_BLSB,GPIO_IRQ_EDGE_FALL|GPIO_IRQ_EDGE_RISE,&SM_gpio_callback);
    GPIO_hndlr_lastState |= (gpio_get(SWTC_BLSB)<<(SWTC_BLSB));
    
    gpio_init(SWTC_BLSG);
    gpio_pull_up(SWTC_BLSG);
    gpio_set_dir(SWTC_BLSG, GPIO_IN);
    GPIO_isrMplex_AddGPIOwatch(SWTC_BLSG,GPIO_IRQ_EDGE_FALL|GPIO_IRQ_EDGE_RISE,&SM_gpio_callback);
    GPIO_hndlr_lastState |= (gpio_get(SWTC_BLSG)<<(SWTC_BLSG));
    
    gpio_init(SWTC_TOGL);
    gpio_pull_up(SWTC_TOGL);
    gpio_set_dir(SWTC_TOGL, GPIO_IN);
    GPIO_isrMplex_AddGPIOwatch(SWTC_TOGL,GPIO_IRQ_EDGE_FALL|GPIO_IRQ_EDGE_RISE,&SM_gpio_callback);
    GPIO_hndlr_lastState |= (gpio_get(SWTC_TOGL)<<(SWTC_TOGL));

    //set up sd card reader
    my_spi_init(spis);
    sd_card_detect(sd_cards);
    
}

void GPIO_hndlr_set_LEDs(uint8_t ledPins)
{
    gpio_put(LED_BASE + LED_YEL, (bool)((ledPins>>LED_YEL)&1));
    gpio_put(LED_BASE + LED_GRN, (bool)((ledPins>>LED_GRN)&1));
    gpio_put(LED_BASE + LED_RED, (bool)((ledPins>>LED_RED)&1));
    gpio_put(LED_BASE + LED_WHT, (bool)((ledPins>>LED_WHT)&1));
    gpio_put(LED_BASE + LED_BLU, (bool)((ledPins>>LED_BLU)&1));
    //gpio_put(LED_BASE + LED_YEL, !((ledPins<<LED_YEL)&1));
}

void GPIO_hndlr_task(void* param)
{
    (void) param;//removes warning for unused arg

    while(1)
    {
        //xSemaphoreTake(GPIO_hndlr_UIhardwareEvent, portMAX_DELAY);//wait for activity
        if( xSemaphoreTake( GPIO_hndlr_UIhardwareEvent, ( TickType_t ) 100 ) == pdTRUE )
        {

/*           switch (GPIO_hndlr_SWTC_pin)
          {
          case SWTC_ESTP:
            if((GPIO_hndlr_SWTC_event&GPIO_IRQ_EDGE_FALL)>0)
            {
                  char stopAlert[] = "E-Stop Triggered!\n";
                //ToDo:check usb/connection before send
                USB_reply((char*)stopAlert,strlen(stopAlert));
            }
            break;
          case SWTC_BOXW:
            if((GPIO_hndlr_SWTC_event&GPIO_IRQ_EDGE_FALL)>0)
            {
                char btnEvent[] = "Box Switch Pressd\n";
                //ToDo:check usb/connection before send
                USB_reply(btnEvent,strlen(btnEvent));
            }
            break;
          case SWTC_BLSB:
            if((GPIO_hndlr_SWTC_event&GPIO_IRQ_EDGE_FALL)>0)
            {
                char btnEvent[] = "Blue Blister Switch Pressd\n";
                  //ToDo:check usb/connection before send
                USB_reply(btnEvent,strlen(btnEvent));
            }
            break;
          case SWTC_BLSG:
            if((GPIO_hndlr_SWTC_event&GPIO_IRQ_EDGE_FALL)>0)
            {
                char btnEvent[] = "Green Blister Switch Pressd\n";
                //ToDo:check usb/connection before send
                USB_reply(btnEvent,strlen(btnEvent));
            }
            break;
          case SWTC_TOGL:
            if((GPIO_hndlr_SWTC_event&GPIO_IRQ_EDGE_FALL)>0)
            {
                char btnEvent[] = "Toggle Switch Off\n";
                //ToDo:check usb/connection before send
                USB_reply(btnEvent,strlen(btnEvent));
            }else if((GPIO_hndlr_SWTC_event&GPIO_IRQ_EDGE_RISE)>0)
            {
                char btnEvent[] = "Toggle Switch On\n";
                //ToDo:check usb/connection before send
                USB_reply(btnEvent,strlen(btnEvent));
            }
            break;
        
          default:
            break;
          } */
        }
        else
        {
          //poll UI GPIO pins
          
        }
        if(((GPIO_hndlr_lastState & 1<<SWTC_ESTP) >0)&&(gpio_get(SWTC_ESTP)== 0))
            {
                  char stopAlert[] = "E-Stop Triggered!\n";
                //ToDo:check usb/connection before send
                USB_reply((char*)stopAlert,strlen(stopAlert));
            }
        if(((GPIO_hndlr_lastState & 1<<SWTC_BOXW) >0)&&(gpio_get(SWTC_BOXW)== 0))
            {
                char btnEvent[] = "Box Switch Pressd\n";
                //ToDo:check usb/connection before send
                USB_reply(btnEvent,strlen(btnEvent));
            }
        if(((GPIO_hndlr_lastState & 1<<SWTC_BLSB) >0)&&(gpio_get(SWTC_BLSB)== 0))
            {
                char btnEvent[] = "Blue Blister Switch Pressd\n";
                  //ToDo:check usb/connection before send
                USB_reply(btnEvent,strlen(btnEvent));
            }
        if(((GPIO_hndlr_lastState & 1<<SWTC_BLSG) >0)&&(gpio_get(SWTC_BLSG)== 0))
            {
                char btnEvent[] = "Green Blister Switch Pressd\n";
                //ToDo:check usb/connection before send
                USB_reply(btnEvent,strlen(btnEvent));
            }
        if(((GPIO_hndlr_lastState & 1<<SWTC_TOGL) >0)&&(gpio_get(SWTC_TOGL)== 0))
            {
                char btnEvent[] = "Toggle Switch Off\n";
                //ToDo:check usb/connection before send
                USB_reply(btnEvent,strlen(btnEvent));
            }
        else if(((GPIO_hndlr_lastState & 1<<SWTC_TOGL) ==0)&&(gpio_get(SWTC_TOGL)> 0 ))
            {
                char btnEvent[] = "Toggle Switch On\n";
                //ToDo:check usb/connection before send
                USB_reply(btnEvent,strlen(btnEvent));
            }

        GPIO_hndlr_lastState = gpio_get_all();
    }
}

static char sendBuff[255];

void SM_TerminalServ(int(*reply)(uint8_t*,uint16_t), uint8_t* newString, uint16_t newStringLength, TerminalCallType conType)
{
    (void) conType;

    char *  pointer = strstr((char*)newString,"test");
    if(pointer != NULL){

        reply("Tested!\n", 8);
    }
    pointer = strstr((char*)newString,"getStatus");
    if(pointer != NULL){
        sprintf(sendBuff,"RX: addr:%d.%d.%d.%d\0", g_net_info.ip[0],
                    g_net_info.ip[1], g_net_info.ip[2], g_net_info.ip[3]);

        reply(sendBuff, strlen(sendBuff));
    }
    pointer = strstr((char*)newString,"EnterBootMode");
    if(pointer != NULL){

        reply("Entering BootLoader!\n", 21);
        reset_usb_boot(0,0);
    }
    pointer = strstr((char*)newString,"SetLED:");
    if(pointer != NULL){

        uint8_t tmp = strtol(&newString[7], (char **)NULL, 16);
        GPIO_hndlr_set_LEDs(tmp);
    }
    pointer = strstr((char*)newString,"SetString:");
    if(pointer != NULL){
        uint8_t tmp = strtol(&newString[10], (char **)NULL, 16);
        taskENTER_CRITICAL( );
        flash_range_erase(FLASHBLOCK_OFFSET, 256);
        flash_range_program(FLASHBLOCK_OFFSET, &newString[10], strlen(&newString[10]));
        taskEXIT_CRITICAL( );
    }
    pointer = strstr((char*)newString,"GetString");
    if(pointer != NULL){
        uint8_t tmp = strtol(&newString[10], (char **)NULL, 16);
        taskENTER_CRITICAL( );
        reply((char*)flashBlock, strlen((char*)flashBlock));
        taskEXIT_CRITICAL( );
    }

    pointer = strstr((char*)newString,"AddToFile:");
    if(pointer != NULL){
      FRESULT fr;
      FATFS fs;
      FIL fil;
      
      char cwdbuf[FF_LFN_BUF - 12] = {0};
      //fr = f_getcwd(cwdbuf, sizeof (cwdbuf));
      fr = f_mount(&sd_cards->fatfs, sd_cards->pcName, 1);

      FIL f;
      fr = f_open(&f, "numbers.txt", FA_OPEN_APPEND | FA_READ | FA_WRITE);
      fr = f_printf(&f,"%s\n",&newString[10]);
      f_close(&f);
    }

    pointer = strstr((char*)newString,"ReadFile");
    if(pointer != NULL){
      volatile FRESULT fr;
      FATFS fs;
      FIL fil;
      TCHAR readBuff[255] ={0};

      char cwdbuf[FF_LFN_BUF - 12] = {0};
      //fr = f_getcwd(cwdbuf, sizeof (cwdbuf));
      fr = f_mount(&sd_cards->fatfs, sd_cards->pcName, 1);
      FIL f;
      fr = f_open(&f, "numbers.txt",  FA_READ );
      while(f_gets(readBuff,255,&f)!=NULL)
      {
        reply((char*)readBuff, strlen((char*)readBuff));
      }
      f_close(&f);
    }
}


static int USB_reply(uint8_t* sendString,uint16_t stringLength)
{
    tud_cdc_write(sendString, stringLength);
    tud_cdc_write_flush();
}

void USBwatcher_task(void* param)
{
  (void) param;//removes warning for unused arg

  // RTOS forever loop
  while ( 1 )
  {
    // connected() check for DTR bit
    // Most but not all terminal client set this when making connection
    //if ( tud_cdc_connected() )
    {
      // There are data available
      if ( tud_cdc_available() )
      {
        uint8_t buf[64];
        memset(buf,(int)('\0'),sizeof(buf));

        // read
        uint32_t count = tud_cdc_read(buf, sizeof(buf));
        (void) count;
        //forward to terminmal here
        SM_TerminalServ(USB_reply, buf, count, SERIAL);

        //tud_cdc_write_flush();
      }
    }

    // For ESP32-S2 this delay is essential to allow idle how to run and reset wdt
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}


// USB Device Driver task
// This top level thread process all usb events and invoke callbacks
void usb_device_task(void* param)
{
  (void) param;

  /* if stops working refix \pico_stdio_usb\stdio_usb_descriptors.c line 69 by removing "const" */
    tusb_desc_device_t *usbd_desc_device = (tusb_desc_device_t*)tud_descriptor_device_cb();
    usbd_desc_device->idVendor = 0xCAFE;//1FC9;
    usbd_desc_device->idProduct = 0x0083;
  // This should be called after scheduler/kernel is started.
  // Otherwise it could cause kernel issue since USB IRQ handler does use RTOS queue API.
  tusb_init();

  // RTOS forever loop
  while (1)
  {
    // tinyusb device task
    tud_task();
  }
}

//--------------------------------------------------------------------+
// Device callbacks
//--------------------------------------------------------------------+

// Invoked when device is mounted
void tud_mount_cb(void)
{
  //xTimerChangePeriod(blinky_tm, pdMS_TO_TICKS(BLINK_MOUNTED), 0);
}

// Invoked when device is unmounted
void tud_umount_cb(void)
{
  //xTimerChangePeriod(blinky_tm, pdMS_TO_TICKS(BLINK_NOT_MOUNTED), 0);
}

// Invoked when usb bus is suspended
// remote_wakeup_en : if host allow us  to perform remote wakeup
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
void tud_suspend_cb(bool remote_wakeup_en)
{
  (void) remote_wakeup_en;
  //xTimerChangePeriod(blinky_tm, pdMS_TO_TICKS(BLINK_SUSPENDED), 0);
}

// Invoked when usb bus is resumed
void tud_resume_cb(void)
{
  //xTimerChangePeriod(blinky_tm, pdMS_TO_TICKS(BLINK_MOUNTED), 0);
}

//--------------------------------------------------------------------+
// USB CDC
//--------------------------------------------------------------------+
void cdc_task(void* params)
{
  (void) params;

  // RTOS forever loop
  while ( 1 )
  {
    // connected() check for DTR bit
    // Most but not all terminal client set this when making connection
    //if ( tud_cdc_connected() )
    {
      // There are data available
      if ( tud_cdc_available() )
      {
        uint8_t buf[64];

        // read and echo back
        uint32_t count = tud_cdc_read(buf, sizeof(buf));
        (void) count;

        // Echo back
        // Note: Skip echo by commenting out write() and write_flush()
        // for throughput test e.g
        //    $ dd if=/dev/zero of=/dev/ttyACM0 count=10000
        tud_cdc_write(buf, count);
        tud_cdc_write(" and your little dog too!", 25);
        tud_cdc_write_flush();
      }
    }

    // For ESP32-S2 this delay is essential to allow idle how to run and reset wdt
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

// Invoked when cdc when line state changed e.g connected/disconnected
void tud_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts)
{
  (void) itf;
  (void) rts;

  // TODO set some indicator
  if ( dtr )
  {
    // Terminal connected
  }else
  {
    // Terminal disconnected
  }
}

// Invoked when CDC interface received data from host
void tud_cdc_rx_cb(uint8_t itf)
{
  (void) itf;
}


static socketConfig * netSocket = NULL;
static uint8_t remoteAddr[] = {0,0,0,0};
static uint16_t remotePort = 55601;
static int UDP_reply(uint8_t* sendString,uint16_t stringLength)
{
    if(netSocket!=NULL)
    {
    NetworkService_TrySendUDP(netSocket, remoteAddr, remotePort, sendString, stringLength);
    }
}

void NetworkUDPwatcher_task(void* params)
{
  (void) params;
    static uint8_t dataBlock[512];
    static uint16_t dbLen = 512;
    dataready = xSemaphoreCreateCounting((unsigned portBASE_TYPE)0x7fffffff, (unsigned portBASE_TYPE)0);
    netSocket = NULL;
    setIPnum(remoteAddr,0,0,0,0);
    uint16_t remotePort = 55601;
    // char outString[256];
    // char outBuff[128];

    while(netSocket == NULL)
    { 
        //vTaskDelay(100);
        //if(g_dhcp_get_ip_flag==1)
        xSemaphoreTake(DHCP_sem, portMAX_DELAY);//wait for DHCP
        xSemaphoreGive(DHCP_sem);//release once avalible
        vTaskDelay(100);
        netSocket = NetworkService_getSoc(2,UDP, 55600, dataBlock,dbLen,dataready);

        //TODO: timeout and error handling
    }

    while(1)
    {
        xSemaphoreTake(dataready, portMAX_DELAY);//wait for activity
        if(netSocket->RXdataReady == READY)
        {
            setIPnum(remoteAddr,netSocket->sourceAddr[0],netSocket->sourceAddr[1],
                netSocket->sourceAddr[2],netSocket->sourceAddr[3]);

            if(netSocket->RXbuffUsed>0)
            {
              //forward to terminmal here
              SM_TerminalServ(UDP_reply, dataBlock, netSocket->RXbuffUsed, NETWORK);

            }
            //netSocket->RXdataReady = WAITING;
        }
        NetworkService_RXhandled(netSocket);
        vTaskDelay(1);
    }

}

sd_set_up()
{

}

size_t sd_get_num() { return count_of(sd_cards); }
sd_card_t *sd_get_by_num(size_t num) {
    if (num <= sd_get_num()) {
        return &sd_cards[num];
    } else {
        return NULL;
    }
}
size_t spi_get_num() { return count_of(spis); }
spi_t *spi_get_by_num(size_t num) {
    if (num <= spi_get_num()) {
        return &spis[num];
    } else {
        return NULL;
    }
}

#endif