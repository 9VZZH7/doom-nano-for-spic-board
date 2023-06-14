#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "entities.h"
#include "types.h"
#include "constants.h"

struct Entity create_entity(uint8_t type, uint8_t x,  uint8_t y, uint8_t initialState, uint8_t initialHealth) {
	UID uid = create_uid(type, x, y);
	struct Coords pos = create_coords((double) x + .5, (double) y + .5);
	struct Entity new_entity = { uid, pos, initialState, initialHealth, 0, 0 };
	return new_entity;
}

struct StaticEntity create_static_entity(UID uid, uint8_t x,  uint8_t y, bool active) {
	struct StaticEntity ret = { uid, x, y, active };
  	return ret;
}

struct Player create_player(double x, double y){ //, struct Coords dir, struct Coords plane, double velocity, uint8_t health){
	struct Player play = {create_coords(x + 0.5, y + 0.5), create_coords(1, 0), create_coords(0, -0.66), 0, 100, 0};
	return play;
}

struct OpenDoor create_open_door(UID uid, uint8_t x, uint8_t y){
	struct OpenDoor door = {uid, create_coords(x, y), x, y};
	return door;
}

