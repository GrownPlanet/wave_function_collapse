#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wfc.h"

// values for hashmaps/-sets
#define INITIAL_CAPACITY 256
#define MAX_LOAD_FACTOR 0.75
#define GROWTH_FACTOR 2

// hashmaps / sets
typedef struct {
    WFC_Bitmap key;
    WFC_Point value;
    bool active;
} BitMapMapEntry;

typedef struct {
    BitMapMapEntry *data;
    size_t capacity;
    size_t length;
    bool had_error;
} BitMapMap;

typedef struct {
    WFC_Point value;
    bool had_error;
} BitMapMapResult;

typedef struct {
    WFC_Neighbors value;
    bool found;
} PatternsResult;

int read_header(FILE *file, WFC_Bitmap *result) {
    char header[3];
    if (fscanf(file, "%2s", header) != 1 || strcmp(header, "P3") != 0) {
        fprintf(stderr, "unsupported PPM format\n");
        return 1;
    }

    int width, height, maxval;
    if (
        fscanf(file, "%d %d %d", &width, &height, &maxval) != 3
        || width <= 0 
        || height <= 0
        || maxval != 255
    ) {
        fprintf(stderr, "invalid PPM file\n");
        return 1;
    }
    result->width = width;
    result->height = height;

    return 0;
}

int read_colors(FILE *file, WFC_Bitmap *result) {
    const size_t size = result->width * result->height;
    result->data = malloc(sizeof(*result->data) * size);
    if (!result->data) {
        fprintf(stderr, "malloc failed\n");
        return 1;
    }

    size_t i = 0;
    int r, g, b;
    while (fscanf(file, "%d %d %d", &r, &g, &b) == 3) {
        if (i >= size) {
            fprintf(stderr, "invalid PPM file\n");
            return 1;
        }
        WFC_Color c = { .r = r, .b = b, .g = g };
        result->data[i] = c;
        i++;
    }

    if (i != size || !feof(file)) {
        fprintf(stderr, "invalid PPM file\n");
        return 1;
    }

    return 0;
}

WFC_Bitmap WFC_read_image(const char *filename) {
    WFC_Bitmap result = {
        .data = NULL,
        .height = 0,
        .width = 0,
        .had_error = true,
    };

    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, "failed to open file\n");
        goto cleanup;
    }

    if (read_header(file, &result) != 0) {
        goto cleanup;
    }

    if (read_colors(file, &result) != 0) {
        goto cleanup;
    }

    result.had_error = false;

cleanup:
    if (file) {
        fclose(file);
    }

    if (result.had_error) {
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
        .had_error = true,
    };

    result.data = malloc(sizeof(*result.data) * result.width * result.height);
    if (result.data == NULL) {
        fprintf(stderr, "malloc failed\n");
        return result;
    }

    for (size_t y_offset = 0; y_offset < region_size; y_offset++) {
        for (size_t x_offset = 0; x_offset < region_size; x_offset++) {
            size_t new_x = x + x_offset;
            size_t new_y = y + y_offset;
            result.data[y_offset * region_size + x_offset] =
                bitmap_get(bitmap, new_x, new_y);
        }
    }

    result.had_error = false;

    return result;
}

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

BitMapMap new_bitmapmap() {
    BitMapMap result = {
        .data = NULL,
        .capacity = INITIAL_CAPACITY,
        .length = 0,
        .had_error = true,
    };

    BitMapMapEntry *data =
        malloc(INITIAL_CAPACITY * sizeof(BitMapMapEntry));
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

int bitmapmap_realloc(BitMapMap *bitmapmap) {
    size_t new_capacity = bitmapmap->capacity * GROWTH_FACTOR;
    BitMapMapEntry *new_data =
        malloc(sizeof(*bitmapmap->data) * new_capacity);
    if (new_data == NULL) {
        fprintf(stderr, "malloc failed\n");
        return 1;
    }

    for (size_t i = 0; i < new_capacity; i++) {
        new_data[i].active = false;
    }

    for (size_t i = 0; i < bitmapmap->capacity; i++) {
        BitMapMapEntry old_entry = bitmapmap->data[i];
        if (!old_entry.active) {
            continue;
        }

        size_t index = hash_bitmap(old_entry.key) % new_capacity;
        BitMapMapEntry *entry = &new_data[index];
        while (entry->active && !compare_bitmaps(entry->key, old_entry.key)) {
            index = (index + 1) % new_capacity;
            entry = &new_data[index];
        }

        entry->key = old_entry.key;
        entry->value = old_entry.value;
        entry->active = true;
    }

    free(bitmapmap->data);

    bitmapmap->data = new_data;
    bitmapmap->capacity = new_capacity;

    return 0;
}

BitMapMapResult bitmapmap_get_or_insert(
    BitMapMap *bitmapmap,
    WFC_Bitmap key,
    WFC_Point value
) {
    BitMapMapResult result = {
        .value = value,
        .had_error = true,
    };

    size_t index = hash_bitmap(key) % bitmapmap->capacity;
    BitMapMapEntry *entry = &bitmapmap->data[index];

    while (entry->active && !compare_bitmaps(entry->key, key)) {
        index = (index + 1) % bitmapmap->capacity;
        entry = &bitmapmap->data[index];
    }

    if (!entry->active) {
        float load_factor =
            ((float)bitmapmap->length + 1.0) / (float)bitmapmap->capacity;
        if (load_factor >= MAX_LOAD_FACTOR) {
            int res = bitmapmap_realloc(bitmapmap);
            if (res != 0) {
                return result;
            }
        }

        entry->key = key;
        entry->value = value;
        entry->active = true;

        bitmapmap->length++;
    } else {
        result.value = entry->value;
    }

    return result;
}

void bitmapmap_free(BitMapMap *bitmapmap) {
    for (size_t i = 0; i < bitmapmap->capacity; i++) {
        free(bitmapmap->data[i].key.data);
    }
    free(bitmapmap->data);
    bitmapmap->data = NULL;
}

size_t hash_point(WFC_Point point) {
    return point.x * 31 + point.y;
}

bool compare_points(WFC_Point a, WFC_Point b) {
    return a.x == b.x && a.y == b.y;
}

WFC_Patterns new_wfc_patterns() {
    WFC_Patterns result = {
        .data = NULL,
        .capacity = INITIAL_CAPACITY,
        .length = 0,
        .had_error = true,
    };

    WFC_Pattern *data =
        malloc(INITIAL_CAPACITY * sizeof(WFC_Pattern));
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

PatternsResult wfc_patterns_get(
    WFC_Patterns patterns, WFC_Point key
) {
    PatternsResult result;
    result.found = false;

    size_t index = hash_point(key) % patterns.capacity;
    WFC_Pattern entry = patterns.data[index];

    while (entry.active && !compare_points(entry.key, key)) {
        index = (index + 1) % patterns.capacity;
        entry = patterns.data[index];
    }

    if (!entry.active) {
        return result;
    }

    result.found = true;
    result.value = entry.value;

    return result;
}

int patterns_realloc(WFC_Patterns *patterns) {
    size_t new_capacity = patterns->capacity * GROWTH_FACTOR;
    WFC_Pattern *new_data =
        malloc(sizeof(*patterns->data) * new_capacity);
    if (new_data == NULL) {
        fprintf(stderr, "malloc failed\n");
        return 1;
    }

    for (size_t i = 0; i < new_capacity; i++) {
        new_data[i].active = false;
    }

    for (size_t i = 0; i < patterns->capacity; i++) {
        WFC_Pattern old_entry = patterns->data[i];
        if (!old_entry.active) {
            continue;
        }

        size_t index = hash_point(old_entry.key) % new_capacity;
        WFC_Pattern *entry = &new_data[index];
        while (entry->active && !compare_points(entry->key, old_entry.key)) {
            index = (index + 1) % new_capacity;
            entry = &new_data[index];
        }

        entry->value = old_entry.value;
        entry->key = old_entry.key;
        entry->active = true;
    }

    free(patterns->data);

    patterns->data = new_data;
    patterns->capacity = new_capacity;

    return 0;
}

int wfc_patterns_set(
    WFC_Patterns *patterns, WFC_Point key, WFC_Neighbors value
) {
    size_t index = hash_point(key) % patterns->capacity;
    WFC_Pattern *entry = &patterns->data[index];

    while (entry->active && !compare_points(entry->key, key)) {
        index = (index + 1) % patterns->capacity;
        entry = &patterns->data[index];
    }

    float load_factor =
        ((float)patterns->length + 1.0) / (float)patterns->capacity;
    if (load_factor >= MAX_LOAD_FACTOR) {
        int res = patterns_realloc(patterns);
        if (res != 0) {
            return res;
        }
    }

    entry->key = key;
    entry->value = value;

    if (!entry->active) {
        entry->active = true;
        patterns->length++;
    }

    return 0;
}

WFC_PointSet new_pointset() {
    WFC_PointSet result = {
        .data = NULL,
        .length = 0,
        .capacity = 4,
        .had_error = true,
    };

    result.data = malloc(sizeof(WFC_PointSetEntry) * result.capacity);
    if (result.data == NULL) {
        fprintf(stderr, "malloc failed\n");
        return result;
    }

    for (size_t i = 0; i < result.capacity; i++) {
        result.data[i].active = false;
    }

    result.had_error = false;
    return result;
}

int pointset_realloc(WFC_PointSet *pointset) {
    size_t new_capacity = pointset->capacity * GROWTH_FACTOR;
    WFC_PointSetEntry *new_data = malloc(sizeof(PointSetEntry) * new_capacity);
    if (new_data == NULL) {
        fprintf(stderr, "malloc failed\n");
        return 1;
    }

    for (size_t i = 0; i < new_capacity; i++) {
        new_data[i].active = false;
    }

    for (size_t i = 0; i < pointset->capacity; i++) {
        WFC_PointSetEntry old_entry = pointset->data[i];
        if (!old_entry.active) {
            continue;
        }

        size_t index = hash_point(old_entry.value) % new_capacity;
        WFC_PointSetEntry *entry = &new_data[index];
        while (entry->active && !compare_points(entry->value, old_entry.value)) {
            index = (index + 1) % new_capacity;
            entry = &new_data[index];
        }

        entry->value = old_entry.value;
        entry->active = true;
    }

    free(pointset->data);

    pointset->data = new_data;
    pointset->capacity = new_capacity;

    return 0;
}

int pointset_insert(WFC_PointSet *pointset, WFC_Point point) {
    size_t index = hash_point(point) % pointset->capacity;
    WFC_PointSetEntry *entry = &pointset->data[index];

    while (entry->active && !compare_points(entry->value, point)) {
        index = (index + 1) % pointset->capacity;
        entry = &pointset->data[index];
    }

    // it already exists
    if (entry->active) {
        return 0;
    }

    float load_factor =
        ((float)pointset->length + 1.0) / (float)pointset->capacity;
    if (load_factor >= MAX_LOAD_FACTOR) {
        int res = pointset_realloc(pointset);
        if (res != 0) {
            return res;
        }
    }

    entry->value = point;
    entry->active = true;
    pointset->length++;
    
    return 0;
}

typedef struct {
    WFC_Point *point;
    bool had_error;
    bool exists;
} FindNeighborDirResult;

FindNeighborDirResult find_neighbor_dir(
    WFC_Bitmap bitmap,
    BitMapMap *bitmapmap,
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

    BitMapMapResult location = bitmapmap_get_or_insert(
        bitmapmap, region, *pattern_location
    );
    pattern_location->x = location.value.x;
    pattern_location->y = location.value.y;

    result.had_error = false;
    result.point =  pattern_location;

    return result;
}

WFC_Neighbors find_neighbors(
    WFC_Bitmap bitmap,
    BitMapMap *bitmapmap,
    size_t region_size,
    size_t x,
    size_t y
) {
    WFC_Neighbors neighbors;
    neighbors.had_error = false;

    int dirs[WFC_DIRECTION_COUNT] = {
        WFC_DIR_UP, WFC_DIR_DOWN, WFC_DIR_LEFT, WFC_DIR_RIGHT
    };

    for (size_t i = 0; i < WFC_DIRECTION_COUNT; i++) {
        int dir = dirs[i];
        FindNeighborDirResult neighbor = find_neighbor_dir(
            bitmap, bitmapmap, region_size, x, y, dir
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
            return 1;
        }
        *data = new_data;
        *capacity = new_capacity;
    }
    (*data)[*length] = new_point;
    (*length)++;
    return 0;
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
        if (res != 0) {
            return res;
        }
    }

    free_neighbors(to_append);

    return 0;
}

WFC_Patterns WFC_extract_patterns(WFC_Bitmap bitmap, size_t region_size) {
    WFC_Patterns patterns = new_wfc_patterns();
    if (patterns.had_error) {
        return patterns;
    }
    patterns.had_error = true;

    BitMapMap bitmapmap = new_bitmapmap();
    if (bitmapmap.had_error) {
        goto cleanup;
    }

    for (size_t y = 0; y <= bitmap.height - region_size; y++) {
        for (size_t x = 0; x <= bitmap.width - region_size; x++) {
            WFC_Bitmap region = extract_region(bitmap, region_size, x, y);
            WFC_Point pattern_location = { .x = x, .y = y };

            BitMapMapResult location = bitmapmap_get_or_insert(
                &bitmapmap, region, pattern_location
            );
            pattern_location = location.value;

            PatternsResult found_neighbors_maybe =
                wfc_patterns_get(patterns, pattern_location);

            WFC_Neighbors neighbors = find_neighbors(
                bitmap, &bitmapmap, region_size, x, y
            );

            if (found_neighbors_maybe.found) {
                WFC_Neighbors found_neighbors = found_neighbors_maybe.value;
                int res = append_neighbors(&found_neighbors, &neighbors);
                if (res != 0) {
                    goto cleanup;
                }
                neighbors = found_neighbors;
            }

            // Doesn't deduplicate the entries (yet), so entries that come
            // up more often have a higher chance of being selected. But
            // that's not a but, it's a feature.
            wfc_patterns_set(&patterns, pattern_location, neighbors);
        }
    }


    patterns.had_error = false;

cleanup:
    bitmapmap_free(&bitmapmap);
    return patterns;
}

typedef struct {
    bool collapsed;
    WFC_Point location;
} Cell;

WFC_Color *generate_bitmap(
    Cell *solution,
    WFC_Bitmap reference,
    WFC_Point output_size,
    size_t region_size
) {
    size_t width = region_size * output_size.x;
    size_t height = region_size * output_size.y;
    WFC_Color *result = malloc(sizeof(WFC_Color) * width * height);
    if (result == NULL) {
        return result;
    }

    for (size_t y = 0; y < output_size.y; y++) {
        for (size_t x = 0; x < output_size.x; x++) {
            WFC_Point ref_point = solution[y * output_size.x + x].location;

            for (size_t xo = 0; xo < region_size; xo++) {
                for (size_t yo = 0; yo < region_size; yo++) {
                    size_t ref_x = ref_point.x + xo;
                    size_t ref_y = ref_point.y + yo;
                    WFC_Color color
                        = reference.data[ref_y * reference.width + ref_x];

                    size_t new_x = x * region_size + xo;
                    size_t new_y = y * region_size + yo;
                    result[new_y * width + new_x] = color;
                }
            }

        }
    }

    return result;
}

size_t rand_range(size_t min, size_t max) {
    size_t val = rand() % (max - min + 1) + min;
    return val;
}

WFC_Bitmap WFC_Solve(
    WFC_Patterns map,
    WFC_Bitmap bitmap,
    size_t region_size,
    WFC_Point output_size
) {
    WFC_Bitmap result = {
        .data = NULL,
        .height = output_size.y * region_size,
        .width = output_size.x * region_size,
        .had_error = true,
    };

    size_t size = output_size.x * output_size.y;
    Cell *solution = malloc(sizeof(Cell) * size);
    if (solution == NULL) {
        fprintf(stderr, "malloc failed\n");
        return result;
    }

    for (size_t i = 0; i < size; i++) {
        solution[i].collapsed = false;
    }

    result.data = generate_bitmap(solution, bitmap, output_size, region_size);
    if (result.data == NULL) {
        goto cleanup;
    }

    result.had_error = false;

cleanup:
    free(solution);
    if (result.had_error) {
        free(result.data);
        result.data = NULL;
    }

    return result;
}

int WFC_write_image(const char *filename, WFC_Bitmap bitmap) {
    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        fprintf(stderr, "failed to open file");
        return 1;
    }

    // header
    fprintf(file, "P3\n");
    fprintf(file, "%d %d\n", bitmap.width, bitmap.height);
    fprintf(file, "255\n");

    // data
    for (size_t i = 0; i < bitmap.width * bitmap.height; i++) {
        WFC_Color color = bitmap.data[i];
        fprintf(file, "%d %d %d\n", color.r, color.g, color.b);
    }

    return 0;
}

void wfc_patterns_free(WFC_Patterns *patterns) {
    if (patterns == NULL) {
        return; 
    }

    for (size_t i = 0; i < patterns->capacity; i++) {
        WFC_Pattern entry = patterns->data[i];
        if (entry.active) {
            free_neighbors(&entry.value);
        }
    }
    free(patterns->data);
    patterns->data = NULL;
}

