#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <math.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "sprites.h"
#include "input.h"
#include "entities.h"
#include "types.h"
#include "display_wrap.h"
#include "constants.h"

#define swap(a, b)            do { typeof(a) temp = a; a = b; b = temp; } while (0)
#define sign(a, b)            (double) (a > b ? 1 : (b > a ? -1 : 0))

//extern variables

extern const uint8_t SCREEN_WIDTH;
extern const uint8_t SCREEN_HEIGHT;
extern const uint8_t HALF_WIDTH;
extern const uint8_t RENDER_HEIGHT;
extern const uint8_t HALF_HEIGHT;
extern volatile double delta;
extern uint8_t display_buf[1024];
extern uint8_t zbuffer[64];

// general
static volatile uint8_t scene = INTRO;
static volatile bool exit_scene = false;
static volatile bool invert_screen = false;
static volatile uint8_t flash_screen = 0;
static uint32_t timer;

// game
// player and entities
static volatile struct Player player;
static struct Entity entity[MAX_ENTITIES];
static struct StaticEntity static_entity[MAX_STATIC_ENTITIES];
static volatile uint8_t num_entities = 0;
static volatile uint8_t num_static_entities = 0;

static void check_eeprom(){
	eeprom_busy_wait();
	if(eeprom_read_byte(0) == 0xFF){
		display_installer_error();
	}
}

static void setup(void){
	setupDisplay();
	input_setup();
	check_eeprom();
	sei();
}

static void jumpTo(uint8_t target_scene){
	scene = target_scene;
	exit_scene = true;
}

static uint8_t getBlockAt(uint8_t x, uint8_t y){
	// filter edges
	if((x <= 0) || (x >= LEVEL_WIDTH - 2)){
		return E_WALL;
	}
	if((y <= 0) || (y >= LEVEL_HEIGHT - 2)){
		return E_WALL;
	}

	// shift indizes
	x = x - 1;
	y = y - 1;
	y = LEVEL_HEIGHT - 3 - 1 - y;

	// get position where block is in array
	eeprom_busy_wait();
	uint16_t first_info = eeprom_read_byte(3 * y);
	eeprom_busy_wait();
	uint16_t second_info = eeprom_read_byte(3 * y + 1);
	eeprom_busy_wait();
	uint16_t third_info = eeprom_read_byte(3 * y + 2); 
	uint16_t pos = ((first_info << 8) | second_info) >> 6;
	uint16_t last_floor = ((first_info << 8) | second_info) & 0b111111;
	uint16_t last_wall = third_info;

	// check whether to check array
	if(x > LEVEL_WIDTH - 3 - last_wall){ 
		return E_WALL;
	}
	if(x > LEVEL_WIDTH - 3 - last_wall - last_floor){ 
		return E_FLOOR;
	}

	// actually read byte
	eeprom_busy_wait();
	uint8_t fields = eeprom_read_byte(pos + (x>>1)); 
	return fields >> (!(x%2) * 4) & 0b1111;
}

// Finds the player in the map
static void initializeLevel(void) {
	eeprom_busy_wait();
	uint8_t x = eeprom_read_byte(1022);
	eeprom_busy_wait();
	uint8_t y = eeprom_read_byte(1023);
	if(x >= LEVEL_WIDTH || x == 0 || y == 0 || y >= LEVEL_HEIGHT){
		display_installer_error();	
	}
	player = create_player(x, y);
	return;
}

static bool isSpawned(UID uid) {
	for (uint8_t i = 0; i < num_entities; i++) {
		if (entity[i].uid == uid){
			return true;
		}
	}

	return false;
}

static bool isOpened(UID uid){
	for(uint8_t i = 0; i < num_entities; i++){
		if(entity[i].uid == uid && entity[i].state == S_OPEN) return true;
	}
	return false;
}

static void open_door(UID uid){
	uint8_t i = 0;
	while(i < num_entities){
		if(entity[i].uid == uid){
			entity[i].state = S_OPEN;
			return;
		}
		i++;
	}
	sb_7seg_showNumber(99);
}

static bool isStatic(UID uid) {
	for (uint8_t i = 0; i < num_static_entities; i++) {
		if (static_entity[i].uid == uid) {
			return true;
		}
	}

	return false;
}

static void spawnEntity(uint8_t type, uint8_t x, uint8_t y) {
	// Limit the number of spawned entities
	if (num_entities >= MAX_ENTITIES) {
		return;
	}

	// todo: read static entity status
	switch (type) {
		case E_ENEMY:
			entity[num_entities] = create_enemy(x, y);
			num_entities++;
			break;

		case E_KEY:
			entity[num_entities] = create_key(x, y);
			num_entities++;
			break;

		case E_MEDIKIT:
			entity[num_entities] = create_medikit(x, y);
			num_entities++;
			break;
		case E_DOOR:
			entity[num_entities] = create_door(x, y);
			num_entities++;
			break;
		case E_LOCKEDDOOR:
			entity[num_entities] = create_locked_door(x, y);
			num_entities++;
			break;
	}
}

static void spawnFireball(double x, double y) {
	// Limit the number of spawned entities
	if (num_entities >= MAX_ENTITIES) {
		return;
	}

	UID uid = create_uid(E_FIREBALL, x, y);
	// Remove if already exists, don't throw anything. Not the best, but shouldn't happen too often
	if (isSpawned(uid)) return;

	// Calculate direction. 32 angles
	int16_t dir = FIREBALL_ANGLES + atan2(y - player.pos.y, x - player.pos.x) / PI * FIREBALL_ANGLES;
	if (dir < 0) dir += FIREBALL_ANGLES * 2;
	entity[num_entities] = create_fireball(x, y, dir);
	num_entities++;
}

static void removeEntity(UID uid, bool makeStatic) { //makeStatic = false
	uint8_t i = 0;
	bool found = false;

	while (i < num_entities) {
		if (!found && entity[i].uid == uid) {
			// todo: doze it
			found = true;
			num_entities--;
		}

		// displace entities
		if (found) {
			// TODO: makeStatic
			entity[i] = entity[i + 1];
		}

		i++;
	}
}

static void removeStaticEntity(UID uid) {
	uint8_t i = 0;
	bool found = false;

	while (i < num_static_entities) {
		if (!found && static_entity[i].uid == uid) {
			found = true;
			num_static_entities--;
		}

		// displace entities
		if (found) {
			static_entity[i] = static_entity[i + 1];
		}

		i++;
	}
}

// Render values for the HUD
static void updateHud(void) {
	drawHealth(player.health);
	drawKeys(player.keys);
}

static UID detectCollision(struct Coords *pos, double relative_x, double relative_y, bool only_walls) { // only_walls = false
	// Wall collision
	uint8_t round_x = (int)(pos->x + relative_x);
	uint8_t round_y = (int)(pos->y + relative_y);
	uint8_t block = getBlockAt(round_x, round_y);

	if (block == E_WALL) {
		return create_uid(block, round_x, round_y);
	}

	if (only_walls) {
		return UID_null;
	}

	// Door collision
	if(block == E_DOOR){
		open_door(create_uid(block, round_x, round_y));
	}

	if(block == E_LOCKEDDOOR){
		if(!isOpened(create_uid(block, round_x, round_y))){
			if(player.keys){
				player.keys--;
				updateHud();
				open_door(create_uid(block, round_x, round_y));
			}
			else{
				return create_uid(block, round_x, round_y);
			}
		}
	}

	// Entity collision
	for (uint8_t i=0; i < num_entities; i++) {
		// Don't collide with itself
		if (&(entity[i].pos) == pos) {
			continue;
		}

		uint8_t type = uid_get_type(entity[i].uid);

		// Only ALIVE enemy collision
		if (type != E_ENEMY || entity[i].state == S_DEAD || entity[i].state == S_HIDDEN || entity[i].state == S_OPEN) {
			continue;
		}

		struct Coords new_coords = { entity[i].pos.x - relative_x, entity[i].pos.y - relative_y };
		uint8_t distance = coords_distance(pos, &new_coords);

		// Check distance and if it's getting closer
		if (distance < ENEMY_COLLIDER_DIST && distance < entity[i].distance) {
			return entity[i].uid;
		}
	}

	return UID_null;
}

static int translateIntoView(struct Coords *pos, struct Coords * buf) {
	//translate sprite position to relative to camera
	double sprite_x = pos->x - player.pos.x;
	double sprite_y = pos->y - player.pos.y;

	//required for correct matrix multiplication
	double inv_det = 1.0 / (player.plane.x * player.dir.y - player.dir.x * player.plane.y);
	double transform_x = inv_det * (player.dir.y * sprite_x - player.dir.x * sprite_y);
	double transform_y = inv_det * (- player.plane.y * sprite_x + player.plane.x * sprite_y); // Z in screen
	buf->x = transform_x;
	buf->y = transform_y;
	return 0;
}

// Shoot
static void fire(void) {
	for (uint8_t i = 0; i < num_entities; i++) {
		// Shoot only ALIVE enemies
		if (uid_get_type(entity[i].uid) != E_ENEMY || entity[i].state == S_DEAD || entity[i].state == S_HIDDEN) {
			continue;
		}

		struct Coords transform;
		translateIntoView(&(entity[i].pos), &transform);
		if (fabs(transform.x) < 20 && transform.y > 0) {
			uint8_t damage = (double) fmin(GUN_MAX_DAMAGE, GUN_MAX_DAMAGE / (fabs(transform.x) * entity[i].distance) / 5);
			if (damage > 0) {
				entity[i].health = fmax(0, entity[i].health - damage);
				entity[i].state = S_HIT;
				entity[i].timer = 4;
			}
		}
	}
}

// Update coords if possible. Return the collided uid, if any
static UID updatePosition(struct Coords *pos, double relative_x, double relative_y, bool only_walls) { // only_walls = false
	UID collide_x = detectCollision(pos, relative_x, 0, only_walls);
	UID collide_y = detectCollision(pos, 0, relative_y, only_walls);

	if (!collide_x) pos->x += relative_x;
	if (!collide_y) pos->y += relative_y;

	return collide_x || collide_y || UID_null;
}

static void updateEntities(void) {
	uint8_t i = 0;
	while (i < num_entities) {
		// update distance
		entity[i].distance = coords_distance(&(player.pos), &(entity[i].pos));

		// Run the timer. Works with actual frames.
		// TODO: use delta here. But needs double type and more memory
		if (entity[i].timer > 0) entity[i].timer--;

		// too far away. put it in doze mode
		if (entity[i].distance > MAX_ENTITY_DISTANCE && (uid_get_type(entity[i].uid) != E_DOOR && uid_get_type(entity[i].uid) != E_LOCKEDDOOR)) {
			removeEntity(entity[i].uid, false);
			// don't increase 'i', since current one has been removed
			continue;
		}

		// bypass render if hidden
		if (entity[i].state == S_HIDDEN) {
			i++;
			continue;
		}

		uint8_t type = uid_get_type(entity[i].uid);

		switch (type) {
			case E_ENEMY: {
				// Enemy "IA"
				if (entity[i].health == 0) {
					if (entity[i].state != S_DEAD) {
						entity[i].state = S_DEAD;
						entity[i].timer = 6;
					}
				} else  if (entity[i].state == S_HIT) {
					if (entity[i].timer == 0) {
						// Back to alert state
						entity[i].state = S_ALERT;
						entity[i].timer = 40;     // delay next fireball thrown
					}
				} else if (entity[i].state == S_FIRING) {
					if (entity[i].timer == 0) {
						// Back to alert state
						entity[i].state = S_ALERT;
						entity[i].timer = 40;     // delay next fireball throwm
					}
				} else {
					// ALERT STATE
					if (entity[i].distance > ENEMY_MELEE_DIST && entity[i].distance < MAX_ENEMY_VIEW) {
						if (entity[i].state != S_ALERT) {
							entity[i].state = S_ALERT;
							entity[i].timer = 20;   // used to throw fireballs
						} else {
							if (entity[i].timer == 0) {
								// Throw a fireball
								spawnFireball(entity[i].pos.x, entity[i].pos.y);
								entity[i].state = S_FIRING;
								entity[i].timer = 6;
							} else {
								// move towards to the player.
								updatePosition(
									&(entity[i].pos),
									sign(player.pos.x, entity[i].pos.x) * ENEMY_SPEED * delta,
									sign(player.pos.y, entity[i].pos.y) * ENEMY_SPEED * delta,
									true
									);
							}
						}
					} else if (entity[i].distance <= ENEMY_MELEE_DIST) {
						if (entity[i].state != S_MELEE) {
							// Preparing the melee attack
							entity[i].state = S_MELEE;
							entity[i].timer = 10;
						} else if (entity[i].timer == 0) {
							// Melee attack
							player.health = fmax(0, player.health - ENEMY_MELEE_DAMAGE);
							entity[i].timer = 14;
							flash_screen = 1;
							updateHud();
						}
					} else {
						// stand
						entity[i].state = S_STAND;
					}
				}
				break;
			}

			case E_FIREBALL: {
				if (entity[i].distance < FIREBALL_COLLIDER_DIST) {
					// Hit the player and disappear
					player.health = fmax(0, player.health - ENEMY_FIREBALL_DAMAGE);
					flash_screen = 1;
					updateHud();
					removeEntity(entity[i].uid, false);
					continue; // continue in the loop
				} else {
					// Move. Only collide with walls.
					// Note: using health to store the angle of the movement
					UID collided = updatePosition(
						&(entity[i].pos),
						cos((double) entity[i].health / FIREBALL_ANGLES * PI) * FIREBALL_SPEED,
						sin((double) entity[i].health / FIREBALL_ANGLES * PI) * FIREBALL_SPEED,
						true
						);

					if (collided) {
						removeEntity(entity[i].uid, false);
						continue; // continue in the entity check loop
					}
				}
				break;
			}

			case E_MEDIKIT: {
				if (entity[i].distance < ITEM_COLLIDER_DIST) {
					// pickup
					entity[i].state = S_HIDDEN;
					player.health = fmin(100, player.health + 50);
					updateHud();
					flash_screen = 1;
				}
				break;
			}

			case E_KEY: {
				if (entity[i].distance < ITEM_COLLIDER_DIST) {
					// pickup
					entity[i].state = S_HIDDEN;
					player.keys++;
					updateHud();
					flash_screen = 1;
				}
				break;
			}
		}

		i++;
	}
}

// The map raycaster. Based on https://lodev.org/cgtutor/raycasting.html
static void renderMap(double view_height) {
	UID last_uid;

	for (uint8_t x = 0; x < SCREEN_WIDTH; x += RES_DIVIDER) {
		double camera_x = 2 * (double) x / SCREEN_WIDTH - 1;
		double ray_x = player.dir.x + player.plane.x * camera_x;
		double ray_y = player.dir.y + player.plane.y * camera_x;
		uint8_t map_x = (uint8_t)(player.pos.x);
		uint8_t map_y = (uint8_t)(player.pos.y);
		struct Coords map_coords = { player.pos.x, player.pos.y };
		double delta_x = fabs(1 / ray_x);
		double delta_y = fabs(1 / ray_y);

		int8_t step_x; 
		int8_t step_y;
		double side_x;
		double side_y;

		if (ray_x < 0) {
			step_x = -1;
			side_x = (player.pos.x - map_x) * delta_x;
		} else {
			step_x = 1;
			side_x = (map_x + 1.0 - player.pos.x) * delta_x;
		}

		if (ray_y < 0) {
			step_y = -1;
			side_y = (player.pos.y - map_y) * delta_y;
		} else {
			step_y = 1;
			side_y = (map_y + 1.0 - player.pos.y) * delta_y;
		}

		// Wall detection
		uint8_t depth = 0;
		bool hit = 0;
		bool side; 
		while (!hit && depth < MAX_RENDER_DEPTH) {
			if (side_x < side_y) {
				side_x += delta_x;
				map_x += step_x;
				side = 0;
			} else {
				side_y += delta_y;
				map_y += step_y;
				side = 1;
			}

			uint8_t block = getBlockAt(map_x, map_y);

			if (block == E_WALL) {
				hit = 1;
			} else {
				// Spawning entities here, as soon they are visible for the
				// player. Not the best place, but would be a very performance
				// cost scan for them in another loop
				if (block == E_ENEMY || (block & 0b00001000) /* all collectable items */) {
					// Check that it's close to the player
					if (coords_distance(&(player.pos), &map_coords) < MAX_ENTITY_DISTANCE) {
						UID uid = create_uid(block, map_x, map_y);
						if (last_uid != uid && !isSpawned(uid)) {
							spawnEntity(block, map_x, map_y);
							last_uid = uid;
						}
					}
				}else if(block == E_DOOR || block == E_LOCKEDDOOR){
					if (coords_distance(&(player.pos), &map_coords) < MAX_ENTITY_DISTANCE) {
						UID uid = create_uid(block, map_x, map_y);
						if (last_uid != uid && !isSpawned(uid)) {
							spawnEntity(block, map_x, map_y);
							last_uid = uid;
						}
					}
				}
			}

			depth++;
		}

		if (hit) {
			double distance;

			if (side == 0) {
				distance = fmax(1, (map_x - player.pos.x + (1 - step_x) / 2) / ray_x);
			} else {
				distance = fmax(1, (map_y - player.pos.y + (1 - step_y) / 2) / ray_y);
			}

			// store zbuffer value for the column
			zbuffer[x / Z_RES_DIVIDER] = fmin(distance * DISTANCE_MULTIPLIER, 255);

			// rendered line height
			uint8_t line_height = RENDER_HEIGHT / distance;

			drawVLine(
				x,
				view_height / distance - line_height / 2 + RENDER_HEIGHT / 2,
				view_height / distance + line_height / 2 + RENDER_HEIGHT / 2,
				GRADIENT_COUNT - (int)(distance / MAX_RENDER_DEPTH * GRADIENT_COUNT) - side * 2
				);
		}
	}
}

// Sort entities from far to close
static uint8_t sortEntities(void) {
	uint8_t gap = num_entities;
	bool swapped = false;
	while (gap > 1 || swapped) {
		//shrink factor 1.3
		gap = (gap * 10) / 13;
		if (gap == 9 || gap == 10) gap = 11;
		if (gap < 1) gap = 1;
		swapped = false;
		for (uint8_t i = 0; i < num_entities - gap; i++){
			uint8_t j = i + gap;
			if (entity[i].distance < entity[j].distance){
				swap(entity[i], entity[j]);
				swapped = true;
			}
		}
	}
	return 0;
}

static void renderEntities(double view_height) {
	sortEntities();

	for (uint8_t i = 0; i < num_entities; i++) {
		if (entity[i].state == S_HIDDEN || entity[i].state == S_OPEN){
			continue;
		}

		struct Coords transform;
		translateIntoView(&(entity[i].pos), &transform);

		// don´t render if behind the player or too far away
		if (transform.y <= 0.1 || transform.y > MAX_SPRITE_DEPTH) {
			continue;
		}

		int16_t sprite_screen_x = HALF_WIDTH * (1.0 + transform.x / transform.y);
		int8_t sprite_screen_y = RENDER_HEIGHT / 2 + view_height / transform.y;
		uint8_t type = uid_get_type(entity[i].uid);

		// don´t try to render if outside of screen
		// doing this pre-shortcut due int16 -> int8 conversion makes out-of-screen
		// values fit into the screen space
		if (sprite_screen_x < - HALF_WIDTH || sprite_screen_x > SCREEN_WIDTH + HALF_WIDTH) {
			continue;
		}

		switch (type) {
			case E_ENEMY: {
				uint8_t sprite;
				if (entity[i].state == S_ALERT) {
					// walking
					sprite = (int) (timer / 500) % 2;
				} else if (entity[i].state == S_FIRING) {
					// fireball
					sprite = 2;
				} else if (entity[i].state == S_HIT) {
					// hit
					sprite = 3;
				} else if (entity[i].state == S_MELEE) {
					// melee atack
					sprite = entity[i].timer > 10 ? 2 : 1;
				} else if (entity[i].state == S_DEAD) {
					// dying
					sprite = entity[i].timer > 0 ? 3 : 4;
				} else {
					// stand
					sprite = 0;
				}

				drawSprite(
					sprite_screen_x - BMP_IMP_WIDTH * .5 / transform.y,
					sprite_screen_y - 8 / transform.y,
					bmp_imp_bits,
					bmp_imp_mask,
					BMP_IMP_WIDTH,
					BMP_IMP_HEIGHT,
					sprite,
					transform.y
					);
				break;
			}

			case E_FIREBALL: {
				drawSprite(
					sprite_screen_x - BMP_FIREBALL_WIDTH / 2 / transform.y,
					sprite_screen_y - BMP_FIREBALL_HEIGHT / 2 / transform.y,
					bmp_fireball_bits,
					bmp_fireball_mask,
					BMP_FIREBALL_WIDTH,
					BMP_FIREBALL_HEIGHT,
					0,
					transform.y
					);
				break;
			}

			case E_MEDIKIT: {
				drawSprite(
					sprite_screen_x - BMP_ITEMS_WIDTH / 2 / transform.y,
					sprite_screen_y + 5 / transform.y,
					bmp_items_bits,
					bmp_items_mask,
					BMP_ITEMS_WIDTH,
					BMP_ITEMS_HEIGHT,
					0,
					transform.y
					);
				break;
			}

			case E_KEY: {
				drawSprite(
					sprite_screen_x - BMP_ITEMS_WIDTH / 2 / transform.y,
					sprite_screen_y + 5 / transform.y,
					bmp_items_bits,
					bmp_items_mask,
					BMP_ITEMS_WIDTH,
					BMP_ITEMS_HEIGHT,
					1,
					transform.y
					);
				break;
			}
			case E_LOCKEDDOOR:
			case E_DOOR:{
				drawSprite(
					sprite_screen_x - BMP_DOOR_WIDTH * .5 / transform.y,
					sprite_screen_y - 8 / transform.y,
					bmp_door_bits,
					bmp_door_bits,
					BMP_DOOR_WIDTH,
					BMP_DOOR_HEIGHT,
					0,
					transform.y
						);
				break;
		    }
		}
	}
}

static void renderGun(uint8_t gun_pos, double amount_jogging) {
	// jogging
	char x = 48 + sin((double) timer * JOGGING_SPEED) * 10 * amount_jogging;
	char y = RENDER_HEIGHT - gun_pos + fabs(cos((double) timer * JOGGING_SPEED)) * 8 * amount_jogging;

	if (gun_pos > GUN_SHOT_POS - 2) {
		// Gun fire
		drawBitmap(x + 6, y - 11, bmp_fire_bits, BMP_FIRE_WIDTH, BMP_FIRE_HEIGHT, 1);
	}

	// Don't draw over the hud!
	uint8_t clip_height = fmax(0, fmin(y + BMP_GUN_HEIGHT, RENDER_HEIGHT) - y);

	// Draw the gun (black mask + actual sprite).
	drawBitmap(x, y, bmp_gun_mask, BMP_GUN_WIDTH, clip_height, 0);
	drawBitmap(x, y, bmp_gun_bits, BMP_GUN_WIDTH, clip_height, 1);
}

// Debug stats
static void renderStats(void) {
	clearRect(58, 58, 70, 6);
	drawText_legacy(114, 58, (int) (getActualFps()));
	drawText_legacy(82, 58, num_entities);
}

// Intro screen
static void loopIntro(void) {
	drawBitmap(
		(SCREEN_WIDTH - BMP_LOGO_WIDTH) / 2,
		(SCREEN_HEIGHT - BMP_LOGO_HEIGHT) / 3,
		bmp_logo_bits,
		BMP_LOGO_WIDTH,
		BMP_LOGO_HEIGHT,
		1
		);
	display();
	sb_timer_delay(1000);
	drawText(SCREEN_WIDTH / 2 - 25, SCREEN_HEIGHT * .8, "PRESS FIRE", 1);
	display();
	// wait for fire
	// TODO: Interrupt
	while (!exit_scene) {
		if(input_fire()){
			jumpTo(GAME_PLAY);
		}
	}
}

static void loopGamePlay(void) {
	bool gun_fired = false;
	uint8_t gun_pos = 0;
	double rot_speed;
	double old_dir_x;
	double old_plane_x;
	double view_height;
	double jogging;
	uint8_t fade = GRADIENT_COUNT - 1;

	initializeLevel();

	do {
		fps();
		// Clear only the 3d view
		memset(display_buf, 0, SCREEN_WIDTH * (RENDER_HEIGHT / 8));


		// If the player is alive
		if (player.health > 0) {
			// Player speed
			uint8_t speed = 0;
			if (speed = input_up()) {
				player.velocity += (MOV_SPEED - player.velocity) * .4 * speed/SPEED_MAX;
				jogging = fabs(player.velocity) * MOV_SPEED_INV;
			} else if (speed = input_down()) {
				player.velocity += (- MOV_SPEED - player.velocity) * .4 * speed/SPEED_MAX;
				jogging = fabs(player.velocity) * MOV_SPEED_INV;
			} else {
				player.velocity *= .5;
				jogging = fabs(player.velocity) * MOV_SPEED_INV;
			}

			// Player rotation
			if (speed = input_right()) {
				rot_speed = ROT_SPEED * delta * speed/SPEED_MAX;
				old_dir_x = player.dir.x;
				player.dir.x = player.dir.x * cos(-rot_speed) - player.dir.y * sin(-rot_speed);
				player.dir.y = old_dir_x * sin(-rot_speed) + player.dir.y * cos(-rot_speed);
				old_plane_x = player.plane.x;
				player.plane.x = player.plane.x * cos(-rot_speed) - player.plane.y * sin(-rot_speed);
				player.plane.y = old_plane_x * sin(-rot_speed) + player.plane.y * cos(-rot_speed);
			} else if (speed = input_left()) {
				rot_speed = ROT_SPEED * delta * speed/SPEED_MAX;
				old_dir_x = player.dir.x;
				player.dir.x = player.dir.x * cos(rot_speed) - player.dir.y * sin(rot_speed);
				player.dir.y = old_dir_x * sin(rot_speed) + player.dir.y * cos(rot_speed);
				old_plane_x = player.plane.x;
				player.plane.x = player.plane.x * cos(rot_speed) - player.plane.y * sin(rot_speed);
				player.plane.y = old_plane_x * sin(rot_speed) + player.plane.y * cos(rot_speed);
			}

			view_height = fabs(sin((double) timer * JOGGING_SPEED)) * 6 * jogging;

			// Update gun
			if (gun_pos > GUN_TARGET_POS) {
				// Right after fire
				gun_pos -= 1;
			} else if (gun_pos < GUN_TARGET_POS) {
				// Showing up
				gun_pos += 2;
			} else if (!gun_fired && input_fire()) {
				// ready to fire and fire pressed
				gun_pos = GUN_SHOT_POS;
				gun_fired = true;
				fire();
			} else if (gun_fired && !input_fire()) {
				// just fired and restored position
				gun_fired = false;
			}
		} else {
			// The player is dead
			if (view_height > -10) view_height--;
			else if (input_fire()) jumpTo(INTRO);

			if (gun_pos > 1) gun_pos -= 2;
		}

		// Player movement
		if (fabs(player.velocity) > 0.003) {
			updatePosition(
				&(player.pos),
				player.dir.x * player.velocity * delta,
				player.dir.y * player.velocity * delta,
				false
				);
		} else {
			player.velocity = 0;
		}
		// Update things
		updateEntities();
		// Render stuff
		renderMap(view_height);
		renderEntities(view_height);
		renderGun(gun_pos, jogging);
		// Fade in effect
		if (fade > 0) {
			fadeScreen(fade, 0);
			fade--;

			if (fade == 0) {
				// Only draw the hud after fade in effect
				updateHud();
			}
		} else {
			renderStats();
		}
		// flash screen
		if (flash_screen > 0) {
			invert_screen = !invert_screen;
			flash_screen--;
		} else if (invert_screen) {
			invert_screen = 0;
		}

		// Draw the frame
		invertDisplay(invert_screen);
		display();

		// Exit routine
		if (input_left() && input_right()) {
			jumpTo(INTRO);
		}
	} while (!exit_scene);
}

static uint8_t loop(void) {
	switch (scene) {
		case INTRO: {
			loopIntro();
			break;
		}
		case GAME_PLAY: {
			loopGamePlay();
			break;
		}
	}
	exit_scene = false;
	meltScreen();
	return 0;
}

void main(void){
	setup();
	while(1){
		uint8_t ret = loop();
	}
}

// end
