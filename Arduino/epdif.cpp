/**
 * @filename   :   epdif.cpp
 * @brief      :   Source file of EPD interface functions
 * This file provides the implementation of EpdIf class methods,
 * wrapping the low-level DEV_ hardware functions.
 * @author     :   Yehui from Waveshare (Modified by Gemini for ESP32-C6 FireBeetle 2)
 *
 * Copyright (C) Waveshare      August 10 2017
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
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "epdif.h" // Now includes all necessary definitions and DEV_ function declarations

// Constructor
EpdIf::EpdIf(void) {
}

// Destructor
EpdIf::~EpdIf(void) {
}

// Initializes the hardware interface (SPI, GPIOs)
int EpdIf::IfInit(void) {
    // This calls the low-level hardware initialization function
    return DEV_Module_Init();
}

// Writes a digital value to a specified pin
void EpdIf::DigitalWrite(int pin, int value) {
    DEV_Digital_Write(pin, value);
}

// Reads a digital value from a specified pin
int EpdIf::DigitalRead(int pin) {
    return DEV_Digital_Read(pin);
}

// Delays execution for a specified number of milliseconds
void EpdIf::DelayMs(unsigned int delaytime) {
    DEV_Delay_ms(delaytime);
}

// Transfers a single byte over SPI
void EpdIf::SpiTransfer(unsigned char data) {
    DEV_SPI_WriteByte(data);
}

// Waits for the busy signal to go low (or high, depending on implementation)
// This is a critical function for e-paper. It MUST yield to FreeRTOS tasks.
void EpdIf::BusyWait(void) {
    // Ensure the BUSY_PIN is correctly defined in epdif.h and corresponds
    // to the display's busy signal (typically LOW when busy, HIGH when ready).
    // The exact logic (== 0 or == 1) depends on your display's busy line polarity.
    Serial.printf("EPD Busy... ");
    while(DEV_Digital_Read(EPD_BUSY_PIN) == 0) { // Assuming LOW means busy
        vTaskDelay(10 / portTICK_PERIOD_MS); // Yield to other tasks, feed watchdog
        Serial.printf("."); // Indicate waiting
    }
    Serial.printf(" Ready.");
}

