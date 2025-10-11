/* 
 * File:   SSD1306.h
 * Author: philippe SIMIER Lycée Touchard
 *
 * Created on 5 octobre 2024, 09:01
 * This is a library for our Monochrome OLEDs based on SSD1306 drivers
 */

#include <iostream>
#include "i2c.h"
#include <cstring>        // Nécessaire pour utiliser memset
#include <sstream>
#include <iomanip>
#include "oled_fonts.h"

#define SSD1306_I2C_ADDRESS   0x3C
#define SSD1306_128_64

#if defined SSD1306_128_32
#define WIDTH 128
#define HEIGHT 32
#endif

#if defined SSD1306_128_64
#define WIDTH 128
#define HEIGHT 64
#endif

#if defined SSD1306_96_16
#define WIDTH 96
#define HEIGHT 16
#endif

#if defined SSD1306_128_64 && defined SSD1306_128_32
#error "Only one SSD1306 display can be specified at once in SSD1306.h"
#endif

#if !defined SSD1306_128_64 && !defined SSD1306_128_32 && !defined SSD1306_96_16
#error "At least one SSD1306 display must be specified in SSD1306.h"
#endif

#if defined SSD1306_128_64
#define SSD1306_LCDWIDTH                  128
#define SSD1306_LCDHEIGHT                 64
#endif
#if defined SSD1306_128_32
#define SSD1306_LCDWIDTH                  128
#define SSD1306_LCDHEIGHT                 32
#endif
#if defined SSD1306_96_16
#define SSD1306_LCDWIDTH                  96
#define SSD1306_LCDHEIGHT                 16
#endif

#define SSD1306_SETCONTRAST 0x81
#define SSD1306_DISPLAYALLON_RESUME 0xA4
#define SSD1306_DISPLAYALLON 0xA5
#define SSD1306_NORMALDISPLAY 0xA6
#define SSD1306_INVERTDISPLAY 0xA7
#define SSD1306_DISPLAYOFF 0xAE
#define SSD1306_DISPLAYON 0xAF

#define SSD1306_SETDISPLAYOFFSET 0xD3
#define SSD1306_SETCOMPINS 0xDA

#define SSD1306_SETVCOMDETECT 0xDB

#define SSD1306_SETDISPLAYCLOCKDIV 0xD5
#define SSD1306_SETPRECHARGE 0xD9

#define SSD1306_SETMULTIPLEX 0xA8

#define SSD1306_SETLOWCOLUMN 0x00
#define SSD1306_SETHIGHCOLUMN 0x10

#define SSD1306_SETSTARTLINE 0x40

#define SSD1306_MEMORYMODE 0x20
#define SSD1306_COLUMNADDR 0x21
#define SSD1306_PAGEADDR   0x22

#define SSD1306_COMSCANINC 0xC0
#define SSD1306_COMSCANDEC 0xC8

#define SSD1306_SEGREMAP 0xA0

#define SSD1306_CHARGEPUMP 0x8D

#define SSD1306_EXTERNALVCC 0x1
#define SSD1306_SWITCHCAPVCC 0x2

// Scrolling #defines
#define SSD1306_ACTIVATE_SCROLL 0x2F
#define SSD1306_DEACTIVATE_SCROLL 0x2E
#define SSD1306_SET_VERTICAL_SCROLL_AREA 0xA3
#define SSD1306_RIGHT_HORIZONTAL_SCROLL 0x26
#define SSD1306_LEFT_HORIZONTAL_SCROLL 0x27
#define SSD1306_VERTICAL_AND_RIGHT_HORIZONTAL_SCROLL 0x29
#define SSD1306_VERTICAL_AND_LEFT_HORIZONTAL_SCROLL 0x2A

#define rotation 0
#define BLACK 0
#define WHITE 1
#define INVERSE 2

#define pgm_read_byte(addr) (*(const unsigned char *)(addr))

class SSD1306 {
public:
    SSD1306(int8_t address = SSD1306_I2C_ADDRESS);
    SSD1306(const SSD1306& orig) = delete;
    virtual ~SSD1306();

    void begin(unsigned int switchvcc = SSD1306_SWITCHCAPVCC);
    void clear();
    void invert(unsigned int i);
    void display();
       
    void drawPixel(int x, int y, unsigned int color);
    void setTextSize(int _size);
    void drawChar(int x, int y, unsigned char c, int color, int size);
    void fillRect(int x, int y, int w, int h, int fillcolor);
    void drawFastHLine(int x, int y, int w, unsigned int color);
    
    void write(const char c);
    void write(const char *str);
    void write(const std::string &str);
    void write(const int n);
    void write(const double n);
    
    void scrollRight(unsigned int start, unsigned int stop);
    void scrollLeft(unsigned int start, unsigned int stop);
    void scrollStop(void);
    
    SSD1306& operator<<(SSD1306& (*)(SSD1306&));
    SSD1306& operator<<(const std::string&);
    SSD1306& operator<<(const int);
    SSD1306& operator<<(const double);
    SSD1306& operator<<(const char);
    SSD1306& operator<<(const char *);
    SSD1306& operator<<(const bool);
       
   
private:

    i2c *deviceI2C; // file descriptor
    bool presence;
    
    int vccstate;
    int cursor_y;
    int cursor_x;
    int size;
    int pixel[1024];    // Tableau des pixels
    

    void swap(int &x, int &y);

};

SSD1306& display(SSD1306& sx);
SSD1306& clear(SSD1306& sx);
