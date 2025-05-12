#pragma once

#include <string>
#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>
#include <websocketpp/common/memory.hpp>
#include <websocketpp/common/thread.hpp>
#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/transport/asio/security/tls.hpp>
#include <nlohmann/json.hpp>
#include <memory>
#include <iostream>
#include <map>
#include <chrono>
#include <atomic>


class DeribitWebSocketClient {
public:
    using client = websocketpp::client<websocketpp::config::asio_tls_client>;
    using connection_hdl = websocketpp::connection_hdl;
    using context_ptr = std::shared_ptr<boost::asio::ssl::context>;

    DeribitWebSocketClient(
        const std::string& uri,
        const std::string& client_id,
        const std::string& client_secret
    );

    void connect();
    int getNextId();
    void run();
    void authenticate();
    void publicSubscribe(const std::vector<std::string>& channels);
    void privateSubscribe(const std::vector<std::string>& channels);
    void publicUnsubscribe(const std::vector<std::string>& channels);
    void privateUnsubscribe(const std::vector<std::string>& channels);

private:
    // WebSocket event handlers
    void onOpen(connection_hdl hdl);
    void onClose(connection_hdl hdl);
    void onFail(connection_hdl hdl);
    void onMessage(connection_hdl hdl, client::message_ptr msg);
    context_ptr onTLSInit(connection_hdl hdl);

    // Message processing
    void send(const nlohmann::json& payload);
    void processMethod(const nlohmann::json& msg);
    void processResult(const nlohmann::json& msg);
    void handleSubscriptionData(const nlohmann::json& msg);
    void logError(const std::string& context, const std::string& error);

    // WebSocket client instance
    std::atomic<int> m_idCounter;
    client m_client;
    connection_hdl m_hdl;
    std::string m_uri;
    std::string m_client_id;
    std::string m_client_secret;
    bool m_isConnected = false;
    bool m_isAuthenticated = false;

    // Message queue for messages that need to be sent after connection is established
    std::unique_ptr<nlohmann::json> m_queuedPayload;
    
    // Latency measurement
    std::map<std::string, std::chrono::time_point<std::chrono::high_resolution_clock>> m_messageTimes;
};