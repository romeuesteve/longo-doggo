/*
 * Dog object script: the long dog itself.  Fully tile-based — the head
 * snaps between cells and the body chain shifts along like a snake; the
 * view eases the sprites.  Owns movement rules, chain growth/shrink and
 * the pickups (apples, skulls, the win zone).
 */
#ifndef LONGO_OBJECT_DOG_H
#define LONGO_OBJECT_DOG_H

#include <stdbool.h>
#include <stdint.h>

#include "../core/world.h"

/* Per-part flags (parallel to the chain; part 0 is nearest the head).
 * FIRST: directly behind the head (front legs).
 * LEGS:  draws walking legs.
 * BUTT:  solid for movement probes; the last part is not solid so the
 *        dog (or a pushed box) can follow onto the vacated cell. */
#define DOG_PART_FIRST 0x1
#define DOG_PART_LEGS 0x2
#define DOG_PART_BUTT 0x4

void dog_reset(void);
void dog_place(float x, float y);         /* room placement (pixel coords) */
void dog_title_arrangement(void);         /* the title room's S-curve */

void dog_tick(const SimInput *input);

bool dog_alive(void);
bool dog_play(void);
void dog_set_play(bool play);
int dog_cx(void);
int dog_cy(void);
int dog_dir(void); /* 0 down, 90 right, 180 up, 270 left */
int dog_length(void);
uint16_t dog_part_cell(int part);
int dog_part_flags(int part);
bool dog_part_is_solid(int part); /* butt parts block; the tail does not */
bool dog_strain(void);
uint16_t dog_detached_cell(void); /* cell the tail last vacated */

/* Test/placement hooks. */
void dog_teleport(int cx, int cy);
void dog_set_alive(bool alive); /* test hook: despawn/respawn in place */
void dog_set_length(int length);

#endif /* LONGO_OBJECT_DOG_H */
