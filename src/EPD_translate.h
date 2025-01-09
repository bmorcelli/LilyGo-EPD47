#ifndef __EPD_TRANSLATE_H
#define __EPD_TRANSLATE_H

#include <Arduino.h>
#include <epd_driver.h>
#include "utilities.h"
#include "firasans.h"
#include <SPI.h>

#define TFT_BLACK       0x0000      /*   0,   0,   0 */
#define TFT_NAVY        0x000F      /*   0,   0, 128 */
#define TFT_DARKGREEN   0x03E0      /*   0, 128,   0 */
#define TFT_DARKCYAN    0x03EF      /*   0, 128, 128 */
#define TFT_MAROON      0x7800      /* 128,   0,   0 */
#define TFT_PURPLE      0x780F      /* 128,   0, 128 */
#define TFT_OLIVE       0x7BE0      /* 128, 128,   0 */
#define TFT_LIGHTGREY   0xC618      /* 192, 192, 192 */
#define TFT_DARKGREY    0x7BEF      /* 128, 128, 128 */
#define TFT_BLUE        0x001F      /*   0,   0, 255 */
#define TFT_GREEN       0x07E0      /*   0, 255,   0 */
#define TFT_CYAN        0x07FF      /*   0, 255, 255 */
#define TFT_RED         0xF800      /* 255,   0,   0 */
#define TFT_MAGENTA     0xF81F      /* 255,   0, 255 */
#define TFT_YELLOW      0xFFE0      /* 255, 255,   0 */
#define TFT_WHITE       0xFFFF      /* 255, 255, 255 */
#define TFT_ORANGE      0xFD20      /* 255, 165,   0 */
#define TFT_GREENYELLOW 0xAFE5      /* 173, 255,  47 */
#define TFT_PINK        0xF81F

#define PSRAM_ENABLE 3

class EPD_translate {
    protected:
    bool changed=false;
    bool callBackOn=false;
    bool busy=false;
    uint8_t fg_color;
    uint8_t bg_color;
    int32_t cursor_x;
    int32_t cursor_y;
    uint8_t *framebuffer = NULL;
    inline uint8_t getColorFrom16(uint16_t c) {
        uint32_t r = ((c >> 11) & 0x1F)*254/31;
        uint32_t g = ((c >> 5) & 0x3F)*254/63;
        uint32_t b = (c & 0x1F)*254/31;
        return 255-(r+g+b)/3; // Assuming 0 to black and 254 to white
    }

    TaskHandle_t taskHandle = NULL;  // Declarando o handle da task

    static void _pushImage(void *parameter) {
        EPD_translate *self = static_cast<EPD_translate *>(parameter);  // Converte para objeto
        while (true) { 
            if (self->changed==true && self->busy==false) { 
                Serial.println("Pushing image...");
                epd_poweron();
                //epd_clear();
                epd_clear_area_cycles(epd_full_screen(), 3, 50);
                delay(50);
                self->_epdPushImage();  // Função do objeto
                epd_poweroff();
                self->changed = false;
                vTaskDelay(500 / portTICK_PERIOD_MS);
            } 
            //else Serial.println("Didn't Push any image...");
            vTaskDelay(500 / portTICK_PERIOD_MS);
        }
    }

    public:
    uint32_t textsize=1; /*where is the text size in EPD???*/
    uint8_t textcolor = fg_color;
    uint8_t textbgcolor = bg_color;
    inline void init() { 
        framebuffer = (uint8_t *)ps_calloc(sizeof(uint8_t), EPD_WIDTH * EPD_HEIGHT / 2);
        if (!framebuffer) {
            Serial.println("Alloc to PSRAM memory failed !!!");
            return; // stops the whole thing, preventing crash
        } Serial.println("Alloc to PSRAM Success");
        memset(framebuffer, 0xFF, EPD_WIDTH * EPD_HEIGHT / 2); 
        epd_init();
        epd_poweron();
        epd_clear();
        epd_poweroff();
        startCallback();
    };
    inline void startCallback() {
          // This task keeps running all the time, will never stop
        xTaskCreate(
        _pushImage,   // Task function
        "EPD_DrawCallback",     // Task Name
        2600,               // Stack size
        this,               // Task parameters
        2,                  // Task priority (0 to 3), loopTask has priority 2.
        &taskHandle            // Task handle (not used)
        ); 
        Serial.println("Task Started");
    }
    inline void stopCallback() {
        if(taskHandle!=NULL) {
            vTaskDelete(taskHandle);
            Serial.println("Task stopped.");
            taskHandle = NULL;
        } 
    }
    inline void _epdPushImage() {
        epd_draw_image(epd_full_screen(), framebuffer,BLACK_ON_WHITE);
        //epd_draw_grayscale_image(epd_full_screen(), framebuffer);
        memset(framebuffer, 0xFF, EPD_WIDTH * EPD_HEIGHT / 2);
    };
    inline void epdPushImage() {
        Serial.println("Pushing image manually...");
        epd_poweron();
        epd_clear_area_cycles(epd_full_screen(), 3, 50);
        delay(50);
        _epdPushImage();  // Função do objeto
        epd_poweroff();
        changed = false;
    };
    inline SPIClass &getSPIinstance() { return SPI; };
    inline uint16_t width() { return EPD_WIDTH; };
    inline uint16_t height() { return EPD_HEIGHT; };
    inline void setAttribute(int x,bool t) { /*Not implemented */ };
    inline void setRotation(int rot) { }; /*Not implemented*/
    inline void setTextColor(uint16_t fgcolor,uint16_t bgcolor) { fg_color = getColorFrom16(fgcolor); textcolor=fg_color;  bg_color = getColorFrom16(bgcolor); textbgcolor=bg_color; }; /*Attention point, crazy conversion*/
    inline void setTextColor(uint16_t fgcolor) { fg_color = getColorFrom16(fgcolor); textcolor=fg_color; }; /*Attention point, crazy conversion*/
    inline void setCursor(uint32_t x, uint32_t y) { cursor_x=x; cursor_y=y; };

    inline void setTextSize(uint32_t c) { /*Not implemented in EPD? */ };
    inline uint32_t getCursorY() { return cursor_y; };
    inline uint32_t getCursorX() { return cursor_x; };

    inline void print(String t) { 
        busy=1;
        int32_t y=0, x0=0, y0=0, x2=0,y2=0, w=0, h=0;
        get_text_bounds((GFXfont *)&FiraSans, t.c_str(), &x0, &y0, &x2, &y2, &w, &h, NULL);
        y = cursor_y+33;
        write_string((GFXfont *)&FiraSans, t.c_str(), &cursor_x, &y, framebuffer); 
        if(cursor_x>EPD_WIDTH) { 
            cursor_x=0;
        }        
        busy=0;
        changed=true;
    }; 
    inline void print(char c){ 
        busy=1;
        int32_t y=0, x0=0, y0=0, x2=0,y2=0, w=0, h=0;
        get_text_bounds((GFXfont *)&FiraSans, String(c).c_str(), &x0, &y0, &x2, &y2, &w, &h, NULL);
        y = cursor_y+33;
        write_string((GFXfont *)&FiraSans, &c, &cursor_x, &y, framebuffer); 
        if(cursor_x>EPD_WIDTH) { 
            cursor_x=0;
            cursor_y+=33;
        }
        busy=0;
        changed=true;
    }; 
    inline void println(String t="") { 
        busy=1;
        int32_t y=0, x0=0, y0=0, x2=0,y2=0, w=0, h=0;
        get_text_bounds((GFXfont *)&FiraSans, t.c_str(), &x0, &y0, &x2, &y2, &w, &h, NULL);
        y = cursor_y+33;
        cursor_y=y;
        write_string((GFXfont *)&FiraSans, String(t).c_str(), &cursor_x, &y, framebuffer); 
        if(cursor_x>EPD_WIDTH) { 
            cursor_x=0;
            cursor_y+=33;
        }        
        busy=0;
        //Serial.printf("println cursor_x=%d, y=%d, x0=%d, y0=%d, x2=%d, y2=%d, w=%d, h=%d\n", cursor_x, y, x0, y0, x2, y2, w, h);
        changed=true;
    };
    inline void fillScreen(uint16_t c) { 
        busy=1;
        epd_clear_area_cycles(epd_full_screen(), 1, 50);
        memset(framebuffer, 0xFF, EPD_WIDTH * EPD_HEIGHT / 2);
        if(getColorFrom16(c)<0xFD) fillRect(0,0, EPD_WIDTH, EPD_HEIGHT, getColorFrom16(c)); 
        busy=0;
        changed=false;
    };

    inline void fillRect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint16_t c) { 
        busy=1;
        epd_fill_rect(x, y, w, h, getColorFrom16(c), framebuffer); 
        busy=0;
        changed=true;
    };
    inline void drawRect(uint32_t x,uint32_t y,uint32_t w,uint32_t h,uint16_t c) { 
        busy=1;
        epd_draw_rect(x, y, w, h, getColorFrom16(c), framebuffer); 
        busy=0;
        changed=true;
    };
    inline void drawRoundRect(uint32_t x,uint32_t y,uint32_t w,uint32_t h,uint32_t r,uint16_t c) { 
        busy=1;
        epd_draw_rect(x, y, w, h, getColorFrom16(c), framebuffer); /*No round rect for now*/ 
        busy=0;
        changed=true;
    };
    inline void fillRoundRect(uint32_t x,uint32_t y,uint32_t w,uint32_t h,uint32_t r,uint16_t c) { 
        busy=1;
        epd_fill_rect(x, y, w, h, getColorFrom16(c), framebuffer); /*No round rect for now*/ 
        busy=0;
        changed=true;
    };
    inline void fillSmoothRoundRect(uint32_t x,uint32_t y,uint32_t w,uint32_t h,uint32_t r,uint16_t c) { 
        busy=1;
        epd_fill_rect(x, y, w, h, getColorFrom16(c), framebuffer); /*No smooth rect for now*/ 
        busy=0;
        changed=true;
    };
    inline void drawArc(uint32_t x,uint32_t y,uint32_t r0,uint32_t r1,uint32_t ang1,uint32_t ang2,uint16_t c, uint16_t bg, bool b) {
        busy=1;
        epd_fill_circle( x, y, r0, getColorFrom16(c), framebuffer);
        busy=0;
        changed=true;
    };

    inline void drawLine(uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1, uint16_t c) { 
        busy=1;
        epd_draw_line(x0, y0, x1, y1,  getColorFrom16(c), framebuffer); 
        busy=0;
        changed=true;
        };

    inline void drawString(String t,int x,int y) {
        busy=1;
        int32_t x0=0, y0=0, x2=0,y2=0, w=0, h=0;
        get_text_bounds((GFXfont *)&FiraSans, t.c_str(), &x0, &y0, &x2, &y2, &w, &h, NULL);
        y = y+h;
        write_string((GFXfont *)&FiraSans, t.c_str(), &x, &y, framebuffer);
        busy=0;
        changed=true;
    };

    inline void drawRightString(String t,int x,int y,uint32_t f) {
        busy=1;
        int32_t x0=0, y0=0, x2=0,y2=0, w=0, h=0;
        get_text_bounds((GFXfont *)&FiraSans, t.c_str(), &x0, &y0, &x2, &y2, &w, &h, NULL);
        x = x-w;
        y = y+h;
        //Serial.printf("RightString x=%d, y=%d, x0=%d, y0=%d, x2=%d, y2=%d, w=%d, h=%d\n\n",x, y, x0, y0, x2, y2, w, h);
        write_string((GFXfont *)&FiraSans, t.c_str(), &x, &y, framebuffer);
        busy=0;
        changed=true;
     };

    inline void drawCentreString(String t,int x,int y,uint32_t f) { 
        busy=1;
        int32_t x0=0, y0=0, x2=0,y2=0, w=0, h=0;
        get_text_bounds((GFXfont *)&FiraSans, t.c_str(), &x0, &y0, &x2, &y2, &w, &h, NULL);
        x = x-w/2;
        y = y+h;
        //Serial.printf("CentreString x=%d, y=%d, x0=%d, y0=%d, x2=%d, y2=%d, w=%d, h=%d\n\n",x, y, x0, y0, x2, y2, w, h);
        write_string((GFXfont *)&FiraSans, t.c_str(), &x, &y, framebuffer);
        busy=0;
        changed=true;
     };

     inline void drawChar(char c, int x, int y) { 
        busy=1;
        write_string((GFXfont *)&FiraSans, String(c).c_str(), &x, &y, framebuffer);
        busy=0;
        changed=true;
    };

    inline void drawPixel(uint32_t x, uint32_t y, uint16_t c) { epd_draw_pixel(x,y,getColorFrom16(c),framebuffer); };
};


#endif /*__EPD_TRANSLATE_H*/