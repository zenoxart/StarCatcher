#ifndef PROJECTILES_H
#define PROJECTILES_H

#include <nds.h>

#define MAX_PROJECTILES 6
#define PROJECTILE_SPEED 5

typedef struct {
    int x;
    int y;
    bool active;
} Projectile;

void projectiles_init(Projectile projectiles[MAX_PROJECTILES]);

// Spawns one projectile at (x, y) if a free slot exists (silently ignored otherwise).
void projectiles_spawn(Projectile projectiles[MAX_PROJECTILES], int x, int y);

// Moves all active projectiles upward and deactivates ones that left the screen.
void projectiles_update(Projectile projectiles[MAX_PROJECTILES]);

void projectiles_draw(const Projectile projectiles[MAX_PROJECTILES], int oamStartId);

#endif // PROJECTILES_H
