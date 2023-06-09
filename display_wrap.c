#include "display_wrap.h"
#include "constants.h"
#include "sprites.h"

#include "led.h"
#include "7seg.h"
#include "display.h"

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <math.h>
#include <avr/interrupt.h>

//extern variables

extern const uint8_t SCREEN_WIDTH;
extern const uint8_t SCREEN_HEIGHT;
extern const uint8_t HALF_WIDTH;
extern const uint8_t RENDER_HEIGHT;
extern const uint8_t HALF_HEIGHT;

// This is slightly faster than bitRead (also bits are read from left to right)
const static uint8_t PROGMEM bit_mask[8] = { 128, 64, 32, 16, 8, 4, 2, 1 };
#define read_bit(b, n)      b & pgm_read_byte(bit_mask + n) ? 1 : 0

// FPS control

volatile double delta = 1;
static volatile uint32_t lastFrameTime = 0;
static volatile uint32_t currentFrameTime = 0;

ISR(TIMER0_OVF_vect){
	currentFrameTime++;
}

// display buffer
uint8_t display_buf[1024]; //8192

// We don't handle more than MAX_RENDER_DEPTH depth, so we can safety store
// z values in a byte with 1 decimal and save some memory,
uint8_t zbuffer[64];

void setupDisplay(void) {
	// Setup display
	sb_display_enable();
	sb_display_fillScreen(NULL);

	TCCR0B &= ~(1<<CS02);
	// TCCR0B &= ~(1<<CS01);
	TCCR0B |= (1<<CS01) | (1<<CS00);
	TIMSK0 |= (1<<TOIE0);
}

void display(void){
	sb_display_fillScreen(display_buf);
}

void invertDisplay(bool invert){
	if(invert){
		for(uint16_t i = 0; i<1024 ; i = i+1){
			display_buf[i] = ~display_buf[i];
		}
	}
}

void drawBitmap(uint8_t x, uint8_t y, uint8_t bitmap[], uint8_t w, uint8_t h, uint8_t color) {
	int16_t byteWidth = (w + 7) / 8; // Bitmap scanline pad = whole byte
	uint8_t byte = 0;

	for(int16_t j=0; j<h; j++, y++) {
		for(int16_t i=0; i<w; i++) {
			if(i & 7) byte <<= 1;
			else      byte   = pgm_read_byte(&bitmap[j * byteWidth + i / 8]);
			if(byte & 0x80){
				if (color) {
					// white
					display_buf[x + i + (y / 8)*SCREEN_WIDTH] |= (1 << (y & 7));
				} else {
					// black
					display_buf[x + i + (y / 8)*SCREEN_WIDTH] &= ~(1 << (y & 7));
			  	}
			}
		}	
	}
}


void clearRect(uint8_t left_anchor, uint8_t top_anchor, uint8_t width, uint8_t height){
	drawBitmap(left_anchor, top_anchor, NULL, width, height, 0);
}

void drawPixelDisplay(uint8_t x, uint8_t y, bool color){
	return;
}

// Adds a delay to limit play to specified fps
// Calculates also delta to keep movement consistent in lower framerates
void fps(void){
	cli();
	uint32_t frameTime = currentFrameTime;
	sei();
	while( (frameTime - lastFrameTime) < FRAME_TIME){
		cli();
		frameTime = currentFrameTime;
		sei();
	}
	delta = (double)(frameTime - lastFrameTime)/FRAME_TIME;
	lastFrameTime = frameTime;
}

double getActualFps(void) {
	return 1000 / (FRAME_TIME * delta);
}

// Faster way to render vertical bits
void drawByte(uint8_t x, uint8_t y, uint8_t b) {
	display_buf[(y / 8)*SCREEN_WIDTH + x] = b;
}

// Helper for melting screen. Picks the relative pixel after melt effect
// Similar to adafruit::getPixel but removed some checks to make it faster.
static bool getMeltedPixel(uint8_t frame, uint8_t x, uint8_t y){
	uint8_t offset = MELT_OFFSETS[x % MELT_OFFSETS_SIZE] - 48; // get "random:" numbers from 0 - 9
	int8_t dy = frame < offset ? y : y - MELT_SPEED;

	// Return black
	if (dy < 0) return false;

	return display_buf[x + (dy / 8) * SCREEN_WIDTH] & (1 << (dy & 7));
}

// Melt the screen DOOM style
void meltScreen(void) {
	uint8_t frames = 0;
	uint8_t x;
	int8_t y;

	do {
		fps();

		// The screen distribution is 8 rows of 128x8 pixels
		for (y = SCREEN_HEIGHT - 8; y >= 0; y -= 8) {
			for (x = 0; x < SCREEN_WIDTH;  x++) {
				drawByte(x, y,
					 (getMeltedPixel(frames, x, y + 7) << 7)
					 | (getMeltedPixel(frames, x, y + 6) << 6)
					 | (getMeltedPixel(frames, x, y + 5) << 5)
					 | (getMeltedPixel(frames, x, y + 4) << 4)
					 | (getMeltedPixel(frames, x, y + 3) << 3)
					 | (getMeltedPixel(frames, x, y + 2) << 2)
					 | (getMeltedPixel(frames, x, y + 1) << 1)
					 | getMeltedPixel(frames, x, y)
					);
			}
		}

		display();

		frames++;
	} while (frames < 30);
}

static bool getGradientPixel(uint8_t x, uint8_t y, uint8_t i) {
	if (i == 0) return 0;
	if (i >= GRADIENT_COUNT - 1) return 1;

	uint8_t index = fmax(0, fmin(GRADIENT_COUNT - 1, i)) * GRADIENT_WIDTH * GRADIENT_HEIGHT // gradient index
		  + y * GRADIENT_WIDTH % (GRADIENT_WIDTH * GRADIENT_HEIGHT)             // y byte offset
		  + x / GRADIENT_HEIGHT % GRADIENT_WIDTH;                               // x byte offset

	// return the bit based on x
	return read_bit(pgm_read_byte(gradient + index), x % 8);
}

void fadeScreen(uint8_t intensity, bool color) { // color = 0
	for (uint8_t x = 0; x < SCREEN_WIDTH; x++) {
		for (uint8_t y = 0; y < SCREEN_HEIGHT; y++) {
			if (getGradientPixel(x, y, intensity)) drawPixel(x, y, color, false);
		}
	}
}


// Faster drawPixel than display.drawPixel.
// Avoids some checks to make it faster.
void drawPixel(int8_t x, int8_t y, bool color, bool raycasterViewport) { // ray... = false
	// prevent write out of screen buffer
	if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= (raycasterViewport ? RENDER_HEIGHT : SCREEN_HEIGHT)) {
		return;
	}

	if (color) {
		// white
		display_buf[x + (y / 8)*SCREEN_WIDTH] |= (1 << (y & 7));
	} else {
		// black
		display_buf[x + (y / 8)*SCREEN_WIDTH] &= ~(1 << (y & 7));
	}
}

// For raycaster only
// Custom draw Vertical lines that fills with a pattern to simulate
// different brightness. Affected by RES_DIVIDER
void drawVLine(uint8_t x, int8_t start_y, int8_t end_y, uint8_t intensity) {
	int8_t y;
	int8_t lower_y = fmax(fmin(start_y, end_y), 0);
	int8_t higher_y = fmin(fmax(start_y, end_y), RENDER_HEIGHT - 1);
	uint8_t c;

	uint8_t bp;
	uint8_t b;
	for (c = 0; c < RES_DIVIDER; c++) {
		y = lower_y;
		b = 0;
		while (y <= higher_y) {
			bp = y % 8;
			b = b | getGradientPixel(x + c, y, intensity) << bp;

			if (bp == 7) {
				// write the whole byte
				drawByte(x + c, y, b);
				b = 0;
			}

			y++;
		}

		// draw last byte
		if (bp != 7) {
			drawByte(x + c, y - 1, b);
		}
	}
}

// draw Keys and Health methods.

void drawHealth(uint8_t health){
	sb_led_showLevel(health, 100);
}

void drawKeys(uint8_t keys){
	sb_7seg_showNumber(keys);
}

// Custom drawBitmap method with scale support, mask, zindex and pattern filling
void drawSprite(int8_t x, int8_t y, const uint8_t bitmap[], const uint8_t mask[], int16_t w, int16_t h, uint8_t sprite, double distance) {
	uint8_t tw = (double) w / distance;
	uint8_t th = (double) h / distance;
	uint8_t byte_width = w / 8;
	uint8_t pixel_size = fmax(1, 1.0 / distance);
	uint16_t sprite_offset = byte_width * h * sprite;

	bool pixel;
	bool maskPixel;

	// Don't draw the whole sprite if the anchor is hidden by z buffer
	// Not checked per pixel for performance reasons
	if (zbuffer[(int)fmin(fmax(x, 0), ZBUFFER_SIZE - 1) / Z_RES_DIVIDER] < distance * DISTANCE_MULTIPLIER) {
		return;
	}

	for (uint8_t ty = 0; ty < th; ty += pixel_size) {
		// Don't draw out of screen
		if (y + ty < 0 || y + ty >= RENDER_HEIGHT) {
			continue;
		}

		uint8_t sy = ty * distance; // The y from the sprite

		for (uint8_t tx = 0; tx < tw; tx += pixel_size) {
			uint8_t sx = tx * distance; // The x from the sprite
			uint16_t byte_offset = sprite_offset + sy * byte_width + sx / 8;

			// Don't draw out of screen
			if (x + tx < 0 || x + tx >= SCREEN_WIDTH) {
				continue;
			}

			maskPixel = read_bit(pgm_read_byte(mask + byte_offset), sx % 8);

			if (maskPixel) {
				pixel = read_bit(pgm_read_byte(bitmap + byte_offset), sx % 8);
				for (uint8_t ox = 0; ox < pixel_size; ox++) {
					for (uint8_t oy = 0; oy < pixel_size; oy++) {
						drawPixel(x + tx + ox, y + ty + oy, pixel, true);
					}
				}
			}
		}
	}
}

// Draw a single character.
// Made for a custom font with some useful sprites. Char size 4 x 6
// Uses less memory than display.print()
void drawChar(int8_t x, int8_t y, char ch) {
	uint8_t c = 0;
	uint8_t n;
	uint8_t bOffset;
	uint8_t b;
	uint8_t line = 0;

	// Find the character
	while (CHAR_MAP[c] != ch && CHAR_MAP[c] != '\0'){
		c++;
	}

	bOffset = c / 2;
	for (; line < CHAR_HEIGHT; line++) {
		b = pgm_read_byte(bmp_font + (line * bmp_font_width + bOffset));
		for (n = 0; n < CHAR_WIDTH; n++){
			if (read_bit(b, (c % 2 == 0 ? 0 : 4) + n)) drawPixel(x + n, y + line, 1, false);
		}
	}
}

// Draw a string
void drawText(int8_t x, int8_t y, char *txt, uint8_t space) {
	uint8_t pos = x;
	uint8_t i = 0;
	char ch;
	while ((ch = txt[i]) != '\0') {
		drawChar(pos, y, ch);
		i++;
		pos += CHAR_WIDTH + space;

		// shortcut on end of screen
		if (pos > SCREEN_WIDTH) return;
	}
}

// Draw an integer (3 digit max!)
void drawText_legacy(uint8_t x, uint8_t y, uint8_t num) {
	char buf[4]; // 3 char + \0
	itoa(num, buf, 10);
	drawText(x, y, buf, 0);
}
