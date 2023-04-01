

#ifndef DAC_SENDER_C
#define DAC_SENDER_C
#include "DAC_sender.h"

void core1_entry(void);



void DAC_sender_init()
{
    // Allocate memory for main buffer and sub-buffers
    uint16_t *buffer = (uint16_t *) malloc(BUFFER_SIZE * sizeof(uint16_t));
    uint16_t **sub_buffers = (uint16_t **)malloc(5 * sizeof(uint16_t*));;
    for (int i = 0; i < 5; i++) {
        sub_buffers[i] = (uint16_t *)malloc(SUB_BUFFER_SIZE * sizeof(uint16_t*));
    }


            gpio_set_function(21, GPIO_IN);
uint16_t offset;
uint16_t sm;
	PIO pio = pio0; 
    offset = pio_add_program(pio0, &DACloader_program);
    //////////////////NOTE 
    //will have to split for XY one poi block and RGB on the other

    // Configure DMA channels for sub-buffer transfers
    for (int i = 0; i < 5; i++) {

	sm = pio_claim_unused_sm(pio, true);
	DACloader_program_init(pio, sm, offset, 20,19,RXpins[i]);

	//pio_sm_put_blocking(pio, sm, level);
	pio_sm_set_enabled(pio, sm, false);

        dma_chans[i] = dma_claim_unused_channel(true);
        dma_channel_config c = dma_channel_get_default_config(dma_chans[i]);
        channel_config_set_transfer_data_size(&c, DMA_SIZE_16);
        channel_config_set_read_increment(&c, true);
        dma_channel_configure(dma_chans[i], &c,
            &pio0_hw->txf[i],                                         // Destination address (PIO program offset)
            sub_buffers[i],                                            // Source address (not used)
            SUB_BUFFER_SIZE,                                 // Number of transfers
            false                                             // Start immediately
        );
    }


    // Configure DMA channel for main buffer transfer and sub-buffer processing
    dma_chan = dma_claim_unused_channel(true);
    dma_channel_config c = dma_channel_get_default_config(dma_chan);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_16);
    channel_config_set_read_increment(&c, true);
    channel_config_set_chain_to(&c, dma_chans[0]);
    dma_channel_configure(dma_chan, &c,
    buffer,   // Destination address
    NULL,           // Source address, set at dataready
    BUFFER_SIZE,      // Number of transfers
    false              // Start immediately


);

    
}

void DAC_sender_dataReady(uint16_t * remoteBuff ,uint16_t length ,bool* isDone)
{
    BufferLength = length;
    isSendDone = isDone;
    dma_channel_set_trans_count(dma_chan, length, false);
    dma_channel_set_read_addr(dma_chan, remoteBuff, true);

    
    // Configure second core to execute core1_entry()
    multicore_launch_core1(core1_entry);
    
}

void core1_entry() {

    
    // Wait for DMA transfer and buffer processing to complete
    //uint dma_chan = (uint) multicore_fifo_pop_blocking();
    dma_channel_wait_for_finish_blocking(dma_chan);
    
    for(uint16_t i = 0; i < (BufferLength/5); i++)
    {
        sub_buffers[0][i] = buffer[(i+0)];//add adjustment for X
        sub_buffers[1][i] = buffer[(i+1)];//add adjustment for Y
        sub_buffers[2][i] = buffer[(i+2)];//add adjustment for R
        sub_buffers[3][i] = buffer[(i+3)];//add adjustment for G
        sub_buffers[4][i] = buffer[(i+4)];//add adjustment for B
    }


    // transfer sub-buffer to PIO device transmission buffers
    //////////////NOTE: may have to split for XY/RGB
    for (int i = 0; i < 5; i++) {
        

    }
    

}


#endif