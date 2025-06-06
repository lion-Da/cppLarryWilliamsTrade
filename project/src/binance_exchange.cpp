#include "binance_exchange.h"
#include "websocket_client.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <chrono>
#include <openssl/hmac.h>
#include <nlohmann/json.hpp>
#include <algorithm>
#include "env_loader.h"
using json =  nlohmann::json;

BinanceExchange::BinanceExchange() : curl(nullptr), websocket(nullptr)
{
    apiKey = EnvLoader::get("BINANCE_API_KEY");
    apiSecret = EnvLoader::get("BINANCE_API_SECRET");
    if (apiKey.empty() || apiSecret.empty()) {
        std::cerr << "Warning: Binance API credentials not found in environment variables" << std::endl;
    }
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
    disconnectWebSocket();
    if(curl)
    {
        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();
}
// Implement or update WebSocket methods
bool BinanceExchange::connectWebSocket(const std::string& symbol, const std::string& channel) {
    // Create WebSocket client if needed
    if (!websocket) {
        websocket = std::make_unique<WebSocketClient>();
        
        if (!websocket->initialize()) {
            std::cerr << "Failed to initialize WebSocket client" << std::endl;
            return false;
        }
        
        // Set callbacks
        websocket->setMessageCallback([this](const std::string& msg) {
            handleWebSocketMessage(msg);
        });
        
        websocket->setConnectionCallback([this](bool connected) {
            std::cout << "Binance WebSocket " << (connected ? "connected" : "disconnected") << std::endl;
        });
        
        websocket->setErrorCallback([](const std::string& error) {
            std::cerr << "Binance WebSocket error: " << error << std::endl;
        });
    }
    
    // Convert symbol to lowercase (Binance requirement)
    std::string lowercaseSymbol = symbol;
    std::transform(lowercaseSymbol.begin(), lowercaseSymbol.end(), lowercaseSymbol.begin(), ::tolower);
    
    // Remove any special characters like "/"
    lowercaseSymbol.erase(std::remove(lowercaseSymbol.begin(), lowercaseSymbol.end(), '/'), lowercaseSymbol.end());
    lowercaseSymbol.erase(std::remove(lowercaseSymbol.begin(), lowercaseSymbol.end(), '-'), lowercaseSymbol.end());
    
    // Build WebSocket URL based on the channel
    std::string wsUrl;
    
    if (channel == "ticker") {
        wsUrl = "wss://stream.binance.com:9443/ws/" + lowercaseSymbol + "@ticker";
    } else if (channel == "trade") {
        wsUrl = "wss://stream.binance.com:9443/ws/" + lowercaseSymbol + "@trade";
    } else if (channel.find("kline_") == 0) {
        std::string interval = channel.substr(6); // Extract interval part
        wsUrl = "wss://stream.binance.com:9443/ws/" + lowercaseSymbol + "@kline_" + interval;
    } else {
        std::cerr << "Unsupported channel: " << channel << std::endl;
        return false;
    }
    
    std::cout << "Connecting to Binance WebSocket URL: " << wsUrl << std::endl;
    
    // Connect to WebSocket
    return websocket->connect(wsUrl);
}

void BinanceExchange::disconnectWebSocket() {
    if (websocket) {
        websocket->disconnect();
    }
}

bool BinanceExchange::isWebSocketConnected() const {
    return websocket && websocket->isConnected();
}
void BinanceExchange::setRealTimePriceCallback(std::function<void(const std::string&, double)> callback) {
    priceUpdateCallback = callback;
}

void BinanceExchange::setRealTimeCandleCallback(std::function<void(const OHLCV&)> callback) {
    candleUpdateCallback = callback;
}

void BinanceExchange::handleWebSocketMessage(const std::string& message) {
    try {
        json data = json::parse(message);
        
        // Process ticker message
        if (data.contains("e") && data["e"] == "24hrTicker") {
            if (priceUpdateCallback && data.contains("s") && data.contains("c")) {
                std::string symbol = data["s"];
                double price = std::stod(data["c"].get<std::string>());
                priceUpdateCallback(symbol, price);
            }
        }
        // Process kline/candlestick message
        else if (data.contains("e") && data["e"] == "kline" && data.contains("k")) {
            if (candleUpdateCallback) {
                json k = data["k"];
                
                OHLCV candle;
                candle.timestamp = k["t"].get<long long>() / 1000; // Convert from ms to s
                candle.open = std::stod(k["o"].get<std::string>());
                candle.high = std::stod(k["h"].get<std::string>());
                candle.low = std::stod(k["l"].get<std::string>());
                candle.close = std::stod(k["c"].get<std::string>());
                candle.volume = std::stod(k["v"].get<std::string>());
                
                candleUpdateCallback(candle);
            }
        }
        // Process trade message
        else if (data.contains("e") && data["e"] == "trade") {
            // Handle trade updates if needed
            std::cout << "Trade update received (not processed): " << message << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error parsing Binance WebSocket message: " << e.what() << std::endl;
    }
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
    const std::string& apiSecret = this->apiSecret;
    
    // Output buffer for the HMAC result
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hashLen;
    
    // Calculate HMAC-SHA256
    HMAC(EVP_sha256(), 
         apiSecret.c_str(),                // Secret key
         static_cast<int>(apiSecret.length()),               // Length of the secret key
         reinterpret_cast<const unsigned char*>(data.c_str()), // Data to sign
         data.length(),                    // Length of data
         hash,                             // Output hash
         &hashLen);                        // Output hash length
    
    // Convert the binary hash to a hex string
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    
    for (unsigned int i = 0; i < hashLen; i++) {
        ss << std::setw(2) << static_cast<unsigned int>(hash[i]);
    }
    
    return ss.str();
}
size_t BinanceExchange::WriteCallBack(void* contents, size_t size, size_t nmemb, std::string* s) {
    size_t newLength = size * nmemb;
    try {
        s->append((char*)contents, newLength);
        return newLength;
    } catch (...) {
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