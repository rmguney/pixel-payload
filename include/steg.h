#ifndef STEG_H
#define STEG_H

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

/* Use Windows Imaging Component (WIC) for PNG support */
#ifdef _WIN32
#include <windows.h>
#include <wincodec.h>
#include <combaseapi.h>
#else
#error "This implementation requires Windows"
#endif

/* Return codes */
#define STEG_SUCCESS               0
#define STEG_ERROR_ARGS            1
#define STEG_ERROR_FORMAT          2
#define STEG_ERROR_CAPACITY        3
#define STEG_ERROR_IO              4
#define STEG_ERROR_PNG             5

/* PNG color types (matching PNG standard values) */
typedef enum {
    PNG_COLOR_GRAYSCALE = 0,        /* 0 - Grayscale */
    PNG_COLOR_RGB = 2,              /* 2 - RGB */
    PNG_COLOR_PALETTE = 3,          /* 3 - Indexed color */
    PNG_COLOR_GRAYSCALE_ALPHA = 4,  /* 4 - Grayscale + Alpha */
    PNG_COLOR_RGBA = 6              /* 6 - RGBA */
} PngColorType;

/* Image metadata - optimized layout for cache efficiency */
typedef struct {
    /* Frequently accessed fields first (cache line 1) */
    uint32_t width;             /* Image width in pixels */
    uint32_t height;            /* Image height in pixels */
    size_t rowbytes;            /* Bytes per row */
    size_t capacity;            /* Available capacity in bits for payload */
    uint8_t **row_pointers;     /* Array of row pointers */
    
    /* Less frequently accessed fields */
    uint8_t channels;           /* Number of channels (1, 2, 3, 4) */
    uint8_t bytes_per_pixel;    /* Bytes per pixel */
    uint8_t bit_depth;          /* Bits per channel (1, 2, 4, 8, 16) */
    uint8_t color_type;         /* PNG color type */
    bool has_alpha;             /* Whether image has alpha channel */
    bool interlaced;            /* Whether image is interlaced */
    
    /* WIC structures (used during I/O only) */
    IWICImagingFactory *wic_factory;    /* WIC factory */
    IWICBitmapDecoder *decoder;         /* WIC decoder */
    IWICBitmapFrameDecode *frame;       /* WIC frame decoder */
    IWICBitmapEncoder *encoder;         /* WIC encoder */
    IWICBitmapFrameEncode *frame_encode; /* WIC frame encoder */
    IWICStream *stream;                 /* WIC stream */
    FILE *fp;                           /* File pointer for I/O operations */
    uint8_t *current_row;               /* Current row being processed */
} ImageInfo;

/* Steganography context */
typedef struct {
    ImageInfo *image;       /* Image being processed */
    uint32_t payload_size;  /* Size of payload in bytes */
    size_t bits_processed;  /* Bits processed so far */
} StegContext;

/* Image handling functions */
bool image_open_read(const char *filename, ImageInfo *info);
bool image_open_write(const char *filename, ImageInfo *info, const ImageInfo *template);
void image_close(ImageInfo *info);
bool image_read_row(ImageInfo *info, uint32_t y);
bool image_write_row(ImageInfo *info, uint32_t y);
bool image_finalize_write(ImageInfo *info);

/* Steganography functions */
bool steg_embed(const char *cover_path, const char *payload_path, const char *steg_path);
bool steg_extract(const char *steg_path, const char *output_path);

/* Inline bit manipulation functions for performance */
static inline bool steg_write_bit(StegContext *ctx, uint8_t bit, uint32_t offset) {
    if (!ctx || !ctx->image || !ctx->image->row_pointers) return false;
    
    uint8_t usable_channels = ctx->image->channels - (ctx->image->has_alpha ? 1 : 0);
    uint32_t width_channels = ctx->image->width * usable_channels;
    uint32_t y = offset / width_channels;
    uint32_t x = (offset % width_channels) / usable_channels;
    uint32_t channel = offset % usable_channels;
    
    if (y >= ctx->image->height || x >= ctx->image->width) return false;
    
    uint32_t pixel_offset = x * ctx->image->bytes_per_pixel + channel;
    uint8_t *pixel = &ctx->image->row_pointers[y][pixel_offset];
    *pixel = (*pixel & 0xFE) | (bit & 0x01);
    
    return true;
}

static inline uint8_t steg_read_bit(StegContext *ctx, uint32_t offset) {
    if (!ctx || !ctx->image || !ctx->image->row_pointers) return 0;
    
    uint8_t usable_channels = ctx->image->channels - (ctx->image->has_alpha ? 1 : 0);
    uint32_t width_channels = ctx->image->width * usable_channels;
    uint32_t y = offset / width_channels;
    uint32_t x = (offset % width_channels) / usable_channels;
    uint32_t channel = offset % usable_channels;
    
    if (y >= ctx->image->height || x >= ctx->image->width) return 0;
    
    uint32_t pixel_offset = x * ctx->image->bytes_per_pixel + channel;
    return ctx->image->row_pointers[y][pixel_offset] & 0x01;
}

#endif /* STEG_H */
