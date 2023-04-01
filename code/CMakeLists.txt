set(TARGET_NAME main)

add_subdirectory(../no-OS-FatFS-SD-SPI-RPi-Pico/FatFs_SPI build)

add_executable(${TARGET_NAME}
        ${TARGET_NAME}.c
        NetworkServices.h
        NetworkServices.c
        SystemManager.h
        SystemManager.c
        GPIO_isrMplex.h
        GPIO_isrMplex.c
        DAC_sender.h
        DAC_sender.c
        )    

        pico_generate_pio_header(main ${CMAKE_CURRENT_LIST_DIR}/DACloader.pio)
target_link_libraries(${TARGET_NAME} PRIVATE
        pico_stdlib
        pico_stdio_usb 
        pico_multicore
        hardware_spi
        hardware_dma
        hardware_flash
        hardware_pio
        FREERTOS_FILES
        ETHERNET_FILES
        IOLIBRARY_FILES
        DHCP_FILES
        DNS_FILES
        TIMER_FILES
        FatFs_SPI
        #mbedcrypto
        #mbedx509
        #mbedtls
        )

pico_enable_stdio_usb(${TARGET_NAME} 0)
pico_enable_stdio_uart(${TARGET_NAME} 0)

pico_add_extra_outputs(${TARGET_NAME})