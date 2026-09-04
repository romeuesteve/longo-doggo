#include "events.h"

#include <string.h>

static SoundEvent sounds[EVENTS_MAX_SOUNDS];
static int sound_cnt;
static FxEvent fx[EVENTS_MAX_FX];
static int fx_cnt;

void events_clear(void)
{
    sound_cnt = 0;
    fx_cnt = 0;
}

void events_sound(int sound, int loop)
{
    if (sound_cnt >= EVENTS_MAX_SOUNDS) return;
    sounds[sound_cnt].sound = sound;
    sounds[sound_cnt].loop = loop;
    sound_cnt++;
}

void events_fx(FxKind kind, float x, float y, float tx, float ty, float angle,
               int count, int variant)
{
    if (fx_cnt >= EVENTS_MAX_FX) return;
    FxEvent *e = &fx[fx_cnt++];
    e->kind = kind;
    e->x = x;
    e->y = y;
    e->tx = tx;
    e->ty = ty;
    e->angle = angle;
    e->count = count;
    e->variant = variant;
}

int events_poll_sounds(SoundEvent *out)
{
    int n = sound_cnt;
    if (n > 0 && out) memcpy(out, sounds, sizeof(SoundEvent) * (size_t)n);
    sound_cnt = 0;
    return n;
}

int events_poll_fx(FxEvent *out)
{
    int n = fx_cnt;
    if (n > 0 && out) memcpy(out, fx, sizeof(FxEvent) * (size_t)n);
    fx_cnt = 0;
    return n;
}
