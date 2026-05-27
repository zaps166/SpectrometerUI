// SPDX-FileCopyrightText: 2026 Błażej Szczygieł <mumei6102@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#include "CH341Uart.hpp"

// Vendor requests and registers based on CH341 datasheet
static constexpr uint8_t REQ_READ_VERSION = 0x5F;
static constexpr uint8_t REQ_WRITE_REG    = 0x9A;
static constexpr uint8_t REQ_SERIAL_INIT  = 0xA1;

static constexpr uint16_t REG_PRESCALER   = 0x12;
static constexpr uint16_t REG_DIVISOR     = 0x13;
static constexpr uint16_t REG_LCR         = 0x18;
static constexpr uint16_t REG_LCR2        = 0x25;
static constexpr uint16_t REG_FLOW_CTL    = 0x27;

static constexpr uint8_t LCR_ENABLE_RX    = 0x80;
static constexpr uint8_t LCR_ENABLE_TX    = 0x40;
static constexpr uint8_t LCR_PAR_EVEN     = 0x10;
static constexpr uint8_t LCR_ENABLE_PAR   = 0x08;
static constexpr uint8_t LCR_STOP_BITS_2  = 0x04;
static constexpr uint8_t LCR_CS8          = 0x03;
static constexpr uint8_t LCR_CS7          = 0x02;
static constexpr uint8_t LCR_CS6          = 0x01;
static constexpr uint8_t LCR_CS5          = 0x00;

static constexpr uint16_t FLOW_CTL_NONE   = 0x00;
static constexpr uint16_t FLOW_CTL_RTSCTS = 0x01;

// Clock calculations
static constexpr uint32_t CH341_CLKRATE   = 48000000u;

// Helper: convert libusb error code to name
static string libusbErrName(int code)
{
    const char *s = libusb_error_name(code);
    if (s)
    {
        return string(s);
    }
    return string("LIBUSB_ERROR_") + to_string(code);
}

CH341Uart::CH341Uart()
{
}
CH341Uart::~CH341Uart()
{
    close();
}

void CH341Uart::init(uint16_t vid, uint16_t pid)
{
    Q_ASSERT(m_handle == nullptr);
    m_vid = vid;
    m_pid = pid;
}
void CH341Uart::init(libusb_device_handle *handle)
{
    Q_ASSERT(m_vid == 0 && m_pid == 0);
    m_handle = handle;
}

void CH341Uart::open(uint32_t baud, uint8_t dataBits, char parity, uint8_t stopBits, bool enableRtsCts)
{
    Q_ASSERT(m_interface < 0);

    int r = 0;

    if (!m_handle)
    {
        Q_ASSERT(m_vid != 0 && m_pid != 0);

        libusb_device **list = nullptr;
        ssize_t cnt = libusb_get_device_list(nullptr, &list);
        if (cnt < 0)
        {
            throw runtime_error("libusb_get_device_list failed: " + libusbErrName((int)cnt));
        }

        libusb_device *found = nullptr;
        for (ssize_t i = 0; i < cnt; ++i)
        {
            libusb_device *dev = list[i];
            libusb_device_descriptor desc;
            int e = libusb_get_device_descriptor(dev, &desc);
            if (e < 0)
            {
                continue;
            }
            if (desc.idVendor == m_vid && desc.idProduct == m_pid)
            {
                found = dev;
                break;
            }
        }

        if (!found)
        {
            libusb_free_device_list(list, 1);
            throw runtime_error("CH341 device not found");
        }

        r = libusb_open(found, &m_handle);
        libusb_free_device_list(list, 1);
        if (r < 0 || !m_handle)
        {
            throw runtime_error("libusb_open failed: " + libusbErrName(r));
        }

        libusb_set_auto_detach_kernel_driver(m_handle, 1);
    }

    libusb_device *dev = libusb_get_device(m_handle);
    libusb_config_descriptor *config = nullptr;
    r = libusb_get_active_config_descriptor(dev, &config);
    if (r < 0)
    {
        r = libusb_get_config_descriptor(dev, 0, &config);
        if (r < 0)
        {
            throw runtime_error("Failed to get config descriptor: " + libusbErrName(r));
        }
    }

    bool foundEp = false;
    for (uint8_t i = 0; i < config->bNumInterfaces && !foundEp; ++i)
    {
        const libusb_interface &iface = config->interface[i];
        for (int a = 0; a < iface.num_altsetting && !foundEp; ++a)
        {
            const libusb_interface_descriptor &alt = iface.altsetting[a];
            uint8_t in = 0, out = 0;
            for (uint8_t e = 0; e < alt.bNumEndpoints; ++e)
            {
                const libusb_endpoint_descriptor &ed = alt.endpoint[e];
                if ((ed.bmAttributes & LIBUSB_TRANSFER_TYPE_MASK) == LIBUSB_TRANSFER_TYPE_BULK)
                {
                    if ((ed.bEndpointAddress & LIBUSB_ENDPOINT_DIR_MASK) == LIBUSB_ENDPOINT_IN)
                    {
                        in = ed.bEndpointAddress;
                    }
                    else
                    {
                        out = ed.bEndpointAddress;
                    }
                }
            }
            if (in && out)
            {
                m_interface = alt.bInterfaceNumber;
                m_epIn = in;
                m_epOut = out;
                foundEp = true;
                break;
            }
        }
    }

    libusb_free_config_descriptor(config);

    if (!foundEp)
    {
        throw runtime_error("No suitable bulk endpoints found");
    }

    r = libusb_claim_interface(m_handle, m_interface);
    if (r < 0)
    {
        throw runtime_error("Claim interface failed: " + libusbErrName(r));
    }

    unsigned char verbuf[2] = { 0 };
    controlIn(REQ_READ_VERSION, 0, 0, verbuf, sizeof(verbuf));
    m_version = verbuf[0];

    controlOut(REQ_SERIAL_INIT, 0, 0);

    setLineCoding(baud, dataBits, parity, stopBits);
    setFlowControl(enableRtsCts);
}

void CH341Uart::close()
{
    if (m_handle)
    {
        if (m_interface >= 0)
        {
            libusb_release_interface(m_handle, m_interface);
        }

        libusb_close(m_handle);
        m_handle = nullptr;
    }
}

int CH341Uart::write(const uint8_t *buf, int length, unsigned int timeoutMs)
{
    int transferred = 0;
    int r = libusb_bulk_transfer(m_handle, m_epOut, const_cast<unsigned char*>(buf), length, &transferred, timeoutMs);
    if (r < 0)
    {
        throw runtime_error("Bulk write failed: " + libusbErrName(r));
    }
    return transferred;
}
int CH341Uart::read(uint8_t *buf, int length, unsigned int timeoutMs)
{
    int transferred = 0;
    int r = libusb_bulk_transfer(m_handle, m_epIn, buf, length, &transferred, timeoutMs);
    if (r < 0)
    {
        throw runtime_error("Bulk read failed: " + libusbErrName(r));
    }
    return transferred;
}

int CH341Uart::controlOut(uint8_t request, uint16_t value, uint16_t index, unsigned int timeoutMs)
{
    constexpr auto bm =
        static_cast<uint8_t>(LIBUSB_REQUEST_TYPE_VENDOR) |
        static_cast<uint8_t>(LIBUSB_RECIPIENT_DEVICE) |
        static_cast<uint8_t>(LIBUSB_ENDPOINT_OUT)
    ;
    int r = libusb_control_transfer(m_handle, bm, request, value, index, nullptr, 0, timeoutMs);
    if (r < 0)
    {
        throw runtime_error("controlOut failed: " + libusbErrName(r));
    }
    return 0;
}
int CH341Uart::controlIn(uint8_t request, uint16_t value, uint16_t index, unsigned char *data, uint16_t length, unsigned int timeoutMs)
{
    constexpr auto bm =
        static_cast<uint8_t>(LIBUSB_REQUEST_TYPE_VENDOR) |
        static_cast<uint8_t>(LIBUSB_RECIPIENT_DEVICE) |
        static_cast<uint8_t>(LIBUSB_ENDPOINT_IN)
    ;
    int r = libusb_control_transfer(m_handle, bm, request, value, index, data, length, timeoutMs);
    if (r < 0)
    {
        throw runtime_error("controlIn failed: " + libusbErrName(r));
    }
    return 0;
}

int CH341Uart::computeDivisor(uint32_t baud, uint16_t &outVal)
{
    // compute device-specific divisor/prescaler value

    if (baud == 0)
    {
        return -1;
    }

    uint32_t bestDiff = UINT32_MAX;
    bool found = false;
    uint16_t bestEncoded = 0;

    for (int ps = 0; ps <= 3; ++ps)
    {
        for (int fact = 0; fact <= 1; ++fact)
        {
            int shift = 12 - 3 * ps - fact;
            if (shift < 0 || shift > 31)
            {
                continue;
            }
            uint64_t clk_div = 1ULL << shift;
            int div_min = (fact == 0) ? 2 : 9;
            for (int div = div_min; div <= 256; ++div)
            {
                uint64_t calc = CH341_CLKRATE / (clk_div * (uint64_t)div);
                uint32_t rate = static_cast<uint32_t>(calc);
                uint32_t diff = (rate > baud) ? (rate - baud) : (baud - rate);
                if (diff < bestDiff)
                {
                    bestDiff = diff;
                    uint16_t encoded = static_cast<uint16_t>(((0x100 - div) & 0xff) << 8) | static_cast<uint16_t>((fact & 0x1) << 2) | static_cast<uint16_t>(ps & 0x3);
                    bestEncoded = encoded;
                    found = true;
                }
                if (bestDiff == 0)
                {
                    break;
                }
            }
            if (bestDiff == 0)
            {
                break;
            }
        }
        if (bestDiff == 0)
        {
            break;
        }
    }

    if (!found)
    {
        return -1;
    }

    outVal = bestEncoded;
    return 0;
}

void CH341Uart::setFlowControl(bool enableRtsCts)
{
    uint16_t fc = enableRtsCts ? FLOW_CTL_RTSCTS : FLOW_CTL_NONE;
    controlOut(REQ_WRITE_REG, (REG_FLOW_CTL << 8) | REG_FLOW_CTL, (fc << 8) | fc);
    return;
}
void CH341Uart::setLineCoding(uint32_t baud, uint8_t dataBits, char parity, uint8_t stopBits)
{
    if (dataBits < 5 || dataBits > 8)
    {
        throw runtime_error("Invalid data bits");
    }
    if (parity != 'N' && parity != 'E' && parity != 'O')
    {
        throw runtime_error("Invalid parity");
    }
    if (stopBits != 1 && stopBits != 2)
    {
        throw runtime_error("Invalid stop bits");
    }

    uint8_t lcr = LCR_ENABLE_RX | LCR_ENABLE_TX;
    switch (dataBits)
    {
        case 5: lcr |= LCR_CS5; break;
        case 6: lcr |= LCR_CS6; break;
        case 7: lcr |= LCR_CS7; break;
        case 8: lcr |= LCR_CS8; break;
    }
    if (parity != 'N')
    {
        lcr |= LCR_ENABLE_PAR;
        if (parity == 'E')
        {
            lcr |= LCR_PAR_EVEN;
        }
    }
    if (stopBits == 2)
    {
        lcr |= LCR_STOP_BITS_2;
    }

    uint16_t val16 = 0;
    if (computeDivisor(baud, val16) < 0)
    {
        throw runtime_error("computeDivisor failed");
    }

    if (m_version > 0x27)
    {
        val16 |= (1 << 7);
    }

    controlOut(REQ_WRITE_REG, (REG_DIVISOR << 8) | REG_PRESCALER, val16);

    if (m_version >= 0x30)
    {
        controlOut(REQ_WRITE_REG, (REG_LCR2 << 8) | REG_LCR, lcr);
    }
    else
    {
        controlOut(REQ_WRITE_REG, (REG_LCR << 8) | REG_LCR, lcr);
    }

    return;
}
