#include <stdbool.h>

#include "adc.h"
#include "button.h"
#include <avr/interrupt.h>
#include <math.h>

#include "input.h"
#include "constants.h"

volatile bool has_fired = false;

static void fire_fun(BUTTON btn, BUTTONEVENT event){
	has_fired = true;
}

void input_setup() {
  sei();
  sb_button_registerCallback(BUTTON0, ONPRESS, &fire_fun);
}

uint8_t input_left() {
	int16_t read = sb_adc_read(POTI);
  if(read < 200 && sb_button_getState(BUTTON1) == RELEASED){
  	return fmin(200, 325 - read);
  }
  return 0;
};

uint8_t input_right() {
	int16_t read = sb_adc_read(POTI);
  if(read > 824 && sb_button_getState(BUTTON1) == RELEASED){
  	return fmin(1024, read - 824 + 125);
  }
  return 0;
};

uint8_t input_up() {
	int16_t read = sb_adc_read(POTI);
  if(read < 200 && sb_button_getState(BUTTON1) == PRESSED){
  	return fmin(200, 325 - read);
  }
  return 0;
};

uint8_t input_down() {
	int16_t read = sb_adc_read(POTI);
  if(read > 824 && sb_button_getState(BUTTON1) == PRESSED){
  	return fmin(1024, read - 824 + 125);
  }
  return 0;
};

bool input_fire() {
  cli();
  bool fire = has_fired;
  has_fired = false;
  sei();
  return fire;
};
