#include "include/clickhouse/clickhouse_client.h"
#include <spdlog/spdlog.h>
#include <sstream>
#include <algorithm>

namespace decode_clickhouse {

    // ClickHouseConnectionPool implementation
    ClickHouseConnectionPool::ClickHouseConnectionPool(const ClickHouseConfig& config, size_t poolSize)
        : config_(config), poolSize_(poolSize) {
        // Pre-populate the pool with connections
        for (size_t i = 0; i < poolSize_; ++i) {
            connections_.push(createConnection());
        }
    }

    ClickHouseConnectionPool::~ClickHouseConnectionPool() {
        std::lock_guard<std::mutex> lock(mutex_);
        while (!connections_.empty()) {
            connections_.pop();
        }
    }

    std::shared_ptr<clickhouse::Client> ClickHouseConnectionPool::createConnection() {
        clickhouse::ClientOptions options;
        options.SetHost(config_.host);
        options.SetPort(config_.port);
        options.SetUser(config_.user);
        options.SetPassword(config_.password);
        options.SetDefaultDatabase(config_.database);
        options.SetPingBeforeQuery(false);

        // Configure SSL for cloud connections
        if (config_.host.find("clickhouse.cloud") != std::string::npos) {
            try {
                clickhouse::ClientOptions::SSLOptions sslOptions;
                sslOptions.ssl_context = nullptr;
                sslOptions.use_default_ca_locations = true;
                sslOptions.skip_verification = false;
                sslOptions.use_sni = true;
                options.SetSSLOptions(sslOptions);
            } catch (const std::exception &ssl_e) {
                spdlog::error("SSL configuration failed: {}", ssl_e.what());
                return nullptr;
            }
        }

        options.SetCompressionMethod(clickhouse::CompressionMethod::None);
        options.SetConnectionConnectTimeout(std::chrono::seconds(30));
        options.SetConnectionRecvTimeout(std::chrono::seconds(30));
        options.SetConnectionSendTimeout(std::chrono::seconds(30));

        return std::make_shared<clickhouse::Client>(options);
    }

    std::shared_ptr<clickhouse::Client> ClickHouseConnectionPool::getConnection() {
        std::unique_lock<std::mutex> lock(mutex_);
        condition_.wait(lock, [this] { return !connections_.empty(); });
        
        auto client = connections_.front();
        connections_.pop();
        return client;
    }

    void ClickHouseConnectionPool::returnConnection(std::shared_ptr<clickhouse::Client> client) {
        std::lock_guard<std::mutex> lock(mutex_);
        connections_.push(client);
        condition_.notify_one();
    }

    // ClickHouseClient implementation
    ClickHouseClient::ClickHouseClient(const ClickHouseConfig &config, size_t poolSize)
        : config_(config), pool_(std::make_unique<ClickHouseConnectionPool>(config, poolSize)) {
    }

    ClickHouseClient::~ClickHouseClient() {
        disconnect();
    }

    bool ClickHouseClient::connect() {
        clickhouse::ClientOptions options;
        options.SetHost(config_.host);
        options.SetPort(config_.port);
        options.SetUser(config_.user);
        options.SetPassword(config_.password);
        options.SetDefaultDatabase(config_.database);
        options.SetPingBeforeQuery(false); // Disable ping to avoid SSL issues during init

        // Configure SSL for cloud connections
        // Note: ClickHouse Cloud requires port 9440 for native protocol with SSL
        if (config_.host.find("clickhouse.cloud") != std::string::npos) {
            try {
                clickhouse::ClientOptions::SSLOptions sslOptions;
                
                // Use default SSL context with proper settings
                sslOptions.ssl_context = nullptr;
                sslOptions.use_default_ca_locations = true;
                sslOptions.skip_verification = false; // Enable proper certificate verification
                sslOptions.use_sni = true; // Server Name Indication is required for cloud

                options.SetSSLOptions(sslOptions);
            } catch (const std::exception &ssl_e) {
                spdlog::error("SSL configuration failed: {}", ssl_e.what());
                return false;
            }
        }

        // Use lighter compression to reduce connection issues
        options.SetCompressionMethod(clickhouse::CompressionMethod::None);

        // Increase timeouts for cloud connections
        options.SetConnectionConnectTimeout(std::chrono::seconds(30));
        options.SetConnectionRecvTimeout(std::chrono::seconds(30));
        options.SetConnectionSendTimeout(std::chrono::seconds(30));

        // Connection pool handles client creation

        return true;
    }

    void ClickHouseClient::disconnect() {
        // Connection pool handles cleanup
    }

    bool ClickHouseClient::testConnection() {
        try {
            auto client = pool_->getConnection();
            client->Execute("SELECT 1");
            pool_->returnConnection(client);
            return true;
        } catch (const std::exception &e) {
            spdlog::error("Connection test failed: {}", e.what());
            return false;
        }
    }

    std::string ClickHouseClient::getConnectionInfo() const {
        std::ostringstream info;
        info << "Host: " << config_.host
                << ", Port: " << config_.port
                << ", User: " << config_.user
                << ", Database: " << config_.database;
        return info.str();
    }

    ClickHouseConnectionPool* ClickHouseClient::getPool() const {
        return pool_.get();
    }
} // namespace decode_clickhouse