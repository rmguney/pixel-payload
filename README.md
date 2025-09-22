# Pixel Payload

**Compact utility to embed and extract steganographic data within images.**

## Features

- **Complete Lossless PNG Support**: All color types, bit depths (1-16), interlaced/non-interlaced, with RGBA transparency preservation
- **C99 Compatible and Secure**: Full C99 standard compliance for compatibility, memory buffers zeroing after use
- **Smol**: 19.5KB CLI and 23.5KB GUI binaries with stealth optimizations using Windows APIs for minimal footprint
- **Full Testing Suite**: Comprehensive Python test suite with RGBA transparency and LSB integrity validation

## Usage

**Download latest [release](https://github.com/rmguney/pixel-payload/releases)** or build from source.

```bash
# Hide data
pxpl.exe embed cover.png secret.txt output.png

# Extract data
pxpl.exe extract output.png extracted.txt

# Launch GUI
pxpl-gui.exe
```

## Technical Details

1. Payload size stored in first 32 LSBs (little-endian)
2. Data embedded sequentially in RGB channel LSBs (alpha channel excluded for RGBA images)
3. Enhanced PNG encoding with LSB preservation:
   - `WICPngFilterNone` - No filtering to preserve exact pixel values
   - `CompressionLevel: 0.0f` - Minimal compression
   - `BGRA` pixel format for RGBA compatibility with WIC encoder
4. Capacity: `(width × height × usable_channels) - 32` bits (usable_channels = 3 for RGB/RGBA)

- 300×300:  ~33KB
- 500×400:  ~75KB  
- 1024×768: ~295KB

## Error Codes

| Code | Description |
|------|-------------|
| 0 | Success |
| 1 | Invalid arguments |
| 2 | Unsupported format |
| 3 | Payload too large |
| 4 | I/O error |
| 5 | PNG error |

### CI/CD and Testing Pipeline

The project uses CMake with GitHub Actions for automated:

- Building ultra compact stealth optimized binaries
- Running comprehensive test suite
- Automatic release packaging

The test suite automatically:

- Creates sample PNG images (RGB, RGBA, Grayscale) with various patterns
- Generates payloads: Small (6B), Medium (176B), Large (708B), Binary (744B)
- Tests embed/extract operations with full content verification
- Validates RGBA transparency preservation and LSB integrity
- Runs 9 comprehensive tests covering all functionality
- Cleans up all temporary files

**Expected Result**: All 9/9 tests should pass for a working implementation.

## Limitations and Future Work

- No built-in encryption in MVP, steganography only
- LSB changes may be detectable by analysis tools
- Optimized for Windows with MSVC, other platforms require adaptation
- Following features need implementation:

| P | Feature                           | Tasks                                                                 |
| - | --------------------------------- | --------------------------------------------------------------------- |
| 1 | **CRC32 Integrity Checks**        | Append CRC32 to header, validate during decode                        |
| 1 | **Batch Mode & Pipelines**        | Support `-` for stdin/stdout, detect binary/text                      |
| 2 | **Multi-Bit-Plane Embedding**     | `--depth N` flag, update extract logic                                |
| 2 | **AES-GCM Encryption (opt-in)**   | `--key` flag, prepend 96-bit nonce                                    |
| 2 | **Steganalysis Resistance**       | ±1 embedding, variance-based pixel selection, `--seed` for RNG        |
| 3 | **Cross-platform Support**        | Platform specific compilation for Linux using LodePNG                 |
