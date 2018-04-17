#ifndef SELFCON_GRID_H
#define SELFCON_GRID_H

// Proto-Types //

// External //
int32_t init_selfcon_grid(void);
int32_t free_selfcon_grid(void);
int32_t update_selfcon_grid(struct GALAXY *g, int32_t grid_idx, int32_t snapshot);
int32_t save_selfcon_grid(void);
#endif