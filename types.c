#include <stdint.h>
#include <math.h>

#include "types.h"
#include "constants.h"

static double sq(double val){
	return val * val;
}

struct Coords create_coords(double x, double y) {
	struct Coords ret = {x,y};
	return ret;
}

uint8_t coords_distance(struct Coords* a, struct Coords* b) {
  	return sqrt(sq(a->x - b->x) + sq(a->y - b->y)) * DISTANCE_MULTIPLIER;
}

UID create_uid(uint8_t type, uint8_t x, uint8_t y) {
  	return ((y << LEVEL_WIDTH_BASE) | x) << 4 | type;
}
  
uint8_t uid_get_type(UID uid) {
  	return uid & 0x0F;
}
