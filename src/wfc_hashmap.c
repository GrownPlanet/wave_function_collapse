#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "wfc_hashmap.h"

size_t hash_bitmap(WFC_Bitmap bitmap) {
    int hash = 17;
    for (int i = 0; i < bitmap.width * bitmap.height; i++) {
        WFC_Color color = bitmap.data[i];
        hash = hash * 31 ^ (color.r + color.g + color.b);
    }
    return hash;
}

bool compare_bitmaps(WFC_Bitmap a, WFC_Bitmap b) {
    if (a.width != b.width || a.height != b.height) {
        return false;
    }

    for (size_t i = 0; i < a.width * a.height; i++) {
        WFC_Color ca = a.data[i];
        WFC_Color cb = b.data[i];
        if (ca.r != cb.r || ca.g != cb.g || ca.b != cb.b) {
            return false;
        }
    }

    return true;
}

WFC_BitmapMap new_wfc_bitmap_map() {
    WFC_BitmapMap result = {
        .data = NULL,
        .capacity = INITIAL_CAPACITY,
        .length = 0,
        .had_error = true,
    };

    WFC_BitmapMapEntry *data =
        malloc(INITIAL_CAPACITY * sizeof(WFC_BitmapMapEntry));
    if (data == NULL) {
        fprintf(stderr, "malloc failed\n");
        return result;
    }

    for (size_t i = 0; i < INITIAL_CAPACITY; i++) {
        data[i].active = false;
    }

    result.data = data;
    result.had_error = false;

    return result;
}

int bitmap_map_realloc(WFC_BitmapMap *bitmap_map) {
    size_t new_capacity = bitmap_map->capacity * GROWTH_FACTOR;
    WFC_BitmapMapEntry *new_data =
        malloc(sizeof(*bitmap_map->data) * new_capacity);
    if (new_data == NULL) {
        fprintf(stderr, "malloc failed\n");
        return 1;
    }

    for (size_t i = 0; i < new_capacity; i++) {
        new_data[i].active = false;
    }

    for (size_t i = 0; i < bitmap_map->capacity; i++) {
        WFC_BitmapMapEntry old_entry = bitmap_map->data[i];
        if (!old_entry.active) {
            continue;
        }

        size_t index = hash_bitmap(old_entry.key) % bitmap_map->capacity;
        WFC_BitmapMapEntry entry = bitmap_map->data[index];
        while (entry.active && !compare_bitmaps(entry.key, old_entry.key)) {
            index = (index + 1) % bitmap_map->capacity;
            entry = bitmap_map->data[index];
        }

        new_data[index] = entry;
    }

    free(bitmap_map->data);

    bitmap_map->data = new_data;
    bitmap_map->capacity = new_capacity;

    return 0;
}

WFC_BitmapMapResult wfc_bitmap_map_get_or_insert(
    WFC_BitmapMap *bitmap_map,
    WFC_Bitmap key,
    WFC_Point value
) {
    WFC_BitmapMapResult result = {
        .value = value,
        .had_error = true,
    };

    size_t index = hash_bitmap(key) % bitmap_map->capacity;
    WFC_BitmapMapEntry *entry = &bitmap_map->data[index];

    while (entry->active && !compare_bitmaps(entry->key, key)) {
        index = (index + 1) % bitmap_map->capacity;
        entry = &bitmap_map->data[index];
    }

    if (!entry->active) {
        float load_factor =
            ((float)bitmap_map->length + 1.0) / (float)bitmap_map->capacity;
        if (load_factor >= MAX_LOAD_FACTOR) {
            int res = bitmap_map_realloc(bitmap_map);
            if (res != 0) {
                return result;
            }
        }

        entry->key = key;
        entry->value = value;
        entry->active = true;

        bitmap_map->length++;
    } else {
        result.value = entry->value;
    }

    return result;
}

void wfc_bitmap_map_free(WFC_BitmapMap *bitmap_map) {
    for (size_t i = 0; i < bitmap_map->capacity; i++) {
        free(bitmap_map->data[i].key.data);
    }
    free(bitmap_map->data);
    bitmap_map->data = NULL;
}

size_t hash_point(WFC_Point point) {
    return point.x * 31 + point.y;
}

bool compare_points(WFC_Point a, WFC_Point b) {
    return a.x == b.x && a.y == b.y;
}

WFC_NeighborMap new_wfc_neighbor_map() {
    WFC_NeighborMap result = {
        .data = NULL,
        .capacity = INITIAL_CAPACITY,
        .length = 0,
        .had_error = true,
    };

    WFC_NeighborMapEntry *data =
        malloc(INITIAL_CAPACITY * sizeof(WFC_NeighborMapEntry));
    if (data == NULL) {
        fprintf(stderr, "malloc failed\n");
        return result;
    }

    for (size_t i = 0; i < INITIAL_CAPACITY; i++) {
        data[i].active = false;
    }

    result.data = data;
    result.had_error = false;

    return result;
}

WFC_NeighborMapResult wfc_neighbor_map_get(
    WFC_NeighborMap neighbor_map, WFC_Point key
) {
    WFC_NeighborMapResult result;
    result.found = false;

    size_t index = hash_point(key) % neighbor_map.capacity;
    WFC_NeighborMapEntry entry = neighbor_map.data[index];

    while (entry.active && !compare_points(entry.key, key)) {
        index = (index + 1) % neighbor_map.capacity;
        entry = neighbor_map.data[index];
    }

    if (!entry.active) {
        return result;
    }

    result.found = true;
    result.value = entry.value;

    return result;
}

int neighbor_map_realloc(WFC_NeighborMap *neighbor_map) {
    size_t new_capacity = neighbor_map->capacity * GROWTH_FACTOR;
    WFC_NeighborMapEntry *new_data =
        malloc(sizeof(*neighbor_map->data) * new_capacity);
    if (new_data == NULL) {
        fprintf(stderr, "malloc failed\n");
        return 1;
    }

    for (size_t i = 0; i < new_capacity; i++) {
        new_data[i].active = false;
    }

    for (size_t i = 0; i < neighbor_map->capacity; i++) {
        WFC_NeighborMapEntry old_entry = neighbor_map->data[i];
        if (!old_entry.active) {
            continue;
        }

        size_t index = hash_point(old_entry.key) % neighbor_map->capacity;
        WFC_NeighborMapEntry entry = neighbor_map->data[index];
        while (entry.active && !compare_points(entry.key, old_entry.key)) {
            index = (index + 1) % neighbor_map->capacity;
            entry = neighbor_map->data[index];
        }

        new_data[index] = entry;
    }

    free(neighbor_map->data);

    neighbor_map->data = new_data;
    neighbor_map->capacity = new_capacity;

    return 0;
}

int wfc_neighbor_map_set(
    WFC_NeighborMap *neighbor_map, WFC_Point key, WFC_Neighbors value
) {
    size_t index = hash_point(key) % neighbor_map->capacity;
    WFC_NeighborMapEntry *entry = &neighbor_map->data[index];

    while (entry->active && !compare_points(entry->key, key)) {
        index = (index + 1) % neighbor_map->capacity;
        entry = &neighbor_map->data[index];
    }

    float load_factor =
        ((float)neighbor_map->length + 1.0) / (float)neighbor_map->capacity;
    if (load_factor >= MAX_LOAD_FACTOR) {
        int res = neighbor_map_realloc(neighbor_map);
        if (res != 0) {
            return res;
        }
    }

    entry->key = key;
    entry->value = value;

    if (!entry->active) {
        entry->active = true;
        neighbor_map->length++;
    }

    return 0;
}

void wfc_neighbor_map_free(WFC_NeighborMap *neighbor_map) {
    if (neighbor_map == NULL) {
        return; 
    }

    for (size_t i = 0; i < neighbor_map->capacity; i++) {
        WFC_NeighborMapEntry entry = neighbor_map->data[i];
        if (entry.active) {
            free_neighbors(&entry.value);
        }
    }
    free(neighbor_map->data);
    neighbor_map->data = NULL;
}
