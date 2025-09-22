#!/usr/bin/env python3

import sys
import subprocess
import argparse
import math
from pathlib import Path
from PIL import Image, ImageDraw

class PxplTest:
    """Main test class for pxpl"""

    def __init__(self):
        # Use CMake release build directory - handle both possible locations
        self.project_root = Path(__file__).parent.parent
        self.exe_path = self.project_root / "release" / "pxpl.exe"
        self.gui_exe_path = self.project_root / "release" / "pxpl-gui.exe"

        # Fallback to build directory if release doesn't exist
        if not self.exe_path.exists():
            self.exe_path = self.project_root / "build" / "Release" / "pxpl.exe"
            self.gui_exe_path = self.project_root / "build" / "Release" / "pxpl-gui.exe"

    def main(self):
        """Main entry point"""
        parser = argparse.ArgumentParser(description="pxpl Steganography Tool Test Suite")
        parser.add_argument("command",
                            nargs="?",
                            choices=["test", "cleanup", "compile", "build", "demo"],
                            help="Command to execute")

        args = parser.parse_args()

        if args.command == "test":
            return 0 if self.run_comprehensive_test() else 1
        elif args.command == "cleanup":
            self.cleanup_files()
            return 0
        elif args.command == "compile":
            return 0 if self.compile_tool() else 1
        elif args.command == "build":
            return 0 if self.build_both_versions() else 1
        elif args.command == "demo":
            return 0 if self.create_demo_image() else 1
        else:
            self.show_usage()
            return 0

    def show_usage(self):
        """Show usage information"""
        print("pxpl Steganography Tool Test Suite")
        print()
        print("Usage:")
        print("  python pxpl_test.py test     - Run full test")
        print("  python pxpl_test.py build    - Build both CLI and GUI versions")
        print("  python pxpl_test.py compile  - Compile the CLI pxpl tool only")
        print("  python pxpl_test.py cleanup  - Clean up test files only")
        print("  python pxpl_test.py demo     - Create a demo cover image")
        print()
        print("Manual testing:")
        print("  1. Ensure pxpl.exe exists in builds directory")
        print("  2. Create sample images and payloads")
        print("  3. Test hide and reveal operations")
        print()
        print("Usage examples:")
        print("  ../builds/pxpl.exe embed sample_large.png payload.txt demo_output.png")
        print("  ../builds/pxpl.exe extract demo_output.png extracted_payload.txt")
        print()
        print("To run comprehensive workflow:")
        print("  python pxpl_test.py test")

    def run_comprehensive_test(self):
        """Run comprehensive test suite"""
        print("=== pxpl STEGANOGRAPHY TOOL COMPREHENSIVE TEST ===")
        print("This program will:")
        print("1. Build both CLI and GUI versions of the tool")
        print("2. Create sample PNG images of various sizes and types")
        print("3. Create test payloads of different sizes")
        print("4. Test hide and reveal operations")
        print("5. Verify extracted content matches original")
        print("6. Clean up all test files")
        print()

        # Ensure we're in the right directory
        if not (self.project_root / "src").exists():
            print(f"Error: Project source directory not found at {self.project_root / 'src'}")
            print("This program should be run from the tests directory of the pxpl project")
            return False

        # Step 1: Check if tools exist or build them
        if not self.exe_path.exists() or not self.gui_exe_path.exists():
            if not self.build_both_versions():
                print("Failed to build tools. Exiting.")
                return False
        else:
            print("âœ“ Both CLI and GUI executables already exist")

        # Step 2: Create sample images
        print("\n=== Creating Sample Images ===")
        self.create_sample_png("sample_small.png", 50, 50, "RGB")
        self.create_sample_png("sample_medium.png", 150, 150, "RGB")
        self.create_grayscale_png("sample_grayscale.png", 100, 100)
        self.create_sample_png("sample_rgba.png", 100, 100, "RGBA")
        self.create_larger_sample("sample_large.png", 300, 300, "RGB")
        self.create_larger_sample("sample_xlarge.png", 500, 400, "RGB")

        # Step 3: Create test payloads
        print("\n=== Creating Test Payloads ===")
        total_payload_size = self.create_text_payloads()
        print(f"Total payload data: {total_payload_size} bytes")

        # Step 4: Run tests
        print("\n=== Running Steganography Tests ===")

        test_cases = [
            ("sample_small.png", "small_payload.txt", "Small Test"),
            ("sample_medium.png", "medium_payload.txt", "Medium Test"),
            ("sample_grayscale.png", "small_payload.txt", "Grayscale Test"),
            ("sample_rgba.png", "medium_payload.txt", "RGBA Test"),
            ("sample_large.png", "large_payload.txt", "Large Test"),
            ("sample_xlarge.png", "large_payload.txt", "Capacity Test")
        ]

        # Add specific tests for recent fixes
        print("\n=== Running Enhanced Feature Tests ===")

        enhanced_test_cases = [
            ("sample_rgba.png", "small_payload.txt", "RGBA Alpha Preservation Test",
             "Tests RGBA PNG transparency preservation during steganography"),
            ("sample_rgba.png", "medium_payload.txt", "RGBA LSB Integrity Test",
             "Tests that RGBA images don't suffer from LSB corruption bug"),
            ("sample_medium.png", "large_payload.txt", "PNG Lossless Encoding Test",
             "Tests enhanced PNG encoder settings for LSB preservation")
        ]

        print(f"Running {len(enhanced_test_cases)} enhanced feature tests...")

        passed_tests = 0
        total_tests = len(test_cases)

        for image, payload, test_name, description in enhanced_test_cases:
            print(f"\n--- {test_name} ---")
            print(f"Description: {description}")

            if self.test_steganography_with_alpha_check(image, payload, test_name):
                passed_tests += 1
            total_tests += 1

        for image, payload, test_name in test_cases:
            if self.test_steganography(image, payload, test_name):
                passed_tests += 1

        # Step 7: Results summary
        print("\n=== Test Results Summary ===")
        print(f"Passed: {passed_tests}/{total_tests} tests")

        if passed_tests == total_tests:
            print("âœ“ All tests passed! The pxpl tool is working correctly.")
            success = True
        else:
            print("âœ— Some tests failed. Please check the output above for details.")
            success = False

        # Step 6: Cleanup
        print("\n=== Cleanup ===")
        self.cleanup_files()

        return success

    def build_both_versions(self):
        """Build both CLI and GUI versions"""
        print("=== Building Tools ===")
        print("Building both CLI and GUI versions using CMake...")
        return self.run_cmake_build("-DBUILD_BOTH=ON")

    def compile_tool(self):
        """Compile CLI tool only"""
        print("Compiling pxpl tool using CMake...")
        return self.run_cmake_build("")

    def create_sample_png(self, filename, width, height, mode):
        """Create a sample PNG image with checkerboard pattern"""
        try:
            img = Image.new(mode, (width, height))
            pixels = img.load()

            # Create a simple checkerboard pattern
            for y in range(height):
                for x in range(width):
                    # Create a simple checkerboard pattern
                    if (x // 10 + y // 10) % 2 == 0:
                        # Light squares
                        r, g, b = 150, 200, 255
                        a = 255 if mode == "RGBA" else None
                    else:
                        # Dark squares
                        r, g, b = 25, 50, 100
                        a = 255 if mode == "RGBA" else None

                    # Add some gradient effect
                    b = max(0, min(255, b - (y * 2)))
                    g = max(0, min(255, g + (x * 1)))

                    if mode == "RGBA":
                        pixels[x, y] = (r, g, b, a)
                    else:
                        pixels[x, y] = (r, g, b)

            img.save(filename, "PNG")
            file_size = Path(filename).stat().st_size
            print(f"Created {filename}: {width}x{height} {mode} PNG ({file_size} bytes)")

        except (OSError, ValueError) as ex:
            print(f"Error creating {filename}: {ex}")

    def create_grayscale_png(self, filename, width, height):
        """Create a grayscale PNG image"""
        try:
            img = Image.new("RGB", (width, height))
            pixels = img.load()

            # Create a simple checkerboard pattern in grayscale
            for y in range(height):
                for x in range(width):
                    if (x // 10 + y // 10) % 2 == 0:
                        # Light squares
                        gray_value = 200
                    else:
                        # Dark squares
                        gray_value = 50

                    # Add some gradient effect
                    gray_value = max(0, min(255, gray_value - y + x))
                    pixels[x, y] = (gray_value, gray_value, gray_value)

            img.save(filename, "PNG")
            file_size = Path(filename).stat().st_size
            print(f"Created {filename}: {width}x{height} Grayscale PNG ({file_size} bytes)")

        except (OSError, ValueError) as ex:
            print(f"Error creating {filename}: {ex}")

    def create_larger_sample(self, filename, width, height, mode):
        """Create a larger sample image with complex patterns"""
        try:
            img = Image.new(mode, (width, height))
            pixels = img.load()

            center_x = width // 2
            center_y = height // 2

            for y in range(height):
                for x in range(width):
                    # More complex pattern with circles and gradients
                    distance = math.sqrt((x - center_x) ** 2 + (y - center_y) ** 2)

                    if int(distance) % 20 < 10:
                        red = int(255 * (1 - distance / (width * 0.7)))
                        green = int(200 * (distance / (height * 0.7)))
                        blue = int(150 + 105 * abs(x - y) / max(width, height))

                        red = max(0, min(255, red))
                        green = max(0, min(255, green))
                        blue = max(0, min(255, blue))

                        if mode == "RGBA":
                            pixels[x, y] = (red, green, blue, 255)
                        else:
                            pixels[x, y] = (red, green, blue)
                    else:
                        red = int(100 + 155 * (x / width))
                        green = int(50 + 200 * (y / height))
                        blue = int(25 + 100 * ((x + y) / (width + height)))

                        red = max(0, min(255, red))
                        green = max(0, min(255, green))
                        blue = max(0, min(255, blue))

                        if mode == "RGBA":
                            pixels[x, y] = (red, green, blue, 255)
                        else:
                            pixels[x, y] = (red, green, blue)

            img.save(filename, "PNG")
            file_size = Path(filename).stat().st_size
            print(f"Created {filename}: {width}x{height} {mode} PNG ({file_size} bytes)")

        except (OSError, ValueError) as ex:
            print(f"Error creating {filename}: {ex}")

    def create_text_payloads(self):
        """Create various text payloads for testing"""
        payloads = {
            "small_payload.txt": "Hi mom",
            "medium_payload.txt": """This is a medium-sized payload for testing.
It contains multiple lines of text and various characters.
Special characters: !@#$%^&*()_+-={}[]|\\:"";'<>?,.
Numbers: 1234567890""",
            "large_payload.txt": """This is a large payload for testing the steganography tool.
It contains multiple paragraphs, various characters, and a significant amount of text data.

His palms are sweaty, knees weak, arms are heavy
There's vomit on his sweater already, mom's spaghetti

Special characters and symbols:
!@#$%^&*()_+-={}[]|\\:"";'<>?,.
1234567890 ABCDEFGHIJKLMNOPQRSTUVWXYZ abcdefghijklmnopqrstuvwxyz

This payload should provide a comprehensive test of the steganography
embedding and extraction capabilities across different image sizes and
content types. The goal is to ensure that the tool works reliably
with various amounts of data and different character sets.

End of large payload - testing complete!""",
            "binary_payload.txt": "".join(chr(i % 256) for i in range(500))
        }

        total_size = 0
        for filename, content in payloads.items():
            try:
                with open(filename, 'w', encoding='utf-8') as f:
                    f.write(content)
                size = len(content.encode('utf-8'))
                total_size += size
                print(f"Created {filename}: {len(content)} characters ({size} bytes)")
            except (OSError, UnicodeError) as ex:
                print(f"Error creating {filename}: {ex}")

        return total_size

    def test_steganography_with_alpha_check(self, image_file, payload_file, test_name):
        """Test steganography with alpha channel preservation checks"""
        print(f"\n--- Testing {test_name} ---")

        # Sanitize test name for filename
        safe_name = test_name.lower().replace(" ", "_").replace("rgba", "alpha")
        output_file = f"demo_output_{safe_name}.png"
        extracted_file = f"demo_extracted_{safe_name}.txt"

        # Check if the executable exists
        if not self.exe_path.exists():
            print(f"âœ— Executable not found: {self.exe_path}")
            return False

        # For RGBA tests, check if the original image has alpha channel
        original_has_alpha = False
        try:
            with Image.open(image_file) as img:
                original_has_alpha = img.mode in ("RGBA", "LA")
                print(f"Original image alpha channel: {'Present' if original_has_alpha else 'Not present'}")
        except (OSError, ValueError) as ex:
            print(f"Warning: Could not analyze original image format: {ex}")

        # Test hide operation
        print(f"Hiding payload in {image_file}...")
        if not self.run_steganography_command("embed", image_file, payload_file, output_file):
            print("âœ— Hide operation failed")
            return False

        if not Path(output_file).exists():
            print("âœ— Output file was not created")
            return False

        output_file_size = Path(output_file).stat().st_size
        print(f"âœ“ Hide operation successful - Output file: {output_file} ({output_file_size} bytes)")

        # For RGBA tests, verify alpha channel preservation
        if original_has_alpha:
            try:
                with Image.open(output_file) as output_img:
                    output_has_alpha = output_img.mode in ("RGBA", "LA")
                    if output_has_alpha:
                        print("âœ“ Alpha channel preserved in output image")
                    else:
                        print("âœ— Alpha channel lost in output image")
                        return False
            except (OSError, ValueError) as ex:
                print(f"Warning: Could not verify alpha channel preservation: {ex}")

        # Test reveal operation
        print(f"Revealing payload from {output_file}...")
        if not self.run_steganography_command("extract", output_file, extracted_file):
            print("âœ— Reveal operation failed")
            return False

        if not Path(extracted_file).exists():
            print("âœ— Extracted file was not created")
            return False

        extracted_file_size = Path(extracted_file).stat().st_size
        print(f"âœ“ Reveal operation successful - Extracted file: {extracted_file} ({extracted_file_size} bytes)")

        # Verify content
        try:
            with open(payload_file, 'r', encoding='utf-8') as f:
                original = f.read()
            with open(extracted_file, 'r', encoding='utf-8') as f:
                extracted = f.read()

            if original == extracted:
                print("âœ“ Content verification successful - extracted content matches original")
                print("âœ“ LSB integrity verified - no corruption detected")
                return True
            else:
                print("âœ— Content verification failed - extracted content differs from original")
                print(f"  Original length: {len(original)}")
                print(f"  Extracted length: {len(extracted)}")

                # Show first difference
                for i in range(min(len(original), len(extracted))):
                    if original[i] != extracted[i]:
                        print(f"  First difference at position {i}: original='{original[i]}' extracted='{extracted[i]}'")
                        break
                return False
        except (OSError, UnicodeError) as ex:
            print(f"âœ— Error during content verification: {ex}")
            return False

    def test_steganography(self, image_file, payload_file, test_name):
        """Test steganography operations"""
        print(f"\n--- Testing {test_name} ---")

        safe_name = test_name.lower().replace(" ", "_")
        output_file = f"demo_output_{safe_name}.png"
        extracted_file = f"demo_extracted_{safe_name}.txt"

        # Check if the executable exists
        if not self.exe_path.exists():
            print(f"âœ— Executable not found: {self.exe_path}")
            return False

        # Test hide operation
        print(f"Hiding payload in {image_file}...")
        if not self.run_steganography_command("embed", image_file, payload_file, output_file):
            print("âœ— Hide operation failed")
            return False

        if not Path(output_file).exists():
            print("âœ— Output file was not created")
            return False

        output_file_size = Path(output_file).stat().st_size
        print(f"âœ“ Hide operation successful - Output file: {output_file} ({output_file_size} bytes)")

        # Test reveal operation
        print(f"Revealing payload from {output_file}...")
        if not self.run_steganography_command("extract", output_file, extracted_file):
            print("âœ— Reveal operation failed")
            return False

        if not Path(extracted_file).exists():
            print("âœ— Extracted file was not created")
            return False

        extracted_file_size = Path(extracted_file).stat().st_size
        print(f"âœ“ Reveal operation successful - Extracted file: {extracted_file} ({extracted_file_size} bytes)")

        # Verify content
        try:
            with open(payload_file, 'r', encoding='utf-8') as f:
                original = f.read()
            with open(extracted_file, 'r', encoding='utf-8') as f:
                extracted = f.read()

            if original == extracted:
                print("âœ“ Content verification successful - extracted content matches original")
                return True
            else:
                print("âœ— Content verification failed - extracted content differs from original")
                print(f"  Original length: {len(original)}")
                print(f"  Extracted length: {len(extracted)}")

                # Show first difference
                for i in range(min(len(original), len(extracted))):
                    if original[i] != extracted[i]:
                        print(f"  First difference at position {i}: original='{original[i]}' extracted='{extracted[i]}'")
                        break
                return False
        except (OSError, UnicodeError) as ex:
            print(f"âœ— Error during content verification: {ex}")
            return False

    def run_steganography_command(self, operation, arg1, arg2, arg3=None):
        """Run a steganography command"""
        try:
            cmd = [str(self.exe_path), operation, arg1, arg2]
            if arg3:
                cmd.append(arg3)

            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                timeout=30,
                check=False
            )

            print(f"Command: {' '.join(cmd)}")
            print(f"Return code: {result.returncode}")

            # Add specific error messages based on return codes from the tool
            if result.returncode != 0:
                error_messages = {
                    1: "Incorrect arguments",
                    2: "Unsupported or corrupt image",
                    3: "Cover image too small",
                    4: "I/O error",
                    5: "PNG error"
                }
                error_msg = error_messages.get(result.returncode, f"Unknown error code {result.returncode}")
                print(f"Error: {error_msg}")

            if result.stdout:
                print(f"STDOUT: {result.stdout}")

            if result.stderr:
                print(f"STDERR: {result.stderr}")

            return result.returncode == 0

        except (subprocess.SubprocessError, OSError) as ex:
            print(f"Error running steganography command: {ex}")
            return False

    def cleanup_files(self):
        """Clean up test files"""
        files_to_remove = [
            # Sample images
            "sample_small.png",
            "sample_medium.png",
            "sample_grayscale.png",
            "sample_rgba.png",
            "sample_large.png",
            "sample_xlarge.png",

            # Payload files
            "small_payload.txt",
            "medium_payload.txt",
            "large_payload.txt",
            "binary_payload.txt",

            # Demo files
            "demo_output_small_test.png",
            "demo_extracted_small_test.txt",
            "demo_output_medium_test.png",
            "demo_extracted_medium_test.txt",
            "demo_output_grayscale_test.png",
            "demo_extracted_grayscale_test.txt",
            "demo_output_rgba_test.png",
            "demo_extracted_rgba_test.txt",
            "demo_output_large_test.png",
            "demo_extracted_large_test.txt",
            "demo_output_capacity_test.png",
            "demo_extracted_capacity_test.txt",

            # Legacy files
            "extracted_text.txt",
            "test_output.png"
        ]

        # Also clean up any files matching demo_* pattern
        try:
            for file in Path(".").glob("demo_*"):
                files_to_remove.append(str(file))
        except OSError:
            pass

        removed_count = 0
        for filename in files_to_remove:
            if Path(filename).exists():
                try:
                    Path(filename).unlink()
                    print(f"Removed {filename}")
                    removed_count += 1
                except OSError as ex:
                    print(f"Could not remove {filename}: {ex}")

        if removed_count > 0:
            print(f"\nCleanup complete: {removed_count} files removed")
        else:
            print("\nNo files to clean up")

    def create_demo_image(self):
        """Create a demo cover image"""
        try:
            print("Creating demo cover image...")

            # Create a simple demo image
            img = Image.new("RGB", (200, 200), color="lightblue")
            draw = ImageDraw.Draw(img)

            # Add some visual elements
            draw.ellipse([50, 50, 150, 150], fill="darkblue")
            draw.rectangle([25, 25, 175, 175], outline="red", width=3)

            # Save as PNG in project root directory
            demo_path = self.project_root / "demo_cover.png"
            img.save(str(demo_path), "PNG")
            print(f"Created {demo_path} (200x200 RGB)")

            return True
        except (OSError, ValueError) as ex:
            print(f"Error creating demo image: {ex}")
            return False

    def run_cmake_build(self, cmake_args):
        """Run CMake build process"""
        try:
            # Check if build directory exists, create if not
            build_dir = self.project_root / "build"
            if not build_dir.exists():
                build_dir.mkdir()

            # First, configure with CMake
            configure_cmd = ["cmake", "-DCMAKE_BUILD_TYPE=Release"]
            if cmake_args:
                configure_cmd.extend(cmake_args.split())
            configure_cmd.append(str(self.project_root))

            print(f"Configure command: {' '.join(configure_cmd)}")

            result = subprocess.run(
                configure_cmd,
                cwd=build_dir,
                capture_output=True,
                text=True,
                check=False
            )

            if result.returncode != 0:
                print(f"CMake configure failed with exit code {result.returncode}")
                if result.stderr:
                    print(f"Error: {result.stderr}")
                return False

            if result.stdout:
                print(f"Configure output: {result.stdout}")

            # Then, build with CMake
            build_cmd = ["cmake", "--build", ".", "--config", "Release"]
            print(f"Build command: {' '.join(build_cmd)}")

            result = subprocess.run(
                build_cmd,
                cwd=build_dir,
                capture_output=True,
                text=True,
                check=False
            )

            if result.returncode != 0:
                print(f"CMake build failed with exit code {result.returncode}")
                if result.stderr:
                    print(f"Error: {result.stderr}")
                return False

            if result.stdout:
                print(f"Build output: {result.stdout}")

            # Verify executables exist
            success = True
            if "BUILD_BOTH" in cmake_args or not cmake_args:
                if not self.exe_path.exists():
                    print(f"CLI executable not found: {self.exe_path}")
                    success = False
                else:
                    print(f"âœ“ CLI executable found: {self.exe_path}")

                if "BUILD_BOTH" in cmake_args and not self.gui_exe_path.exists():
                    print(f"GUI executable not found: {self.gui_exe_path}")
                    success = False
                elif "BUILD_BOTH" in cmake_args:
                    print(f"âœ“ GUI executable found: {self.gui_exe_path}")

            if success:
                print("âœ“ CMake build completed successfully")

            return success

        except (subprocess.SubprocessError, OSError) as ex:
            print(f"CMake build failed: {ex}")
            return False


def main():
    """Main function"""
    test = PxplTest()
    return test.main()


if __name__ == "__main__":
    sys.exit(main())
