-- Insert statement for decoded logs
-- Parameters: Individual field placeholders for each log entry
-- 
-- This template allows customization of:
-- - Target table name (change 'decoded_logs' to your table)
-- - Column names and order
-- - Additional columns or transformations
--
-- The placeholders will be replaced with escaped values from each log
INSERT INTO decoded_logs
(
    transactionHash,
    logIndex,
    contractAddress,
    eventName,
    eventSignature,
    signature,
    args
)
VALUES
(
    '{transactionHash}',
    {logIndex},
    '{contractAddress}',
    '{eventName}',
    '{eventSignature}',
    '{signature}',
    '{args}'
)