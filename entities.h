#ifndef _entities_h
#define _entities_h

#include "types.h"

#define create_enemy(x, y)            create_entity(E_ENEMY, x, y, S_STAND, 100)
#define create_medikit(x, y)          create_entity(E_MEDIKIT, x, y, S_STAND, 0)
#define create_key(x, y)              create_entity(E_KEY, x, y, S_STAND, 0)
#define create_fireball(x, y, dir)    create_entity(E_FIREBALL, x, y, S_STAND, dir)

// entity statuses
#define S_STAND               0
#define S_ALERT               1
#define S_FIRING              2
#define S_MELEE               3
#define S_HIT                 4
#define S_DEAD                5
#define S_HIDDEN              6
#define S_OPEN                7
#define S_CLOSE               8

struct Player { 
  struct Coords pos;
  struct Coords dir;
  struct Coords plane;
  double velocity;
  uint8_t health;
  uint8_t keys;  
};

struct Entity {
	UID uid;
	struct Coords pos;
	uint8_t state;
	uint8_t health;     // angle for fireballs
	uint8_t distance;
	uint8_t timer;
};

struct StaticEntity  { 
	UID uid;
	uint8_t x;
	uint8_t y;
	bool active;
};

struct Entity create_entity(uint8_t type, uint8_t x,  uint8_t y, uint8_t initialState, uint8_t initialHealth);
struct StaticEntity create_static_entity(UID uid, uint8_t x,  uint8_t y, bool active);
struct Player create_player(double x, double y); //, struct Coords dir, struct Coords plane, double velocity, uint8_t health);
struct Entity create_locked_door(uint8_t x, uint8_t y);
struct Entity create_door(uint8_t x, uint8_t y);

#endif
