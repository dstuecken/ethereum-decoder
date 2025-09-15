# ClickHouse Query Configuration System

The ClickHouse query system has been refactored to support configurable SQL statements. This makes queries easily customizable and maintainable.

## Features

- **Configurable SQL queries**: Load queries from external SQL files including insert statements
- **Configurable page size**: Set via command line with `--logs-page-size`
- **Configurable insert statements**: Customize decoded_logs insert via SQL template
- **Configurable ClickHouse settings**: Customize async insert settings
- **Template system**: Support for parameterized queries
- **Fallback to defaults**: Works without configuration files

## Usage Examples

### Basic Usage (Default Configuration)
```cpp
ClickHouseClient client(config);
ClickHouseEthereum ethereum(client);  // Uses default queries and settings
```

### Using Custom SQL Configuration Directory
```cpp
ClickHouseClient client(config);
ClickHouseEthereum ethereum(client, "resources/sql/");  // Loads from resources/sql/ directory
```

### Customizing via Command Line
```bash
# Set custom page size for better performance
./bin/decode_clickhouse \
  --logs-page-size 50000 \
  --host myhost --user myuser --password mypass \
  --database ethereum --blockrange 1-1000
```

### Customizing at Runtime (C++)
```cpp
ClickHouseClient client(config);
ClickHouseEthereum ethereum(client);

// Change pagination size
ethereum.getQueryConfig().setPageSize(50000);
```

## Configuration Files

### SQL Query Files

#### `resources/sql/log_stream.sql`
Main query for streaming logs with pagination:
```sql
SELECT transactionHash, blockHash, blockNumber, address, data, logIndex, removed,
       topic0, topic1, topic2, topic3
FROM logs
WHERE blockNumber >= {START_BLOCK} AND blockNumber <= {END_BLOCK}
  AND removed = 0
ORDER BY blockNumber, logIndex
LIMIT {PAGE_SIZE} OFFSET {OFFSET}
```

#### `resources/sql/contract_abi.sql`
Query for fetching contract ABIs:
```sql
SELECT ADDRESS, NAME, ABI, IMPLEMENTATION_ADDRESS
FROM decoded_contracts
WHERE (ADDRESS IN ({ADDRESS_LIST}) OR IMPLEMENTATION_ADDRESS IN ({ADDRESS_LIST}))
  AND ABI != '' AND ABI IS NOT NULL
```

#### `resources/sql/decoded_logs_insert.sql`
Template for inserting decoded logs:
```sql
INSERT INTO decoded_logs (
    transactionHash,
    logIndex,
    contractAddress,
    eventName,
    eventSignature,
    signature,
    args
) VALUES ('{transactionHash}', {logIndex}, '{contractAddress}', '{eventName}', '{eventSignature}', '{signature}', '{args}')
```

#### `resources/sql/clickhouse_settings.sql`
ClickHouse optimization settings:
```sql
SET async_insert = 1
SET wait_for_async_insert = 0
SET async_insert_threads = 4
SET async_insert_max_data_size = 100000000
SET max_insert_block_size = 100000
```

### Configuration File

#### `resources/sql/config.json`
```json
{
  "queries": {
    "log_stream_file": "log_stream.sql",
    "contract_abi_file": "contract_abi.sql",
    "decoded_logs_insert_file": "decoded_logs_insert.sql",
    "clickhouse_settings_file": "clickhouse_settings.sql"
  }
}
```

## Template Variables

The query templates support the following variables:

### Log Stream Query
- `{START_BLOCK}` - Starting block number
- `{END_BLOCK}` - Ending block number
- `{PAGE_SIZE}` - Number of records per page
- `{OFFSET}` - Offset for pagination

### Contract ABI Query
- `{ADDRESS_LIST}` - Comma-separated list of contract addresses

### Decoded Logs Insert Query
- `{transactionHash}` - Transaction hash (string)
- `{logIndex}` - Log index (numeric)
- `{contractAddress}` - Contract address (string)
- `{eventName}` - Event name (string)
- `{eventSignature}` - Event signature hash (string)
- `{signature}` - Full signature (string)
- `{args}` - Decoded arguments JSON (string)

## Migration from Hardcoded Queries

The old hardcoded approach:
```cpp
// Old way - hardcoded SQL
std::ostringstream query;
query << "SELECT * FROM logs WHERE blockNumber >= " << start 
      << " AND blockNumber <= " << end;
```

The new configurable approach:
```cpp
// New way - configurable queries
std::string queryStr = queryConfig_.formatLogStreamQuery(start, end, pageSize, offset);
```

## Benefits

1. **Easy Customization**: Change queries without recompiling
2. **Different Environments**: Different configurations for different deployments
3. **Table Name Flexibility**: Support different database schemas
4. **Performance Tuning**: Easily adjust ClickHouse settings
5. **Maintainability**: SQL queries are in dedicated files
6. **Version Control**: Track query changes separately from code

## Backward Compatibility

The system maintains full backward compatibility. Existing code continues to work without any changes, using the default configuration.