#include <iostream>
#include <curl/curl.h>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include <openssl/buffer.h>
#include "trader.hpp"
#include "websocket.hpp"
#include "logger.hpp"
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using namespace std;

int main() {
    // Initialize CURL globally
    curl_global_init(CURL_GLOBAL_DEFAULT);
    LOG_INFO("Application started");

    try {
        std::ifstream configFile("/home/pratham/gq_task/config.json");
        if (!configFile.is_open()) {
            LOG_ERROR("Failed to open config.json");
            return 1;
        }
        json config;
        configFile >> config;

        std::string clientId = config["clientId"];
        std::string clientSecret = config["clientSecret"];
        Trader trader(clientId, clientSecret);

        std::string deribitUri = "wss://test.deribit.com/ws/api/v2";
        DeribitWebSocketClient wsClient(deribitUri, clientId, clientSecret);
        
        LOG_INFO("WebSocket client initialized");

        // Start WebSocket client in a separate thread
        std::thread wsThread([&]() {
            LOG_INFO("WebSocket thread started");
            wsClient.connect();
            wsClient.run();
        });
        
        int flag = 1;
        while(flag) {
            int choice, action;
            double price, amount; 
            string instrument_name, type, channel;
            string order_id;

            cout << "\n===== Bitget Trading Interface =====\n";
            cout << "Select action: " << endl;
            cout << "1) Buy" << endl;
            cout << "2) Sell" << endl;
            cout << "3) Cancel" << endl;
            cout << "4) Modify" << endl;
            cout << "5) View Current Positions" << endl;
            cout << "6) Order Book" << endl;
            cout << "7) Market data streaming" << endl;
            cout << "8) Exit" << endl;
            cout << "Enter choice: ";
            cin >> choice;

            string endpoint;
            json result;
            
            switch (choice) {
                case 1: // Buy
                    endpoint = "private/buy";
                    cout << "Enter instrument name: ";
                    cin >> instrument_name;
                    cout << "Enter amount: ";
                    cin >> amount;
                    cout << "Enter order type: ";
                    cin >> type;
                    
                    endpoint = endpoint + "?instrument_name=" + instrument_name + "&amount=" + std::to_string(amount) + "&type=" + type;
                    if(type == "limit") {
                        cout << "Enter price: ";
                        cin >> price;
                        endpoint += "&price=" + std::to_string(price);
                    }
                    try {
                        START_MEASUREMENT(buy_order_placement);
                        result = trader.sendRequest(endpoint);
                        END_MEASUREMENT(buy_order_placement);
                        
                        LOG_TRADE("BUY", result);
                        std::cout << "Buy result: " << result.dump(2) << std::endl;
                    } catch (const std::exception& e) {
                        LOG_ERROR_CTX("Buy Order", e.what());
                        std::cerr << "Error executing buy order: " << e.what() << std::endl;
                    }
                    break;
                    
                case 2: // Sell
                    endpoint = "private/sell";
                    cout << "Enter instrument name: ";
                    cin >> instrument_name;
                    cout << "Enter amount: ";
                    cin >> amount;
                    cout << "Enter order type: ";
                    cin >> type;
                    
                    endpoint = endpoint + "?instrument_name=" + instrument_name + "&amount=" + std::to_string(amount) + "&type=" + type;
                    if(type == "limit") {
                        cout << "Enter price: ";
                        cin >> price;
                        endpoint += "&price=" + std::to_string(price);
                    }
                    try {
                        START_MEASUREMENT(sell_order_placement);
                        result = trader.sendRequest(endpoint);
                        END_MEASUREMENT(sell_order_placement);
                        
                        LOG_TRADE("SELL", result);
                        std::cout << "Sell result: " << result.dump(2) << std::endl;
                    } catch (const std::exception& e) {
                        LOG_ERROR_CTX("Sell Order", e.what());
                        std::cerr << "Error executing sell order: " << e.what() << std::endl;
                    }
                    break;
                    
                case 3: // Cancel
                    endpoint = "private/cancel";
                    cout << "Enter order ID: ";
                    cin >> order_id;
                    endpoint = endpoint + "?order_id=" + order_id;
                    try {
                        START_MEASUREMENT(cancel_order);
                        result = trader.sendRequest(endpoint);
                        END_MEASUREMENT(cancel_order);
                        
                        LOG_INFO("Order canceled: " + order_id);
                        std::cout << "Cancel result: " << result.dump(2) << std::endl;
                    } catch (const std::exception& e) {
                        LOG_ERROR_CTX("Cancel Order", e.what());
                        std::cerr << "Error canceling order: " << e.what() << std::endl;
                    }
                    break;
                    
                case 4: // Modify
                    endpoint = "private/edit";
                    cout << "Enter order ID: ";
                    cin >> order_id;
                    cout << "Enter new amount (or 0 to keep current): ";
                    cin >> amount;
                    cout << "Enter new price (or 0 to keep current): ";
                    cin >> price;

                    endpoint = endpoint + "?order_id=" + order_id + "&amount=" + std::to_string(amount) + "&price=" + std::to_string(price);

                    try {
                        START_MEASUREMENT(modify_order);
                        result = trader.sendRequest(endpoint);
                        END_MEASUREMENT(modify_order);
                        
                        LOG_INFO("Order modified: " + order_id);
                        std::cout << "Modify result: " << result.dump(2) << std::endl;
                    } catch (const std::exception& e) {
                        LOG_ERROR_CTX("Modify Order", e.what());
                        std::cerr << "Error modifying order: " << e.what() << std::endl;
                    }
                    break;
                    
                case 5: // View Positions
                    endpoint = "private/get_position";
                    cout << "Enter instrument name: ";
                    cin >> instrument_name;

                    endpoint = endpoint + "?instrument_name=" + instrument_name;
                    try {
                        START_MEASUREMENT(get_position);
                        result = trader.sendRequest(endpoint);
                        END_MEASUREMENT(get_position);
                        
                        std::cout << "Positions: " << result.dump(2) << std::endl;
                    } catch (const std::exception& e) {
                        LOG_ERROR_CTX("View Positions", e.what());
                        std::cerr << "Error fetching positions: " << e.what() << std::endl;
                    }
                    break;
                case 6: // Order Book
                    endpoint = "public/get_order_book";
                    cout << "Enter instrument name ";
                    cin >> instrument_name;

                    endpoint = endpoint + "?instrument_name=" + instrument_name;
                    try {
                        START_MEASUREMENT(get_order_book);
                        result = trader.sendRequest(endpoint);
                        END_MEASUREMENT(get_order_book);
                        
                        std::cout << "Order Book: " << result.dump(2) << std::endl;
                    } catch (const std::exception& e) {
                        LOG_ERROR_CTX("Order Book", e.what());
                        std::cerr << "Error fetching order book: " << e.what() << std::endl;
                    }
                    break;
                case 7: // Market data
                    
                    cout << "1) Public Subscribe" << endl;
                    cout << "2) Private Subscribe" << endl;
                    cout << "3) Public Unsubscribe" << endl;
                    cout << "4) Private Unsubscribe" << endl;
                    cout << "Enter choice: ";
                    cin >> action;

                    cout << "Enter channel name: ";
                    cin>>channel;

                    
                    if (action < 1 || action > 4) {
                        LOG_WARNING("Invalid market data action selected: " + std::to_string(action));
                        cout << "Invalid choice" << endl;
                        break;
                    }
                    switch (action) {
                        case 1: // Public Subscribe
                            wsClient.publicSubscribe({channel});
                            LOG_INFO("Subscribed to public channel: " + channel);
                            break;
                            
                        case 2: // Private Subscribe
                            wsClient.privateSubscribe({channel});
                            LOG_INFO("Subscribed to private channel: " + channel);
                            break;
                            
                        case 3: // Public Unsubscribe
                            wsClient.publicUnsubscribe({channel});
                            LOG_INFO("Unsubscribed from public channel: " + channel);
                            break;
                            
                        case 4: // Private Unsubscribe
                            wsClient.privateUnsubscribe({channel});
                            LOG_INFO("Unsubscribed from private channel: " + channel);
                            break;
                    }
                    break;
                case 8: // Exit
                    flag = 0;
                    LOG_INFO("User initiated exit");
                    cout << "Exiting program." << endl;
                    break;
                    
                default:
                    LOG_WARNING("Invalid menu option selected: " + std::to_string(choice));
                    cout << "Invalid choice" << endl;
                    break;
            }
        }
        
        LOG_INFO("Waiting for WebSocket thread to terminate");
        if (wsThread.joinable()) {
            wsThread.join();
        }
    } catch (const std::exception& e) {
        LOG_ERROR(std::string("Main: ") + e.what());
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    LOG_INFO("Application shutting down");
    // Clean up CURL global resources
    curl_global_cleanup();
    return 0;
}