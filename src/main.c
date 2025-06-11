/**
 * pxpl - PNG LSB Steganography Tool
 * MIT License
 * 
 * Usage:
 *   pxpl embed   <cover.png> <payload.bin> <steg.png>
 *   pxpl extract <steg.png> <output.bin>
 * 
 * Return codes:
 *   0 - Success
 *   1 - Incorrect arguments
 *   2 - Unsupported or corrupt image
 *   3 - Cover image too small
 *   4 - I/O error
 *   5 - PNG error
 */

#include "steg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Display program usage information */
static void show_usage(void) {
    fprintf(stderr, "PNG LSB Steganography Tool\n"
                    "Usage:\n"
                    "  pxpl embed   <cover.png> <payload.bin> <steg.png>\n"
                    "  pxpl extract <steg.png> <output.bin>\n"
                    "Return codes:\n"
                    "  0 - Success\n"
                    "  1 - Incorrect arguments\n"
                    "  2 - Unsupported or corrupt image\n"
                    "  3 - Cover image too small\n"
                    "  4 - I/O error\n"
                    "  5 - PNG error\n");
}

int main(int argc, char **argv) {
    /* Check for correct argument count */
    if (argc < 2) {
        show_usage();
        return STEG_ERROR_ARGS;
    }
    
    /* Optimized command dispatch using string length + first char */
    const char *cmd = argv[1];
    bool success = false;
    
    if (cmd[0] == 'e') {
        if (cmd[1] == 'm' && argc == 5) { /* embed */
            success = steg_embed(argv[2], argv[3], argv[4]);
        } else if (cmd[1] == 'x' && argc == 4) { /* extract */
            success = steg_extract(argv[2], argv[3]);
        } else {
            show_usage();
            return STEG_ERROR_ARGS;
        }
    } else {
        show_usage();
        return STEG_ERROR_ARGS;
    }
    
    return success ? STEG_SUCCESS : STEG_ERROR_IO;
}
