#ifndef WFC_HASHMAP_H
#define WFC_HASHMAP_H

#include <stdbool.h>

#include "wfc.h"

#define INITIAL_CAPACITY 256
#define MAX_LOAD_FACTOR 0.75
#define GROWTH_FACTOR 2

// an entry for WFC_BitmapMap
typedef struct {
    WFC_Bitmap key;
    WFC_Point value;
    bool active;
} WFC_BitmapMapEntry;

// hashmap with a bitmap as key and a point as value
typedef struct {
    WFC_BitmapMapEntry *data;
    size_t capacity;
    size_t length;
    bool had_error;
} WFC_BitmapMap;

// stores the value of a entry and if there was an error
typedef struct {
    WFC_Point value;
    bool had_error;
} WFC_BitmapMapResult;

/*
 * Creates a new bitmap_map
 *
 * sets had_error to true in case of an error
 */
WFC_BitmapMap new_wfc_bitmap_map();

/*
 * Checks if the given key is in the bitmap_map, if so return the value
 * associated with the key. If no value is found it inserts the given value.
 * 
 * sets had_error to true in case of an error
 */
WFC_BitmapMapResult wfc_bitmap_map_get_or_insert(
    WFC_BitmapMap *bitmap_map,
    WFC_Bitmap key,
    WFC_Point value
);

/*
 * Frees a bitmap_maps heap allocated data.
 */
void wfc_bitmap_map_free(WFC_BitmapMap *bitmap_map);

// stores the value of a entry and if there was an error
typedef struct {
    WFC_Neighbors value;
    bool found;
} WFC_NeighborMapResult;

/*
 * Creates a new neighbor_map
 *
 * sets had_error to true in case of an error
 */
WFC_NeighborMap new_wfc_neighbor_map();

/*
 * Finds the value given a key
 *
 * sets had_error to true in case of an error
 */
WFC_NeighborMapResult wfc_neighbor_map_get(
    WFC_NeighborMap neighbor_map, WFC_Point key
);

/*
 * Sets a value, if it already exists it overwrites it (without freeing the
 * corresponding data), else it overwrites it
 *
 * returns WFC_ERROR in case of an error
 */
int wfc_neighbor_map_set(
    WFC_NeighborMap *neighbor_map, WFC_Point key, WFC_Neighbors value
);

/*
 * Frees a neighbor_maps heap allocated data. If neighbor_map is NULL, nothing
 * is done.
 */
void wfc_neighbor_map_free(WFC_NeighborMap *neighbor_map);

#endif
