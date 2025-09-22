#include "steg.h"
#include <stdlib.h>
#include <string.h>

/* Calculate capacity in bits for steganography */
static size_t calculate_capacity(const ImageInfo *info) {
    size_t total_bits = 0;
    
    /* Calculate usable channels (exclude alpha if present) */
    uint8_t usable_channels = info->channels;
    if (info->has_alpha) {
        usable_channels--;
    }
    
    /* Calculate bits per channel based on bit depth */
    uint8_t usable_bits_per_channel = 1; /* We use LSB only for now */
    
    /* Total capacity = pixels * usable_channels * usable_bits_per_channel - header */
    total_bits = (size_t)info->width * info->height * usable_channels * usable_bits_per_channel;
    
    /* Reserve 32 bits for payload length header */
    if (total_bits > 32) {
        total_bits -= 32;
    } else {
        total_bits = 0;
    }
    
    return total_bits;
}bool image_open_read(const char *filename, ImageInfo *info) {
    HRESULT hr;
    WCHAR wfilename[MAX_PATH];
    WICPixelFormatGUID pixelFormat;
    
    if (!filename || !info) {
        return false;
    }
    
    /* Initialize structure */
    memset(info, 0, sizeof(ImageInfo));
    
    /* Initialize COM */
    hr = CoInitialize(NULL);
    if (FAILED(hr)) {
        fprintf(stderr, "Error: Failed to initialize COM\n");
        return false;
    }
    
    /* Create WIC factory */
    hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
                         &IID_IWICImagingFactory, (LPVOID*)&info->wic_factory);
    if (FAILED(hr)) {
        fprintf(stderr, "Error: Failed to create WIC factory\n");
        CoUninitialize();
        return false;
    }
    
    /* Convert filename to wide char */
    MultiByteToWideChar(CP_UTF8, 0, filename, -1, wfilename, MAX_PATH);
    
    /* Create decoder from filename */
    hr = info->wic_factory->lpVtbl->CreateDecoderFromFilename(info->wic_factory,
                                                             wfilename, NULL, GENERIC_READ,
                                                             WICDecodeMetadataCacheOnLoad,
                                                             &info->decoder);
    if (FAILED(hr)) {
        fprintf(stderr, "Error: Cannot open file %s or not a valid image\n", filename);
        info->wic_factory->lpVtbl->Release(info->wic_factory);
        CoUninitialize();
        return false;
    }
    
    /* Get first frame */
    hr = info->decoder->lpVtbl->GetFrame(info->decoder, 0, &info->frame);
    if (FAILED(hr)) {
        fprintf(stderr, "Error: Failed to get image frame\n");
        info->decoder->lpVtbl->Release(info->decoder);
        info->wic_factory->lpVtbl->Release(info->wic_factory);
        CoUninitialize();
        return false;
    }
    
    /* Get image dimensions */
    hr = info->frame->lpVtbl->GetSize(info->frame, &info->width, &info->height);
    if (FAILED(hr)) {
        fprintf(stderr, "Error: Failed to get image dimensions\n");
        goto cleanup_read;
    }
    
    /* Get pixel format */
    hr = info->frame->lpVtbl->GetPixelFormat(info->frame, &pixelFormat);
    if (FAILED(hr)) {
        fprintf(stderr, "Error: Failed to get pixel format\n");
        goto cleanup_read;
    }
      /* Store original pixel format for processing decision */
    info->bit_depth = 8; /* WIC normalizes to 8-bit */
    
    /* Allocate row pointers */
    info->row_pointers = (uint8_t**)malloc(sizeof(uint8_t*) * info->height);
    if (!info->row_pointers) {
        fprintf(stderr, "Error: Failed to allocate row pointers\n");
        goto cleanup_read;
    }
      /* Copy pixels - read in original format to preserve LSBs */
    WICRect rect = {0, 0, info->width, info->height};
    
    /* Get original pixel format and determine the appropriate parameters */
    if (IsEqualGUID(&pixelFormat, &GUID_WICPixelFormat24bppRGB)) {
        /* Already 24bpp RGB, copy directly */
        UINT stride = info->width * 3;
        UINT bufferSize = stride * info->height;
        uint8_t *buffer = (uint8_t*)malloc(bufferSize);
        
        if (!buffer) {
            fprintf(stderr, "Error: Failed to allocate image buffer\n");
            goto cleanup_read;
        }
          hr = info->frame->lpVtbl->CopyPixels(info->frame, &rect, stride, bufferSize, buffer);
          if (SUCCEEDED(hr)) {
            /* Set final format parameters */
            info->channels = 3;
            info->has_alpha = false;
            info->bytes_per_pixel = 3;
            info->rowbytes = info->width * 3;
            
            /* Allocate memory for image data */
            for (uint32_t y = 0; y < info->height; y++) {
                info->row_pointers[y] = (uint8_t*)malloc(info->rowbytes);
                if (!info->row_pointers[y]) {
                    fprintf(stderr, "Error: Failed to allocate row %u\n", y);
                    /* Clean up previously allocated rows */
                    for (uint32_t i = 0; i < y; i++) {
                        free(info->row_pointers[i]);
                    }
                    free(info->row_pointers);
                    free(buffer);
                    goto cleanup_read;
                }
            }
            
            /* Copy to row pointers and convert BGR back to RGB */
            for (uint32_t y = 0; y < info->height; y++) {
                uint8_t *src_row = buffer + (y * stride);
                uint8_t *dst_row = info->row_pointers[y];
                
                for (uint32_t x = 0; x < info->width; x++) {
                    // Convert BGR back to RGB
                    dst_row[x * 3 + 0] = src_row[x * 3 + 2]; // Red = src Blue
                    dst_row[x * 3 + 1] = src_row[x * 3 + 1]; // Green = src Green
                    dst_row[x * 3 + 2] = src_row[x * 3 + 0]; // Blue = src Red
                }
            }
        }
          free(buffer);
    } else if (IsEqualGUID(&pixelFormat, &GUID_WICPixelFormat32bppRGBA) ||
               IsEqualGUID(&pixelFormat, &GUID_WICPixelFormat32bppBGRA)) {
        /* Handle RGBA formats - preserve alpha channel */
        UINT stride = info->width * 4;
        UINT bufferSize = stride * info->height;
        uint8_t *buffer = (uint8_t*)malloc(bufferSize);
        
        if (!buffer) {
            fprintf(stderr, "Error: Failed to allocate image buffer\n");
            goto cleanup_read;
        }        /* Create format converter to standardize to RGBA */
        IWICFormatConverter *converter = NULL;
        hr = info->wic_factory->lpVtbl->CreateFormatConverter(info->wic_factory, &converter);
        if (SUCCEEDED(hr)) {
            hr = converter->lpVtbl->Initialize(converter, (IWICBitmapSource*)info->frame,
                                              &GUID_WICPixelFormat32bppRGBA, WICBitmapDitherTypeNone,
                                              NULL, 0.0, WICBitmapPaletteTypeCustom);
            if (SUCCEEDED(hr)) {
                hr = converter->lpVtbl->CopyPixels(converter, &rect, stride, bufferSize, buffer);
            }
            converter->lpVtbl->Release(converter);
        }
          if (SUCCEEDED(hr)) {
            /* Set final format parameters */
            info->channels = 4;
            info->has_alpha = true;
            info->bytes_per_pixel = 4;
            info->rowbytes = info->width * 4;
            
            /* Allocate memory for image data */
            for (uint32_t y = 0; y < info->height; y++) {
                info->row_pointers[y] = (uint8_t*)malloc(info->rowbytes);
                if (!info->row_pointers[y]) {
                    fprintf(stderr, "Error: Failed to allocate row %u\n", y);
                    /* Clean up previously allocated rows */
                    for (uint32_t i = 0; i < y; i++) {
                        free(info->row_pointers[i]);
                    }
                    free(info->row_pointers);
                    free(buffer);
                    goto cleanup_read;
                }
            }
            
            /* Copy to row pointers - RGBA format */
            for (uint32_t y = 0; y < info->height; y++) {
                memcpy(info->row_pointers[y], buffer + (y * stride), stride);
            }
        }
        
        free(buffer);
    } else {
        /* Convert to 24bpp RGB for other formats */
        UINT stride = info->width * 3;
        UINT bufferSize = stride * info->height;
        uint8_t *buffer = (uint8_t*)malloc(bufferSize);
        
        if (!buffer) {
            fprintf(stderr, "Error: Failed to allocate image buffer\n");
            goto cleanup_read;
        }
        
        /* Create format converter */
        IWICFormatConverter *converter = NULL;
        hr = info->wic_factory->lpVtbl->CreateFormatConverter(info->wic_factory, &converter);
        if (SUCCEEDED(hr)) {
            hr = converter->lpVtbl->Initialize(converter, (IWICBitmapSource*)info->frame,
                                              &GUID_WICPixelFormat24bppRGB, WICBitmapDitherTypeNone,
                                              NULL, 0.0, WICBitmapPaletteTypeCustom);
            if (SUCCEEDED(hr)) {
                hr = converter->lpVtbl->CopyPixels(converter, &rect, stride, bufferSize, buffer);
            }
            converter->lpVtbl->Release(converter);
        }
          if (SUCCEEDED(hr)) {
            /* Set final format parameters */
            info->channels = 3;
            info->has_alpha = false;
            info->bytes_per_pixel = 3;
            info->rowbytes = info->width * 3;
            
            /* Allocate memory for image data */
            for (uint32_t y = 0; y < info->height; y++) {
                info->row_pointers[y] = (uint8_t*)malloc(info->rowbytes);
                if (!info->row_pointers[y]) {
                    fprintf(stderr, "Error: Failed to allocate row %u\n", y);
                    /* Clean up previously allocated rows */
                    for (uint32_t i = 0; i < y; i++) {
                        free(info->row_pointers[i]);
                    }
                    free(info->row_pointers);
                    free(buffer);
                    goto cleanup_read;
                }
            }
            
            /* Copy to row pointers */
            for (uint32_t y = 0; y < info->height; y++) {
                memcpy(info->row_pointers[y], buffer + (y * stride), stride);
            }
        }
          free(buffer);
    }
      /* Calculate capacity for steganography after format is determined */
    info->capacity = calculate_capacity(info);
    
    if (FAILED(hr)) {
        fprintf(stderr, "Error: Failed to copy image pixels\n");
        goto cleanup_read;
    }
    
    return true;

cleanup_read:
    if (info->row_pointers) {
        for (uint32_t y = 0; y < info->height; y++) {
            if (info->row_pointers[y]) {
                free(info->row_pointers[y]);
            }
        }
        free(info->row_pointers);
    }
    if (info->frame) info->frame->lpVtbl->Release(info->frame);
    if (info->decoder) info->decoder->lpVtbl->Release(info->decoder);
    if (info->wic_factory) info->wic_factory->lpVtbl->Release(info->wic_factory);
    CoUninitialize();
    return false;
}

bool image_open_write(const char *filename, ImageInfo *info, const ImageInfo *template) {
    HRESULT hr;
    WCHAR wfilename[MAX_PATH];
    
    if (!filename || !info || !template) {
        return false;
    }
    
    /* Copy template information */
    memcpy(info, template, sizeof(ImageInfo));
    
    /* Reset WIC structures */
    info->wic_factory = NULL;
    info->encoder = NULL;
    info->frame_encode = NULL;
    info->stream = NULL;
    info->decoder = NULL;
    info->frame = NULL;
    info->fp = NULL;
    info->current_row = NULL;
    
    /* Initialize COM */
    hr = CoInitialize(NULL);
    if (FAILED(hr)) {
        fprintf(stderr, "Error: Failed to initialize COM for writing\n");
        return false;
    }
    
    /* Create WIC factory */
    hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
                         &IID_IWICImagingFactory, (LPVOID*)&info->wic_factory);
    if (FAILED(hr)) {
        fprintf(stderr, "Error: Failed to create WIC factory for writing\n");
        CoUninitialize();
        return false;
    }
    
    /* Convert filename to wide char */
    MultiByteToWideChar(CP_UTF8, 0, filename, -1, wfilename, MAX_PATH);
    
    /* Create stream */
    hr = info->wic_factory->lpVtbl->CreateStream(info->wic_factory, &info->stream);
    if (FAILED(hr)) {
        fprintf(stderr, "Error: Failed to create WIC stream\n");
        goto cleanup_write;
    }
    
    /* Initialize stream from filename */
    hr = info->stream->lpVtbl->InitializeFromFilename(info->stream, wfilename, GENERIC_WRITE);
    if (FAILED(hr)) {
        fprintf(stderr, "Error: Cannot create file %s\n", filename);
        goto cleanup_write;
    }
    
    /* Create PNG encoder */
    hr = info->wic_factory->lpVtbl->CreateEncoder(info->wic_factory, &GUID_ContainerFormatPng,
                                                 NULL, &info->encoder);
    if (FAILED(hr)) {
        fprintf(stderr, "Error: Failed to create PNG encoder\n");
        goto cleanup_write;
    }
      /* Initialize encoder */
    hr = info->encoder->lpVtbl->Initialize(info->encoder, (IStream*)info->stream,
                                          WICBitmapEncoderNoCache);
    if (FAILED(hr)) {
        fprintf(stderr, "Error: Failed to initialize PNG encoder\n");
        goto cleanup_write;
    }
      /* Create frame encoder */
    IPropertyBag2 *propertyBag = NULL;
    hr = info->encoder->lpVtbl->CreateNewFrame(info->encoder, &info->frame_encode, &propertyBag);
    if (FAILED(hr)) {
        fprintf(stderr, "Error: Failed to create PNG frame encoder\n");
        goto cleanup_write;
    }
      /* Configure PNG encoder for lossless steganography */    if (propertyBag) {
        PROPBAG2 option = { 0 };
        VARIANT varValue;
        
        /* Set PNG filter to NONE to preserve exact pixel values */
        option.pstrName = L"FilterOption";
        VariantInit(&varValue);
        varValue.vt = VT_UI1;
        varValue.bVal = WICPngFilterNone;  /* No filtering - preserves LSBs */
        propertyBag->lpVtbl->Write(propertyBag, 1, &option, &varValue);
        VariantClear(&varValue);
        
        /* Disable interlacing to maintain pixel order */
        option.pstrName = L"InterlaceOption";
        VariantInit(&varValue);
        varValue.vt = VT_BOOL;
        varValue.boolVal = VARIANT_FALSE;
        propertyBag->lpVtbl->Write(propertyBag, 1, &option, &varValue);
        VariantClear(&varValue);
        
        /* Set minimal compression to preserve LSBs */
        option.pstrName = L"CompressionLevel";
        VariantInit(&varValue);
        varValue.vt = VT_R4;
        varValue.fltVal = 0.0f;  /* Minimal compression */
        propertyBag->lpVtbl->Write(propertyBag, 1, &option, &varValue);
        VariantClear(&varValue);
        
        /* Force bit depth to 8 to ensure no bit depth conversion */
        option.pstrName = L"BitDepth";
        VariantInit(&varValue);
        varValue.vt = VT_UI1;
        varValue.bVal = 8;
        propertyBag->lpVtbl->Write(propertyBag, 1, &option, &varValue);
        VariantClear(&varValue);
        
        /* Disable gamma correction to preserve raw pixel values */
        option.pstrName = L"EnableV5Header32bppBGRA";
        VariantInit(&varValue);
        varValue.vt = VT_BOOL;
        varValue.boolVal = VARIANT_FALSE;
        propertyBag->lpVtbl->Write(propertyBag, 1, &option, &varValue);
        VariantClear(&varValue);
    }
    
    /* Initialize frame encoder */
    hr = info->frame_encode->lpVtbl->Initialize(info->frame_encode, propertyBag);
    if (propertyBag) {
        propertyBag->lpVtbl->Release(propertyBag);
    }
    if (FAILED(hr)) {
        fprintf(stderr, "Error: Failed to initialize PNG frame encoder\n");
        goto cleanup_write;
    }
    
    /* Set frame size */
    hr = info->frame_encode->lpVtbl->SetSize(info->frame_encode, info->width, info->height);
    if (FAILED(hr)) {
        fprintf(stderr, "Error: Failed to set frame size\n");
        goto cleanup_write;
    }    /* Set pixel format based on whether image has alpha channel */
    WICPixelFormatGUID pixelFormat;
    if (info->has_alpha) {
        pixelFormat = GUID_WICPixelFormat32bppBGRA;  /* Use BGRA instead of RGBA */
    } else {
        pixelFormat = GUID_WICPixelFormat24bppBGR;
    }
    hr = info->frame_encode->lpVtbl->SetPixelFormat(info->frame_encode, &pixelFormat);
    if (FAILED(hr)) {
        fprintf(stderr, "Error: Failed to set pixel format\n");
        goto cleanup_write;
    }
    
    /* Allocate row pointers for writing */
    info->row_pointers = (uint8_t**)malloc(sizeof(uint8_t*) * info->height);
    if (!info->row_pointers) {
        fprintf(stderr, "Error: Failed to allocate row pointers for writing\n");
        goto cleanup_write;
    }
    
    /* Allocate memory for each row */
    for (uint32_t y = 0; y < info->height; y++) {
        info->row_pointers[y] = (uint8_t*)malloc(info->rowbytes);
        if (!info->row_pointers[y]) {
            fprintf(stderr, "Error: Failed to allocate row %u for writing\n", y);
            /* Clean up previously allocated rows */
            for (uint32_t i = 0; i < y; i++) {
                free(info->row_pointers[i]);
            }
            free(info->row_pointers);
            info->row_pointers = NULL;
            goto cleanup_write;
        }
    }
    
    return true;

cleanup_write:
    if (info->frame_encode) {
        info->frame_encode->lpVtbl->Release(info->frame_encode);
        info->frame_encode = NULL;
    }
    if (info->encoder) {
        info->encoder->lpVtbl->Release(info->encoder);
        info->encoder = NULL;
    }
    if (info->stream) {
        info->stream->lpVtbl->Release(info->stream);
        info->stream = NULL;
    }
    if (info->wic_factory) {
        info->wic_factory->lpVtbl->Release(info->wic_factory);
        info->wic_factory = NULL;
    }
    CoUninitialize();
    return false;
}

bool image_read_row(ImageInfo *info, uint32_t y) {
    if (!info || !info->row_pointers || y >= info->height) {
        return false;
    }
    
    /* Set current row pointer */
    info->current_row = info->row_pointers[y];
    return true;
}

bool image_write_row(ImageInfo *info, uint32_t y) {
    /* For PNG, we'll write all rows at once in finalize_write */
    /* This function just validates parameters */
    return (info && info->row_pointers && y < info->height);
}

bool image_finalize_write(ImageInfo *info) {
    HRESULT hr;
    
    if (!info || !info->frame_encode || !info->encoder || !info->row_pointers) {
        return false;
    }
      /* Create a single buffer with all image data */
    UINT stride = info->width * info->bytes_per_pixel;
    UINT bufferSize = stride * info->height;
    uint8_t *buffer = (uint8_t*)malloc(bufferSize);
    
    if (!buffer) {
        fprintf(stderr, "Error: Failed to allocate write buffer\n");
        return false;
    }
      /* Copy row data to single buffer and handle RGB/RGBA conversion for WIC */
    for (uint32_t y = 0; y < info->height; y++) {
        uint8_t *src_row = info->row_pointers[y];
        uint8_t *dst_row = buffer + (y * stride);        if (info->has_alpha) {
            /* Handle RGBA -> BGRA conversion for WIC PNG encoder */
            for (uint32_t x = 0; x < info->width; x++) {
                dst_row[x * 4 + 0] = src_row[x * 4 + 2]; // Blue = src Red
                dst_row[x * 4 + 1] = src_row[x * 4 + 1]; // Green = src Green  
                dst_row[x * 4 + 2] = src_row[x * 4 + 0]; // Red = src Blue
                dst_row[x * 4 + 3] = src_row[x * 4 + 3]; // Alpha = src Alpha
            }
        } else {
            /* Handle RGB - convert RGB to BGR for WIC */
            for (uint32_t x = 0; x < info->width; x++) {
                dst_row[x * 3 + 0] = src_row[x * 3 + 2]; // Blue = src Red
                dst_row[x * 3 + 1] = src_row[x * 3 + 1]; // Green = src Green  
                dst_row[x * 3 + 2] = src_row[x * 3 + 0]; // Red = src Blue
            }
        }
    }
    
    /* Write pixels to frame */
    hr = info->frame_encode->lpVtbl->WritePixels(info->frame_encode, info->height,
                                                stride, bufferSize, buffer);
    free(buffer);
    
    if (FAILED(hr)) {
        fprintf(stderr, "Error: Failed to write pixels to PNG\n");
        return false;
    }
    
    /* Commit frame */
    hr = info->frame_encode->lpVtbl->Commit(info->frame_encode);
    if (FAILED(hr)) {
        fprintf(stderr, "Error: Failed to commit PNG frame\n");
        return false;
    }
    
    /* Commit encoder */
    hr = info->encoder->lpVtbl->Commit(info->encoder);
    if (FAILED(hr)) {
        fprintf(stderr, "Error: Failed to commit PNG encoder\n");
        return false;
    }
    
    return true;
}

void image_close(ImageInfo *info) {
    if (!info) {
        return;
    }
    
    /* Clean up row pointers */
    if (info->row_pointers) {
        for (uint32_t y = 0; y < info->height; y++) {
            if (info->row_pointers[y]) {
                /* Security: zero the buffer before freeing */
                memset(info->row_pointers[y], 0, info->rowbytes);
                free(info->row_pointers[y]);
            }
        }
        free(info->row_pointers);
        info->row_pointers = NULL;
    }
    
    /* Clean up WIC structures */
    if (info->frame_encode) {
        info->frame_encode->lpVtbl->Release(info->frame_encode);
        info->frame_encode = NULL;
    }
    if (info->encoder) {
        info->encoder->lpVtbl->Release(info->encoder);
        info->encoder = NULL;
    }
    if (info->stream) {
        info->stream->lpVtbl->Release(info->stream);
        info->stream = NULL;
    }
    if (info->frame) {
        info->frame->lpVtbl->Release(info->frame);
        info->frame = NULL;
    }
    if (info->decoder) {
        info->decoder->lpVtbl->Release(info->decoder);
        info->decoder = NULL;
    }
    if (info->wic_factory) {
        info->wic_factory->lpVtbl->Release(info->wic_factory);
        info->wic_factory = NULL;
    }
    
    /* Close file handle if present */
    if (info->fp) {
        fclose(info->fp);
        info->fp = NULL;
    }
    
    /* Uninitialize COM */
    CoUninitialize();
    
    /* Clear structure */
    memset(info, 0, sizeof(ImageInfo));
}
