// SPDX-FileCopyrightText: 2026 Błażej Szczygieł <mumei6102@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

// CH341Uart - C++ wrapper for CH341 UART using libusb
// Datasheet: https://www.wch-ic.com/downloads/CH341DS1_PDF.html

#include "Headers.hpp"

class CH341Uart
{
public:
    CH341Uart();
    ~CH341Uart();

    void init(uint16_t vid, uint16_t pid);
    void init(libusb_device_handle *handle);

    void open(
        uint32_t baud = 115200,
        uint8_t dataBits = 8,
        char parity = 'N',
        uint8_t stopBits = 1,
        bool enableRtsCts = false
    );

    void close();

    int write(const uint8_t *buf, int length, unsigned int timeoutMs = 1000);
    int read(uint8_t *buf, int length, unsigned int timeoutMs = 1000);

private:
    int controlOut(uint8_t request, uint16_t value, uint16_t index, unsigned int timeoutMs = 1000);
    int controlIn(uint8_t request, uint16_t value, uint16_t index, unsigned char *data, uint16_t length, unsigned int timeoutMs = 1000);

    int computeDivisor(uint32_t baud, uint16_t &outVal);

    void setFlowControl(bool enableRtsCts);
    void setLineCoding(uint32_t baud, uint8_t dataBits, char parity, uint8_t stopBits);

private:
    uint16_t m_vid = 0;
    uint16_t m_pid = 0;

    libusb_device_handle *m_handle = nullptr;

    int m_interface = -1;
    uint8_t m_epIn = 0;
    uint8_t m_epOut = 0;

    uint8_t m_version = 0;
};
