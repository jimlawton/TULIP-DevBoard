/*----------------------------------------------------------------------/
/ Low level disk I/O module function checker                            /
/-----------------------------------------------------------------------/
/ WARNING: The data on the target drive will be lost!
*/
/* app4-IO_module_function_checker.c
Originally from [Compatibility Checker for Storage Device Control Module]
(http://elm-chan.org/fsw/ff/res/app4.c).
*/
/*----------------------------------------------------------------------------/
/  FatFs - Generic FAT Filesystem Module  Rx.xx                               /
/-----------------------------------------------------------------------------/
/
/ Copyright (C) 20xx, ChaN, all right reserved.
/
/ FatFs module is an open source software. Redistribution and use of FatFs in
/ source and binary forms, with or without modification, are permitted provided
/ that the following condition is met:
/
/ 1. Redistributions of source code must retain the above copyright notice,
/    this condition and the following disclaimer.
/
/ This software is provided by the copyright holder and contributors "AS IS"
/ and any warranties related to this software are DISCLAIMED.
/ The copyright owner or contributors be NOT LIABLE for any damages caused
/ by use of this software.
/----------------------------------------------------------------------------*/
/*
Modifications: Copyright 2023 Carl John Kugler III

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
This example runs a low level I/O test than can be helpful for debugging hardware.
It will destroy the format of the SD card!
*/
#include "FatFsSd_C.h"
#include "SerialUART.h"

#define printf Serial1.printf
#define puts Serial1.println

/* Implement library message callbacks */
void put_out_error_message(const char *s) {
    Serial1.write(s);
}
void put_out_info_message(const char *s) {
    Serial1.write(s);
}
// This will not be called unless build_flags include "-D USE_DBG_PRINTF":
void put_out_debug_message(const char *s) {
    Serial1.write(s);
}

/* ********************************************************************** */
    /*
    This example assumes the following wiring for SD card 0:
        | GPIO | Function                         | SD Card | SPI0     |
        | ---- | -------------------------------- | ------- | -------- |
        | GP2  | SCK                              | CLK     | SPI0_SCK |
        | GP3  | MOSI (COPI or Peripheral's SDI)  | CMD/DI  | SPI0_TX  |
        | GP4  | MISO (CIPO or Peripheral's SDO)  | D0/DO   | SPI0_RX  |
        | GP7  | SS (CS)                          | D3/CS   |          |
        | GP9  | Card Detect                      | DET     |          |

    This example assumes the following wiring for SD card 1:
        | GPIO | SD Card |
        | ---- | ------- |
        | GP16 | CLK     |
        | GP17 | CMD     |
        | GP18 | D0      |
        | GP19 | D1      |
        | GP20 | D2      |
        | GP21 | D3      |
        | GP22 | DET     |
    */

    /* Hardware Configuration of SPI object */
    static spi_t spi = {
        .hw_inst = spi0,  // RP2040 SPI component
        .miso_gpio = 4,
        .mosi_gpio = 3,
        .sck_gpio = 2,                 // GPIO number (not Pico pin number)
        .baud_rate = 12 * 1000 * 1000  // Actual frequency: 10416666.
    };

    static sd_spi_if_t spi_if = {
        .spi = &spi,  // Pointer to the SPI driving this card
        .ss_gpio = 7  // The SPI slave select GPIO for this SD card
    };

    static sd_sdio_if_t sdio_if = {
        /*
        Pins CLK_gpio, D1_gpio, D2_gpio, and D3_gpio are at offsets from pin D0_gpio.
        The offsets are determined by sd_driver\SDIO\rp2040_sdio.pio.
            CLK_gpio = (D0_gpio + SDIO_CLK_PIN_D0_OFFSET) % 32;
            As of this writing, SDIO_CLK_PIN_D0_OFFSET is 30,
                which is -2 in mod32 arithmetic, so:
            CLK_gpio = D0_gpio -2.
            D1_gpio = D0_gpio + 1;
            D2_gpio = D0_gpio + 2;
            D3_gpio = D0_gpio + 3;
        */
        .CMD_gpio = 17,
        .D0_gpio = 18,
        .SDIO_PIO = pio1,
        .DMA_IRQ_num = DMA_IRQ_1,
        .baud_rate = 15 * 1000 * 1000  // 15 MHz
    };

    // Hardware Configuration of the SD Card "objects"
    static sd_card_t sd_cards[] = {
        {   // sd_cards[0]
            .type = SD_IF_SPI,
            .spi_if_p = &spi_if,  // Pointer to the SPI interface driving this card
            .use_card_detect = true,
            .card_detect_gpio = 9,
            .card_detected_true = 0,  // What the GPIO read returns when a card is present.
            .card_detect_use_pull = true,
            .card_detect_pull_hi = true
        },
        {   // sd_cards[1]
            .type = SD_IF_SDIO,
            .sdio_if_p = &sdio_if,
            // SD Card detect:
            .use_card_detect = true,
            .card_detect_gpio = 22,
            .card_detected_true = 0,  // What the GPIO read returns when a card is present.
            .card_detect_use_pull = true,
            .card_detect_pull_hi = true
        }
    };
/*
The following *get_num, *get_by_num functions are required by the library API.
They are how the library finds out about the configuration.
*/
extern "C" size_t sd_get_num() { return count_of(sd_cards); }

extern "C" sd_card_t *sd_get_by_num(size_t num) {
    if (num <= sd_get_num()) {
        return &sd_cards[num];
    } else {
        return NULL;
    }
}

/* ********************************************************************** */

void setup() {
    Serial1.begin(115200);  // set up Serial library at 9600 bps
    while (!Serial1)
        ;  // Serial is via USB; wait for enumeration
}

static DWORD pn(          /* Pseudo random number generator */
                DWORD pns /* !0:Initialize, 0:Read */
) {
    static DWORD lfsr;
    UINT n;

    if (pns) {
        lfsr = pns;
        for (n = 0; n < 32; n++) pn(0);
    }
    if (lfsr & 1) {
        lfsr >>= 1;
        lfsr ^= 0x80200003;
    } else {
        lfsr >>= 1;
    }
    return lfsr;
}

static int test_diskio(
    BYTE pdrv,   /* Physical drive number to be checked (all data on the drive will be lost) */
    UINT ncyc,   /* Number of test cycles */
    DWORD *buff, /* Pointer to the working buffer */
    UINT sz_buff /* Size of the working buffer in unit of byte */
) {
    UINT n, cc, ns;
    DWORD sz_drv, lba, lba2, sz_eblk, pns = 1;
    WORD sz_sect;
    BYTE *pbuff = (BYTE *)buff;
    DSTATUS ds;
    DRESULT dr;

    printf("test_diskio(%u, %u, 0x%08X, 0x%08X)\n", pdrv, ncyc, (UINT)buff, sz_buff);

    if (sz_buff < FF_MAX_SS + 8) {
        printf("Insufficient work area to run the program.\n");
        return 1;
    }

    for (cc = 1; cc <= ncyc; cc++) {
        printf("**** Test cycle %u of %u start ****\n", cc, ncyc);

        printf(" disk_initalize(%u)", pdrv);
        ds = disk_initialize(pdrv);
        if (ds & STA_NOINIT) {
            printf(" - failed.\n");
            return 2;
        } else {
            printf(" - ok.\n");
        }

        printf("**** Get drive size ****\n");
        printf(" disk_ioctl(%u, GET_SECTOR_COUNT, 0x%08X)", pdrv, (UINT)&sz_drv);
        sz_drv = 0;
        dr = disk_ioctl(pdrv, GET_SECTOR_COUNT, &sz_drv);
        if (dr == RES_OK) {
            printf(" - ok.\n");
        } else {
            printf(" - failed.\n");
            return 3;
        }
        if (sz_drv < 128) {
            printf("Failed: Insufficient drive size to test.\n");
            return 4;
        }
        printf(" Number of sectors on the drive %u is %lu.\n", pdrv, sz_drv);

#if FF_MAX_SS != FF_MIN_SS
        printf("**** Get sector size ****\n");
        printf(" disk_ioctl(%u, GET_SECTOR_SIZE, 0x%X)", pdrv, (UINT)&sz_sect);
        sz_sect = 0;
        dr = disk_ioctl(pdrv, GET_SECTOR_SIZE, &sz_sect);
        if (dr == RES_OK) {
            printf(" - ok.\n");
        } else {
            printf(" - failed.\n");
            return 5;
        }
        printf(" Size of sector is %u bytes.\n", sz_sect);
#else
        sz_sect = FF_MAX_SS;
#endif

        printf("**** Get block size ****\n");
        printf(" disk_ioctl(%u, GET_BLOCK_SIZE, 0x%X)", pdrv, (UINT)&sz_eblk);
        sz_eblk = 0;
        dr = disk_ioctl(pdrv, GET_BLOCK_SIZE, &sz_eblk);
        if (dr == RES_OK) {
            printf(" - ok.\n");
        } else {
            printf(" - failed.\n");
        }
        if (dr == RES_OK || sz_eblk >= 2) {
            printf(" Size of the erase block is %lu sectors.\n", sz_eblk);
        } else {
            printf(" Size of the erase block is unknown.\n");
        }

        /* Single sector write test */
        printf("**** Single sector write test ****\n");
        lba = 0;
        for (n = 0, pn(pns); n < sz_sect; n++) pbuff[n] = (BYTE)pn(0);
        printf(" disk_write(%u, 0x%X, %lu, 1)", pdrv, (UINT)pbuff, lba);
        dr = disk_write(pdrv, pbuff, lba, 1);
        if (dr == RES_OK) {
            printf(" - ok.\n");
        } else {
            printf(" - failed.\n");
            return 6;
        }
        printf(" disk_ioctl(%u, CTRL_SYNC, NULL)", pdrv);
        dr = disk_ioctl(pdrv, CTRL_SYNC, 0);
        if (dr == RES_OK) {
            printf(" - ok.\n");
        } else {
            printf(" - failed.\n");
            return 7;
        }
        memset(pbuff, 0, sz_sect);
        printf(" disk_read(%u, 0x%X, %lu, 1)", pdrv, (UINT)pbuff, lba);
        dr = disk_read(pdrv, pbuff, lba, 1);
        if (dr == RES_OK) {
            printf(" - ok.\n");
        } else {
            printf(" - failed.\n");
            return 8;
        }
        for (n = 0, pn(pns); n < sz_sect && pbuff[n] == (BYTE)pn(0); n++)
            ;
        if (n == sz_sect) {
            printf(" Read data matched.\n");
        } else {
            printf(" Read data differs from the data written.\n");
            return 10;
        }
        pns++;

        printf("**** Multiple sector write test ****\n");
        lba = 5;
        ns = sz_buff / sz_sect;
        if (ns > 4) ns = 4;
        if (ns > 1) {
            for (n = 0, pn(pns); n < (UINT)(sz_sect * ns); n++) pbuff[n] = (BYTE)pn(0);
            printf(" disk_write(%u, 0x%X, %lu, %u)", pdrv, (UINT)pbuff, lba, ns);
            dr = disk_write(pdrv, pbuff, lba, ns);
            if (dr == RES_OK) {
                printf(" - ok.\n");
            } else {
                printf(" - failed.\n");
                return 11;
            }
            printf(" disk_ioctl(%u, CTRL_SYNC, NULL)", pdrv);
            dr = disk_ioctl(pdrv, CTRL_SYNC, 0);
            if (dr == RES_OK) {
                printf(" - ok.\n");
            } else {
                printf(" - failed.\n");
                return 12;
            }
            memset(pbuff, 0, sz_sect * ns);
            printf(" disk_read(%u, 0x%X, %lu, %u)", pdrv, (UINT)pbuff, lba, ns);
            dr = disk_read(pdrv, pbuff, lba, ns);
            if (dr == RES_OK) {
                printf(" - ok.\n");
            } else {
                printf(" - failed.\n");
                return 13;
            }
            for (n = 0, pn(pns); n < (UINT)(sz_sect * ns) && pbuff[n] == (BYTE)pn(0); n++)
                ;
            if (n == (UINT)(sz_sect * ns)) {
                printf(" Read data matched.\n");
            } else {
                printf(" Read data differs from the data written.\n");
                return 14;
            }
        } else {
            printf(" Test skipped.\n");
        }
        pns++;

        printf("**** Single sector write test (unaligned buffer address) ****\n");
        lba = 5;
        for (n = 0, pn(pns); n < sz_sect; n++) pbuff[n + 3] = (BYTE)pn(0);
        printf(" disk_write(%u, 0x%X, %lu, 1)", pdrv, (UINT)(pbuff + 3), lba);
        dr = disk_write(pdrv, pbuff + 3, lba, 1);
        if (dr == RES_OK) {
            printf(" - ok.\n");
        } else {
            printf(" - failed.\n");
            return 15;
        }
        printf(" disk_ioctl(%u, CTRL_SYNC, NULL)", pdrv);
        dr = disk_ioctl(pdrv, CTRL_SYNC, 0);
        if (dr == RES_OK) {
            printf(" - ok.\n");
        } else {
            printf(" - failed.\n");
            return 16;
        }
        memset(pbuff + 5, 0, sz_sect);
        printf(" disk_read(%u, 0x%X, %lu, 1)", pdrv, (UINT)(pbuff + 5), lba);
        dr = disk_read(pdrv, pbuff + 5, lba, 1);
        if (dr == RES_OK) {
            printf(" - ok.\n");
        } else {
            printf(" - failed.\n");
            return 17;
        }
        for (n = 0, pn(pns); n < sz_sect && pbuff[n + 5] == (BYTE)pn(0); n++)
            ;
        if (n == sz_sect) {
            printf(" Read data matched.\n");
        } else {
            printf(" Read data differs from the data written.\n");
            return 18;
        }
        pns++;

        printf("**** 4GB barrier test ****\n");
        if (sz_drv >= 128 + 0x80000000 / (sz_sect / 2)) {
            lba = 6;
            lba2 = lba + 0x80000000 / (sz_sect / 2);
            for (n = 0, pn(pns); n < (UINT)(sz_sect * 2); n++) pbuff[n] = (BYTE)pn(0);
            printf(" disk_write(%u, 0x%X, %lu, 1)", pdrv, (UINT)pbuff, lba);
            dr = disk_write(pdrv, pbuff, lba, 1);
            if (dr == RES_OK) {
                printf(" - ok.\n");
            } else {
                printf(" - failed.\n");
                return 19;
            }
            printf(" disk_write(%u, 0x%X, %lu, 1)", pdrv, (UINT)(pbuff + sz_sect), lba2);
            dr = disk_write(pdrv, pbuff + sz_sect, lba2, 1);
            if (dr == RES_OK) {
                printf(" - ok.\n");
            } else {
                printf(" - failed.\n");
                return 20;
            }
            printf(" disk_ioctl(%u, CTRL_SYNC, NULL)", pdrv);
            dr = disk_ioctl(pdrv, CTRL_SYNC, 0);
            if (dr == RES_OK) {
                printf(" - ok.\n");
            } else {
                printf(" - failed.\n");
                return 21;
            }
            memset(pbuff, 0, sz_sect * 2);
            printf(" disk_read(%u, 0x%X, %lu, 1)", pdrv, (UINT)pbuff, lba);
            dr = disk_read(pdrv, pbuff, lba, 1);
            if (dr == RES_OK) {
                printf(" - ok.\n");
            } else {
                printf(" - failed.\n");
                return 22;
            }
            printf(" disk_read(%u, 0x%X, %lu, 1)", pdrv, (UINT)(pbuff + sz_sect), lba2);
            dr = disk_read(pdrv, pbuff + sz_sect, lba2, 1);
            if (dr == RES_OK) {
                printf(" - ok.\n");
            } else {
                printf(" - failed.\n");
                return 23;
            }
            for (n = 0, pn(pns); pbuff[n] == (BYTE)pn(0) && n < (UINT)(sz_sect * 2); n++)
                ;
            if (n == (UINT)(sz_sect * 2)) {
                printf(" Read data matched.\n");
            } else {
                printf(" Read data differs from the data written.\n");
                return 24;
            }
        } else {
            printf(" Test skipped.\n");
        }
        pns++;

        printf("**** Test cycle %u of %u completed ****\n\n", cc, ncyc);
    }

    return 0;
}

// int main (int argc, char* argv[])
int lliot(size_t pnum) {
    int rc;
    DWORD buff[FF_MAX_SS]; /* Working buffer (4 sector in size) */

    /* Check function/compatibility of the physical drive #0 */
    rc = test_diskio(pnum, 3, buff, sizeof buff);

    if (rc) {
        printf("Sorry the function/compatibility test failed. (rc=%d)\nFatFs will not work with this disk driver.\n", rc);
    } else {
        printf("Congratulations! The disk driver works well.\n");
    }

    return rc;
}

void loop() {
    for (size_t i = 0; i < sd_get_num(); ++i) {
        printf("\nTesting drive %lu\n", i);
        lliot(i);
        sleep_ms(10000);
    }
}