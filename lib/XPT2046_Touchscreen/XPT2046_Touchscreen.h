#pragma once
#ifndef VORTEX_XPT2046_TOUCHSCREEN_H
#define VORTEX_XPT2046_TOUCHSCREEN_H

/*
 * Vortex standalone XPT2046 driver
 * --------------------------------
 * Header-only replacement for the external XPT2046_Touchscreen library.
 * Designed for the ESP32-S3 CYD and the common 240x320 XPT2046 resistive
 * touchscreen controller.
 *
 * No separate .cpp file or third-party touchscreen library is required.
 * The driver uses the SPI bus supplied to begin().
 *
 * Typical CYD wiring:
 *   T_CS  = GPIO 33
 *   T_IRQ = GPIO 36
 *   MOSI  = GPIO 13
 *   MISO  = GPIO 12
 *   SCLK  = GPIO 14
 *
 * The XPT2046 shares SPI with the display and SD card, so CS must be asserted
 * only while talking to the touch controller.
 */

#include <Arduino.h>
#include <SPI.h>

class TS_Point {
public:
    int16_t x;
    int16_t y;
    int16_t z;

    TS_Point() : x(0), y(0), z(0) {}
    TS_Point(int16_t _x, int16_t _y, int16_t _z = 0)
        : x(_x), y(_y), z(_z) {}
};

class XPT2046_Touchscreen {
public:
    explicit XPT2046_Touchscreen(uint8_t cs, uint8_t irq = 255)
        : _cs(cs), _irq(irq), _spi(&SPI), _rotation(0), _spiHz(2500000),
          _minX(200), _maxX(3900), _minY(200), _maxY(3900),
          _pressureMin(80), _pressureMax(4000) {}

    bool begin(SPIClass &spi = SPI, uint32_t frequency = 2500000) {
        _spi = &spi;
        _spiHz = frequency;
        pinMode(_cs, OUTPUT);
        digitalWrite(_cs, HIGH);

        if (_irq != 255) {
            pinMode(_irq, INPUT_PULLUP);
        }

        // Do not call SPI.begin() here: the display/SD firmware may already
        // own the bus and has its own pin configuration.
        return true;
    }

    void setRotation(uint8_t rotation) {
        _rotation = rotation & 3;
    }

    bool touched() {
        if (_irq != 255) {
            return digitalRead(_irq) == LOW;
        }
        TS_Point p = getPointRaw();
        return p.z >= _pressureMin;
    }

    TS_Point getPoint() {
        TS_Point p = getPointRaw();
        if (p.z < _pressureMin) return TS_Point(0, 0, 0);

        // Keep the raw controller coordinates available through setCalibration.
        // Rotation mapping follows the normal Adafruit-style orientation:
        // 0 = portrait, 1 = landscape, 2 = inverted portrait, 3 = inverted
        // landscape. Vortex's UI uses rotation 1.
        int32_t x = p.x;
        int32_t y = p.y;

        switch (_rotation) {
            case 0:
                break;
            case 1: {
                int32_t nx = y;
                int32_t ny = _maxX - x;
                x = nx;
                y = ny;
                break;
            }
            case 2:
                x = _maxX - x;
                y = _maxY - y;
                break;
            case 3: {
                int32_t nx = _maxY - y;
                int32_t ny = x;
                x = nx;
                y = ny;
                break;
            }
        }

        x = constrain(x, 0L, 4095L);
        y = constrain(y, 0L, 4095L);
        return TS_Point((int16_t)x, (int16_t)y, p.z);
    }

    void setCalibration(uint16_t minX, uint16_t maxX,
                        uint16_t minY, uint16_t maxY) {
        _minX = minX;
        _maxX = maxX;
        _minY = minY;
        _maxY = maxY;
    }

    void setPressureThreshold(uint16_t minimum, uint16_t maximum = 4000) {
        _pressureMin = minimum;
        _pressureMax = maximum;
    }

private:
    uint8_t _cs;
    uint8_t _irq;
    SPIClass *_spi;
    uint8_t _rotation;
    uint32_t _spiHz;
    uint16_t _minX, _maxX, _minY, _maxY;
    uint16_t _pressureMin, _pressureMax;

    uint16_t read12(uint8_t command) {
        _spi->beginTransaction(SPISettings(_spiHz, MSBFIRST, SPI_MODE0));
        digitalWrite(_cs, LOW);
        _spi->transfer(command);
        uint16_t value = ((uint16_t)_spi->transfer(0x00) << 8);
        value |= _spi->transfer(0x00);
        digitalWrite(_cs, HIGH);
        _spi->endTransaction();
        return (value >> 3) & 0x0FFF;
    }

    uint16_t readAverage(uint8_t command, uint8_t samples = 3) {
        uint32_t sum = 0;
        for (uint8_t i = 0; i < samples; ++i) {
            sum += read12(command);
        }
        return (uint16_t)(sum / samples);
    }

    TS_Point getPointRaw() {
        // XPT2046 commands:
        // 0xD0 = X, 0x90 = Y, 0xB0 = Z1, 0xC0 = Z2.
        // Three samples smooth the noisy resistive panel without adding much
        // latency on an ESP32-S3.
        uint16_t x = readAverage(0xD0, 3);
        uint16_t y = readAverage(0x90, 3);
        uint16_t z1 = readAverage(0xB0, 2);
        uint16_t z2 = readAverage(0xC0, 2);

        if (z1 == 0 || z2 >= z1) {
            return TS_Point(x, y, 0);
        }

        // Approximate pressure value. This is sufficient for touch/no-touch
        // detection and avoids requiring an external library.
        uint32_t pressure = (uint32_t)x * (uint32_t)(z2 > 0 ? (4095 - z2) : 0);
        pressure /= z1;
        if (pressure > 4095) pressure = 4095;

        if (pressure < _pressureMin || pressure > _pressureMax) {
            return TS_Point(x, y, 0);
        }
        return TS_Point(x, y, (int16_t)pressure);
    }
};

#endif // VORTEX_XPT2046_TOUCHSCREEN_H
