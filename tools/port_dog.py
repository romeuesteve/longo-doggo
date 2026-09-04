"""Final extraction: dog/door/button scripts; world.c becomes orchestration."""
import pathlib, re

root = pathlib.Path('.')

# ---------- dog.c: own constants instead of world.h ones ----------
p = root / 'src/objects/dog.c'
s = p.read_text(encoding='utf-8')
s = s.replace("""#define DOG_MAX_CHAIN SIM_MAX_CHAIN
#define DOG_STEP_INTERVAL SIM_STEP_INTERVAL""",
"""#define DOG_MAX_CHAIN 64
#define DOG_STEP_INTERVAL 2 /* one cell every 2 ticks while held (30 cells/s)""")
p.write_text(s, encoding='utf-8')

# ---------- world.h: drop absorbed types ----------
p = root / 'src/core/world.h'
s = p.read_text(encoding='utf-8')

# SimDog struct block
start = s.find('typedef struct SimDog {')
end = s.find('/* Buttons cover one or two cells')
assert start > 0 and end > start
s = s[:start] + s[end:]

# SimButton + SimDoor blocks (from the button comment to the house comment)
start = s.find('/* Buttons cover one or two cells')
end = s.find('/* The win tile is placed')
if start > 0 and end > start:
    s = s[:start] + s[end:]

# fields
s = s.replace("""    SimDog dog;
""", "")
s = s.replace("""    SimButton buttons[SIM_MAX_BUTTONS];
    int button_count;
    int buttons_pressed;
    SimDoor doors[SIM_MAX_DOORS];
    int door_count;
""", "")
# limits now owned by the object scripts / view
s = s.replace("#define SIM_MAX_BUTTONS 16\n", "")
s = s.replace("#define SIM_MAX_DOORS 8\n", "")
s = s.replace("#define SIM_MAX_ZONE 16  /* cells covered by one placed object bbox */\n", "")
s = s.replace("#define SIM_MAX_CHAIN 64 /* dog body parts (original LONGO_MAX_DOG_INS) */\n", "")
# static solid grid: the solid map replaces it
s = s.replace("""    /* static solids: oBlock cells plus the goal house footprint */
    uint8_t solid[SIM_MAX_CELLS_W * SIM_MAX_CELLS_H];

""", "")
# dog step decl + shrink constant
s = s.replace("""/* Dog movement, chain and pickup rules (sim_dog.c). */
void sim_dog_step(SimWorld *w, const SimInput *input);
""", "")
s = s.replace("""/* Dialogue box shrink after the final press (lerp 0.15 below 0.65 scale). */
#define SIM_DIALOGUE_SHRINK_TICKS 34

""", "")
s = s.replace("""/* Fixed movement repeat: the original derived its held-key cadence from a
 * 2-tick alarm plus the OS key-repeat rate, which lands on one cell every
 * 2 ticks (30 cells/s) once a key is held.  The sim now owns the cadence
 * explicitly; a fresh press still steps immediately. */
#define SIM_STEP_INTERVAL 2

""", "")
p.write_text(s, encoding='utf-8')

# ---------- world.c ----------
p = root / 'src/core/world.c'
s = p.read_text(encoding='utf-8')

# includes
s = s.replace('#include "../objects/box.h"',
              '#include "../objects/button.h"\n#include "../objects/box.h"\n#include "../objects/door.h"\n#include "../objects/dog.h"')

# remove door_at (door.h API replaces it)
s = s.replace("""static SimDoor *door_at(SimWorld *w, uint16_t cell)
{
    for (int i = 0; i < w->door_count; i++)
        if (w->doors[i].alive && w->doors[i].cell == cell) return &w->doors[i];
    return NULL;
}

""", "")

# load_room resets: solid map + objects
s = s.replace("""    memset(w->solid, 0, sizeof(w->solid));
    memset(&w->dog, 0, sizeof(w->dog));
    box_reset();
    hole_reset();
    items_reset();
    house_reset();
    button... 
""", "")
s = s.replace("""    memset(w->solid, 0, sizeof(w->solid));
    memset(&w->dog, 0, sizeof(w->dog));
    box_reset();
    hole_reset();
    items_reset();
    house_reset();
""", """    solid_reset();
    dog_reset();
    box_reset();
    hole_reset();
    items_reset();
    house_reset();
    button_reset();
    door_reset();
""")
s = s.replace("""    memset(w->buttons, 0, sizeof(w->buttons));
    w->button_count = 0;
    w->buttons_pressed = 0;
    memset(w->doors, 0, sizeof(w->doors));
    w->door_count = 0;
""", "")

# wall placement -> solid map
s = s.replace("""        case LONGO_OBJ_BLOCK: {
            int cx = (int)floorf(p->x / SIM_CELL);
            int cy = (int)floorf(p->y / SIM_CELL);
            if (in_bounds(w, cx, cy)) w->solid[sim_cell_of(cx, cy)] = 1;
            break;
        }""",
"""        case LONGO_OBJ_BLOCK: {
            int cx = (int)floorf(p->x / SIM_CELL);
            int cy = (int)floorf(p->y / SIM_CELL);
            if (in_bounds(w, cx, cy)) solid_place(sim_cell_of(cx, cy), SOLID_WALL, 0);
            break;
        }""")

# button placement -> button script
s = s.replace("""        case LONGO_OBJ_BUTTON:
            if (w->button_count < SIM_MAX_BUTTONS) {
                SimButton *b = &w->buttons[w->button_count++];
                b->alive = 1;
                b->pressed = 0;
                compute_zone(w, b->zone, &b->zone_count, p->x, p->y, 16, 16);
                compute_zone_ex(w, b->box_zone, &b->box_zone_count, p->x,
                                p->y, 16, 16, 1);
            }
            break;""",
"""        case LONGO_OBJ_BUTTON: {
            uint16_t zone[BUTTON_ZONE_MAX];
            uint16_t box_zone[BUTTON_ZONE_MAX];
            int zone_count, box_zone_count;
            compute_zone(w, zone, &zone_count, p->x, p->y, 16, 16);
            compute_zone_ex(w, box_zone, &box_zone_count, p->x, p->y, 16, 16,
                            1);
            button_place(zone, zone_count, box_zone, box_zone_count);
            break;
        }""")

# door placement -> door script
s = s.replace("""        case LONGO_OBJ_DOOR:
            if (w->door_count < SIM_MAX_DOORS) {
                SimDoor *d = &w->doors[w->door_count++];
                d->alive = 1;
                d->open = 0;
                d->open_timer = 0;
                d->cell = sim_cell_of((int)floorf(p->x / SIM_CELL),
                                      (int)floorf(p->y / SIM_CELL));
            }
            break;""",
"""        case LONGO_OBJ_DOOR:
            door_place(sim_cell_of((int)floorf(p->x / SIM_CELL),
                                   (int)floorf(p->y / SIM_CELL)));
            break;""")

# goal footprint -> solid map
s = s.replace("""            if (rects_strictly_overlap(rx, ry, SIM_CELL, SIM_CELL, bx, by, bw,
                                       bh))
                w->solid[sim_cell_of(cx, cy)] = 1;""",
"""            if (rects_strictly_overlap(rx, ry, SIM_CELL, SIM_CELL, bx, by, bw,
                                       bh))
                solid_place(sim_cell_of(cx, cy), SOLID_GOAL, 0);""")

# spawn + title arrangement
s = s.replace("""    if (have_dog) spawn_dog(w, dog_x, dog_y);
    if (title) arrange_title_dog(w);""",
"""    if (have_dog) dog_place(dog_x, dog_y);
    if (title) dog_title_arrangement();""")

# cut update_buttons / update_doors / update_goal and the DOOR_OPEN_TICKS define
start = s.find('/* Buttons cover one or two cells')
anchor = '/* ------------------------------------------------------------------ */\n/* Tick'
if start < 0:
    start = s.find('static void update_buttons(SimWorld *w)')
end = s.find(anchor)
assert start > 0 and end > start
s = s[:start] + s[end:]

# tick: script calls
s = s.replace("""    /* 2. world object steps + draw-mutation ports */
    update_buttons(w);
    update_doors(w);
    update_goal(w);""",
"""    /* 2. world object steps */
    button_tick();
    door_tick(button_all_pressed());
    house_tick(dog_alive() ? dog_length() : -1);""")

# dog step call via script
s = s.replace("""    /* 1. dog step + pickups (oDog Step, then collision events) */
    sim_dog_step(w, input);""",
"""    /* 1. dog step + pickups (oDog Step, then collision events) */
    dog_tick(&w->input);""")

# drop spawn_dog / arrange_title_dog definitions (moved into dog.c)
start = s.find('static void spawn_dog(SimWorld *w, float x, float y)')
end = s.find('/* Dialogue texts, ported verbatim')
if start > 0 and end > start:
    s = s[:start] + s[end:]

p.write_text(s, encoding='utf-8')
print("world.c ported")

# ---------- delete dog_rules.c (absorbed into objects/dog.c) ----------
(root / 'src/core/dog_rules.c').unlink()

# ---------- CMake ----------
p = root / 'CMakeLists.txt'
s = p.read_text(encoding='utf-8')
s = s.replace('src/core/world.c src/core/dog_rules.c src/core/solid.c src/core/events.c src/objects/hole.c src/objects/box.c src/objects/items.c src/objects/house.c src/objects/dialogue.c src/objects/transition.c src/objects/title.c',
              'src/core/world.c src/core/solid.c src/core/events.c src/objects/dog.c src/objects/hole.c src/objects/box.c src/objects/door.c src/objects/button.c src/objects/items.c src/objects/house.c src/objects/dialogue.c src/objects/transition.c src/objects/title.c')
s = s.replace("""    src/core/world.c
    src/core/dog_rules.c
    src/core/solid.c
    src/core/events.c
    src/objects/hole.c
    src/objects/box.c
    src/objects/items.c
    src/objects/house.c""",
"""    src/core/world.c
    src/core/solid.c
    src/core/events.c
    src/objects/dog.c
    src/objects/hole.c
    src/objects/box.c
    src/objects/door.c
    src/objects/button.c
    src/objects/items.c
    src/objects/house.c""")
p.write_text(s, encoding='utf-8')
print("dog/door/button extraction complete")
