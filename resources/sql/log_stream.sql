-- Query for streaming logs with pagination
-- Parameters: {START_BLOCK}, {END_BLOCK}, {PAGE_SIZE}, {OFFSET}, {LOGS_TABLE}
SELECT transactionHash, blockNumber, address, data, logIndex,
       topic0, topic1, topic2, topic3
FROM {LOGS_TABLE}
WHERE blockNumber >= {START_BLOCK} AND blockNumber <= {END_BLOCK}
  AND removed = 0
ORDER BY blockNumber, logIndex
LIMIT {PAGE_SIZE} OFFSET {OFFSET}