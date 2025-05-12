#include "websocket.hpp"
#include "logger.hpp"
#include <websocketpp/common/thread.hpp>
#include <thread>
#include <chrono>
#include <stdexcept>
#include <atomic>

DeribitWebSocketClient::DeribitWebSocketClient(
    const std::string& uri,
    const std::string& client_id,
    const std::string& client_secret
) :
    m_uri(uri),
    m_client_id(client_id),
    m_client_secret(client_secret),
    m_idCounter(1) // Initialize atomic ID counter
{
    try {
        LOG_INFO("Initializing WebSocket client for " + uri);
        m_client.set_access_channels(websocketpp::log::alevel::none);
        m_client.set_error_channels(websocketpp::log::elevel::all);

        m_client.init_asio();

        m_client.set_tls_init_handler(std::bind(&DeribitWebSocketClient::onTLSInit, this, std::placeholders::_1));

        m_client.set_open_handler(std::bind(&DeribitWebSocketClient::onOpen, this, std::placeholders::_1));
        m_client.set_close_handler(std::bind(&DeribitWebSocketClient::onClose, this, std::placeholders::_1));
        m_client.set_fail_handler(std::bind(&DeribitWebSocketClient::onFail, this, std::placeholders::_1));
        m_client.set_message_handler(std::bind(&DeribitWebSocketClient::onMessage, this, std::placeholders::_1, std::placeholders::_2));

        m_client.start_perpetual();
        LOG_INFO("WebSocket client initialization complete");
    }
    catch (const std::exception& e) {
        LOG_ERROR_CTX("WebSocket Initialization", e.what());
    }
}

int DeribitWebSocketClient::getNextId() {
    return m_idCounter++;
}

void DeribitWebSocketClient::connect() {
    LOG_INFO("Connecting to WebSocket server: " + m_uri);
    START_MEASUREMENT(websocket_connect);
    
    websocketpp::lib::error_code ec;
    client::connection_ptr con = m_client.get_connection(m_uri, ec);
    if (ec) {
        LOG_ERROR_CTX("WebSocket Connection Creation", ec.message());
        END_MEASUREMENT(websocket_connect);
        return;
    }
    m_hdl = con->get_handle();
    m_client.connect(con);
    
    LOG_INFO("WebSocket connection initiated");
    END_MEASUREMENT(websocket_connect);
}

void DeribitWebSocketClient::authenticate() {
    LOG_INFO("Starting WebSocket authentication");

    using json = nlohmann::json;
    int id = getNextId();

    json auth_payload = {
        {"jsonrpc", "2.0"},
        {"method", "public/auth"},
        {"id", id},
        {"params", {
            {"grant_type", "client_credentials"},
            {"client_id", m_client_id},
            {"client_secret", m_client_secret}
        }}
    };

    m_messageTimes[std::to_string(id)] = std::chrono::high_resolution_clock::now();
    send(auth_payload);
    LOG_INFO("Authentication request sent");
}

void DeribitWebSocketClient::publicSubscribe(const std::vector<std::string>& channels) {
    LOG_INFO("Subscribing to public channels: " + channels[0]);
    START_MEASUREMENT(public_subscribe);
    
    using json = nlohmann::json;
    int id = getNextId();

    json subscribe_payload = {
        {"jsonrpc", "2.0"},
        {"method", "public/subscribe"},
        {"id", id},
        {"params", {{"channels", channels}}}
    };

    m_messageTimes[std::to_string(id)] = std::chrono::high_resolution_clock::now();
    send(subscribe_payload);
    END_MEASUREMENT(public_subscribe);
}

void DeribitWebSocketClient::privateSubscribe(const std::vector<std::string>& channels) {
    LOG_INFO("Subscribing to private channels: " + channels[0]);
    START_MEASUREMENT(private_subscribe);

    try {
        if (!m_isConnected) {
            LOG_ERROR_CTX("Private Subscribe", "WebSocket not connected.");
            END_MEASUREMENT(private_subscribe);
            throw std::runtime_error("WebSocket not connected.");
        }

        if (!m_isAuthenticated) {
            LOG_INFO("Not authenticated, sending auth request first");
            authenticate();
        }

        int id = getNextId();

        nlohmann::json request = {
            {"jsonrpc", "2.0"},
            {"method", "private/subscribe"},
            {"id", id},
            {"params", {{"channels", channels}}}
        };

        m_messageTimes[std::to_string(id)] = std::chrono::high_resolution_clock::now();
        send(request);
        LOG_INFO("Private subscription request sent");

    } catch (const std::exception& e) {
        LOG_ERROR_CTX("Private Subscribe", e.what());
        END_MEASUREMENT(private_subscribe);
    }
}

void DeribitWebSocketClient::publicUnsubscribe(const std::vector<std::string>& channels) {
    LOG_INFO("Unsubscribing from public channels: " + channels[0]);
    START_MEASUREMENT(public_unsubscribe);

    try {
        if (!m_isConnected) {
            LOG_ERROR_CTX("Public Unsubscribe", "WebSocket not connected.");
            END_MEASUREMENT(public_unsubscribe);
            throw std::runtime_error("WebSocket not connected.");
        }

        int id = getNextId();

        nlohmann::json request = {
            {"jsonrpc", "2.0"},
            {"method", "public/unsubscribe"},
            {"id", id},
            {"params", {{"channels", channels}}}
        };

        m_messageTimes[std::to_string(id)] = std::chrono::high_resolution_clock::now();
        send(request);
        END_MEASUREMENT(public_unsubscribe);
    } catch (const std::exception& e) {
        LOG_ERROR_CTX("Public Unsubscribe", e.what());
        END_MEASUREMENT(public_unsubscribe);
    }
}

void DeribitWebSocketClient::privateUnsubscribe(const std::vector<std::string>& channels) {
    LOG_INFO("Unsubscribing from private channels: " + channels[0]);
    START_MEASUREMENT(private_unsubscribe);

    try {
        if (!m_isConnected) {
            LOG_ERROR_CTX("Private Unsubscribe", "WebSocket not connected.");
            END_MEASUREMENT(private_unsubscribe);
            throw std::runtime_error("WebSocket not connected.");
        }

        if (!m_isAuthenticated) {
            LOG_INFO("Not authenticated, sending auth request first");
            authenticate();
        }

        int id = getNextId();

        nlohmann::json request = {
            {"jsonrpc", "2.0"},
            {"method", "private/unsubscribe"},
            {"id", id},
            {"params", {{"channels", channels}}}
        };

        m_messageTimes[std::to_string(id)] = std::chrono::high_resolution_clock::now();
        send(request);
        END_MEASUREMENT(private_unsubscribe);
    } catch (const std::exception& e) {
        LOG_ERROR_CTX("Private Unsubscribe", e.what());
        END_MEASUREMENT(private_unsubscribe);
    }
}

void DeribitWebSocketClient::run() {
    LOG_INFO("Starting WebSocket IO service");
    m_client.run();
    LOG_INFO("WebSocket IO service stopped");
}

void DeribitWebSocketClient::onOpen(connection_hdl hdl) {
    LOG_INFO("WebSocket connection established");
    m_isConnected = true;

    if (m_queuedPayload) {
        LOG_INFO("Sending queued message");
        send(*m_queuedPayload);
        m_queuedPayload.reset();
    }

    authenticate();
}

void DeribitWebSocketClient::onClose(connection_hdl hdl) {
    auto con = m_client.get_con_from_hdl(hdl);
    m_isConnected = false;
    m_isAuthenticated = false;

    auto close_code = con->get_remote_close_code();
    auto close_reason = con->get_remote_close_reason();

    if (close_code != websocketpp::close::status::normal) {
        LOG_WARNING("WebSocket closed with code: " + std::to_string(close_code) + ", Reason: " + close_reason);
    } else {
        LOG_INFO("WebSocket connection closed normally");
    }
}

void DeribitWebSocketClient::onFail(connection_hdl hdl) {
    auto con = m_client.get_con_from_hdl(hdl);
    LOG_ERROR_CTX("WebSocket Connection", "Connection failed: " + con->get_ec().message());
}

void DeribitWebSocketClient::onMessage(connection_hdl hdl, client::message_ptr msg) {
    using json = nlohmann::json;
    START_MEASUREMENT(message_processing);

    try {
        auto parsed_msg = json::parse(msg->get_payload());

        if (parsed_msg.contains("id") && parsed_msg["id"].is_number()) {
            std::string id_str = std::to_string(parsed_msg["id"].get<int>());
            auto it = m_messageTimes.find(id_str);
            if (it != m_messageTimes.end()) {
                auto latency = std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::high_resolution_clock::now() - it->second
                ).count();
                LOG_INFO("Latency for ID " + id_str + ": " + std::to_string(latency) + "µs (" + std::to_string(latency / 1000.0) + "ms)");
                m_messageTimes.erase(it);
            }
        }

        if (parsed_msg.contains("error")) {
            LOG_ERROR_CTX("WebSocket Error", parsed_msg["error"].dump(4));
            END_MEASUREMENT(message_processing);
            return;
        }

        if (parsed_msg.contains("method")) {
            processMethod(parsed_msg);
        } else if (parsed_msg.contains("result")) {
            processResult(parsed_msg);
        }
    } catch (const std::exception& e) {
        LOG_ERROR_CTX("Message Processing", e.what());
    }

    END_MEASUREMENT(message_processing);
}

DeribitWebSocketClient::context_ptr DeribitWebSocketClient::onTLSInit(connection_hdl) {
    context_ptr ctx = websocketpp::lib::make_shared<boost::asio::ssl::context>(
        boost::asio::ssl::context::tlsv12_client
    );

    try {
        ctx->set_verify_mode(boost::asio::ssl::verify_none);
        ctx->set_default_verify_paths();
        ctx->set_verify_callback([](bool preverified, boost::asio::ssl::verify_context& ctx) {
            char subject_name[256];
            X509* cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
            X509_NAME_oneline(X509_get_subject_name(cert), subject_name, sizeof(subject_name));
            return preverified;
        });
        LOG_INFO("TLS context initialized successfully");
    } catch (const std::exception& e) {
        LOG_ERROR_CTX("TLS Initialization", e.what());
    }

    return ctx;
}

void DeribitWebSocketClient::send(const nlohmann::json& payload) {
    START_MEASUREMENT(websocket_send);
    websocketpp::lib::error_code ec;

    if (!m_isConnected) {
        LOG_INFO("Connection not yet open, queuing message...");
        m_queuedPayload = std::make_unique<nlohmann::json>(payload);
        END_MEASUREMENT(websocket_send);
        return;
    }

    auto con = m_client.get_con_from_hdl(m_hdl, ec);
    if (ec) {
        LOG_ERROR_CTX("Error retrieving connection", ec.message());
        END_MEASUREMENT(websocket_send);
        return;
    }

    if (con->get_state() != websocketpp::session::state::open) {
        LOG_ERROR("WebSocket is not open.");
        END_MEASUREMENT(websocket_send);
        return;
    }

    m_client.send(m_hdl, payload.dump(), websocketpp::frame::opcode::text, ec);
    if (ec) {
        LOG_ERROR_CTX("Message Send", ec.message());
    } else {
        LOG_INFO("Message sent successfully");
    }

    END_MEASUREMENT(websocket_send);
}

void DeribitWebSocketClient::processMethod(const nlohmann::json& msg) {
    if (msg["method"] == "subscription") {
        START_MEASUREMENT(subscription_processing);
        handleSubscriptionData(msg);
        END_MEASUREMENT(subscription_processing);
    }
}

void DeribitWebSocketClient::processResult(const nlohmann::json& msg) {
    if (msg.contains("id") && msg["id"] == 1 && msg.contains("result")) {
        LOG_INFO("Authentication Successful!");
        m_isAuthenticated = true;
    } else {
        LOG_INFO("Received result for ID: " + std::to_string(msg["id"].get<int>()));
    }
}

void DeribitWebSocketClient::handleSubscriptionData(const nlohmann::json& msg) {
    if (msg.contains("params") && msg["params"].contains("data")) {
        auto market_data = msg["params"]["data"];
        LOG_INFO("Market Update received");
        if (msg["params"].contains("channel")) {
            LOG_INFO("Channel: " + msg["params"]["channel"].get<std::string>());
        }

        std::cout << "Market Update: " << market_data.dump(4) << std::endl;
    }
}

void DeribitWebSocketClient::logError(const std::string& context, const std::string& error) {
    LOG_ERROR_CTX(context, error);
    std::cerr << "[" << context << "] Error: " << error << std::endl;
}
