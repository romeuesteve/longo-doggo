/*
 * Undo history: a stack of verbatim board snapshots.
 *
 * Object scripts own their state, so each one that holds gameplay state
 * also owns its snapshot: a plain struct plus capture/restore (see the
 * *_capture/*_restore pairs in objects/ and core/solid.h).  This module
 * owns the stack and the timing: the dog wraps every real step in
 * undo_begin_step()/undo_commit_step(), and an undo press restores the
 * newest snapshot (the tick then runs on over the restored board).
 *
 * Only board state is captured.  Cosmetics (rng, fx, view easing) and
 * meta objects (dialogue, transition) keep running across an undo, and
 * a room load drops the history.
 */
#ifndef LONGO_UNDO_H
#define LONGO_UNDO_H

#include <stdbool.h>

#include "world.h"

/* Drop the history (room load / restart). */
void undo_reset(void);

/* Capture the board before a step mutates it, then keep the capture
 * once the step has actually moved something.  A failed step never
 * commits, so blocked attempts leave no phantom undo points. */
void undo_begin_step(void);
void undo_commit_step(void);

/* Restore the newest snapshot; false when the history is empty.  Plays
 * the undo sound. */
bool undo_pop(void);

/* The Z press.  (The other undo input — stepping backwards into the
 * dog's own neck — resolves in dog.c, which owns facing.) */
bool undo_tick(const SimInput *input);

#endif /* LONGO_UNDO_H */
