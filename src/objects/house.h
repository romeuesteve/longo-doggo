/*
 * House object script: the goal (goal counter + open-house pulse state)
 * and the win zone in front of it.  The number on the house is the length
 * the dog still has to lose (dog length - 2); at zero the dog may enter
 * the win zone to finish the level.
 */
#ifndef LONGO_OBJECT_HOUSE_H
#define LONGO_OBJECT_HOUSE_H

#include <stdbool.h>
#include <stdint.h>

#define HOUSE_WIN_ZONE_MAX 16

void house_reset(void);
void house_place_goal(uint16_t goal_cell);
void house_place_win_zone(const uint16_t *cells, int count);

/* remain := dog_length - 2 (pass -1 when the dog is dead: stays stale,
 * like the original); plays the win jingle on the ready edge. */
void house_tick(int dog_length);

bool house_alive(void);
uint16_t house_goal_cell(void);
int house_remain(void);
bool house_win_ready(void);
bool house_win_alive(void);

/* Head entered the win zone while the house is ready?  Consumes the win
 * (the caller arms the next-level transition). */
uint16_t house_win_zone_cell(int index); /* test/render readback */

bool house_try_win(uint16_t head_cell);

#endif /* LONGO_OBJECT_HOUSE_H */
