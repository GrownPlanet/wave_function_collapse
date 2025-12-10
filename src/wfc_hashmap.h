#ifndef WFC_HASHMAP_H
#define WFC_HASHMAP_H

#include <stdbool.h>

#include "wfc.h"

#define INITIAL_CAPACITY 16
#define MAX_LOAD_FACTOR 0.75
#define GROWTH_FACTOR 2

// an entry for WFC_PosMap
typedef struct {
    WFC_Bitmap key;
    WFC_Point value;
    bool active;
} WFC_PosMapEntry;

// hashmap with a bitmap as key and a point as value
typedef struct {
    WFC_PosMapEntry *data;
    size_t capacity;
    size_t length;
    bool had_error;
} WFC_PosMap;

// stores the value of a entry and if there was an error
typedef struct {
    WFC_Point value;
    bool had_error;
} WFC_PosMapResult;

/*
 * Creates a new posmap
 *
 * sets had_error to true in case of an error
 */
WFC_PosMap new_wfc_posmap();

/*
 * Checks if the given key is in the posmap, if so return the value associated
 * with the key. If no value is found it inserts the given value.
 * 
 * sets had_error to true in case of an error
 */
WFC_PosMapResult wfc_posmap_find_or_insert(
    WFC_PosMap *posmap,
    WFC_Bitmap key,
    WFC_Point value
);

/*
 * Frees a posmap's heap allocated data.
 */
void wfc_posmap_free(WFC_PosMap *posmap);

// hashmap with a color as key and pattern as value
typedef struct {
    WFC_Point *up, *down, *left, *right;
    size_t up_length, down_length, left_length, right_length;
} WFC_Neighbors;

typedef struct {
    WFC_Point key;
    WFC_Neighbors value;
    bool active;
} WFC_NeighborMapEntry;

typedef struct {
    WFC_NeighborMapEntry *data;
    size_t capacity;
    size_t length;
    bool had_error;
} WFC_NeighborMap;

#endif
