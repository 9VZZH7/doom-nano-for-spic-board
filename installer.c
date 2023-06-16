#include "level.h"

#include <display.h>

#include <avr/sleep.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>

static volatile uint8_t install = 0;
static volatile uint8_t erase = 0;

ISR(INT0_vect){
	install = 1;
}

ISR(INT1_vect){
	erase = 1;
}

static void init(void){
	// configure button 0
	DDRD &= ~(1 << PD2);
	PORTD |= (1 << PD2);
	EICRA &= ~(1 << ISC00);
	EICRA |= (1 << ISC01);
	EIMSK |= (1 << INT0);
	// config button 1
	DDRD &= ~(1 << PD3);
	PORTD |= (1 << PD3);
	EICRA &= ~(1 << ISC10);
	EICRA |= (1 << ISC11);
	EIMSK |= (1 << INT1);
	// config display
	sb_display_enable();
	sb_display_fillScreen(NULL);
}

static void start_installation(void){
	for(uint16_t i = 0; i < 1024; i++){
		eeprom_busy_wait();
		eeprom_update_byte(i, *(sto_level_1 + i));
	}
	return;
}

static void start_erase(void){
	for(uint16_t i = 0; i < 1024; i++){
		eeprom_busy_wait();
		eeprom_update_byte(i, 0xFF); 
	}
	return;
}

void main(void){
	sei();
	init();

	const char str1[] = "This installer uses the on-";
	const char str2[] = "board EEPROM memory. Press";
	const char str3[] = "BUTTON0 to continue.";
	const char str4[] = "DO NOT INTERRUPT THE PROCESS";
	sb_display_showStringSmall(1, 0, str1);
	sb_display_showStringSmall(2, 0, str2);
	sb_display_showStringSmall(3, 0, str3);
	sb_display_showStringSmall(4, 0, str4);

	sleep_enable();
	cli();
	while(install == 0 && erase == 0){
		sei();
		sleep_cpu();
		cli();
	}
	sei();
	sleep_disable();
	
	if(install){
		sb_display_showString(7, 5, "Starting...");
		start_installation();
		sb_display_showString(7, 3, "Finished! GLHF");
		while(1);
	}
	if(erase){
		sb_display_showString(7, 5, "Starting...");
		start_erase();
		sb_display_showString(7, 3, "ERASED EEPROM");
		while(1);
	}

}
