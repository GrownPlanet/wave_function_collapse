#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "wfc.h"

int main(int argc, char **argv) {
    const size_t region_size = 3;

    int status_code = 1;
    WFC_Bitmap img = {0};
    WFC_Patterns patterns = {0};
    WFC_Bitmap output = {0};

    if (argc != 3) {
        fprintf(stderr, "missing file operand\n");
        fprintf(stderr, "usage: %s SOURCE DEST\n", argv[0]);
        goto cleanup;
    }

    img = WFC_read_image(argv[1]);
    if (img.had_error) {
        goto cleanup;
    }

    patterns = WFC_extract_patterns(img, region_size);
    if (patterns.had_error) {
        goto cleanup;
    }

    WFC_Point output_size = { .x = 32, .y = 32 };
    output = WFC_Solve(patterns, img, region_size, output_size);
    if (output.had_error) {
        goto cleanup;
    }

    int result = WFC_write_image(argv[2], output);
    if (result != 0) {
        goto cleanup;
    }

    status_code = 0;

cleanup:
    free(img.data);
    free(output.data);
    wfc_patterns_free(&patterns);
    return status_code;
}
