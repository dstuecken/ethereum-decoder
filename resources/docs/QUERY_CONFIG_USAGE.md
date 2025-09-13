# ClickHouse Query Configuration System

The ClickHouse query system has been refactored to support configurable SQL statements. This makes queries easily customizable and maintainable.

## Features

- **Configurable SQL queries**: Load queries from external SQL files
- **Configurable table names**: Easily change table names without code changes
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

### Customizing Table Names at Runtime
```cpp
ClickHouseClient client(config);
ClickHouseEthereum ethereum(client);

// Customize table names
ethereum.getQueryConfig().setLogsTableName("custom_logs");
ethereum.getQueryConfig().setContractsTableName("custom_contracts");
ethereum.getQueryConfig().setDecodedLogsTableName("custom_decoded_logs");
```

### Customizing Page Size
```cpp
ClickHouseClient client(config);
ClickHouseEthereum ethereum(client);

// Change pagination size
ethereum.getQueryConfig().setPageSize(5000);
```

## Configuration Files

### SQL Query Files

#### `resources/sql/log_stream.sql`
Main query for streaming logs with pagination:
```sql
SELECT transactionHash, blockHash, blockNumber, address, data, logIndex, removed,
       topic0, topic1, topic2, topic3
FROM {LOGS_TABLE}
WHERE blockNumber >= {START_BLOCK} AND blockNumber <= {END_BLOCK}
  AND removed = 0
ORDER BY blockNumber, logIndex
LIMIT {PAGE_SIZE} OFFSET {OFFSET}
```

#### `resources/sql/contract_abi.sql`
Query for fetching contract ABIs:
```sql
SELECT ADDRESS, NAME, ABI, IMPLEMENTATION_ADDRESS
FROM {CONTRACTS_TABLE}
WHERE (ADDRESS IN ({ADDRESS_LIST}) OR IMPLEMENTATION_ADDRESS IN ({ADDRESS_LIST}))
  AND ABI != '' AND ABI IS NOT NULL
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
  "tables": {
    "logs": "logs",
    "contracts": "decoded_contracts", 
    "decoded_logs": "decoded_logs"
  },
  "pagination": {
    "page_size": 10000
  },
  "queries": {
    "log_stream_file": "log_stream.sql",
    "contract_abi_file": "contract_abi.sql",
    "clickhouse_settings_file": "clickhouse_settings.sql"
  }
}
```

## Template Variables

The query templates support the following variables:

### Log Stream Query
- `{LOGS_TABLE}` - Table name for logs
- `{START_BLOCK}` - Starting block number
- `{END_BLOCK}` - Ending block number
- `{PAGE_SIZE}` - Number of records per page
- `{OFFSET}` - Offset for pagination

### Contract ABI Query
- `{CONTRACTS_TABLE}` - Table name for contracts
- `{ADDRESS_LIST}` - Comma-separated list of contract addresses

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