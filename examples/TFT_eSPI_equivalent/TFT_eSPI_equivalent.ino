#include <Arduino.h>
#include "lib.h"
EPD_translate tft;
uint8_t *framebuffer = NULL;

#include <TouchDrvGT911.hpp>
TouchDrvGT911 touch;
int16_t  x, y;

void touch_f(void *parameter) {
    uint8_t touched=0;
    while(1) {
     touched = touch.getPoint(&x, &y);
        if (touched) {
            Serial.printf("X:%d Y:%d\n", x, y);
        }
        vTaskDelay(50/portTICK_PERIOD_MS);
    }
}

void setup() {
    Serial.begin(115200);
    tft.init();
    Wire.begin(BOARD_SDA, BOARD_SCL);

    // Assuming that the previous touch was in sleep state, wake it up
    pinMode(TOUCH_INT, OUTPUT);
    digitalWrite(TOUCH_INT, HIGH);

    /*
    * The touch reset pin uses hardware pull-up,
    * and the function of setting the I2C device address cannot be used.
    * Use scanning to obtain the touch device address.*/
    uint8_t touchAddress = 0;
    Wire.beginTransmission(0x14);
    if (Wire.endTransmission() == 0) {
        touchAddress = 0x14;
    }
    Wire.beginTransmission(0x5D);
    if (Wire.endTransmission() == 0) {
        touchAddress = 0x5D;
    }
    if (touchAddress == 0) {
        while (1) {
            Serial.println("Failed to find GT911 - check your wiring!");
            delay(1000);
        }
    }
    touch.setPins(-1, TOUCH_INT);
    if (!touch.begin(Wire, touchAddress, BOARD_SDA, BOARD_SCL )) {
        while (1) {
            Serial.println("Failed to find GT911 - check your wiring!");
            delay(1000);
        }
    }
    touch.setMaxCoordinates(EPD_WIDTH, EPD_HEIGHT);
    touch.setSwapXY(true);
    touch.setMirrorXY(false, true);

    Serial.println("Started Touchscreen poll...");


    xTaskCreate(
        touch_f,   // Task function
        "Touch_Callback",     // Task Name
        2600,               // Stack size
        NULL,               // Task parameters
        1,                  // Task priority (0 to 3), loopTask has priority 2.
        NULL            // Task handle (not used)
    ); 
}

void loop() {

    tft.drawLine(random(10, EPD_WIDTH), random(10, EPD_HEIGHT), EPD_WIDTH - 20, 0,TFT_WHITE);
    delay(1000);

    tft.drawRect(random(10, EPD_WIDTH), random(10, EPD_HEIGHT), random(10, 300), random(10, 300), TFT_WHITE);
    delay(1000);

    tft.drawArc(random(10, EPD_WIDTH), random(10, EPD_HEIGHT), random(10, 300), random(10, 300), 0,360,TFT_WHITE,TFT_BLACK,true);
    delay(1000);

    tft.fillRect(random(10, EPD_WIDTH), random(10, EPD_HEIGHT), random(10, 300), random(10, 300), TFT_WHITE);
    delay(1000);
    tft.setCursor(200,400);
    tft.println("Hello World");


    tft.drawCentreString("Centered High",EPD_WIDTH/2, 0,1);

    tft.drawCentreString("Centered Low",EPD_WIDTH/2, EPD_HEIGHT-50,1);

    tft.drawRightString("Right", EPD_WIDTH, 100,1);
    tft.drawChar('<', EPD_WIDTH-20, 400);
    tft.drawChar('>', 20, 400);

    delay(1000);
    tft.fillRect(random(10, EPD_WIDTH), random(10, EPD_HEIGHT), random(10, 300), random(10, 300), TFT_WHITE);
    delay(1000);

    tft.fillScreen(TFT_BLACK);
    delay(1000);
}