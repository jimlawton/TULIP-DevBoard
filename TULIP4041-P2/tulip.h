/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

// -----------------------------------------------------
// NOTE: THIS HEADER IS ALSO INCLUDED BY ASSEMBLER SO
//       SHOULD ONLY CONSIST OF PREPROCESSOR DIRECTIVES
// -----------------------------------------------------

// This header may be included by other board headers as "boards/pico2.h"

// file adapted for the TULIP Module, based on the original pico.h
// change FLASH mmemory capacity to 16 MByte

// pico_cmake_set PICO_PLATFORM=rp2350

#ifndef _BOARDS_TULIP_H
#define _BOARDS_TULIP_H

// For board detection
#define TULIP_MODULE

// --- RP2350 VARIANT ---
#define PICO_RP2350A 1

// --- UART ---
#ifndef PICO_DEFAULT_UART
#define PICO_DEFAULT_UART 0
#endif
#ifndef PICO_DEFAULT_UART_TX_PIN
#define PICO_DEFAULT_UART_TX_PIN 0
#endif
#ifndef PICO_DEFAULT_UART_RX_PIN
#define PICO_DEFAULT_UART_RX_PIN 1
#endif

// --- LED ---
#ifndef PICO_DEFAULT_LED_PIN
#define PICO_DEFAULT_LED_PIN 25
#endif
// no PICO_DEFAULT_WS2812_PIN

// --- I2C ---
#ifndef PICO_DEFAULT_I2C
#define PICO_DEFAULT_I2C 0
#endif
#ifndef PICO_DEFAULT_I2C_SDA_PIN
#define PICO_DEFAULT_I2C_SDA_PIN 4
#endif
#ifndef PICO_DEFAULT_I2C_SCL_PIN
#define PICO_DEFAULT_I2C_SCL_PIN 5
#endif

// --- SPI ---
#ifndef PICO_DEFAULT_SPI
#define PICO_DEFAULT_SPI 0
#endif
#ifndef PICO_DEFAULT_SPI_SCK_PIN
#define PICO_DEFAULT_SPI_SCK_PIN 18
#endif
#ifndef PICO_DEFAULT_SPI_TX_PIN
#define PICO_DEFAULT_SPI_TX_PIN 19
#endif
#ifndef PICO_DEFAULT_SPI_RX_PIN
#define PICO_DEFAULT_SPI_RX_PIN 16
#endif
#ifndef PICO_DEFAULT_SPI_CSN_PIN
#define PICO_DEFAULT_SPI_CSN_PIN 17
#endif

// --- FLASH ---

#define PICO_BOOT_STAGE2_CHOOSE_W25Q080 1

#ifndef PICO_FLASH_SPI_CLKDIV
#define PICO_FLASH_SPI_CLKDIV 2
#endif

// pico_cmake_set_default PICO_FLASH_SIZE_BYTES = (4 * 1024 * 1024)
#ifndef PICO_FLASH_SIZE_BYTES
// #define PICO_FLASH_SIZE_BYTES (4 * 1024 * 1024)
#define PICO_FLASH_SIZE_BYTES_16MB (16 * 1024 * 1024)           
#endif

// Drive high to force power supply into PWM mode (lower ripple on 3V3 at light loads)
// #define PICO_SMPS_MODE_PIN 23   not used on the TULIP Module


// The GPIO Pin used to read VBUS to determine if the device is battery powered.
#ifndef PICO_VBUS_PIN
#define PICO_VBUS_PIN 24
#endif

// The GPIO Pin used to monitor VSYS. Typically you would use this with ADC.
// There is an example in adc/read_vsys in pico-examples.
// #ifndef PICO_VSYS_PIN
// #define PICO_VSYS_PIN 29
// #endif

// pico_cmake_set_default PICO_RP2350_A2_SUPPORTED = 1
#ifndef PICO_RP2350_A2_SUPPORTED
#define PICO_RP2350_A2_SUPPORTED 1
#endif

#endif
