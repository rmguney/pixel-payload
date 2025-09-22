#include "steg.h"
#include <stdlib.h>
#include <string.h>

/* Embeds payload into cover image and saves result as steg image */
bool steg_embed(const char *cover_path, const char *payload_path, const char *steg_path) {
    ImageInfo cover = {0};
    ImageInfo steg = {0};
    FILE *payload_file = NULL;
    uint8_t *payload_data = NULL;
    uint32_t payload_size = 0;
    uint32_t required_bits;
    bool success = false;
    StegContext ctx = {0};
    uint32_t y, i, j, bit_offset;
    uint8_t byte;
    
    /* Open cover image */
    if (!image_open_read(cover_path, &cover)) {
        fprintf(stderr, "Error: Could not open cover image\n");
        return false;
    }
    
    /* Open payload file and get size */
    payload_file = fopen(payload_path, "rb");
    if (!payload_file) {
        fprintf(stderr, "Error: Could not open payload file\n");
        image_close(&cover);
        return false;
    }
    
    /* Get payload size efficiently */
    fseek(payload_file, 0, SEEK_END);
    payload_size = (uint32_t)ftell(payload_file);
    fseek(payload_file, 0, SEEK_SET);
    
    /* Check capacity early */
    required_bits = payload_size * 8 + 32; /* +32 for size header */
    if (required_bits > cover.capacity) {
        fprintf(stderr, "Error: Cover image too small for payload\n");
        fprintf(stderr, "       Required: %u bits, Available: %zu bits\n", 
                required_bits, cover.capacity);
        fclose(payload_file);
        image_close(&cover);
        return false;
    }
    
    /* Allocate buffer for payload */
    payload_data = (uint8_t *)malloc(payload_size);
    if (!payload_data) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        fclose(payload_file);
        image_close(&cover);
        return false;
    }
    
    /* Read payload data */
    if (fread(payload_data, 1, payload_size, payload_file) != payload_size) {
        fprintf(stderr, "Error: Failed to read payload data\n");
        goto cleanup;
    }
    fclose(payload_file);
    payload_file = NULL;
    
    /* Create steg image file */
    if (!image_open_write(steg_path, &steg, &cover)) {
        fprintf(stderr, "Error: Could not create steg image\n");
        goto cleanup;
    }
    
    /* Copy cover image data to steg image efficiently */
    for (y = 0; y < cover.height; y++) {
        memcpy(steg.row_pointers[y], cover.row_pointers[y], cover.rowbytes);
    }
    
    /* Set up steganography context */
    ctx.image = &steg;
    ctx.payload_size = payload_size;
    
    /* Embed payload size in first 32 bits (little-endian) - optimized loop */
    for (i = 0; i < 32; i++) {
        if (!steg_write_bit(&ctx, (payload_size >> i) & 1, i)) {
            fprintf(stderr, "Error: Failed to embed payload size\n");
            goto cleanup;
        }
    }
    
    /* Embed payload bits - optimized nested loop */
    bit_offset = 32;
    for (i = 0; i < payload_size; i++) {
        byte = payload_data[i];
        for (j = 0; j < 8; j++) {
            if (!steg_write_bit(&ctx, (byte >> j) & 1, bit_offset++)) {
                fprintf(stderr, "Error: Failed to embed payload data\n");
                goto cleanup;
            }
        }
    }
    
    /* Finalize the PNG file */
    if (!image_finalize_write(&steg)) {
        fprintf(stderr, "Error: Failed to finalize PNG output\n");
        goto cleanup;
    }
    
    fprintf(stderr, "Successfully embedded %u bytes (%u bits)\n", 
            payload_size, payload_size * 8);
    success = true;
    
cleanup:
    if (payload_data) {
        /* Security: zero the buffer before freeing */
        memset(payload_data, 0, payload_size);
        free(payload_data);
    }
    if (payload_file) {
        fclose(payload_file);
    }
    image_close(&cover);
    image_close(&steg);
    
    return success;
}

/* Extracts hidden payload from steg image */
bool steg_extract(const char *steg_path, const char *output_path) {
    ImageInfo steg = {0};
    FILE *output_file = NULL;
    uint8_t *payload_data = NULL;
    uint32_t payload_size = 0;
    bool success = false;
    StegContext ctx = {0};
    uint32_t i, j, bit_offset;
    uint8_t byte;
    
    /* Open steg image */
    if (!image_open_read(steg_path, &steg)) {
        fprintf(stderr, "Error: Could not open steg image\n");
        return false;
    }
    
    /* Setup steganography context */
    ctx.image = &steg;
    
    /* Extract payload size from the first 32 LSBs (little-endian) - optimized */
    for (i = 0; i < 32; i++) {
        payload_size |= ((uint32_t)steg_read_bit(&ctx, i) << i);
    }
    fprintf(stderr, "Extracted payload_size: %u\n", payload_size);
    
    /* Validate extracted size against image capacity */
    if (payload_size > steg.capacity / 8) {
        fprintf(stderr, "Error: Invalid payload size detected (%u bytes)\n", payload_size);
        image_close(&steg);
        return false;
    }
    
    /* Allocate buffer for payload */
    payload_data = (uint8_t *)malloc(payload_size);
    if (!payload_data) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        image_close(&steg);
        return false;
    }
    
    /* Extract payload bits - optimized loop */
    bit_offset = 32;
    for (i = 0; i < payload_size; i++) {
        byte = 0;
        for (j = 0; j < 8; j++) {
            byte |= (steg_read_bit(&ctx, bit_offset++) << j);
        }
        payload_data[i] = byte;
    }
    
    /* Open output file */
    output_file = fopen(output_path, "wb");
    if (!output_file) {
        fprintf(stderr, "Error: Could not create output file\n");
        free(payload_data);
        image_close(&steg);
        return false;
    }
    
    /* Write payload to output file */
    if (fwrite(payload_data, 1, payload_size, output_file) == payload_size) {
        fprintf(stderr, "Successfully extracted %u bytes\n", payload_size);
        success = true;
    } else {
        fprintf(stderr, "Error: Failed to write payload data\n");
    }
    
    /* Cleanup */
    if (payload_data) {
        /* Security: zero the buffer before freeing */
        memset(payload_data, 0, payload_size);
        free(payload_data);
    }
    fclose(output_file);
    image_close(&steg);
    
    return success;
}
