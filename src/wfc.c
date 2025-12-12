#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wfc.h"
#include "wfc_hashmap.h"

int read_header(FILE *file, WFC_Bitmap *result) {
    char header[3];
    if (fscanf(file, "%2s", header) != 1 || strcmp(header, "P3") != 0) {
        fprintf(stderr, "unsupported PPM format\n");
        return WFC_ERROR;
    }

    int width, height, maxval;
    if (
        fscanf(file, "%d %d %d", &width, &height, &maxval) != 3
        || width <= 0 
        || height <= 0
        || maxval != 255
    ) {
        fprintf(stderr, "invalid PPM file\n");
        return WFC_ERROR;
    }
    result->width = width;
    result->height = height;

    return WFC_OK;
}

int read_colors(FILE *file, WFC_Bitmap *result) {
    const size_t size = result->width * result->height;
    result->data = malloc(sizeof(*result->data) * size);
    if (!result->data) {
        fprintf(stderr, "malloc failed\n");
        return WFC_ERROR;
    }

    size_t i = 0;
    int r, g, b;
    while (fscanf(file, "%d %d %d", &r, &g, &b) == 3) {
        if (i >= size) {
            fprintf(stderr, "invalid PPM file\n");
            return WFC_ERROR;
        }
        WFC_Color c = { .r = r, .b = b, .g = g };
        result->data[i] = c;
        i++;
    }

    if (i != size || !feof(file)) {
        fprintf(stderr, "invalid PPM file\n");
        return WFC_ERROR;
    }

    return WFC_OK;
}

WFC_Bitmap WFC_read_file(const char *filename) {
    WFC_Bitmap result = {
        .data = NULL,
        .height = 0,
        .width = 0,
        .status_code = WFC_ERROR
    };

    FILE *file = fopen(filename, "rb");
    if (!file) {
        fprintf(stderr, "failed to open file\n");
        goto cleanup;
    }

    if (read_header(file, &result) != WFC_OK) {
        goto cleanup;
    }

    if (read_colors(file, &result) != WFC_OK) {
        goto cleanup;
    }

    result.status_code = WFC_OK;

cleanup:
    if (file) {
        fclose(file);
    }

    if (result.status_code != WFC_OK) {
        free(result.data);
        result.data = NULL;
    }

    return result;
}

void free_neighbors(WFC_Neighbors *neighbors) {
    for (size_t i = 0; i < WFC_DIRECTION_COUNT; i++) {
        free(neighbors->neighbors[i]);
        neighbors->neighbors[i] = NULL;
    }
}

WFC_Color bitmap_get(WFC_Bitmap bitmap, size_t x, size_t y) {
    return bitmap.data[y * bitmap.width + x];
}

WFC_Bitmap extract_region(
    WFC_Bitmap bitmap, size_t region_size, size_t x, size_t y
) {
    WFC_Bitmap result = {
        .data = NULL,
        .height = region_size,
        .width = region_size,
        .status_code = WFC_ERROR,
    };

    result.data = malloc(sizeof(*result.data) * result.width * result.height);
    if (result.data == NULL) {
        fprintf(stderr, "malloc failed\n");
        goto cleanup;
    }

    for (size_t y_offset = 0; y_offset < region_size; y_offset++) {
        for (size_t x_offset = 0; x_offset < region_size; x_offset++) {
            size_t new_x = x + x_offset;
            size_t new_y = y + y_offset;
            result.data[y_offset * region_size + x_offset] =
                bitmap_get(bitmap, new_x, new_y);
        }
    }

    result.status_code = WFC_OK;

cleanup:
    if (result.status_code != WFC_OK) {
        free(result.data);
        result.data = NULL;
    }

    return result;
}

typedef struct {
    WFC_Point *point;
    bool had_error;
    bool exists;
} FindNeighborDirResult;

FindNeighborDirResult find_neighbor_dir(
    WFC_Bitmap bitmap,
    WFC_BitmapMap *bitmap_map,
    size_t region_size,
    size_t x,
    size_t y,
    int dir
) {
    FindNeighborDirResult result = {
        .point = NULL, .had_error = true, .exists = false,
    };

    int new_x, new_y;
    switch (dir) {
        case WFC_DIR_UP:
            new_x = x; new_y = y - region_size; break;
        case WFC_DIR_DOWN:
            new_x = x; new_y = y + region_size; break;
        case WFC_DIR_LEFT:
            new_x = x - region_size; new_y = y; break;
        case WFC_DIR_RIGHT:
            new_x = x + region_size; new_y = y; break;
    }

    if (
        new_x < 0
        || new_y < 0
        || new_x + region_size > bitmap.width
        || new_y + region_size > bitmap.height
    ) {
        result.had_error = false;
        return result;
    }

    result.exists = true;

    WFC_Bitmap region = extract_region(bitmap, region_size, new_x, new_y);
    WFC_Point *pattern_location = malloc(sizeof(WFC_Point));
    if (pattern_location == NULL) {
        fprintf(stderr, "malloc failed\n");
        return result;
    }
    pattern_location->x = new_x;
    pattern_location->y = new_y;

    WFC_BitmapMapResult location = wfc_bitmap_map_get_or_insert(
        bitmap_map, region, *pattern_location
    );
    pattern_location->x = location.value.x;
    pattern_location->y = location.value.y;

    result.had_error = false;
    result.point =  pattern_location;

    return result;
}

WFC_Neighbors find_neighbors(
    WFC_Bitmap bitmap,
    WFC_BitmapMap *bitmap_map,
    size_t region_size,
    size_t x,
    size_t y
) {
    WFC_Neighbors neighbors;
    neighbors.had_error = false;

    int dirs[WFC_DIRECTION_COUNT] = { WFC_DIR_UP, WFC_DIR_DOWN, WFC_DIR_LEFT, WFC_DIR_RIGHT };

    for (size_t i = 0; i < WFC_DIRECTION_COUNT; i++) {
        int dir = dirs[i];
        FindNeighborDirResult neighbor = find_neighbor_dir(
            bitmap, bitmap_map, region_size, x, y, dir
        );
        if (neighbor.had_error) {
            neighbors.had_error = true;
            return neighbors;
        }
        neighbors.lengths[i] = neighbor.exists ? 1 : 0;
        neighbors.capacities[i] = neighbor.exists ? 1 : 0;
        neighbors.neighbors[i] = neighbor.point;
    }

    return neighbors;
}

int append_point(
    WFC_Point **data, size_t *length, size_t *capacity, WFC_Point new_point
) {
    if (*length >= *capacity) {
        size_t new_capacity = (*capacity == 0) ? 1 : (*capacity * 2);
        WFC_Point *new_data = realloc(*data, sizeof(**data) * new_capacity);
        if (new_data == NULL) {
            fprintf(stderr, "realloc failed\n");
            return WFC_ERROR;
        }
        *data = new_data;
        *capacity = new_capacity;
    }
    (*data)[*length] = new_point;
    (*length) += 1;
    return WFC_OK;
}

int append_neighbors(WFC_Neighbors *neighbors, WFC_Neighbors *to_append) {
    for (size_t i = 0; i < WFC_DIRECTION_COUNT; i++) {
        if (to_append->lengths[i] == 0) {
            continue;
        }

        WFC_Point **points = &neighbors->neighbors[i];
        size_t *length = &neighbors->lengths[i];
        size_t *capacity = &neighbors->capacities[i];
        WFC_Point new_point = *to_append->neighbors[i];

        int res = append_point(
            points, length, capacity, new_point
        );
        if (res != WFC_OK) {
            return res;
        }
    }

    free_neighbors(to_append);

    return WFC_OK;
}

WFC_NeighborMap WFC_extract_patterns(WFC_Bitmap bitmap, size_t region_size) {
    WFC_NeighborMap neighbor_map = new_wfc_neighbor_map();
    if (neighbor_map.had_error) {
        return neighbor_map;
    }
    neighbor_map.had_error = true;

    WFC_BitmapMap bitmap_map = new_wfc_bitmap_map();
    if (bitmap_map.had_error) {
        goto cleanup;
    }

    for (size_t y = 0; y <= bitmap.height - region_size; y++) {
        for (size_t x = 0; x <= bitmap.width - region_size; x++) {
            WFC_Bitmap region = extract_region(bitmap, region_size, x, y);
            WFC_Point pattern_location = { .x = x, .y = y };

            WFC_BitmapMapResult location = wfc_bitmap_map_get_or_insert(
                &bitmap_map, region, pattern_location
            );
            pattern_location = location.value;

            WFC_NeighborMapResult found_neighbors_maybe =
                wfc_neighbor_map_get(neighbor_map, pattern_location);

            WFC_Neighbors neighbors = find_neighbors(
                bitmap, &bitmap_map, region_size, x, y
            );

            if (found_neighbors_maybe.found) {
                WFC_Neighbors found_neighbors = found_neighbors_maybe.value;
                int res = append_neighbors(&found_neighbors, &neighbors);
                if (res != WFC_OK) {
                    goto cleanup;
                }
                neighbors = found_neighbors;
            }

            // Doesn't deduplicate the entries (yet), so entries that come
            // up more often have a higher chance of being selected. But
            // that's not a but, it's a feature.
            wfc_neighbor_map_set(&neighbor_map, pattern_location, neighbors);
        }
    }


    neighbor_map.had_error = false;

cleanup:
    wfc_bitmap_map_free(&bitmap_map);
    return neighbor_map;
}
