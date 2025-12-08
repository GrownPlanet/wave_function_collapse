#include <stdio.h>
#include <stdlib.h>

#include "wfc.h"

int main(int argc, char **argv) {
    int return_code = 1;

    WFC_Bitmap img;

    if (argc != 3) {
        fprintf(stderr, "wfc: missing file operand\n");
        printf("usage: %s SOURCE DEST\n", argv[0]);
        goto cleanup;
    }

    img = WFC_read_file(argv[1]);
    if (img.status_code != WFC_OK) {
         goto cleanup;
    }

    return_code = 0;
cleanup:
    free(img.data);

    return return_code;
}
