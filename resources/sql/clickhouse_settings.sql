-- ClickHouse settings for optimized async inserts
-- These settings will be executed before bulk insert operations
SET async_insert = 1
SET wait_for_async_insert = 0
SET async_insert_threads = 4
SET async_insert_max_data_size = 100000000
SET max_insert_block_size = 100000