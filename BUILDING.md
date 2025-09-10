# Build Instructions

## Prerequisites

### macOS

Install the required tools using Homebrew:

```bash
# Install Homebrew if not already installed
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install CMake and other build tools
brew install cmake
brew install pkg-config  # Optional, for finding libraries
```

### Linux (Ubuntu/Debian)

```bash
sudo apt update
sudo apt install cmake build-essential git
```

### Windows

Download and install:
1. CMake from https://cmake.org/download/
2. Visual Studio or MinGW for C++ compilation

## Building the Project

### Standard Build

```bash
# Create build directory
mkdir -p build
cd build

# Configure with CMake
cmake ..

# Build the project
make -j$(nproc)  # Linux/macOS
# or
cmake --build .  # Cross-platform
```

### Build with Tests

```bash
cmake .. -DBUILD_TESTS=ON
make
```

### Build with CryptoPP (Optional)

If you want to use CryptoPP instead of the built-in Keccak256:

```bash
# Install CryptoPP first
brew install cryptopp  # macOS
# or
sudo apt install libcrypto++-dev  # Linux

# Build with CryptoPP
cmake .. -DUSE_CRYPTOPP=ON
make
```

## Running the Examples

After building:

```bash
# Run the example decoder
./decode_log ../examples/erc20_abi.json

# Run tests
./test_decoder
```

## Troubleshooting

### CMake not found

If CMake is not found after installation, you may need to add it to your PATH:

```bash
# macOS with Homebrew
export PATH="/opt/homebrew/bin:$PATH"

# Add to your shell profile (.zshrc or .bash_profile)
echo 'export PATH="/opt/homebrew/bin:$PATH"' >> ~/.zshrc
```

### JSON library not downloading

If the automatic download of nlohmann/json fails, you can install it manually:

```bash
# macOS
brew install nlohmann-json

# Linux
sudo apt install nlohmann-json3-dev
```

Then modify CMakeLists.txt to use the system-installed version:

```cmake
find_package(nlohmann_json REQUIRED)
```

### Compilation errors

Make sure you have a C++17 compatible compiler:

```bash
# Check compiler version
g++ --version  # Should be 7.0 or higher
clang++ --version  # Should be 5.0 or higher
```