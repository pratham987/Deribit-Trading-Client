#ifndef TRADER_HPP
#define TRADER_HPP

#include <string>
#include <nlohmann/json.hpp> 
using json = nlohmann::json;

// Forward declaration of WriteCallback function
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* s);

class Trader{
public:
    Trader(const std::string& clientId, const std::string& clientSecret);

    std::string authenticate();
    json sendRequest(const std::string &endpoint);

private:
    std::string clientId;
    std::string clientSecret;
    std::string accessToken;
    const std::string baseUrl = "https://test.deribit.com/api/v2/";
};

#endif // TRADER_HPP