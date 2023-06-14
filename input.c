#include <stdbool.h>
#include <avr/interrupt.h>
#include <math.h>

#include "adc.h"

#include "input.h"
#include "constants.h"

#define RELEASED 0
#define PRESSED 1

volatile uint8_t has_fired = 0;

ISR(INT0_vect){
	has_fired++;
}

void input_setup(void) {
	DDRD &= ~(1<<PD2);
	PORTD |= (1<<PD2);
	EICRA &= ~(1<<ISC00);
	EICRA |= (1<<ISC01);
	EIMSK |= (1<<INT0);

	DDRD &= ~(1<<PD3);
	PORTD |= (1<<PD3);
}

static uint8_t getButtonState(void){
	return (PIND & (1<<PD3)) == 0;
}

uint8_t input_left(void) {
	int16_t read = sb_adc_read(POTI);
  	if(read < 350 && getButtonState() == RELEASED){
  		return (350 - read) / 7;
  	}
 	return 0;
}

uint8_t input_right(void) {
	int16_t read = sb_adc_read(POTI);
	if(read > 674 && getButtonState() == RELEASED){
		return (read - 674) / 7;
	}
	return 0;
}

uint8_t input_up(void) {
	int16_t read = sb_adc_read(POTI);
	if(read < 350 && getButtonState() == PRESSED){
  		return (350 - read) / 7;
	}
	return 0;
}

uint8_t input_down(void) {
	int16_t read = sb_adc_read(POTI);
	if(read > 674 && getButtonState() == PRESSED){
		return (read - 674) / 7;
	}
	return 0;
}

uint8_t input_fire(void) {
	cli();
	if(has_fired){
		has_fired--;
		sei();
		return 1;
	}
	sei();
	return 0;
}
