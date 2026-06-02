#!/bin/bash
# Build and run unit tests for Smart Bell firmware

set -e  # Exit on error

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${PROJECT_ROOT}/build/x86_64/Release"

echo "=== Smart Bell Unit Test Builder ==="
echo ""

# Check for required tools
command -v cmake >/dev/null 2>&1 || { echo "❌ cmake not found. Install: sudo apt install cmake"; exit 1; }
command -v conan >/dev/null 2>&1 || { echo "❌ conan not found. Install: pip install conan"; exit 1; }

# Parse arguments
CLEAN=false
VERBOSE=false
FILTER=""

while [[ $# -gt 0 ]]; do
  case $1 in
    --clean)
      CLEAN=true
      shift
      ;;
    --verbose)
      VERBOSE=true
      shift
      ;;
    --filter)
      FILTER="$2"
      shift 2
      ;;
    *)
      echo "Unknown option: $1"
      echo "Usage: $0 [--clean] [--verbose] [--filter <pattern>]"
      exit 1
      ;;
  esac
done

# Clean build if requested
if [ "$CLEAN" = true ]; then
  echo "🧹 Cleaning build directory..."
  rm -rf "${BUILD_DIR}"
fi

# Create build directory
mkdir -p "${BUILD_DIR}"

# Step 1: Conan install
echo "📦 Installing dependencies with Conan..."
cd "${PROJECT_ROOT}"
conan install . \
  --output-folder="${BUILD_DIR}" \
  --build=missing \
  --profile:build=default \
  --profile:host=default \
  -o platform=linux \
  -o tests=True \
  -c tools.cmake.cmaketoolchain:generator="Unix Makefiles"

# Step 2: CMake configure
echo "🔧 Configuring CMake..."
cd "${BUILD_DIR}"
cmake -G "Unix Makefiles" \
  -DCMAKE_TOOLCHAIN_FILE=generators/conan_toolchain.cmake \
  -DCMAKE_BUILD_TYPE=Release \
  -DENABLE_UNIT_TESTS=ON \
  ../../..

# Step 3: Build tests
echo "🔨 Building tests..."
make run_gTest -j$(nproc)

# Step 4: Run tests
echo ""
echo "✅ Build successful!"
echo ""
echo "🧪 Running tests..."
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

TEST_ARGS="--gtest_color=yes"

if [ "$VERBOSE" = true ]; then
  TEST_ARGS="$TEST_ARGS --gtest_verbose"
fi

if [ -n "$FILTER" ]; then
  TEST_ARGS="$TEST_ARGS --gtest_filter=$FILTER"
  echo "Running filtered tests: $FILTER"
fi

./run_gTest $TEST_ARGS

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "✅ All tests completed!"
echo ""
echo "📊 To generate XML report:"
echo "   cd ${BUILD_DIR} && ./run_gTest --gtest_output=xml:test_report.xml"
echo ""
echo "🔍 To run specific tests:"
echo "   $0 --filter 'CircularBufferTest.*'"
echo "   $0 --filter 'LightweightConfigTest.SetDeviceIP'"
