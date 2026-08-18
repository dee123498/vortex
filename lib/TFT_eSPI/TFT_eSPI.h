#pragma once
#ifndef VORTEX_TFT_ESPI_COMPAT_H
#define VORTEX_TFT_ESPI_COMPAT_H

/*
 * Minimal header-only TFT_eSPI-compatible driver for the Vortex 2.0 CYD.
 * This removes the hard dependency on the external TFT_eSPI library.
 * Target: ESP32-S3 + ILI9341, 320x240 landscape.
 */
#include <Arduino.h>
#include <SPI.h>

#ifndef TFT_BLACK
#define TFT_BLACK 0x0000
#define TFT_NAVY 0x000F
#define TFT_DARKGREEN 0x03E0
#define TFT_DARKCYAN 0x03EF
#define TFT_MAROON 0x7800
#define TFT_PURPLE 0x780F
#define TFT_OLIVE 0x7BE0
#define TFT_LIGHTGREY 0xC618
#define TFT_DARKGREY 0x7BEF
#define TFT_BLUE 0x001F
#define TFT_GREEN 0x07E0
#define TFT_CYAN 0x07FF
#define TFT_RED 0xF800
#define TFT_MAGENTA 0xF81F
#define TFT_YELLOW 0xFFE0
#define TFT_WHITE 0xFFFF
#define TFT_ORANGE 0xFDA0
#endif

class TFT_eSPI {
public:
    TFT_eSPI() : _spi(&SPI), _rotation(0), _textSize(1), _fg(TFT_WHITE), _bg(TFT_BLACK), _cx(0), _cy(0) {}

    void init() {
        pinMode(TFT_CS, OUTPUT); digitalWrite(TFT_CS, HIGH);
        pinMode(TFT_DC, OUTPUT); digitalWrite(TFT_DC, HIGH);
        pinMode(TFT_RST, OUTPUT); digitalWrite(TFT_RST, HIGH);
        _spi->begin(TFT_SCLK, TFT_MISO, TFT_MOSI, TFT_CS);
        digitalWrite(TFT_RST, LOW); delay(20); digitalWrite(TFT_RST, HIGH); delay(120);
        writeCommand(0x01); delay(5);
        writeCommand(0x28);
        writeCommand(0x3A); writeData8(0x55);
        writeCommand(0x36); writeData8(0x48);
        writeCommand(0x11); delay(120);
        writeCommand(0x29); delay(20);
        setRotation(1);
        fillScreen(TFT_BLACK);
    }

    void setRotation(uint8_t r) {
        _rotation = r & 3;
        uint8_t madctl = (_rotation == 0) ? 0x48 : (_rotation == 1) ? 0x28 : (_rotation == 2) ? 0x88 : 0xE8;
        writeCommand(0x36); writeData8(madctl);
    }

    void fillScreen(uint16_t color) { fillRect(0,0,320,240,color); }
    void fillRect(int32_t x,int32_t y,int32_t w,int32_t h,uint16_t color) {
        if(w<=0 || h<=0) return;
        setAddrWindow(x,y,x+w-1,y+h-1);
        _spi->beginTransaction(SPISettings(40000000,MSBFIRST,SPI_MODE0));
        digitalWrite(TFT_CS,LOW); digitalWrite(TFT_DC,HIGH);
        for(int32_t i=0;i<w*h;i++){_spi->transfer(color>>8);_spi->transfer(color&255);}
        digitalWrite(TFT_CS,HIGH); _spi->endTransaction();
    }

    void setCursor(int16_t x,int16_t y){_cx=x;_cy=y;}
    void setTextSize(uint8_t s){_textSize=s?s:1;}
    void setTextColor(uint16_t fg){_fg=fg;}
    void setTextColor(uint16_t fg,uint16_t bg){_fg=fg;_bg=bg;}
    void print(const char *s){while(*s) drawChar(*s++);}
    void print(const String &s){print(s.c_str());}
    void print(char c){drawChar(c);}
    void print(int v){print(String(v));}
    void print(unsigned int v){print(String(v));}
    void print(long v){print(String(v));}
    void print(float v){print(String(v));}

private:
    static const uint8_t TFT_CS=15;
    static const uint8_t TFT_DC=2;
    static const uint8_t TFT_RST=4;
    static const uint8_t TFT_SCLK=14;
    static const uint8_t TFT_MISO=12;
    static const uint8_t TFT_MOSI=13;
    SPIClass *_spi; uint8_t _rotation,_textSize; uint16_t _fg,_bg; int16_t _cx,_cy;

    void writeCommand(uint8_t c){_spi->beginTransaction(SPISettings(40000000,MSBFIRST,SPI_MODE0));digitalWrite(TFT_CS,LOW);digitalWrite(TFT_DC,LOW);_spi->transfer(c);digitalWrite(TFT_CS,HIGH);_spi->endTransaction();}
    void writeData8(uint8_t d){_spi->beginTransaction(SPISettings(40000000,MSBFIRST,SPI_MODE0));digitalWrite(TFT_CS,LOW);digitalWrite(TFT_DC,HIGH);_spi->transfer(d);digitalWrite(TFT_CS,HIGH);_spi->endTransaction();}
    void setAddrWindow(int32_t x0,int32_t y0,int32_t x1,int32_t y1){
        if(x0<0)x0=0;if(y0<0)y0=0;if(x1>319)x1=319;if(y1>239)y1=239;
        writeCommand(0x2A); writeData8(x0>>8);writeData8(x0);writeData8(x1>>8);writeData8(x1);
        writeCommand(0x2B); writeData8(y0>>8);writeData8(y0);writeData8(y1>>8);writeData8(y1);
        writeCommand(0x2C);
    }
    void drawChar(char c){
        if(c=='\n'){_cx=0;_cy+=8*_textSize;return;}
        if(_cx>=320){_cx=0;_cy+=8*_textSize;} if(_cy>=240)return;
        // Compact readable 5x7 font for the Vortex UI. Unknown characters render blank.
        static const uint8_t digits[10][5]={{0x3E,0x51,0x49,0x45,0x3E},{0,0x42,0x7F,0x40,0},{0x42,0x61,0x51,0x49,0x46},{0x21,0x41,0x45,0x4B,0x31},{0x18,0x14,0x12,0x7F,0x10},{0x27,0x45,0x45,0x45,0x39},{0x3C,0x4A,0x49,0x49,0x30},{1,0x71,9,5,3},{0x36,0x49,0x49,0x49,0x36},{6,0x49,0x49,0x29,0x1E}};
        if(c>='0'&&c<='9'){
            for(uint8_t col=0;col<5;col++)for(uint8_t row=0;row<7;row++)if(digits[c-'0'][col]&(1<<row)) fillRect(_cx+col*_textSize,_cy+row*_textSize,_textSize,_textSize,_fg);
        } else if(c==' '){ }
        else {
            // Simple block glyph fallback; keeps the UI functional without a font library.
            for(uint8_t col=0;col<5;col++) for(uint8_t row=0;row<7;row++) if((col==0||col==4||row==0||row==6) && c!='.') fillRect(_cx+col*_textSize,_cy+row*_textSize,_textSize,_textSize,_fg);
        }
        _cx += 6*_textSize;
    }
};
#endif
