/* 
 * File:   SSD1306.cpp
 * Author: philippe SIMIER Lycée Touchard Washington
 * 
 * Created on 5 octobre 2024, 09:01
 */

#include "SSD1306.h"

SSD1306::SSD1306(int8_t address) :
deviceI2C(new i2c(address)),
presence(false),
vccstate(SSD1306_SWITCHCAPVCC),
cursor_y(0),
cursor_x(0),
size(1)
 {
    if (deviceI2C->getError()) {

        throw std::runtime_error("Exception in constructor SSD1306");
    }
    std::memset(pixel, 0, (SSD1306_LCDWIDTH * SSD1306_LCDHEIGHT / 8) * sizeof (int));

}

SSD1306::~SSD1306() {
    if (deviceI2C != nullptr)
        delete deviceI2C;
}

void SSD1306::begin(unsigned int switchvcc) {

    vccstate = switchvcc;

    // Init sequence
    deviceI2C->WriteReg8(0x00, SSD1306_DISPLAYOFF); // 0xAE
    deviceI2C->WriteReg8(0x00, SSD1306_SETDISPLAYCLOCKDIV); // 0xD5
    deviceI2C->WriteReg8(0x00, 0x80); // the suggested ratio 0x80
    deviceI2C->WriteReg8(0x00, SSD1306_SETMULTIPLEX); // 0xA8
    deviceI2C->WriteReg8(0x00, SSD1306_LCDHEIGHT - 1);
    deviceI2C->WriteReg8(0x00, SSD1306_SETDISPLAYOFFSET); // 0xD3
    deviceI2C->WriteReg8(0x00, 0x0); // no offset
    deviceI2C->WriteReg8(0x00, SSD1306_SETSTARTLINE | 0x0); // line #0
    deviceI2C->WriteReg8(0x00, SSD1306_CHARGEPUMP); // 0x8D
    if (vccstate == SSD1306_EXTERNALVCC) {
        deviceI2C->WriteReg8(0x00, 0x10);
    } else {
        deviceI2C->WriteReg8(0x00, 0x14);
    }
    deviceI2C->WriteReg8(0x00, SSD1306_MEMORYMODE); // 0x20
    deviceI2C->WriteReg8(0x00, 0x00); // 0x0 act like ks0108
    deviceI2C->WriteReg8(0x00, SSD1306_SEGREMAP | 0x1);
    deviceI2C->WriteReg8(0x00, SSD1306_COMSCANDEC);

#if defined SSD1306_128_32
    deviceI2C->WriteReg8(0x00, SSD1306_SETCOMPINS); // 0xDA
    deviceI2C->WriteReg8(0x00, 0x02);
    deviceI2C->WriteReg8(0x00, SSD1306_SETCONTRAST); // 0x81
    deviceI2C->WriteReg8(0x00, 0x8F);

#elif defined SSD1306_128_64
    deviceI2C->WriteReg8(0x00, SSD1306_SETCOMPINS); // 0xDA
    deviceI2C->WriteReg8(0x00, 0x12);
    deviceI2C->WriteReg8(0x00, SSD1306_SETCONTRAST); // 0x81
    if (vccstate == SSD1306_EXTERNALVCC) {
        deviceI2C->WriteReg8(0x00, 0x9F);
    } else {
        deviceI2C->WriteReg8(0x00, 0xCF);
    }

#elif defined SSD1306_96_16
    deviceI2C->WriteReg8(0x00, SSD1306_SETCOMPINS); // 0xDA
    deviceI2C->WriteReg8(0x00, 0x2); // ada x12
    deviceI2C->WriteReg8(0x00, SSD1306_SETCONTRAST); // 0x81
    if (vccstate == SSD1306_EXTERNALVCC) {
        deviceI2C->WriteReg8(0x00, 0x10);
    } else {
        deviceI2C->WriteReg8(0x00, 0xAF);
    }

#endif
    deviceI2C->WriteReg8(0x00, SSD1306_SETPRECHARGE); // 0xd9
    if (vccstate == SSD1306_EXTERNALVCC) {
        deviceI2C->WriteReg8(0x00, 0x22);
    } else {
        deviceI2C->WriteReg8(0x00, 0xF1);
    }
    deviceI2C->WriteReg8(0x00, SSD1306_SETVCOMDETECT); // 0xDB
    deviceI2C->WriteReg8(0x00, 0x40);
    deviceI2C->WriteReg8(0x00, SSD1306_DISPLAYALLON_RESUME); // 0xA4
    deviceI2C->WriteReg8(0x00, SSD1306_NORMALDISPLAY); // 0xA6
    deviceI2C->WriteReg8(0x00, SSD1306_DEACTIVATE_SCROLL);
    deviceI2C->WriteReg8(0x00, SSD1306_DISPLAYON); // --turn on oled panel

}

/**
 * @brief  clear everything
 */
void SSD1306::clear() {
    memset(pixel, 0, (SSD1306_LCDWIDTH * SSD1306_LCDHEIGHT / 8) * sizeof (int));
    cursor_y = 0;
    cursor_x = 0;
}

void SSD1306::invert(unsigned int i) {
    if (i) {
        deviceI2C->WriteReg8(0x00, SSD1306_INVERTDISPLAY);
    } else {
        deviceI2C->WriteReg8(0x00, SSD1306_NORMALDISPLAY);
    }
}

void SSD1306::display() {

    deviceI2C->WriteReg8(0x00, SSD1306_COLUMNADDR);
    deviceI2C->WriteReg8(0x00, 0); // Column start address (0 = reset)
    deviceI2C->WriteReg8(0x00, SSD1306_LCDWIDTH - 1); // Column end address (127 
    // = reset)

    deviceI2C->WriteReg8(0x00, SSD1306_PAGEADDR);
    deviceI2C->WriteReg8(0x00, 0); // Page start address (0 = reset)
#if SSD1306_LCDHEIGHT == 64
    deviceI2C->WriteReg8(0x00, 7); // Page end address
#endif
#if SSD1306_LCDHEIGHT == 32
    deviceI2C->WriteReg8(0x00, 3); // Page end address
#endif
#if SSD1306_LCDHEIGHT == 16
    deviceI2C->WriteReg8(0x00, 1); // Page end address
#endif


    for (int i = 0; i < (SSD1306_LCDWIDTH * SSD1306_LCDHEIGHT / 8); i++) {
        deviceI2C->WriteReg8(0x40, pixel[i]);
    }
}

void SSD1306::scrollRight(unsigned int start, unsigned int stop) {
    
    deviceI2C->WriteReg8(0x00, SSD1306_RIGHT_HORIZONTAL_SCROLL);
    deviceI2C->WriteReg8(0x00, 0X00);
    deviceI2C->WriteReg8(0x00, start);
    deviceI2C->WriteReg8(0x00, 0X00);
    deviceI2C->WriteReg8(0x00, stop);
    deviceI2C->WriteReg8(0x00, 0X00);
    deviceI2C->WriteReg8(0x00, 0XFF);
    deviceI2C->WriteReg8(0x00, SSD1306_ACTIVATE_SCROLL);
}

/**
 * @brief To scroll the whole display, run:
 *        scrollLeft(0x00, 0x0F)
 * @param start
 * @param stop
 */
void SSD1306::scrollLeft(unsigned int start, unsigned int stop) {
    
    deviceI2C->WriteReg8(0x00, SSD1306_LEFT_HORIZONTAL_SCROLL);
    deviceI2C->WriteReg8(0x00, 0X00);
    deviceI2C->WriteReg8(0x00, start);
    deviceI2C->WriteReg8(0x00, 0X00);
    deviceI2C->WriteReg8(0x00, stop);
    deviceI2C->WriteReg8(0x00, 0X00);
    deviceI2C->WriteReg8(0x00, 0XFF);
    deviceI2C->WriteReg8(0x00, SSD1306_ACTIVATE_SCROLL);    
}

void SSD1306::scrollStop(void){
    deviceI2C->WriteReg8(0x00, SSD1306_DEACTIVATE_SCROLL);
}

void SSD1306::drawPixel(int x, int y, unsigned int color) {


    if ((x < 0) || (x >= WIDTH) || (y < 0) || (y >= HEIGHT))
        return;

    // check rotation, move pixel around if necessary
    switch (rotation) {
        case 1:
            swap(x, y);
            x = WIDTH - x - 1;
            break;
        case 2:
            x = WIDTH - x - 1;
            y = HEIGHT - y - 1;
            break;
        case 3:
            swap(x, y);
            y = HEIGHT - y - 1;
            break;
    }

    // x is which column
    switch (color) {
        case WHITE:
            pixel[x + (y / 8) * SSD1306_LCDWIDTH] |= (1 << (y & 7));
            break;
        case BLACK:
            pixel[x + (y / 8) * SSD1306_LCDWIDTH] &= ~(1 << (y & 7));
            break;
        case INVERSE:
            pixel[x + (y / 8) * SSD1306_LCDWIDTH] ^= (1 << (y & 7));
            break;
    }

}

void SSD1306::swap(int &x, int &y) {

    int t = x;
    x = y;
    y = t;

}

void SSD1306::setTextSize(int _s) {

    size = _s;
}

void SSD1306::drawChar(int x, int y, unsigned char c, int color, int size) {

    if ((x >= WIDTH) || // Clip right
            (y >= HEIGHT) || // Clip bottom
            ((x + 6 * size - 1) < 0) || // Clip left
            ((y + 8 * size - 1) < 0)) // Clip top
        return;

    int i, j;
    for (i = 0; i < 6; i++) {
        int line;
        if (i == 5)
            line = 0x0;
        else
            line = pgm_read_byte(font + (c * 5) + i);
        for (j = 0; j < 8; j++) {
            if (line & 0x1) {
                if (size == 1) // default size
                    drawPixel(x + i, y + j, color);
                else { // big size
                    fillRect(x + (i * size),
                            y + (j * size), size,
                            size, color);
                }
            }
            line >>= 1;
        }
    }
}

void SSD1306::fillRect(int x, int y, int w, int h, int fillcolor) {
    // Bounds check
    if ((x >= WIDTH) || (y >= HEIGHT))
        return;

    // Y bounds check
    if (y + h > HEIGHT) {
        h = HEIGHT - y - 1;
    }
    // X bounds check
    if (x + w > WIDTH) {
        w = WIDTH - x - 1;
    }

    switch (rotation) {
        case 1:
            swap(x, y);
            x = WIDTH - x - 1;
            break;
        case 2:
            x = WIDTH - x - 1;
            y = HEIGHT - y - 1;
            break;
        case 3:
            swap(x, y);
            y = HEIGHT - y - 1;
            break;
    }
    int i;
    for (i = 0; i < h; i++)
        drawFastHLine(x, y + i, w, fillcolor);
}

void SSD1306::drawFastHLine(int x, int y, int w, unsigned int color) {

    // Do bounds/limit checks
    if (y < 0 || y >= HEIGHT) {
        return;
    }
    // make sure we don't try to draw below 0
    if (x < 0) {
        w += x;
        x = 0;
    }
    // make sure we don't go off the edge of the display
    if ((x + w) > WIDTH) {
        w = (WIDTH - x);
    }
    // if our width is now negative, punt
    if (w <= 0) {
        return;
    }
    // set up the pointer for movement through the buffer
    int *pBuf = pixel;
    // adjust the buffer pointer for the current row
    pBuf += ((y / 8) * SSD1306_LCDWIDTH);
    // and offset x columns in
    pBuf += x;

    unsigned int mask = 1 << (y & 7);

    switch (color) {
        case WHITE:
            while (w--) {
                *pBuf++ |= mask;
            };
            break;
        case BLACK:
            mask = ~mask;
            while (w--) {
                *pBuf++ &= mask;
            };
            break;
        case INVERSE:
            while (w--) {
                *pBuf++ ^= mask;
            };
            break;
    }
}

void SSD1306::write(const char c) {

    int wrap = 1;
    if (c == '\n') {
        cursor_y += 4 + size * 8;
        cursor_x = 0;
    } else if (c == '\r') {
        // skip em
    } else {
        drawChar(cursor_x, cursor_y, c, WHITE, size);
        cursor_x += size * 6;
        if (wrap && (cursor_x > (WIDTH - size * 6))) {
            cursor_y += 4 + size * 8;
            cursor_x = 0;
        }
    }

}

void SSD1306::write(const char *str) {

    int i, end;
    end = strlen(str);
    for (i = 0; i < end; i++)
        write(str[i]);
}

void SSD1306::write(const std::string &str) {
    write(str.c_str());

}

void SSD1306::write(const int n) {
    write(std::to_string(n).c_str());
}

void SSD1306::write(const double f) {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2) << f;
    write(ss.str());
}

SSD1306& SSD1306::operator<<(SSD1306& (*fp)(SSD1306&)){
    return (*fp)(*this);
}

SSD1306& SSD1306::operator<<(const std::string& str){
        
    write(str);
    return *this;
}

SSD1306& SSD1306::operator<<(const int n){
    write(n);
    return *this;
}

SSD1306& SSD1306::operator<<(const double x){
    write(x);
    return *this;
}

SSD1306& SSD1306::operator<<(const char c){
    write(c);
    return *this;
}

SSD1306& SSD1306::operator<<(const char * str){
    
    write(str);
    return *this;
}
    
SSD1306& SSD1306::operator<<(const bool b){
    
    write(std::to_string(b));
    return *this;
}

SSD1306& display(SSD1306& sx){
      
    sx.display();
    sx.clear();
    return sx;
}

SSD1306& clear(SSD1306& sx){
    sx.clear();
    return sx;
}