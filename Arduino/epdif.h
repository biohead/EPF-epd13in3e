/**
 *  @filename   :   epdif.h
 *  @brief      :   Header file of epdif.cpp providing EPD interface functions
 *                  Users have to implement all the functions in epdif.cpp
 *  @author     :   Yehui from Waveshare
 *
 *  Copyright (C) Waveshare     August 10 2017
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documnetation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to  whom the Software is
 * furished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS OR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef EPDIF_H
#define EPDIF_H

#include <Arduino.h>   // Core Arduino functions like digitalWrite, digitalRead, delay
#include <stdint.h>    // For explicit integer types like uint8_t, uint16_t, uint32_t
#include <stdio.h>     // For printf (if used for Debug messages)

// For ESP-IDF specific functions like SPI initialization (if not handled directly by Arduino SPI library)
// These includes assume you are compiling in an ESP-IDF environment or Arduino ESP32 core
// If DEV_Config.cpp directly uses ESP-IDF SPI functions, these are needed.
#ifdef ESP32
    #include "driver/gpio.h"
    #include "driver/spi_master.h"
    #include "esp_log.h"
    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h" // For vTaskDelay
#endif

/**
 * Data Type Definitions
 * These replace UBYTE, UWORD, UDOUBLE from DEV_Config.h
**/
#define UBYTE   uint8_t
#define UWORD   uint16_t
#define UDOUBLE uint32_t

/**
 * GPIO Pin Definitions for Waveshare 13.3inch e-Paper (E)
 * Mapped to Olimex ESP32-S3-Devkit-LiPo pins.
 * These replace the old pin definitions in epdif.h and use the values
 * that are available on your Olimex board.
**/
#define EPD_SCK_PIN     12    // Onboard: 12 (for SPI Clock)
#define EPD_MOSI_PIN    11    // Onboard: 11 (for SPI Master Out Slave In)
#define EPD_CS_M_PIN    15    // Onboard: 15 (Chip Select for Master Controller)
#define EPD_CS_S_PIN    14    // Onboard: 14 (Chip Select for Slave Controller)
#define EPD_RST_PIN     9     // Onboard:  9 (Reset pin)
#define EPD_DC_PIN      18    // Onboard: 18 (Data/Command pin)
#define EPD_BUSY_PIN    8     // Onboard:  8 (Busy status pin)
#define EPD_PWR_PIN     7     // Onboard:  6 (Power control pin for display panel)

// SPI Host definition for ESP-IDF SPI (as found in original DEV_Config.h)
#define EPD_SPI_HOST    SPI2_HOST

// Generic GPIO state definitions
#define GPIO_PIN_SET    1
#define GPIO_PIN_RESET  0

/**
 * Hardware Abstraction Layer (HAL) Macros
 * These are directly from DEV_Config.h and will be used by EpdIf methods.
**/
// Macro for writing digital pin values
#define DEV_Digital_Write(_pin, _value) digitalWrite(_pin, _value == 0 ? LOW : HIGH)
// Macro for reading digital pin values
#define DEV_Digital_Read(_pin) digitalRead(_pin)
// Macro for delaying execution in milliseconds
#define DEV_Delay_ms(__xms) delay(__xms)

/*------------------------------------------------------------------------------------------------------*/
/**
 * Declarations of low-level hardware functions (originally from DEV_Config.h).
 * These functions are implemented in the corresponding .cpp file (which was DEV_Config.cpp,
 * but now will be renamed to `epdif_hardware_impl.cpp` or similar, and will still be compiled
 * as part of your sketch).
**/
UBYTE DEV_Module_Init(void);
void GPIO_Mode(UWORD GPIO_Pin, UWORD Mode); // For setting pin mode, though pinMode is also available
void DEV_SPI_WriteByte(UBYTE data);
UBYTE DEV_SPI_ReadByte(); // Not always used, but good to include if exists
void DEV_SPI_Write_nByte(UBYTE *pData, UDOUBLE len);
void DEV_Module_Exit(void);


/**
 * EpdIf Class - Interface for E-Paper Display
 * The methods of this class will wrap the underlying DEV_ functions.
 * This keeps the main e-paper driver code (like EPD_13in3e.cpp) clean and
 * only needing to interact with EpdIf.
**/
class EpdIf
{
public:
    EpdIf(void);
    ~EpdIf(void);

    // Initializes the hardware interface (e.g., SPI, GPIOs)
    static int IfInit(void);
    // Writes a digital value to a pin
    static void DigitalWrite(int pin, int value);
    // Reads a digital value from a pin
    static int DigitalRead(int pin);
    // Delays execution for a specified number of milliseconds
    static void DelayMs(unsigned int delaytime);
    // Transfers a single byte over SPI
    static void SpiTransfer(unsigned char data);
    // Waits for the busy signal to go low (or high, depending on implementation)
    // This is a crucial function for e-paper timing. It should yield to FreeRTOS.
    static void BusyWait(void);
};

#endif

