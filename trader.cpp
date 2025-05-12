#include <curl/curl.h>
#include <stdexcept>
#include <sstream>
#include <nlohmann/json.hpp>
#include <iostream>
#include "trader.hpp"
#include "logger.hpp" 
using json = nlohmann::json;

using namespace std;

// Define WriteCallback function
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* s) {
    size_t newLength = size * nmemb;
    try {
        s->append((char*)contents, newLength);
        return newLength;
    }
    catch (std::bad_alloc& e) {
        return 0;
    }
}

Trader::Trader(const std::string& clientId, const std::string& clientSecret)
    : clientId(clientId), clientSecret(clientSecret) {
    LOG_INFO("Trader initialized with client ID: " + clientId);
}

string Trader::authenticate() {
    LOG_INFO("Starting authentication process");
    START_MEASUREMENT(authentication);
    
    // Prepare the authentication URL
    std::string authUrl = baseUrl + "public/auth?client_id=" + clientId +
        "&client_secret=" + clientSecret +
        "&grant_type=client_credentials";

    // Use libcurl to send the request
    CURL* curl;
    CURLcode res;
    std::string readBuffer;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (curl) {
        // Set up the curl request
        curl_easy_setopt(curl, CURLOPT_URL, authUrl.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);

        // Set up callback to capture response
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        // Perform the request
        res = curl_easy_perform(curl);

        // Check for errors
        if (res != CURLE_OK) {
            std::string error_msg = "Authentication failed: " + std::string(curl_easy_strerror(res));
            LOG_ERROR_CTX("Authentication", error_msg);
            curl_easy_cleanup(curl);
            curl_global_cleanup();
            throw std::runtime_error(error_msg);
        }

        // Clean up
        curl_easy_cleanup(curl);
        curl_global_cleanup();

        // Parse the JSON response
        try {
            nlohmann::json jsonResponse = nlohmann::json::parse(readBuffer);

            // Extract and store the access token
            if (jsonResponse.contains("result") &&
                jsonResponse["result"].contains("access_token")) {
                accessToken = jsonResponse["result"]["access_token"];
                LOG_INFO("Authentication successful");
                END_MEASUREMENT(authentication);
                return accessToken;
            }
            else {
                std::string error_msg = "Invalid authentication response";
                LOG_ERROR_CTX("Authentication", error_msg);
                END_MEASUREMENT(authentication);
                throw std::runtime_error(error_msg);
            }
        }
        catch (const nlohmann::json::exception& e) {
            std::string error_msg = "JSON parsing error: " + std::string(e.what());
            LOG_ERROR_CTX("Authentication", error_msg);
            END_MEASUREMENT(authentication);
            throw std::runtime_error(error_msg);
        }
    }
    else {
        std::string error_msg = "Failed to initialize CURL";
        LOG_ERROR_CTX("Authentication", error_msg);
        END_MEASUREMENT(authentication);
        throw std::runtime_error(error_msg);
    }
}

json Trader::sendRequest(const std::string &endpoint) {
    START_MEASUREMENT(api_request);
    LOG_INFO("Sending request to: " + endpoint);
    
    if (accessToken.empty()) {
        LOG_INFO("No access token found, authenticating first");
        authenticate(); // Automatically authenticate if no token exists
    }

    CURL *curl = curl_easy_init();
    std::string response_string;
    if (!curl) {
        std::string error_msg = "Failed to initialize curl";
        LOG_ERROR_CTX("API Request", error_msg);
        END_MEASUREMENT(api_request);
        std::cerr << error_msg << std::endl;
        return json::object();  // Return empty JSON object
    }

    // Set up headers
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, ("Authorization: Bearer " + accessToken).c_str());
    headers = curl_slist_append(headers, "Content-Type: application/json");

    std::string url = baseUrl + endpoint;
    // std::string post_fields = params.dump();

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    // curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_fields.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);

    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        std::string error_msg = "curl_easy_perform() failed: " + std::string(curl_easy_strerror(res));
        LOG_ERROR_CTX("API Request", error_msg);
        END_MEASUREMENT(api_request);
        std::cerr << error_msg << std::endl;
        return json::object();  // Return empty JSON object
    }

    try {
        json jsonResponse = json::parse(response_string);
        LOG_INFO("Response received from: " + endpoint);
        END_MEASUREMENT(api_request);
        return jsonResponse;  // Return the full JSON response
    }
    catch (const json::exception& e) {
        std::string error_msg = "JSON parsing error: " + std::string(e.what());
        LOG_ERROR_CTX("API Request", error_msg);
        END_MEASUREMENT(api_request);
        std::cerr << error_msg << std::endl;
        return json::object();  // Return empty JSON object
    }
}