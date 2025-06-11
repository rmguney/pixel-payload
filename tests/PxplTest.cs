/*
 * pxpl - PNG LSB Steganography Tool Test Suite
 * 
 * Comprehensive C# test program to create sample PNG images and test payloads
 * for steganography testing. Creates various sized images and payloads, tests
 * the steganography tool, and cleans up.
 */

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Drawing;
using System.Drawing.Imaging;
using System.IO;
using System.Linq;
using System.Text;

namespace PxplTest
{
    class Program
    {        // Use CMake release build directory
        private static readonly string ExePath = @"..\release\pxpl.exe";
        private static readonly string GuiExePath = @"..\release\pxpl-gui.exe";

        static int Main(string[] args)
        {
            Console.WriteLine("Starting pxpl test suite...");
              if (args.Length > 0)
            {
                string command = args[0].ToLower();
                switch (command)
                {
                    case "test":
                        return RunComprehensiveTest() ? 0 : 1;
                    case "cleanup":
                        CleanupFiles();
                        return 0;
                    case "compile":
                        return CompileTool() ? 0 : 1;
                    case "build":
                        return BuildBothVersions() ? 0 : 1;
                    case "demo":
                        return CreateDemoImage() ? 0 : 1;
                    default:
                        ShowUsage();
                        return 0;
                }
            }

            ShowUsage();
            return 0;
        }        static void ShowUsage()
        {
            Console.WriteLine("pxpl Steganography Tool Test Suite");
            Console.WriteLine();
            Console.WriteLine("Usage:");
            Console.WriteLine("  dotnet run test     - Run comprehensive workflow (compile + test + cleanup)");
            Console.WriteLine("  dotnet run build    - Build both CLI and GUI versions");
            Console.WriteLine("  dotnet run compile  - Compile the CLI pxpl tool only");
            Console.WriteLine("  dotnet run cleanup  - Clean up test files only");
            Console.WriteLine("  dotnet run demo     - Create a demo cover image");
            Console.WriteLine();
            Console.WriteLine("Manual testing:");
            Console.WriteLine("  1. Ensure pxpl.exe exists in builds directory");
            Console.WriteLine("  2. Create sample images and payloads");
            Console.WriteLine("  3. Test hide and reveal operations");
            Console.WriteLine();
            Console.WriteLine("Usage examples:");
            Console.WriteLine("  ..\\builds\\pxpl.exe embed sample_large.png payload.txt demo_output.png");
            Console.WriteLine("  ..\\builds\\pxpl.exe extract demo_output.png extracted_payload.txt");
            Console.WriteLine();
            Console.WriteLine("To run comprehensive workflow:");
            Console.WriteLine("  dotnet run test");
        }

        static bool RunComprehensiveTest()
        {
            Console.WriteLine("=== pxpl STEGANOGRAPHY TOOL COMPREHENSIVE TEST ===");
            Console.WriteLine("This program will:");
            Console.WriteLine("1. Build both CLI and GUI versions of the tool");
            Console.WriteLine("2. Create sample PNG images of various sizes and types");
            Console.WriteLine("3. Create test payloads of different sizes");
            Console.WriteLine("4. Test hide and reveal operations");
            Console.WriteLine("5. Verify extracted content matches original");
            Console.WriteLine("6. Clean up all test files");
            Console.WriteLine();

            // Ensure we're in the right directory
            if (!Directory.Exists("../src"))
            {
                Console.WriteLine("Error: This program should be run from the tests directory");
                return false;
            }            // Step 1: Check if tools exist or build them
            if (!File.Exists(ExePath) || !File.Exists(GuiExePath))
            {
                if (!BuildBothVersions())
                {
                    Console.WriteLine("Failed to build tools. Exiting.");
                    return false;
                }
            }
            else
            {
                Console.WriteLine("✓ Both CLI and GUI executables already exist");
            }

            // Step 2: Create sample images
            Console.WriteLine("\n=== Creating Sample Images ===");            CreateSamplePng("sample_small.png", 50, 50, PixelFormat.Format24bppRgb);
            CreateSamplePng("sample_medium.png", 150, 150, PixelFormat.Format24bppRgb);
            CreateGrayscalePng("sample_grayscale.png", 100, 100);
            CreateSamplePng("sample_rgba.png", 100, 100, PixelFormat.Format32bppArgb);
            CreateLargerSample("sample_large.png", 300, 300, PixelFormat.Format24bppRgb);
            CreateLargerSample("sample_xlarge.png", 500, 400, PixelFormat.Format24bppRgb);

            // Step 3: Create test payloads
            Console.WriteLine("\n=== Creating Test Payloads ===");
            int totalPayloadSize = CreateTextPayloads();
            Console.WriteLine($"Total payload data: {totalPayloadSize} bytes");

            // Step 4: Run tests
            Console.WriteLine("\n=== Running Steganography Tests ===");

            var testCases = new List<(string image, string payload, string testName)>
            {
                ("sample_small.png", "small_payload.txt", "Small Test"),
                ("sample_medium.png", "medium_payload.txt", "Medium Test"),
                ("sample_grayscale.png", "small_payload.txt", "Grayscale Test"),
                ("sample_rgba.png", "medium_payload.txt", "RGBA Test"),
                ("sample_large.png", "large_payload.txt", "Large Test"),
                ("sample_xlarge.png", "large_payload.txt", "Capacity Test")
            };

            // Add specific tests for recent fixes
            Console.WriteLine("\n=== Running Enhanced Feature Tests ===");
            
            var enhancedTestCases = new List<(string image, string payload, string testName, string description)>
            {
                ("sample_rgba.png", "small_payload.txt", "RGBA Alpha Preservation Test", 
                 "Tests RGBA PNG transparency preservation during steganography"),
                ("sample_rgba.png", "medium_payload.txt", "RGBA LSB Integrity Test", 
                 "Tests that RGBA images don't suffer from LSB corruption bug"),
                ("sample_medium.png", "large_payload.txt", "PNG Lossless Encoding Test", 
                 "Tests enhanced PNG encoder settings for LSB preservation")
            };            Console.WriteLine($"Running {enhancedTestCases.Count} enhanced feature tests...");
            
            int passedTests = 0;
            int totalTests = testCases.Count;
            
            foreach (var (image, payload, testName, description) in enhancedTestCases)
            {
                Console.WriteLine($"\n--- {testName} ---");
                Console.WriteLine($"Description: {description}");
                
                if (TestSteganographyWithAlphaCheck(image, payload, testName))
                {
                    passedTests++;
                }
                totalTests++;
            }

            foreach (var (image, payload, testName) in testCases)
            {
                if (TestSteganography(image, payload, testName))
                {
                    passedTests++;
                }
            }

            // Step 7: Results summary
            Console.WriteLine("\n=== Test Results Summary ===");
            Console.WriteLine($"Passed: {passedTests}/{totalTests} tests");

            bool success;
            if (passedTests == totalTests)
            {
                Console.WriteLine("✓ All tests passed! The pxpl tool is working correctly.");
                success = true;
            }
            else
            {
                Console.WriteLine("✗ Some tests failed. Please check the output above for details.");
                success = false;
            }

            // Step 6: Cleanup
            Console.WriteLine("\n=== Cleanup ===");
            CleanupFiles();

            return success;
        }        static bool BuildBothVersions()
        {
            Console.WriteLine("=== Building Tools ===");
            Console.WriteLine("Building both CLI and GUI versions using CMake...");
            return RunCMakeBuild("-DBUILD_BOTH=ON");
        }static bool CompileTool()
        {
            Console.WriteLine("Compiling pxpl tool using CMake...");
            return RunCMakeBuild("");
        }

        static bool RunPowerShellScript(string scriptPath)
        {
            try
            {                var psi = new ProcessStartInfo
                {
                    FileName = "powershell.exe",
                    Arguments = $"-ExecutionPolicy Bypass -File \"{scriptPath}\"",
                    WorkingDirectory = Path.GetDirectoryName(Directory.GetCurrentDirectory()), // Parent directory
                    UseShellExecute = false,
                    RedirectStandardOutput = true,
                    RedirectStandardError = true,
                    CreateNoWindow = true
                };using (var process = Process.Start(psi))
                {
                    if (process == null) return false;
                    
                    string output = process.StandardOutput.ReadToEnd();
                    string error = process.StandardError.ReadToEnd();
                    
                    process.WaitForExit();

                    if (!string.IsNullOrWhiteSpace(output))
                        Console.WriteLine(output);
                    
                    if (!string.IsNullOrWhiteSpace(error))
                        Console.WriteLine($"Error: {error}");

                    return process.ExitCode == 0;
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine($"Error running PowerShell script: {ex.Message}");
                return false;
            }
        }

        static void CreateSamplePng(string filename, int width, int height, PixelFormat format)
        {
            try
            {
                using (var bitmap = new Bitmap(width, height, format))
                {
                    using (var graphics = Graphics.FromImage(bitmap))
                    {
                        // Create a simple checkerboard pattern
                        for (int y = 0; y < height; y++)
                        {
                            for (int x = 0; x < width; x++)
                            {
                                Color color;
                                if ((x / 10 + y / 10) % 2 == 0)
                                {
                                    // Light squares
                                    color = Color.FromArgb(255, 150, 200, 255);
                                }
                                else
                                {
                                    // Dark squares
                                    color = Color.FromArgb(255, 25, 50, 100);
                                }

                                // Add some gradient effect
                                int blue = Math.Max(0, Math.Min(255, color.B - (y * 2)));
                                int green = Math.Max(0, Math.Min(255, color.G + (x * 1)));
                                
                                color = Color.FromArgb(color.A, color.R, green, blue);
                                bitmap.SetPixel(x, y, color);
                            }
                        }
                    }

                    bitmap.Save(filename, ImageFormat.Png);
                }

                var fileInfo = new FileInfo(filename);
                Console.WriteLine($"Created {filename}: {width}x{height} {format} PNG ({fileInfo.Length} bytes)");
            }
            catch (Exception ex)
            {
                Console.WriteLine($"Error creating {filename}: {ex.Message}");
            }
        }

        static void CreateGrayscalePng(string filename, int width, int height)
        {
            try
            {
                using (var bitmap = new Bitmap(width, height, PixelFormat.Format24bppRgb))
                {
                    // Create a simple checkerboard pattern in grayscale
                    for (int y = 0; y < height; y++)
                    {
                        for (int x = 0; x < width; x++)
                        {
                            int grayValue;
                            if ((x / 10 + y / 10) % 2 == 0)
                            {
                                // Light squares
                                grayValue = 200;
                            }
                            else
                            {
                                // Dark squares
                                grayValue = 50;
                            }

                            // Add some gradient effect
                            grayValue = Math.Max(0, Math.Min(255, grayValue - (y * 1) + (x * 1)));
                            
                            Color color = Color.FromArgb(255, grayValue, grayValue, grayValue);
                            bitmap.SetPixel(x, y, color);
                        }
                    }

                    bitmap.Save(filename, ImageFormat.Png);
                }

                var fileInfo = new FileInfo(filename);
                Console.WriteLine($"Created {filename}: {width}x{height} Grayscale PNG ({fileInfo.Length} bytes)");
            }
            catch (Exception ex)
            {
                Console.WriteLine($"Error creating {filename}: {ex.Message}");
            }
        }

        static void CreateLargerSample(string filename, int width, int height, PixelFormat format)
        {
            try
            {
                using (var bitmap = new Bitmap(width, height, format))
                {
                    using (var graphics = Graphics.FromImage(bitmap))
                    {
                        int centerX = width / 2;
                        int centerY = height / 2;

                        for (int y = 0; y < height; y++)
                        {
                            for (int x = 0; x < width; x++)
                            {
                                // More complex pattern with circles and gradients
                                double distance = Math.Sqrt((x - centerX) * (x - centerX) + (y - centerY) * (y - centerY));

                                Color color;
                                if ((int)distance % 20 < 10)
                                {
                                    int red = (int)(255 * (1 - distance / (width * 0.7)));
                                    int green = (int)(200 * (distance / (height * 0.7)));
                                    int blue = (int)(150 + 105 * Math.Abs(x - y) / Math.Max(width, height));
                                    
                                    red = Math.Max(0, Math.Min(255, red));
                                    green = Math.Max(0, Math.Min(255, green));
                                    blue = Math.Max(0, Math.Min(255, blue));
                                    
                                    color = Color.FromArgb(255, red, green, blue);
                                }
                                else
                                {
                                    int red = (int)(100 + 155 * (x / (double)width));
                                    int green = (int)(50 + 200 * (y / (double)height));
                                    int blue = (int)(25 + 100 * ((x + y) / (double)(width + height)));
                                    
                                    red = Math.Max(0, Math.Min(255, red));
                                    green = Math.Max(0, Math.Min(255, green));
                                    blue = Math.Max(0, Math.Min(255, blue));
                                    
                                    color = Color.FromArgb(255, red, green, blue);
                                }

                                bitmap.SetPixel(x, y, color);
                            }
                        }
                    }

                    bitmap.Save(filename, ImageFormat.Png);
                }

                var fileInfo = new FileInfo(filename);
                Console.WriteLine($"Created {filename}: {width}x{height} {format} PNG ({fileInfo.Length} bytes)");
            }
            catch (Exception ex)
            {
                Console.WriteLine($"Error creating {filename}: {ex.Message}");
            }
        }

        static int CreateTextPayloads()
        {
            var payloads = new Dictionary<string, string>
            {
                ["small_payload.txt"] = "Hi mom",
                ["medium_payload.txt"] = @"This is a medium-sized payload for testing.
It contains multiple lines of text and various characters.
Special characters: !@#$%^&*()_+-={}[]|\:"";'<>?,.
Numbers: 1234567890",
                ["large_payload.txt"] = @"This is a large payload for testing the steganography tool.
It contains multiple paragraphs, various characters, and a significant amount of text data.

His palms are sweaty, knees weak, arms are heavy
There's vomit on his sweater already, mom's spaghetti

Special characters and symbols:
!@#$%^&*()_+-={}[]|\:"";'<>?,.
1234567890 ABCDEFGHIJKLMNOPQRSTUVWXYZ abcdefghijklmnopqrstuvwxyz

This payload should provide a comprehensive test of the steganography
embedding and extraction capabilities across different image sizes and
content types. The goal is to ensure that the tool works reliably
with various amounts of data and different character sets.

End of large payload - testing complete!",
                ["binary_payload.txt"] = string.Join("", Enumerable.Range(0, 500).Select(i => (char)(i % 256)))
            };

            int totalSize = 0;
            foreach (var kvp in payloads)
            {
                try
                {
                    File.WriteAllText(kvp.Key, kvp.Value, Encoding.UTF8);
                    int size = Encoding.UTF8.GetByteCount(kvp.Value);
                    totalSize += size;
                    Console.WriteLine($"Created {kvp.Key}: {kvp.Value.Length} characters ({size} bytes)");
                }
                catch (Exception ex)
                {
                    Console.WriteLine($"Error creating {kvp.Key}: {ex.Message}");
                }
            }

            return totalSize;
        }

        static bool TestSteganographyWithAlphaCheck(string imageFile, string payloadFile, string testName)
        {
            Console.WriteLine($"\n--- Testing {testName} ---");            string outputFile = $"demo_output_{testName.ToLower().Replace(" ", "_").Replace("rgba", "alpha").Replace("ı", "i").Replace("ü", "u").Replace("ö", "o").Replace("ş", "s").Replace("ğ", "g").Replace("ç", "c")}.png";
            string extractedFile = $"demo_extracted_{testName.ToLower().Replace(" ", "_").Replace("rgba", "alpha").Replace("ı", "i").Replace("ü", "u").Replace("ö", "o").Replace("ş", "s").Replace("ğ", "g").Replace("ç", "c")}.txt";

            // Check if the executable exists
            if (!File.Exists(ExePath))
            {
                Console.WriteLine($"✗ Executable not found: {ExePath}");
                return false;
            }

            // For RGBA tests, check if the original image has alpha channel
            bool originalHasAlpha = false;
            try
            {
                using (var img = Image.FromFile(imageFile))
                {
                    originalHasAlpha = Image.IsAlphaPixelFormat(img.PixelFormat);                    Console.WriteLine($"Original image alpha channel: {(originalHasAlpha ? "Present" : "Not present")}");
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine($"Warning: Could not analyze original image format: {ex.Message}");
            }

            // Test hide operation
            Console.WriteLine($"Hiding payload in {imageFile}...");
            if (!RunSteganographyCommand("embed", imageFile, payloadFile, outputFile))
            {
                Console.WriteLine("✗ Hide operation failed");
                return false;
            }

            if (!File.Exists(outputFile))
            {
                Console.WriteLine("✗ Output file was not created");
                return false;
            }

            var outputFileInfo = new FileInfo(outputFile);
            Console.WriteLine($"✓ Hide operation successful - Output file: {outputFile} ({outputFileInfo.Length} bytes)");

            // For RGBA tests, verify alpha channel preservation
            if (originalHasAlpha)
            {
                try
                {
                    using (var outputImg = Image.FromFile(outputFile))
                    {
                        bool outputHasAlpha = Image.IsAlphaPixelFormat(outputImg.PixelFormat);
                        if (outputHasAlpha)
                        {
                            Console.WriteLine("✓ Alpha channel preserved in output image");
                        }
                        else
                        {
                            Console.WriteLine("✗ Alpha channel lost in output image");
                            return false;
                        }
                    }
                }
                catch (Exception ex)
                {
                    Console.WriteLine($"Warning: Could not verify alpha channel preservation: {ex.Message}");
                }
            }

            // Test reveal operation
            Console.WriteLine($"Revealing payload from {outputFile}...");
            if (!RunSteganographyCommand("extract", outputFile, extractedFile))
            {
                Console.WriteLine("✗ Reveal operation failed");
                return false;
            }

            if (!File.Exists(extractedFile))
            {
                Console.WriteLine("✗ Extracted file was not created");
                return false;
            }

            var extractedFileInfo = new FileInfo(extractedFile);
            Console.WriteLine($"✓ Reveal operation successful - Extracted file: {extractedFile} ({extractedFileInfo.Length} bytes)");

            // Verify content
            try
            {
                string original = File.ReadAllText(payloadFile, Encoding.UTF8);
                string extracted = File.ReadAllText(extractedFile, Encoding.UTF8);

                if (original == extracted)
                {
                    Console.WriteLine("✓ Content verification successful - extracted content matches original");
                    Console.WriteLine("✓ LSB integrity verified - no corruption detected");
                    return true;
                }
                else
                {
                    Console.WriteLine("✗ Content verification failed - extracted content differs from original");
                    Console.WriteLine($"  Original length: {original.Length}");
                    Console.WriteLine($"  Extracted length: {extracted.Length}");

                    // Show first difference
                    for (int i = 0; i < Math.Min(original.Length, extracted.Length); i++)
                    {
                        if (original[i] != extracted[i])
                        {
                            Console.WriteLine($"  First difference at position {i}: original='{original[i]}' extracted='{extracted[i]}'");
                            break;
                        }
                    }
                    return false;
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine($"✗ Error during content verification: {ex.Message}");
                return false;
            }
        }

        static bool TestSteganography(string imageFile, string payloadFile, string testName)
        {
            Console.WriteLine($"\n--- Testing {testName} ---");

            string outputFile = $"demo_output_{testName.ToLower().Replace(" ", "_")}.png";
            string extractedFile = $"demo_extracted_{testName.ToLower().Replace(" ", "_")}.txt";

            // Check if the executable exists
            if (!File.Exists(ExePath))
            {
                Console.WriteLine($"✗ Executable not found: {ExePath}");
                return false;
            }

            // Test hide operation
            Console.WriteLine($"Hiding payload in {imageFile}...");
            if (!RunSteganographyCommand("embed", imageFile, payloadFile, outputFile))
            {
                Console.WriteLine("✗ Hide operation failed");
                return false;
            }

            if (!File.Exists(outputFile))
            {
                Console.WriteLine("✗ Output file was not created");
                return false;
            }

            var outputFileInfo = new FileInfo(outputFile);
            Console.WriteLine($"✓ Hide operation successful - Output file: {outputFile} ({outputFileInfo.Length} bytes)");

            // Test reveal operation
            Console.WriteLine($"Revealing payload from {outputFile}...");
            if (!RunSteganographyCommand("extract", outputFile, extractedFile))
            {
                Console.WriteLine("✗ Reveal operation failed");
                return false;
            }

            if (!File.Exists(extractedFile))
            {
                Console.WriteLine("✗ Extracted file was not created");
                return false;
            }

            var extractedFileInfo = new FileInfo(extractedFile);
            Console.WriteLine($"✓ Reveal operation successful - Extracted file: {extractedFile} ({extractedFileInfo.Length} bytes)");

            // Verify content
            try
            {
                string original = File.ReadAllText(payloadFile, Encoding.UTF8);
                string extracted = File.ReadAllText(extractedFile, Encoding.UTF8);

                if (original == extracted)
                {
                    Console.WriteLine("✓ Content verification successful - extracted content matches original");
                    return true;
                }
                else
                {
                    Console.WriteLine("✗ Content verification failed - extracted content differs from original");
                    Console.WriteLine($"  Original length: {original.Length}");
                    Console.WriteLine($"  Extracted length: {extracted.Length}");

                    // Show first difference
                    for (int i = 0; i < Math.Min(original.Length, extracted.Length); i++)
                    {
                        if (original[i] != extracted[i])
                        {
                            Console.WriteLine($"  First difference at position {i}: original='{original[i]}' extracted='{extracted[i]}'");
                            break;
                        }
                    }
                    return false;
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine($"✗ Error during content verification: {ex.Message}");
                return false;
            }
        }

        static bool RunSteganographyCommand(string operation, string arg1, string arg2, string? arg3 = null)
        {
            try
            {
                var psi = new ProcessStartInfo
                {
                    FileName = ExePath,
                    UseShellExecute = false,
                    RedirectStandardOutput = true,
                    RedirectStandardError = true,
                    CreateNoWindow = true
                };

                if (arg3 != null)
                {
                    psi.Arguments = $"{operation} \"{arg1}\" \"{arg2}\" \"{arg3}\"";
                }
                else
                {
                    psi.Arguments = $"{operation} \"{arg1}\" \"{arg2}\"";
                }                using (var process = Process.Start(psi))
                {
                    if (process == null) return false;
                    
                    string output = process.StandardOutput.ReadToEnd();
                    string error = process.StandardError.ReadToEnd();

                    process.WaitForExit(30000); // 30 second timeout

                    Console.WriteLine($"Command: {ExePath} {psi.Arguments}");
                    Console.WriteLine($"Return code: {process.ExitCode}");

                    if (!string.IsNullOrWhiteSpace(output))
                        Console.WriteLine($"STDOUT: {output}");
                    
                    if (!string.IsNullOrWhiteSpace(error))
                        Console.WriteLine($"STDERR: {error}");

                    return process.ExitCode == 0;
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine($"Error running steganography command: {ex.Message}");
                return false;
            }
        }

        static void CleanupFiles()
        {
            var filesToRemove = new List<string>
            {
                // Sample images
                "sample_small.png",
                "sample_medium.png",
                "sample_grayscale.png",
                "sample_rgba.png",
                "sample_large.png",
                "sample_xlarge.png",

                // Payload files
                "small_payload.txt",
                "medium_payload.txt",
                "large_payload.txt",
                "binary_payload.txt",

                // Demo files
                "demo_output_small_test.png",
                "demo_extracted_small_test.txt",
                "demo_output_medium_test.png",
                "demo_extracted_medium_test.txt",
                "demo_output_grayscale_test.png",
                "demo_extracted_grayscale_test.txt",
                "demo_output_rgba_test.png",
                "demo_extracted_rgba_test.png",
                "demo_output_large_test.png",
                "demo_extracted_large_test.txt",
                "demo_output_capacity_test.png",
                "demo_extracted_capacity_test.txt",

                // Legacy files
                "extracted_text.txt",
                "test_output.png"
            };

            // Also clean up any files matching demo_* pattern
            try
            {
                foreach (string file in Directory.GetFiles(".", "demo_*"))
                {
                    filesToRemove.Add(Path.GetFileName(file));
                }
            }
            catch { }

            int removedCount = 0;
            foreach (string filename in filesToRemove)
            {
                if (File.Exists(filename))
                {
                    try
                    {
                        File.Delete(filename);
                        Console.WriteLine($"Removed {filename}");
                        removedCount++;
                    }
                    catch (Exception ex)
                    {
                        Console.WriteLine($"Could not remove {filename}: {ex.Message}");
                    }
                }
            }            if (removedCount > 0)
            {
                Console.WriteLine($"\nCleanup complete: {removedCount} files removed");
            }
            else
            {
                Console.WriteLine("\nNo files to clean up");
            }
        }

        static bool CreateDemoImage()
        {
            try
            {
                Console.WriteLine("Creating demo cover image...");
                
                // Create a simple demo image
                using (var bitmap = new Bitmap(200, 200))
                {
                    using (var graphics = Graphics.FromImage(bitmap))
                    {
                        // Fill with a gradient background
                        graphics.Clear(Color.LightBlue);
                        
                        // Add some visual elements
                        using (var brush = new SolidBrush(Color.DarkBlue))
                        {
                            graphics.FillEllipse(brush, 50, 50, 100, 100);
                        }
                        
                        using (var pen = new Pen(Color.Red, 3))
                        {
                            graphics.DrawRectangle(pen, 25, 25, 150, 150);
                        }
                    }
                    
                    // Save as PNG in parent directory
                    bitmap.Save("../demo_cover.png", ImageFormat.Png);
                    Console.WriteLine("Created ../demo_cover.png (200x200 RGB)");
                }
                
                return true;
            }
            catch (Exception ex)
            {
                Console.WriteLine($"Error creating demo image: {ex.Message}");
                return false;
            }
        }

        static bool RunCMakeBuild(string cmakeArgs)
        {
            try
            {
                // Check if build directory exists, create if not
                string buildDir = "..\\build";
                if (!Directory.Exists(buildDir))
                {
                    Directory.CreateDirectory(buildDir);
                }

                // First, configure with CMake
                string configureCmd = $"cmake -DCMAKE_BUILD_TYPE=Release {cmakeArgs} ..";
                Console.WriteLine($"Configure command: {configureCmd}");
                
                ProcessStartInfo configureInfo = new ProcessStartInfo
                {
                    FileName = "cmake",
                    Arguments = $"-DCMAKE_BUILD_TYPE=Release {cmakeArgs} ..",
                    WorkingDirectory = buildDir,
                    UseShellExecute = false,
                    RedirectStandardOutput = true,
                    RedirectStandardError = true,
                    CreateNoWindow = true
                };

                using (Process process = Process.Start(configureInfo))
                {
                    process.WaitForExit();
                    string output = process.StandardOutput.ReadToEnd();
                    string error = process.StandardError.ReadToEnd();
                    
                    if (process.ExitCode != 0)
                    {
                        Console.WriteLine($"CMake configure failed with exit code {process.ExitCode}");
                        if (!string.IsNullOrEmpty(error))
                            Console.WriteLine($"Error: {error}");
                        return false;
                    }
                    
                    if (!string.IsNullOrEmpty(output))
                        Console.WriteLine($"Configure output: {output}");
                }

                // Then, build with CMake
                string buildCmd = "cmake --build . --config Release";
                Console.WriteLine($"Build command: {buildCmd}");
                
                ProcessStartInfo buildInfo = new ProcessStartInfo
                {
                    FileName = "cmake",
                    Arguments = "--build . --config Release",
                    WorkingDirectory = buildDir,
                    UseShellExecute = false,
                    RedirectStandardOutput = true,
                    RedirectStandardError = true,
                    CreateNoWindow = true
                };

                using (Process process = Process.Start(buildInfo))
                {
                    process.WaitForExit();
                    string output = process.StandardOutput.ReadToEnd();
                    string error = process.StandardError.ReadToEnd();
                    
                    if (process.ExitCode != 0)
                    {
                        Console.WriteLine($"CMake build failed with exit code {process.ExitCode}");
                        if (!string.IsNullOrEmpty(error))
                            Console.WriteLine($"Error: {error}");
                        return false;
                    }
                    
                    if (!string.IsNullOrEmpty(output))
                        Console.WriteLine($"Build output: {output}");
                }

                // Verify executables exist
                bool success = true;
                if (cmakeArgs.Contains("BUILD_BOTH") || string.IsNullOrEmpty(cmakeArgs))
                {
                    if (!File.Exists(ExePath))
                    {
                        Console.WriteLine($"CLI executable not found: {ExePath}");
                        success = false;
                    }
                    
                    if (cmakeArgs.Contains("BUILD_BOTH") && !File.Exists(GuiExePath))
                    {
                        Console.WriteLine($"GUI executable not found: {GuiExePath}");
                        success = false;
                    }
                }

                if (success)
                {
                    Console.WriteLine("✓ CMake build completed successfully");
                }

                return success;
            }
            catch (Exception ex)
            {
                Console.WriteLine($"CMake build failed: {ex.Message}");
                return false;
            }
        }
    }
}
