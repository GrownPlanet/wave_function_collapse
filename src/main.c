#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "wfc.h"

int main(int argc, char **argv) {
    int return_code = 1;

    if (argc != 3) {
        fprintf(stderr, "wfc: missing file operand\n");
        printf("usage: %s SOURCE DEST\n", argv[0]);
        return 1;
    }

    WFC_Bitmap img = WFC_read_file(argv[1]);
    if (img.status_code != WFC_OK) {
         goto cleanup;
    }

    const size_t region_size = 3;
    WFC_Patterns patterns = WFC_extract_patterns(img, region_size);

    return_code = 0;

cleanup:
    free(img.data);
    free(patterns.patterns);

    return return_code;
}
