# Ethereum Log Decoder (C++)

A high-performance C++ library and applications for decoding Ethereum event logs using ABI (Application Binary Interface) files. This project provides both a core decoding library and two specialized applications for different use cases.

## Project Structure

This project consists of:

### 1. **ethereum_decoder** (Core Library)
- **Location**: `app/ethereum_decoder/`
- **Purpose**: Core library providing Ethereum log decoding functionality
- **Features**:
  - ABI parsing from JSON files
  - Event log decoding with full Solidity type support
  - Keccak256 hashing for event signatures
  - Type decoding for all Ethereum ABI types
- **Build Output**: `lib/libethereum_decoder.a`

### 2. **decode_log** (CLI Application)
- **Location**: `app/decode_log/`
- **Purpose**: Command-line tool for decoding individual Ethereum logs
- **Features**:
  - Decode single logs from command-line input
  - Support for human-readable and JSON output formats
  - Quick testing and debugging of log data or use within scripts
- **Build Output**: `bin/decode_log`

### 3. **decode_clickhouse** (Streaming Application)
- **Location**: `app/decode_clickhouse/`
- **Purpose**: High-performance streaming processor for bulk log decoding
- **Features**:
  - Stream logs directly from ClickHouse database
  - Parallel processing with configurable worker threads
  - Output to Parquet files (with Apache Arrow) or JSON
  - Optional direct insertion back to ClickHouse
  - Real-time progress tracking
  - Comprehensive logging with configurable verbosity
- **Build Output**: `bin/decode_clickhouse`

## Building the Project

### Prerequisites

- C++17 compatible compiler (GCC 7+, Clang 6+)
- Git (for fetching dependencies)
- (Optional) Apache Arrow for Parquet support: `brew install apache-arrow`
- (Optional) ClickHouse C++ client dependencies

### Quick Start

```bash
# 1. Clone the repository
git clone git@github.com:dstuecken/ethereum-decoder.git
cd ethereum-decoder

# 2. Install dependencies
./install_dependencies.sh

# 3. Build all components (library + both applications)
# Build individual components:
./make_ethereum_decoder_lib.sh    # Build core library
./make_decode_log.sh              # Build decode_log application
ENABLE_PARQUET=1 ./make_decode_clickhouse.sh  # Build decode_clickhouse with Parquet
```

### Individual Build Scripts

Build specific components:

```bash
# Build only the core library
./make_ethereum_decoder_lib.sh

# Build only decode_log application (auto-builds library if needed)
./make_decode_log.sh

# Build only decode_clickhouse application (auto-builds library if needed)
ENABLE_PARQUET=1 ./make_decode_clickhouse.sh  # With Parquet support
./make_decode_clickhouse.sh                    # Without Parquet (JSON only)
```

### Build Output

After building, you'll find:
```
ethereum-decoder/
├── lib/
│   └── libethereum_decoder.a    # Core decoding library
├── bin/
│   ├── decode_log               # CLI decoder application
│   └── decode_clickhouse        # Streaming decoder application
└── decoded_logs/                # Default output directory (created at runtime)
```

## Using the Applications

### decode_log - CLI Log Decoder

Decode individual Ethereum event logs from the command line.

**Basic Usage:**
```bash
# Decode a single log
./bin/decode_log resources/abis/erc20.json --log-data "topics:data"

# Output as JSON
./bin/decode_log resources/abis/erc20.json --log-data "topics:data" --format json

# Show help
./bin/decode_log --help
```

**Example - Decoding an ERC20 Transfer Event:**
```bash
./bin/decode_log resources/abis/erc20.json --log-data \
  "0xddf252ad1be2c89b69c2b068fc378daa952ba7f163c4a11628f55a4df523b3ef,\
0x000000000000000000000000a9d1e08c7793af67e9d92fe308d5697fb81d3e43,\
0x00000000000000000000000077696bb39917c91a0c3908d577d5e322095425ca:\
0x00000000000000000000000000000000000000000000000000000000000003e8"
```

**Output Formats:**
- **human** (default): Human-readable format with detailed information
- **json**: Clean JSON output for programmatic use

### decode_clickhouse - Streaming Log Decoder

Process large volumes of logs from ClickHouse with parallel processing and batch output.

**Basic Usage:**
```bash
# Decode logs from ClickHouse and save to Parquet files
./bin/decode_clickhouse \
  --host clickhouse.example.com \
  --user myuser \
  --password mypass \
  --database ethereum \
  --port 8443 \
  --blockrange 18000000-18001000 \
  --output-dir decoded_logs

# With all options
./bin/decode_clickhouse \
  --host localhost \
  --user default \
  --password '' \
  --database ethereum \
  --port 9440 \
  --blockrange 1000-5000 \
  --workers 16 \
  --output-dir results \
  --json \
  --log-level debug \
  --log-file decode.log

# Insert decoded logs back to ClickHouse into decoded_logs table
./bin/decode_clickhouse \
  --host localhost \
  --user default \
  --password '' \
  --database ethereum \
  --port 9440 \
  --blockrange 1000-5000 \
  --workers 16 \
  --insert-decoded-logs
```

**Command-Line Options:**

**Required:**
- `--host <hostname>`: ClickHouse server hostname
- `--user <username>`: ClickHouse username
- `--password <password>`: ClickHouse password
- `--database <database>`: ClickHouse database name
- `--port <port>`: ClickHouse port (8443 for HTTPS, 9440 for native SSL)
- `--blockrange <start-end>`: Block range to process (e.g., 1000-2000)

**Optional:**
- `--workers <count>`: Number of parallel workers (default: 8)
- `--insert-decoded-logs`: Insert decoded logs back to ClickHouse
- `--output-dir <dir>`: Output directory for files (default: decoded_logs)
- `--json`: Force JSON output instead of Parquet
- `--log-level <level>`: Logging verbosity: debug, info, warning, error (default: info)
- `--log-file <path>`: Log file path (default: decode_clickhouse.log)
- `--sql-config-dir <dir>`: Directory with custom SQL queries

**Output Formats:**
- **Parquet** (default if Apache Arrow available): One `.parquet` file per block
- **JSON** (fallback or with --json flag): One `.json` file per block

**Progress Display:**
The application shows real-time progress:
```
⠋ Decoding │ Blocks: 234/1000 │ Page: 15 │ Logs: 12,345 │ Decoded: 10,234 (83.0%) │ Workers: 4 │ Time: 2m 15s
```

## Library Integration

For detailed information on using the ethereum_decoder library in your own C++ projects, see [Library Usage Guide](resources/docs/LIBRARY_USE.md).

## Dependencies

The project uses the following dependencies (automatically installed by `install_dependencies.sh`):

- **spdlog**: Fast C++ logging library
- **nlohmann/json**: JSON parsing and serialization
- **ClickHouse C++ client**: For database connectivity
- **Apache Arrow** (optional): For Parquet file support
- **Abseil**: Required by ClickHouse client

## Performance

- **decode_log**: Instant decoding of individual logs
- **decode_clickhouse**: 
  - Processes thousands of logs per second
  - Parallel processing with configurable workers
  - Memory-efficient streaming (configurable batch sizes)
  - Automatic chunking by block number


## Testing

Test the applications:
```bash
# Test the library by building decode_log
./make_decode_log.sh

# Test decode_log with ERC20 transfer example
./bin/decode_log resources/abis/erc20.json --log-data \
  "0xddf252ad1be2c89b69c2b068fc378daa952ba7f163c4a11628f55a4df523b3ef,0x000000000000000000000000a9d1e08c7793af67e9d92fe308d5697fb81d3e43,0x00000000000000000000000077696bb39917c91a0c3908d577d5e322095425ca:0x00000000000000000000000000000000000000000000000000000000000003e8"

# Test decode_clickhouse (requires ClickHouse connection)
./bin/decode_clickhouse --help
```

## Troubleshooting

### Missing ABIs
If decode_clickhouse reports "No ABI found for contract", ensure:
1. ABIs are loaded in your ClickHouse database
2. The contract addresses in logs match the ABIs

### Parquet Support
If Parquet files aren't being created:
1. Install Apache Arrow: `brew install apache-arrow`
2. Rebuild with: `ENABLE_PARQUET=1 ./make_decode_clickhouse.sh`
3. Or use `--json` flag to force JSON output

### OpenSSL Issues
If you encounter OpenSSL errors during `clickhouse-cpp` build:
```bash
# Ensure OpenSSL is installed
brew install openssl

# Clean and rebuild if needed
./clean.sh --deps
./install_dependencies.sh
```

### Build Issues
If dependencies fail to build:
1. Ensure you have a C++17 compatible compiler (GCC 7+, Clang 6+)
2. Run `./install_dependencies.sh` to fetch all dependencies
3. Check `deps/` directory for successful builds
4. Try building components individually:
   - `./make_ethereum_decoder_lib.sh` (core library)
   - `./make_decode_log.sh` (CLI application)
   - `./make_decode_clickhouse.sh` (streaming application)

### Cleaning Build Files
```bash
# Clean build artifacts only (keep dependencies)
./clean.sh
# or: make -f Makefile.manual clean

# Clean everything including dependencies
./clean.sh --deps
# or: make -f Makefile.manual clean-all

# Clean only dependencies (keep build files)
./clean.sh --deps-only
# or: make -f Makefile.manual clean-deps
```

## License

MIT License