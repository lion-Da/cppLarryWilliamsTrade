#include "bybit_exchange.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <chrono>
#include "websocket_client.h"
#include <algorithm>
#include <nlohmann/json.hpp>
#include "env_loader.h"
using json = nlohmann::json;

BybitExchange::BybitExchange(): curl(nullptr), websocket(nullptr)
{

    apiKey = EnvLoader::get("Bybit_API_KEY");
    apiSecret = EnvLoader::get("Bybit_API_SECRET");
    connected = false;

    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    
    if (!curl) {
        std::cerr << "Failed to initialize libcurl for Bybit Exchange" << std::endl;
    }
}
BybitExchange::~BybitExchange() {
    disconnectWebSocket();
    if (curl) {
        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();
}
bool BybitExchange::connectWebSocket(const std::string& symbol, const std::string& channel) {
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
        
        websocket->setConnectionCallback([this,&channel, &symbol](bool connected) {
            std::cout << "Bybit WebSocket " << (connected ? "connected" : "disconnected") << std::endl;
            
            // Subscribe to channels after connection is established
            if (connected) {
                // Add a small delay to ensure the connection is fully established
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                
                // Send subscription message
                json subscribeMsg = {
                    {"op", "subscribe"},
                    {"args", json::array()}
                };
                
                // Add subscription args
                if (channel == "tickers") {
                    subscribeMsg["args"].push_back(channel + "." + symbol);
                } 
                else if (channel == "orderbook") {
                    subscribeMsg["args"].push_back(channel + ".1." + symbol);
                }
                
                websocket->send(subscribeMsg.dump());
            }
        });
        
        websocket->setErrorCallback([](const std::string& error) {
            std::cerr << "Bybit WebSocket error: " << error << std::endl;
        });
    }
    
    // Bybit WebSocket URL
    std::string wsUrl = "wss://stream.bybit.com/v5/public/spot";
    
    // Connect to WebSocket
    return websocket->connect(wsUrl);
}

void BybitExchange::disconnectWebSocket() {
    if (websocket) {
        websocket->disconnect();
    }
}

bool BybitExchange::isWebSocketConnected() const {
    return websocket && websocket->isConnected();
}

void BybitExchange::setRealTimePriceCallback(std::function<void(const std::string&, double, const std::string& ts)> callback) {
    priceUpdateCallback = callback;
}

void BybitExchange::setRealTimeCandleCallback(std::function<void(const OHLCV&)> callback) {
    candleUpdateCallback = callback;
}
void BybitExchange::setRealTimeOrderBookCallback(std::function<void(const CommonFormatData&)> callback) {
    orderbookCallback = callback;
}
void BybitExchange::handleWebSocketMessage(const std::string& message) {
    try {
        json data = json::parse(message);
        /*
        Bybit WebSocket message: {
            "cs": 76686182593,
            "data": {
                "highPrice24h": "110389.9",
                "lastPrice": "109656.1",
                "lowPrice24h": "108334.1",
                "prevPrice24h": "109673.6",
                "price24hPcnt": "-0.0002",
                "symbol": "BTCUSDT",
                "turnover24h": "883199669.53583323",
                "usdIndexPrice": "109666.207928",
                "volume24h": "8073.70254"
            },
            "topic": "tickers.BTCUSDT",
            "ts": 1749646866830,
            "type": "snapshot"
        } 
        */    
        
        // Handle data message
        if (data.contains("data") && data.contains("topic")) {
            std::string channel;
            std::string symbol;
            //"topic": "tickers.BTCUSDT"
            std::string topic = data["topic"];
            // get channel and synmbol from topic
            if (topic.find('.') != std::string::npos) {
                channel = topic.substr(0, topic.find('.'));
                symbol = topic.substr(topic.find('.') + 1);
            }

            // Process ticker data
            if (channel == "tickers" && priceUpdateCallback) {
                const auto& data_element = data["data"];
                if(data_element.contains("lastPrice") && data_element["lastPrice"].is_string()) {
                    // Extract last price from ticker data
                    double price = std::stod(data_element["lastPrice"].get<std::string>());
                   
                    priceUpdateCallback(symbol, price, std::to_string(data["ts"].get<int64_t>()));
                }
            }
            else if (channel == "orderbook" && orderbookCallback) {
                // Handle order book updates
                // std::cout << "Order book update for " << symbol << ": " << data.dump(4) << std::endl;
                CommonFormatData orderbook_data;
                orderbook_data.exchange = "Bybit";
                orderbook_data.symbol = symbol.substr(symbol.find('.') + 1); // Remove "1." prefix
                orderbook_data.timestamp = data["ts"].get<int64_t>();

                if(data["data"].contains("a") && data["data"].contains("b")) {
                    try
                    {
                        const auto& asks = data["data"]["a"];
                        const auto& bids = data["data"]["b"];
                        
                        for(const auto& ask : asks) {
                            orderbook_data.asks.push_back(ask[0].get<std::string>());
                        }

                        for(const auto& bid : bids) {
                            orderbook_data.bids.push_back(bid[0].get<std::string>());
                        }

                        orderbookCallback(orderbook_data);
                    }
                    catch(const std::exception& e)
                    {
                        std::cerr << e.what() << '\n';
                    }
                }

                /*
                Order book update for 1.BTCUSDT: {
                    "cts": 1750148961525,
                    "data": {
                        "a": [
                            [
                                "106798",
                                "1.10634"
                            ]
                        ],
                        "b": [
                            [
                                "106797.9",
                                "0.429797"
                            ]
                        ],
                        "s": "BTCUSDT",
                        "seq": 77377992351,
                        "u": 293507
                    },
                    "topic": "orderbook.1.BTCUSDT",
                    "ts": 1750148961528,
                    "type": "snapshot"
                }
                */
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error parsing Bybit WebSocket message: " << e.what() << std::endl;
    }
}

bool BybitExchange::initialize(const std::string& api_key, const std::string& api_secret) {
    this->api_key = api_key;
    this->api_secret = api_secret;
    
    // Test connection by fetching server time
    std::string url = buildApiUrl("/v5/asset/withdraw/withdrawable-amount?coin=USDT");
    std::string response = makeRequest(url);
    
    if (!response.empty()) {
        try {
            json responseJson = json::parse(response);
            std::cout << "Bybit API response: " << responseJson.dump(4) << std::endl; // Debug output
            if (responseJson.contains("retCode") && responseJson["retCode"] == 0 && responseJson.contains("retMsg") && 
                responseJson["retMsg"] == "success") {
                connected = true;
                std::cout << "Connected to Bybit API successfully" << std::endl;
                return true;
            }
        } catch (const std::exception& e) {
            std::cerr << "Error parsing Bybit response: " << e.what() << std::endl;
        }
    }
    
    std::cerr << "Failed to connect to Bybit API" << std::endl;
    return false;
}

std::vector<OHLCV> BybitExchange::fetchHistoricalData(const std::string& symbol, 
    const std::string& timeframe,
    const std::string& start_time,
    const std::string& end_time) {
    std::vector<OHLCV> result;

    // Format symbol for Bybit (e.g., BTC-USDT)
    std::string formattedSymbol = formatSymbol(symbol);

    // Debug output
    std::cout << "Fetching data for instrument: " << formattedSymbol << std::endl;

    // Map timeframe to Bybit format
    std::string BybitTimeframe;
    if (timeframe == "1m") BybitTimeframe = "1m";
    else if (timeframe == "5m") BybitTimeframe = "5m";
    else if (timeframe == "15m") BybitTimeframe = "15m";
    else if (timeframe == "30m") BybitTimeframe = "30m";
    else if (timeframe == "1h") BybitTimeframe = "1H";
    else if (timeframe == "4h") BybitTimeframe = "4H";
    else if (timeframe == "1d") BybitTimeframe = "1D";
    else {
        std::cerr << "Unsupported timeframe: " << timeframe << std::endl;
        return result;
    }

// Build API endpoint
    std::stringstream ss;
    ss << "/api/v5/market/candles?instId=" << formattedSymbol;
    ss << "&bar=" << BybitTimeframe;

    if (!start_time.empty()) {
        ss << "&after=" << start_time;
    }

    if (!end_time.empty()) {
        ss << "&before=" << end_time;
    }

    ss << "&limit=100"; // Bybit has a maximum limit of 100

    std::string url = buildApiUrl(ss.str());
    std::cout << "Request URL: " << url << std::endl; // Debug output

    std::string response = makeRequest(url);

    if (response.empty()) {
        std::cerr << "Empty response from Bybit API" << std::endl;
        return result;
    }

    try {
        json responseJson = json::parse(response);

        if (responseJson.contains("code") && responseJson["code"] == "0" && responseJson.contains("data")) {
            for (const auto& item : responseJson["data"]) {
                if (item.size() < 6) continue;

                OHLCV candle;
                // Bybit format: [0]=timestamp, [1]=open, [2]=high, [3]=low, [4]=close, [5]=volume
                candle.timestamp = std::stoll(item[0].get<std::string>()) / 1000; // Convert from ms to s
                candle.open = std::stod(item[1].get<std::string>());
                candle.high = std::stod(item[2].get<std::string>());
                candle.low = std::stod(item[3].get<std::string>());
                candle.close = std::stod(item[4].get<std::string>());
                candle.volume = std::stod(item[5].get<std::string>());

                result.push_back(candle);
            }
    } else {
            std::cerr << "Error in Bybit API response: " << response << std::endl;
        }
    }catch (const std::exception& e) {
        std::cerr << "Error parsing Bybit response: " << e.what() << std::endl;
    }

    return result;
}
double BybitExchange::getCurrentPrice(const std::string& symbol) {
    std::string formattedSymbol = formatSymbol(symbol);
    std::string url = buildApiUrl("/api/v5/market/ticker?instId=" + formattedSymbol);
    std::string response = makeRequest(url);
    
    if (response.empty()) {
        return 0.0;
    }
    
    try {
        json responseJson = json::parse(response);
        
        if (responseJson.contains("code") && responseJson["code"] == "0" && 
            responseJson.contains("data") && !responseJson["data"].empty()) {
            
            // Extract last price from the ticker data
            return std::stod(responseJson["data"][0]["last"].get<std::string>());
        }
    } catch (const std::exception& e) {
        std::cerr << "Error parsing Bybit response: " << e.what() << std::endl;
    }
    
    return 0.0;
}
bool BybitExchange::placeBuyOrder(const std::string& symbol, double quantity, double price) {
    if (!connected || api_key.empty() || api_secret.empty()) {
        std::cerr << "API credentials not set" << std::endl;
        return false;
    }
    
    std::string formattedSymbol = formatSymbol(symbol);
    std::string timestamp = getTimestamp();
    
    // Create order data
    json orderData;
    orderData["instId"] = formattedSymbol;
    orderData["tdMode"] = "cash"; // Cash mode (alternatives: isolated, cross)
    orderData["side"] = "buy";
    orderData["ordType"] = (price > 0) ? "limit" : "market";
    orderData["sz"] = std::to_string(quantity);
    if (price > 0) {
        orderData["px"] = std::to_string(price);
    }
    std::string requestBody = orderData.dump();
    // Build URL and make request
    std::string url = buildApiUrl("/api/v5/trade/order");
    std::string response = makeRequest(url, "POST", requestBody, timestamp);
    if (response.empty()) {
        return false;
    }
    try {
        json responseJson = json::parse(response);
        
        if (responseJson.contains("code") && responseJson["code"] == "0") {
            return true;
        } else {
            std::cerr << "Error placing buy order: " << response << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error parsing Bybit response: " << e.what() << std::endl;
    }
    return false;
}
bool BybitExchange::placeSellOrder(const std::string& symbol, double quantity, double price) {
    if (!connected || api_key.empty() || api_secret.empty()) {
        std::cerr << "API credentials not set" << std::endl;
        return false;
    }
    
    std::string formattedSymbol = formatSymbol(symbol);
    std::string timestamp = getTimestamp();
    
    // Create order data
    json orderData;
    orderData["instId"] = formattedSymbol;
    orderData["tdMode"] = "cash"; // Cash mode
    orderData["side"] = "sell";
    orderData["ordType"] = (price > 0) ? "limit" : "market";
    orderData["sz"] = std::to_string(quantity);
    
    if (price > 0) {
        orderData["px"] = std::to_string(price);
    }
    
    std::string requestBody = orderData.dump();
    
    // Build URL and make request
    std::string url = buildApiUrl("/api/v5/trade/order");
    std::string response = makeRequest(url, "POST", requestBody, timestamp);
    
    if (response.empty()) {
        return false;
    }
    
    try {
        json responseJson = json::parse(response);
        
        if (responseJson.contains("code") && responseJson["code"] == "0") {
            return true;
        } else {
            std::cerr << "Error placing sell order: " << response << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error parsing Bybit response: " << e.what() << std::endl;
    }
    
    return false;
}
std::vector<Order> BybitExchange::getOpenOrders(const std::string& symbol) {
    std::vector<Order> orders;
    
    if (!connected || api_key.empty() || api_secret.empty()) {
        std::cerr << "API credentials not set" << std::endl;
        return orders;
    }
    
    std::string formattedSymbol = formatSymbol(symbol);
    std::string timestamp = getTimestamp();
    
    // Build URL
    std::string path = "/api/v5/trade/orders-pending?instId=" + formattedSymbol;
    std::string url = buildApiUrl(path);
    
    // Make request
    std::string response = makeRequest(url, "GET", "", timestamp);
    
    if (response.empty()) {
        return orders;
    }
    
    try {
        json responseJson = json::parse(response);
        
        if (responseJson.contains("code") && responseJson["code"] == "0" && 
            responseJson.contains("data")) {
            
            for (const auto& item : responseJson["data"]) {
                Order order;
                order.orderId = item["ordId"].get<std::string>();
                order.symbol = item["instId"].get<std::string>();
                
                // Map Bybit order type to our enum
                std::string ordType = item["ordType"].get<std::string>();
                if (ordType == "limit") {
                    order.type = OrderType::LIMIT;
                } else if (ordType == "market") {
                    order.type = OrderType::MARKET;
                }
                
                // Map side
                std::string side = item["side"].get<std::string>();
                order.side = (side == "buy") ? OrderSide::BUY : OrderSide::SELL;
                
                // Extract quantities and price
                order.quantity = std::stod(item["sz"].get<std::string>());
                
                if (item.contains("px") && !item["px"].get<std::string>().empty()) {
                    order.price = std::stod(item["px"].get<std::string>());
                }
                
                // Convert timestamp
                if (item.contains("cTime")) {
                    order.timestamp = std::stoll(item["cTime"].get<std::string>()) / 1000;
                }
                
                order.status = item["state"].get<std::string>();
                
                orders.push_back(order);
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error parsing Bybit response: " << e.what() << std::endl;
    }
    return orders;
}
std::string BybitExchange::buildApiUrl(const std::string& endpoint) {
    return "https://api.bybit.com" + endpoint;
}

std::string BybitExchange::signRequest(const std::string& data) {
    // Bybit requires HMAC-SHA256 signing
    return generateHMAC(apiSecret, data);
}
std::string BybitExchange::formatSymbol(const std::string& symbol) {
    // Bybit uses specific format: BTC-USDT
    
    // If already in correct format (contains a dash)
    if (symbol.find('-') != std::string::npos) {
        return symbol;
    }
    
    // If in format like BTC/USDT
    if (symbol.find('/') != std::string::npos) {
        std::string result = symbol;
        std::replace(result.begin(), result.end(), '/', '-');
        return result;
    }
    
    // For format like BTCUSDT - find the base/quote boundary
    // Common quote currencies
    std::vector<std::string> quotes = {"USDT", "USD", "BTC", "ETH", "USDC"};
    
    for (const auto& quote : quotes) {
        if (symbol.length() > quote.length() && 
            symbol.substr(symbol.length() - quote.length()) == quote) {
            // Found a matching quote currency
            std::string base = symbol.substr(0, symbol.length() - quote.length());
            return base + "-" + quote;
        }
    }
    
    // If we can't determine the format, return as-is
    return symbol;
}

std::string BybitExchange::generateHMAC(const std::string& key, const std::string& data) {
    unsigned char* digest = HMAC(EVP_sha256(), 
                                key.c_str(), key.length(),
                                reinterpret_cast<const unsigned char*>(data.c_str()), data.length(),
                                NULL, NULL);
    
    std::stringstream ss;
    for (int i = 0; i < 32; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(digest[i]);
    }
    
    return ss.str();
}
std::string BybitExchange::getTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(now);
    long long timestamp_ms = now_ms.time_since_epoch().count();
    return std::to_string(timestamp_ms);
}

size_t BybitExchange::WriteCallback(void* contents, size_t size, size_t nmemb, std::string* s) {
    size_t newLength = size * nmemb;
    try {
        s->append((char*)contents, newLength);
        return newLength;
    } catch (const std::bad_alloc& e) {
        // Handle memory problem
        return 0;
    }
}

std::string BybitExchange::makeRequest(const std::string& url, const std::string& method, 
                                     const std::string& data, const std::string& timestamp) {
    if (!curl) {
        std::cerr << "CURL not initialized" << std::endl;
        return "";
    }
    const std::string recvWindow = "50000";
    const std::string queryString = "coin=USDT";

    std::string responseString;
    std::string actualTimestamp = timestamp.empty() ? getTimestamp() : timestamp;
    const std::string API_SIGN = signRequest(actualTimestamp + apiKey + recvWindow + queryString);

    curl_easy_reset(curl);
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseString);
    
    struct curl_slist* headers = NULL;
    
    // For authenticated requests
    headers = curl_slist_append(headers, ("X-BAPI-API-KEY: " + apiKey).c_str());
    headers = curl_slist_append(headers, ("X-BAPI-TIMESTAMP: " + actualTimestamp).c_str());
    headers = curl_slist_append(headers, ("X-BAPI-RECV-WINDOW: " + recvWindow).c_str());
    headers = curl_slist_append(headers, ("X-BAPI-SIGN: " + API_SIGN).c_str());
    // Common headers
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    
    
    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(headers);

    nlohmann::json response;
    if (res != CURLE_OK) 
    {
        std::cerr << "CURL error: " << curl_easy_strerror(res) << std::endl;
        return "";
    }
    else {
        try {
            response = nlohmann::json::parse(responseString);
        } catch (const std::exception& e) {
            std::cerr << "Failed to parse JSON: " << e.what() << std::endl;
            return 0;
        }
    }

    return responseString;
}