#pragma once
#ifndef VORTEX_XPT2046_TOUCHSCREEN_H
#define VORTEX_XPT2046_TOUCHSCREEN_H

#include <Arduino.h>
#include <SPI.h>

class TS_Point {
public:
  int16_t x, y, z;
  TS_Point() : x(0), y(0), z(0) {}
  TS_Point(int16_t _x, int16_t _y, int16_t _z=0) : x(_x), y(_y), z(_z) {}
};

class XPT2046_Touchscreen {
public:
  explicit XPT2046_Touchscreen(uint8_t cs, uint8_t irq=255)
    : _cs(cs), _irq(irq), _spi(&SPI), _rotation(0), _hz(2500000),
      _minX(200), _maxX(3900), _minY(200), _maxY(3900), _pressureMin(80) {}

  bool begin(SPIClass &spi=SPI, uint32_t frequency=2500000) {
    _spi=&spi; _hz=frequency;
    pinMode(_cs, OUTPUT); digitalWrite(_cs, HIGH);
    if (_irq != 255) pinMode(_irq, INPUT_PULLUP);
    return true;
  }

  void setRotation(uint8_t r) { _rotation=r&3; }
  void setCalibration(uint16_t minX,uint16_t maxX,uint16_t minY,uint16_t maxY) {
    _minX=minX; _maxX=maxX; _minY=minY; _maxY=maxY;
  }
  void setPressureThreshold(uint16_t minimum) { _pressureMin=minimum; }

  bool touched() {
    if (_irq != 255) return digitalRead(_irq)==LOW;
    return getPointRaw().z >= _pressureMin;
  }

  TS_Point getPoint() {
    TS_Point p=getPointRaw();
    if (p.z < _pressureMin) return TS_Point();
    int32_t x=p.x, y=p.y;
    switch(_rotation) {
      case 1: { int32_t n=x; x=y; y=_maxX-n; } break;
      case 2: x=_maxX-x; y=_maxY-y; break;
      case 3: { int32_t n=x; x=_maxY-y; y=n; } break;
      default: break;
    }
    return TS_Point((int16_t)constrain(x,0L,4095L),(int16_t)constrain(y,0L,4095L),p.z);
  }

private:
  uint8_t _cs,_irq,_rotation;
  SPIClass *_spi;
  uint32_t _hz;
  uint16_t _minX,_maxX,_minY,_maxY,_pressureMin;

  uint16_t read12(uint8_t command) {
    _spi->beginTransaction(SPISettings(_hz,MSBFIRST,SPI_MODE0));
    digitalWrite(_cs,LOW);
    _spi->transfer(command);
    uint16_t v=((uint16_t)_spi->transfer(0)<<8)|_spi->transfer(0);
    digitalWrite(_cs,HIGH);
    _spi->endTransaction();
    return (v>>3)&0x0FFF;
  }

  uint16_t avg(uint8_t command,uint8_t samples=3) {
    uint32_t s=0; for(uint8_t i=0;i<samples;i++) s+=read12(command);
    return (uint16_t)(s/samples);
  }

  TS_Point getPointRaw() {
    uint16_t x=avg(0xD0), y=avg(0x90), z1=avg(0xB0,2), z2=avg(0xC0,2);
    if(z1==0 || z2>=z1) return TS_Point(x,y,0);
    uint32_t pressure=(uint32_t)x*(4095UL-z2)/z1;
    if(pressure>4095) pressure=4095;
    return TS_Point(x,y,(int16_t)pressure);
  }
};

#endif
