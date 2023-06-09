#ifndef _display_wrap_h
#define _display_wrap_h
#include <stdint.h>
#include <stdbool.h>


void setupDisplay();
void display(void);
void invertDisplay(bool invert);


void drawBitmap(uint8_t left_anchor, uint8_t top_anchor, uint8_t bits[], uint8_t width, uint8_t height, uint8_t mode);
void clearRect(uint8_t left_anchor, uint8_t top_anchor, uint8_t width, uint8_t height);

void fps();

bool getGradientPixel(uint8_t x, uint8_t y, uint8_t i);
void fadeScreen(uint8_t intensity, bool color);
bool getMeltedPixel(uint8_t frame, uint8_t x, uint8_t y);
void meltScreen();

void drawByte(uint8_t x, uint8_t y, uint8_t b);
uint8_t getByte(uint8_t x, uint8_t y);

void drawPixel(int8_t x, int8_t y, bool color, bool raycasterViewport);
void drawVLine(uint8_t x, int8_t start_y, int8_t end_y, uint8_t intensity);
void drawSprite(int8_t x, int8_t y, const uint8_t bitmap[], const uint8_t mask[], int16_t w, int16_t h, uint8_t sprite, double distance);
void drawChar(int8_t x, int8_t y, char ch);
void drawText(int8_t x, int8_t y, char *txt, uint8_t space); // space = 1
// void drawText(int8_t x, int8_t y, const __FlashStringHelper txt, uint8_t space); // space = 1
void drawHealth(uint8_t health);
void drawKeys(uint8_t keys);
#endif
