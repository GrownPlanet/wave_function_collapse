#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "wfc_hashmap.h"

// General hahsmap functions, that are overloaded with the specific posmaps
int hash_bitmap(WFC_Bitmap bitmap) {
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

WFC_PosMap new_wfc_posmap() {
    WFC_PosMap result = {
        .data = NULL,
        .capacity = INITIAL_CAPACITY,
        .length = 0,
        .had_error = true,
    };

    WFC_PosMapEntry *data = malloc(INITIAL_CAPACITY * sizeof(WFC_PosMapEntry));
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

int posmap_realloc(WFC_PosMap *posmap) {
    size_t new_capacity = posmap->capacity * GROWTH_FACTOR;
    WFC_PosMapEntry *new_data = malloc(sizeof(WFC_PosMapEntry) * new_capacity);
    if (new_data == NULL) {
        fprintf(stderr, "malloc failed\n");
        return WFC_ERROR;
    }

    for (size_t i = 0; i < new_capacity; i++) {
        new_data[i].active = false;
    }

    for (size_t i = 0; i < posmap->capacity; i++) {
        WFC_PosMapEntry old_entry = posmap->data[i];
        if (!old_entry.active) {
            continue;
        }

        size_t index = hash_bitmap(old_entry.key) % posmap->capacity;
        WFC_PosMapEntry entry = posmap->data[index];
        while (entry.active && !compare_bitmaps(entry.key, old_entry.key)) {
            index = (index + 1) % posmap->capacity;
            entry = posmap->data[index];
        }

        new_data[index] = entry;
    }

    free(posmap->data);

    posmap->data = new_data;
    posmap->capacity = new_capacity;

    return WFC_OK;
}

WFC_PosMapResult wfc_posmap_find_or_insert(
    WFC_PosMap *posmap,
    WFC_Bitmap key,
    WFC_Point value
) {
    WFC_PosMapResult result = {
        .value = value,
        .had_error = true,
    };

    size_t index = hash_bitmap(key) % posmap->capacity;
    WFC_PosMapEntry *entry = &posmap->data[index];

    while (entry->active && !compare_bitmaps(entry->key, key)) {
        index = (index + 1) % posmap->capacity;
        entry = &posmap->data[index];
    }

    if (!entry->active) {
        float load_factor = ((float)posmap->length + 1.0) / (float)posmap->capacity;
        if (load_factor >= MAX_LOAD_FACTOR) {
            int res = posmap_realloc(posmap);
            if (res != WFC_OK) {
                return result;
            }
        }

        entry->key = key;
        entry->value = value;
        entry->active = true;

        posmap->length++;
    } else {
        result.value = entry->value;
    }

    return result;
}

void wfc_posmap_free(WFC_PosMap *posmap) {
    for (size_t i = 0; i < posmap->capacity; i++) {
        free(posmap->data[i].key.data);
    }
    free(posmap->data);
    posmap->data = NULL;
}

// NeighborMap specific
