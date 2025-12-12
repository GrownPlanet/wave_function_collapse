#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "wfc.h"
#include "wfc_hashmap.h"

int main(int argc, char **argv) {
    const size_t region_size = 3;

    int status_code = 1;
    WFC_Bitmap img = {0};
    WFC_NeighborMap patterns = {0};

    if (argc != 3) {
        fprintf(stderr, "wfc: missing file operand\n");
        printf("usage: %s SOURCE DEST\n", argv[0]);
        goto cleanup;
    }

    img = WFC_read_file(argv[1]);
    if (img.status_code != WFC_OK) {
        goto cleanup;
    }

    patterns = WFC_extract_patterns(img, region_size);
    if (patterns.had_error) {
        goto cleanup;
    }

    status_code = 0;

cleanup:
    free(img.data);
    wfc_neighbor_map_free(&patterns);
    return status_code;
}
