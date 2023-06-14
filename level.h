#ifndef _level_h
#define _level_h

#include <stdint.h>

/*
  Based on E1M1 from Wolfenstein 3D

  ################################################################
  #############################...........########################
  ######....###################........E..########################
  ######....########..........#...........#...####################
  ######.....#######..........L.....E.......M.####################
  ######.....#######..........#...........#...####################
  ##################...########...........########################
  ######.........###...########...........########################
  ######.........###...#############D#############################
  ######.........#......E##########...############################
  ######....E....D...E...##########...############################
  ######.........#.......##########...############################
  ######....E....##################...############################
  #...##.........##################...############################
  #.K.######D######################...############################
  #...#####...###############...#E.....K##########################
  ##D######...###############..####...############################
  #...#####...###############..####...############################
  #...#...#...###############..####...############################
  #...D...#...#####################...############################
  #...#...#...#####################...############################
  #...######D#######################L#############################
  #.E.##.........#################.....#################........##
  #...##.........############...............############........##
  #...##...E.....############...............############........##
  #....#.........############...E.......E....#.........#........##
  #....L....K....############................D....E....D....E...##
  #....#.........############................#.........#........##
  #...##.....E...############...............####....####........##
  #...##.........############...............#####..#####.....M..##
  #...##.........#################.....##########..#####........##
  #...######L#######################D############..###############
  #...#####...#####################...###########..###############
  #E.E#####...#####################...###########..###############
  #...#...#...#####################.E.###########..###############
  #...D.M.#...#####################...###########..###############
  #...#...#...#####################...###########..###.#.#.#.#####
  #...#####...#####################...###########...#.........####
  #...#####...#####################...###########...D....E..K.####
  #................##......########...###########...#.........####
  #....E........E...L...E...X######...################.#.#.#.#####
  #................##......########...############################
  #################################...############################
  #############..#..#..#############L#############################
  ###########....#..#.########....#...#....#######################
  #############.....##########.P..D...D....#######################
  ############################....#...#....#######################
  ##############..#################...############################
  ##############..############....#...#....#######################
  ############################....D...D....#######################
  ############################....#...#....#######################
  #################################...############################
  ############################.............#######################
  ############################..........EK.#######################
  ############################.............#######################
  ################################################################
*/

/*
   Same map above built from some regexp replacements using the legend above.
   Using this way lets me use only 4 bit to store each block
*/

/*
	The first 9 lines consist of 3 Bytes each, totaling 6 * 54 Bytes. Every block of 3 Bytes is used to save information about one line in the level above. 
	As there are 3 full rows of wall, only 54 instead of 57 are needed. 
	Start: The first Byte of each block and the leftmost 2 Bits of the second Byte from a 10Bit number indicating at which address in the 1024Byte array the infromation aout 
		the line starts. 
	Floor: From each row, the last continuous block of floor is not saved in blocks but as an 6Bit value as the rightmost Bits of the second Byte
	Wall: The same holds for walls. However the information about how much wall the row has at the end is saved as the third Byte.
	
	Example:
	Lets look at the first block of three Bytes, 0x28, 0x8b, 0x17. This block has information about row two in the image above.
	((0x28 << 8) | 0x8b) >> 6 extracts the Start position, 162. Considering we store 3*54 Bytes as line info, the 162th Byte is the first byte with an actual block!
	0x8b & 0b111111 gets the amount of floor at the end of the row, 11.
	0x17 is the amount of wall at the end of the row, 23.
	We now know, that the last 34 Blocks of the row do not need to be saved, so starting at position 162, only 27 of 61 blocks are stored. Two blocks can be packed into one Byte. 14 Byte are needed.
	From the second block of 3 Bytes, we extract the start of the next row to be 176, which equals 162 + 14!
	
	Some rows in the level are the same, this can be exploited to save even less blocks as individual values and enables to reuse already saved information. 
	This can be seen when looking at the 3rd and 5th group of three Byte.

	The last two bytes are reserved for Player placement for now.
*/
const static uint8_t sto_level_1[1024] = {
	0x28, 0x8b, 0x17, 	0x2c, 0x02, 0x17, 	0x30, 0xc3, 0x13, 	0x35, 0xc1, 0x13, 	0x30, 0xc3, 0x13, 	0x3b, 0x0b, 0x17,
	0x3e, 0x8b, 0x17, 	0x42, 0x00, 0x1c, 	0x46, 0x43, 0x1b, 	0x4a, 0x43, 0x1b, 	0x46, 0x43, 0x1b, 	0x4e, 0x43, 0x1b,
	0x52, 0x43, 0x1b, 	0x56, 0x43, 0x1b, 	0x5a, 0x43, 0x1b, 	0x5e, 0x40, 0x19, 	0x5a, 0x43, 0x1b, 	0x63, 0x03, 0x1b, 
	0x67, 0x03, 0x1b, 	0x63, 0x03, 0x1b, 	0x6b, 0x00, 0x1c, 	0x6f, 0x48, 0x01, 	0x76, 0x08, 0x01, 	0x7c, 0xc8, 0x01, 
	0x83, 0x88, 0x01, 	0x8a, 0x43, 0x01, 	0x83, 0x88, 0x01, 	0x91, 0x88, 0x01, 	0x98, 0x42, 0x01, 	0x9f, 0xc8, 0x01, 
	0xa6, 0x82, 0x0e, 	0xac, 0x42, 0x0e, 	0xb2, 0x02, 0x0e, 	0xb7, 0xc2, 0x0e, 	0xbd, 0x82, 0x0e, 	0xb7, 0xc2, 0x0e, 
	0xac, 0x42, 0x0e, 	0xc3, 0x41, 0x04, 	0xca, 0x89, 0x03, 	0xd0, 0xc1, 0x03, 	0xca, 0x89, 0x03, 	0xd8, 0x01, 0x04, 
	0xdf, 0x40, 0x1c, 	0xe3, 0x84, 0x16, 	0xe8, 0x04, 0x16, 	0xe3, 0x84, 0x16, 	0xec, 0x83, 0x1b, 	0xe3, 0x84, 0x16, 
	0xe8, 0x04, 0x16, 	0xe3, 0x84, 0x16, 	0xec, 0x83, 0x1b, 	0xf0, 0x8d, 0x16, 	0xf4, 0x01, 0x16, 	0xf0, 0x8d, 0x16,
	
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x0, 0x0, 0x0, 0x0, 0x20, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf0, 0x0, 0x0, 0x0, 0x0, 0x5, 0x0, 0x0, 0x2, 0x0, 0x0, 0x0, 0x8, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf0, 0x0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf0, 0x0, 0x0, 0x0, 0x0, 0xff, 0xf0, 0x0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf0, 0x0, 0x0, 0x0, 0x0, 0xff, 0xf0, 0x0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf4, 0xff, 0xff, 0xf0, 0x0, 0x0, 0x0, 0x0, 0xf0, 0x0, 0x0, 0x0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf0, 0x0, 0x2, 0x0, 0x0, 0x40, 0x0, 0x20, 0x2, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf0, 0x0, 0x2, 0x0, 0x0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x0, 0xf, 0xf0, 0x0, 0x0, 0x0, 0x0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x9, 0xf, 0xff, 0xff, 0xf4, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x0, 0xf, 0xff, 0xff, 0x0, 0xf, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf4, 0xff, 0xff, 0xff, 0x0, 0xf, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x20, 0x0, 0x0, 0x90, 0x0, 0xf, 0x0, 0xf, 0x0, 0xf, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x0, 0x4, 0x0, 0xf, 0x0, 0xf, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x2, 0xf, 0xff, 0xff, 0xf4, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf5, 0x0, 0xf, 0xf0, 0x0, 0x0, 0x0, 0x0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf0, 0x0, 0x0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf0, 0x0, 0xf, 0xf0, 0x0, 0x0, 0x0, 0x0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xf, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf0, 0x0, 0xf, 0xf0, 0x0, 0x20, 0x0, 0x0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xf, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf0, 0x0, 0x0, 0xf0, 0x0, 0x0, 0x0, 0x0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xf0, 0x0, 0x0, 0x0, 0x0, 0xf0, 0x0, 0x0, 0x50, 0x0, 0x9, 0x0, 0x0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x0, 0x2, 0x0, 0x0, 0x0, 0x2, 0x0, 0x0, 0x40, 0x0, 0x2, 0x0, 0x0, 0x40, 0x0, 0x2, 0x0, 0xf, 0xf0, 0x0, 0x0, 0x20, 0x0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xf, 0xff, 0xf0, 0x0, 0xf, 0xff, 0xf0, 0x0, 0xf, 0xf0, 0x0, 0x0, 0x0, 0x0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xf, 0xff, 0xff, 0x0, 0xff, 0xff, 0xf0, 0x0, 0x0, 0x80, 0x0, 0xf, 0xf0, 0x0, 0x0, 0x0, 0x0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf0, 0x0, 0x0, 0xff, 0xff, 0xff, 0xff, 0xff, 0x0, 0xff, 0xff, 0xf0, 0x0, 0xf, 0xff, 0xff, 0xf5, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf4, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x0, 0xf, 0xff, 0xff, 0x0, 0xf, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x0, 0xf, 0xff, 0xff, 0xff, 0xff, 0xff, 0x20, 0x2f, 0xff, 0xff, 0x0, 0xf, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x0, 0xf, 0xff, 0xff, 0xff, 0xff, 0xff, 0x0, 0xf, 0x0, 0xf, 0x0, 0xf, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x0, 0xf, 0xff, 0xff, 0xff, 0xff, 0xff, 0x0, 0x4, 0x8, 0xf, 0x0, 0xf, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x2, 0xf, 0xff, 0xff, 0xff, 0xff, 0xff, 0x0, 0xf, 0xff, 0xff, 0x0, 0xf, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x0, 0xf, 0xff, 0xff, 0xff, 0xff, 0xff, 0x0, 0xff, 0xf0, 0xf0, 0xf0, 0xf0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xff, 0x0, 0x0, 0x0, 0xff, 0xff, 0xff, 0xff, 0x0, 0xf, 0xff, 0xff, 0xff, 0xff, 0xff, 0x0, 0xf, 0x0, 0x0, 0x20, 0x0, 0x0, 0x0, 0x2, 0x0, 0x5, 0x0, 0x2, 0x0, 0x7, 0xff, 0xff, 0xff, 0x0, 0xf, 0xff, 0xff, 0xff, 0xff, 0xff, 0x0, 0x4, 0x0, 0x0, 0x20, 0x9, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x0, 0xf, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf0, 0xf0, 0xf0, 0xf0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf5, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf0, 0x0, 0xf, 0x0, 0xf, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf0, 0x00, 0x4, 0x0, 0x4, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf0, 0x0, 0x0, 0x0, 0x0, 0x2, 0x90, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 29, 10};

#endif

