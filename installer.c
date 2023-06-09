#include "level.h"

#include <display.h>
#include <button.h>
#include <timer.h>
#include <7seg.h>

#include <avr/eeprom.h>
#include <avr/interrupt.h>

static void init(void){
	sb_display_enable();
	sb_display_fillScreen(NULL);
}

static void start_installation(void){
	for(uint16_t i = 0; i < 1024; i++){
		eeprom_busy_wait();
		eeprom_update_byte(i, *(sto_level_1 + i));
		// eeprom_busy_wait();
		// sb_7seg_showHexNumber(eeprom_read_byte(i));
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
	
	while(sb_button_getState(BUTTON0) == RELEASED){}
	sb_display_showString(7, 5, "Starting...");
	start_installation();

	sb_display_showString(7, 3, "Finished! GLHF");
	while(1);
}
