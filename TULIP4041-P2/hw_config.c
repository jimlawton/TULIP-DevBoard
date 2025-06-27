/* hw_config.c
Copyright 2021 Carl John Kugler III

Licensed under the Apache License, Version 2.0 (the License); you may not use
this file except in compliance with the License. You may obtain a copy of the
License at

   http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software distributed
under the License is distributed on an AS IS BASIS, WITHOUT WARRANTIES OR
CONDITIONS OF ANY KIND, either express or implied. See the License for the
specific language governing permissions and limitations under the License.
*/
/*   
This file should be tailored to match the hardware design.

See 
https://github.com/carlk3/no-OS-FatFS-SD-SDIO-SPI-RPi-Pico/tree/main#customizing-for-the-hardware-configuration
*/

#ifdef __cplusplus 
extern "C" { 
#endif 

#include "hw_config.h"
#include "ff.h"
#include "diskio.h"

#include "hpinterface_hardware.h"

// #define P_uSD_DO                8           // uSD card DO
// #define P_uSD_CS                9           // uSD card CS
// #define P_uSD_SCK               10          // uSD card SCK
// #define P_uSD_DI                11          // uSD card DI
// card detect is not used on TULIP

// on TULIP spi0 is used for FRAM, spi1 is for the uSD card

#define USEPRINTF 0
#define USE_DBG_PRINTF 0
#define USE_LED 1


/* Configuration of RP2040 hardware SPI object */
static spi_t spis[] = {
    {   // spis[0]
    .hw_inst = SPI_PORT_uSD,            // RP2350 SPI component
    .spi_mode = 0,
    .miso_gpio = P_uSD_DI,              // GPIO number, GPIO 11 for TULIP
    .sck_gpio = P_uSD_SCK,              // GPIO number, GPIO 10 for TULIP
    .mosi_gpio = P_uSD_DO,              // GPIO number, GPIO 8 for TULIP

    // other as default
    .set_drive_strength = true,
    .sck_gpio_drive_strength = GPIO_DRIVE_STRENGTH_2MA,
    .mosi_gpio_drive_strength = GPIO_DRIVE_STRENGTH_2MA, 
    .no_miso_gpio_pull_up = false,
    // DMA references to be removed for SDK 2.0
    // .DMA_IRQ_num = DMA_IRQ_0,
    // .baud_rate = 125 * 1000 * 1000 / 50
    .baud_rate = 125 * 1000 * 1000 / 10  // 12500000 Hz or 12 MHz
    // .baud_rate = 125 * 1000 * 1000 / 8  // 15625000 Hz
    // .baud_rate = 125 * 1000 * 1000 / 6  // 20833333 Hz
    // .baud_rate = 125 * 1000 * 1000 / 4  // 31250000 Hz
    }
};


/* SPI Interfaces */
static sd_spi_if_t spi_ifs[] = {
    {   // spi_ifs[0]
        .spi = &spis[0],            // Pointer to the SPI driving this card
        .ss_gpio = P_uSD_CS,        // The SPI slave select GPIO for this SD card, GPIO 9
        .set_drive_strength = true,
        .ss_gpio_drive_strength = GPIO_DRIVE_STRENGTH_2MA,        
    }
};


// Hardware Configuration of the SD Card "objects"
static sd_card_t sd_cards[] = {  // One for each SD card
// #ifdef SPI_SD0
    {   // sd_cards[0]: Socket sd0
        .type = SD_IF_SPI,
        .spi_if_p = &spi_ifs[0],        // Pointer to the SPI interface driving this card
        // SD Card detect:
        .use_card_detect = false,       // we do not use card detect
        // .card_detect_gpio = 9,  
        // .card_detected_true = -1,       // What the GPIO read returns when a card is present.
        // .card_detect_use_pull = true,
        // .card_detect_pull_hi = true                                 
    },
};

sd_timeouts_t sd_timeouts = {
    .sd_command = 1000,                 // 1000 ms
    .sd_command_retries = 10,            // 3 retries
};

/* ********************************************************************** */

// size_t sd_get_num() { return count_of(sd_cards); }
size_t sd_get_num() { return 1; }

sd_card_t *sd_get_by_num(size_t num) {
    assert(num < sd_get_num());
    if (num < sd_get_num()) {
        return &sd_cards[num];
    } else {
        return NULL;
    }
}


#ifdef __cplusplus 
} 
#endif 

/* [] END OF FILE */