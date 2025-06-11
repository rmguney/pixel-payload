# Pixel Payload (pxpl)

A lightweight steganography tool for embedding and extracting data in PNG images using Least Significant Bit (LSB) embedding with comprehensive PNG support.

## Features

- **Complete Lossless PNG Support**: All color types, bit depths (1-16), interlaced/non-interlaced, with RGBA transparency preservation
- **Dual Interface**: Available in CLI and GUI for Windows (CLI for Linux planned)
- **Windows API**: Uses the Windows APIs for the Windows build, suitable for living off the land
- **Smol**: Standalone 27KB CLI and 31KB GUI binaries for the P0 MVP, written in pure C
- **Secure**: Memory buffers zeroed after use
- **Tested**: Comprehensive test suite with RGBA transparency and LSB integrity validation

## Quickstart

```bash
# Build using CMake (all platforms)
cmake -B build
cmake --build build                    # CLI version (default)
cmake --build build -t pxpl-gui        # GUI version only  
cmake -DBUILD_BOTH=ON -B build && cmake --build build  # Both versions

# Debug builds
cmake -DCMAKE_BUILD_TYPE=Debug -B build-debug
cmake --build build-debug

# Run tests
cmake --build build -t test_all
# or
cd build && ctest --output-on-failure
```

```bash
# Hide data
.\release\pxpl.exe embed cover.png secret.txt output.png

# Extract data
.\release\pxpl.exe extract output.png extracted.txt

# Launch GUI
.\release\pxpl-gui.exe
```

## Project Structure

``` dir
pxpl/
├── CMakeLists.txt        # CMake build configuration
├── include/steg.h        # API definitions
├── src/                  # Source files (main.c, gui.c, steg.c, image.c)
├── tests/PxplTest.cs     # C# test suite
├── tests/PxplTest.csproj # C# project file
└── README.md
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

## Testing

```bash
# Build and run tests using CMake
cmake -DBUILD_BOTH=ON -B build
cmake --build build -t test_all

# Or run tests manually after building
cd build && ctest --output-on-failure

# Build both versions first, then run C# test suite directly
cmake -DBUILD_BOTH=ON -B build && cmake --build build
cd tests && dotnet run test
```

The test suite creates sample images, embeds various payloads, extracts them, and validates the results. Enhanced tests include:

- **RGBA Transparency Preservation**: Verifies alpha channel is maintained through steganography process
- **LSB Integrity Validation**: Confirms no bit corruption during PNG encoding/decoding
- **File Extension Auto-Append**: GUI feature automatically adds .png/.txt extensions when saving
- **Cross-Format Compatibility**: Tests RGB, RGBA, and grayscale PNG formats

All tests should pass for a working implementation.

## Error Codes

| Code | Description |
|------|-------------|
| 0 | Success |
| 1 | Invalid arguments |
| 2 | Unsupported format |
| 3 | Payload too large |
| 4 | I/O error |

## Requirements

- C11 compiler (MSVC/GCC/MinGW)
- CMake 3.16+
- .NET 6.0+ (test suite only, otherwise skip)

## Future Directions

| P | Feature                           | Tasks                                                                 |
| - | --------------------------------- | --------------------------------------------------------------------- |
| 0 | **Complete PNG Support**          | All bit depths, color types, interlaced/non-interlaced * MVP   |
| 0 | **Enhanced PNG Encoding**         | Lossless settings, BGRA format compatibility * MVP            |
| 0 | **Dual Interface (CLI + GUI)**    | command-line and GUI versions fow Windows * MVP              |
| 0 | **Comprehensive Test Suite**      | C# test suite with RGBA/LSB validation * MVP                  |
| 1 | **CRC32 Integrity Checks**        | Append CRC32 to header, validate during decode                        |
| 1 | **Batch Mode & Pipelines**        | Support `-` for stdin/stdout, detect binary/text                      |
| 2 | **Multi-Bit-Plane Embedding**     | `--depth N` flag, update extract logic                                |
| 2 | **AES-GCM Encryption (opt-in)**   | `--key` flag, prepend 96-bit nonce                                    |
| 3 | **Steganalysis Resistance**       | ±1 embedding, variance-based pixel selection, `--seed` for RNG        |
| 3 | **Cross-platform Support**        | Platform specific compilation for Linux using LodePNG (seperate image-linux.c) |

- **P0 features are completed in the current version**

## Disclaimers

- No built-in encryption in MVP - steganography only
- LSB changes may be detectable by analysis tools
- For educational and research purposes under MIT license
