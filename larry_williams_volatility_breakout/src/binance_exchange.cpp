#include "../include/binance_exchange.h"
#include <iostream> 
#include <sstream>
#include <nlohmann/json.hpp>

using json =  nlohmann::json;

BinanceExchange::BinanceExchange() : curl(nullptr)
{
    name = "Binance";
    connected = false;
    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init(); 
    if(!curl)
    {
        std::cerr << "Failed to initialize CURL" << std::endl;
    }
}
BinanceExchange::~BinanceExchange()
{
    if(curl)
    {
        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();
}
bool BinanceExchange::initialize(const std::string& api_key, const std::string& api_secret)
{
    this->api_key = api_key; 
    this->api_secret = api_secret; 
    std::string url = buildApiUrl("/api/v3/ping");
    std::string response = makeRequest(url);
    if(!response.empty())
    {
        connected = true; 
        return true;
    }
    return false;
}
std::vector<OHLCV> BinanceExchange::fetchHistoricalData(
    const std::string& symbol,
    const std::string& timeframe,
    const std::string& start_time,
    const std::string& end_time
)
{
    std::vector<OHLCV> result;
    std::stringstream ss;
    ss << "/api/v3/klines?symbol=" << symbol << "&interval=" << timeframe;
    if(!start_time.empty())
    {
        ss << "&startTime=" << start_time;
    }
    if(!end_time.empty())
    {
        ss << "&endTime=" << end_time;
    }
    ss << "&limit=1000";
    std::string url = buildApiUrl(ss.str());
    std::string response = makeRequest(url);
    if(response.empty())
    {
        return result;
    }
    try{
        json responseJson = json::parse(response);
        for (const auto& item: responseJson)
        {
            if(item.size() < 6)
            {
                continue;
            }
            OHLCV candle;
            candle.timestamp = item[0].get<long long>();
            candle.open = std::stod(item[1].get<std::string>());
            candle.high = std::stod(item[2].get<std::string>());
            candle.low = std::stod(item[3].get<std::string>());
            candle.close = std::stod(item[4].get<std::string>());
            candle.volume = std::stod(item[5].get<std::string>());
            result.push_back(candle);
        }
    }
    catch(const std::exception& e)
    {
        std::cerr << "Error parsing response: " << e.what() << std::endl;
    }
    return result;
}
double BinanceExchange::getCurrentPrice(const std::string& symbol)
{
    std::string url = buildApiUrl("/api/v3/ticker/price?symbol=" + symbol);
    std::string response = makeRequest(url);
    if(response.empty())
    {
        return 0.0;
    }
    try
    {
        json responseJson = json::parse(response); 
        return std::stod(responseJson["price"].get<std::string>());
    }catch(const std::exception& e)
    {
        std::cerr << "Error parsing response: " << e.what() << std::endl;
        return 0.0;
    }
}
bool BinanceExchange::placeBuyOrder(const std::string& symbol, double quantity, double price)
{
    if (!connected || api_key.empty() || api_secret.empty()) {
        std::cerr << "API credentials not set" << std::endl;
        return false;
    }
    std::stringstream ss;
    ss << "symbol=" << symbol << "&side=BUY&type=";
    if (price > 0) {
        ss << "LIMIT&price=" << price;
    } else {
        ss << "MARKET";
    }
    ss << "&quantity=" << quantity << "&timestamp=" << std::time(nullptr) * 1000;
    std::string data = ss.str();
    std::string signature = signRequest(data);
    data += "&signature=" + signature;
    std::string url = buildApiUrl("/api/v3/order");
    std::string response = makeRequest(url, "POST", data);
    if (response.empty()) {
        return false;
    }
    try{
        json responseJson = json::parse(response);
        return responseJson.contains("orderId");
    }catch(const std::exception& e)
    {
        std::cerr << "Error parsing response: " << e.what() << std::endl;
        return false;
    }
}
bool BinanceExchange::placeSellOrder(const std::string& symbol, double quantity, double price)
{
    if(!connected || api_key.empty() || api_secret.empty())
    {
        std::cerr << "API credentials not set" << std::endl;
        return false;
    }
    std::stringstream ss;
    ss << "symbol=" << symbol << "&side=SELL&type=";
    if(price > 0)
    {
        ss << "LIMIT&price=" << price;
    }
    else
    {
        ss << "MARKET";
    }
    ss << "&quantity=" << quantity << "&timestamp=" << std::time(nullptr) * 1000;
    std::string data = ss.str();
    std::string signature = signRequest(data);
    data += "&signature=" + signature; 

    std::string url = buildApiUrl("/api/v3/order");
    std::string response = makeRequest(url, "POST", data);
    if(response.empty())
    {
        return false;
    }
    try{
        json responseJson = json::parse(response);
        return responseJson.contains("orderId");
    }catch(const std::exception& e)
    {
        std::cerr << "Error parsing response: " << e.what() << std::endl;
        return false;
    }
}
std::vector<Order> BinanceExchange::getOpenOrders(const std::string& symbol)
{
    std::vector<Order> orders;
    if (!connected || api_key.empty() || api_secret.empty()) {
        std::cerr << "API credentials not set" << std::endl;
        return orders;
    }
    
    // Build request data using http query method 
    std::string data = "symbol=" + symbol + "&timestamp=" + std::to_string(std::time(nullptr) * 1000);
    std::string signature = signRequest(data);
    data += "&signature=" + signature;
    
    std::string url = buildApiUrl("/api/v3/openOrders?" + data);
    std::string response = makeRequest(url);
    
    if (response.empty()) {
        return orders;
    }
    try {
        json responseJson = json::parse(response);
        
        for (const auto& item : responseJson) {
            Order order;
            order.orderId = item["orderId"].get<std::string>();
            order.symbol = item["symbol"].get<std::string>();
            order.type = (item["type"] == "LIMIT") ? OrderType::LIMIT : OrderType::MARKET;
            order.side = (item["side"] == "BUY") ? OrderSide::BUY : OrderSide::SELL;
            order.quantity = std::stod(item["origQty"].get<std::string>());
            
            if (item.contains("price")) {
                order.price = std::stod(item["price"].get<std::string>());
            }
            
            order.timestamp = item["time"].get<long long>() / 1000; // Convert from ms to seconds
            order.status = item["status"].get<std::string>();
            
            orders.push_back(order);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error parsing JSON: " << e.what() << std::endl;
    }
    return orders;
}
std::string BinanceExchange::buildApiUrl(const std::string& endpoint) {
    return "https://api.binance.com" + endpoint;
}
std::string BinanceExchange::signRequest(const std::string& data) {
    //TODO: Implement HMAC-SHA256 signing
    // This is a simplified version - in production, implement proper HMAC-SHA256
    // For learning purposes, this returns a placeholder
    return "signature_placeholder";
}
size_t BinanceExchange::WriteCallBack(void* contents, size_t size, size_t nmemb, std::string* s) {
    size_t newLength = size * nmemb;
    try {
        s->append((char*)contents, newLength);
        return newLength;
    } catch (const std::bad_alloc& e) {
        // Handle memory problem
        return 0;
    }
}
std::string BinanceExchange::makeRequest(const std::string& url, const std::string& method, const std::string& data)
{
    if (!curl) {
        std::cerr << "CURL not initialized" << std::endl;
        return "";
    }
    std::string responseString;
    curl_easy_reset(curl);
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallBack);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseString);
    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/x-www-form-urlencoded");
    
    if (!api_key.empty()) {
        std::string authHeader = "X-MBX-APIKEY: " + api_key;
        headers = curl_slist_append(headers, authHeader.c_str());
    }
    
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    
    if (method == "POST") {
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
    } else if (method != "GET") {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method.c_str());
    }
    
    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    
    if (res != CURLE_OK) {
        std::cerr << "CURL error: " << curl_easy_strerror(res) << std::endl;
        return "";
    }
    return responseString;
}