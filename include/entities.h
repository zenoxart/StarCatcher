#ifndef ENTITIES_H
#define ENTITIES_H

#include <nds.h>
#include "player.h"

#define MAX_ENTITIES 10

// Idle animation speeds (frames per idle-anim frame advance).
#define STAR_ANIM_INTERVAL   20
#define METEOR_ANIM_INTERVAL 25
#define PORTAL_ANIM_INTERVAL 6

// Destruction timings.
#define COLLECTED_DURATION 16 // star sparkle (frame 2) hold time
#define CRACK_DURATION     14 // meteor cracking (frame 2) hold time
#define BOOM_DURATION      20 // meteor boom (reuses the ship explosion art) hold time

typedef enum {
    ENTITY_NONE,
    ENTITY_STAR,
    ENTITY_METEOR,
    ENTITY_PORTAL
} EntityType;

typedef enum {
    ENTITY_ALIVE,
    ENTITY_COLLECTED, // star: brief sparkle before vanishing
    ENTITY_EXPLODING  // meteor: crack, then boom, before vanishing
} EntityState;

typedef struct {
    EntityType type;
    EntityState state;
    int x;
    int y;
    int speed;      // pixels fallen per frame (frozen once collected/exploding)
    int idleTick;   // increments every frame while ALIVE; drives idle-anim frame choice
    int animFrame;  // phase within the COLLECTED/EXPLODING sequence
    int animTimer;  // frames left until animFrame/state advances
    bool active;
} Entity;

void entities_init(Entity entities[MAX_ENTITIES]);

// Spawns one inactive entity slot with a random x position, a random
// type (mostly stars, occasional meteors) and a fall speed in [minSpeed, maxSpeed].
void entities_spawn(Entity entities[MAX_ENTITIES], int minSpeed, int maxSpeed);

// Spawns a portal (level exit) in a free slot, if one is available.
void entities_spawnPortal(Entity entities[MAX_ENTITIES]);

// Starts the destruction animation on the meteor at the given slot, if it is
// currently an alive meteor. Returns true if a meteor was actually destroyed.
bool entities_hitByProjectile(Entity entities[MAX_ENTITIES], int index);

// Advances all entities (falling + idle/destruction animation) and resolves
// collisions against the player. Increases *score/*starsCaught for caught
// stars (which play a brief collected animation rather than vanishing
// instantly), decrements *lives for meteors that hit the player (which also
// triggers their crack+boom animation), and sets *portalReached when the
// player touches an active portal.
void entities_update(Entity entities[MAX_ENTITIES], const Player* player,
                      int* score, int* lives, int* starsCaught, bool* portalReached);

void entities_draw(const Entity entities[MAX_ENTITIES], int oamStartId);

#endif // ENTITIES_H
