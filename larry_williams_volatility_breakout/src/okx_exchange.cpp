#include "../include/okx_exchange.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <chrono>
#include "websocket_client.h"
#include <algorithm>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

OKXExchange::OKXExchange(): curl(nullptr), websocket(nullptr)
{
    name = "OKX";
    connected = false;
    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    
    if (!curl) {
        std::cerr << "Failed to initialize libcurl for OKX Exchange" << std::endl;
    }
}
OKXExchange::~OKXExchange() {
    disconnectWebSocket();
    if (curl) {
        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();
}
bool OKXExchange::connectWebSocket(const std::string& symbol, const std::string& channel) {
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
            std::cout << "OKX WebSocket " << (connected ? "connected" : "disconnected") << std::endl;
            
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
                    subscribeMsg["args"].push_back({
                        {"channel", "tickers"},
                        {"instId", symbol}
                    });
                } else if (channel.find("candle") == 0) {
                    // Extract interval from channel name (e.g., "candle1m" -> "1m")
                    std::string interval = channel.substr(6);
                    subscribeMsg["args"].push_back({
                        {"channel", "candle" + interval},
                        {"instId", symbol}
                    });
                } else if (channel == "trades") {
                    subscribeMsg["args"].push_back({
                        {"channel", "trades"},
                        {"instId", symbol}
                    });
                }
                
                websocket->send(subscribeMsg.dump());
            }
        });
        
        websocket->setErrorCallback([](const std::string& error) {
            std::cerr << "OKX WebSocket error: " << error << std::endl;
        });
    }
    
    // OKX WebSocket URL
    std::string wsUrl = "wss://ws.okx.com:8443/ws/v5/public";
    
    // Connect to WebSocket
    return websocket->connect(wsUrl);
}

void OKXExchange::disconnectWebSocket() {
    if (websocket) {
        // Send unsubscribe message if connected
        if (websocket->isConnected()) {
            json unsubscribeMsg = {
                {"op", "unsubscribe"},
                {"args", json::array()}
            };
            websocket->send(unsubscribeMsg.dump());
            
            // Give it a moment to process the unsubscribe
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        websocket->disconnect();
    }
}

bool OKXExchange::isWebSocketConnected() const {
    return websocket && websocket->isConnected();
}

void OKXExchange::setRealTimePriceCallback(std::function<void(const std::string&, double)> callback) {
    priceUpdateCallback = callback;
}

void OKXExchange::setRealTimeCandleCallback(std::function<void(const OHLCV&)> callback) {
    candleUpdateCallback = callback;
}

void OKXExchange::handleWebSocketMessage(const std::string& message) {
    try {
        json data = json::parse(message);
        
        // Handle subscription confirmation
        if (data.contains("event") && data["event"] == "subscribe") {
            std::cout << "Successfully subscribed to OKX channel" << std::endl;
            return;
        }
        
        // Handle ping message
        if (data.contains("event") && data["event"] == "ping") {
            // Respond with pong
            json pongMsg = {
                {"event", "pong"}
            };
            websocket->send(pongMsg.dump());
            return;
        }
        
        // Handle data message
        if (data.contains("data") && data.contains("arg")) {
            std::string channel = data["arg"]["channel"];
            std::string symbol = data["arg"]["instId"];
            
            // Process ticker data
            if (channel == "tickers" && priceUpdateCallback) {
                for (const auto& item : data["data"]) {
                    double price = std::stod(item["last"].get<std::string>());
                    priceUpdateCallback(symbol, price);
                }
            }
            // Process candle data
            else if (channel.find("candle") == 0 && candleUpdateCallback) {
                for (const auto& item : data["data"]) {
                    if (item.size() >= 6) {
                        OHLCV candle;
                        candle.timestamp = std::stoll(item[0].get<std::string>()) / 1000; // Convert from ms to s
                        candle.open = std::stod(item[1].get<std::string>());
                        candle.high = std::stod(item[2].get<std::string>());
                        candle.low = std::stod(item[3].get<std::string>());
                        candle.close = std::stod(item[4].get<std::string>());
                        candle.volume = std::stod(item[5].get<std::string>());
                        
                        candleUpdateCallback(candle);
                    }
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error parsing OKX WebSocket message: " << e.what() << std::endl;
    }
}

bool OKXExchange::webSocketLogin() {
    if (!websocket || !websocket->isConnected() || 
        api_key.empty() || api_secret.empty() || passphrase.empty()) {
        return false;
    }
    
    // Create timestamp
    std::string timestamp = getTimestamp();
    
    // Create signature string: timestamp + GET + /users/self/verify
    std::string signString = timestamp + "GET" + "/users/self/verify";
    
    // Generate HMAC-SHA256 signature
    std::string signature = generateHMAC(api_secret, signString);
    
    // Create login message
    json loginMsg = {
        {"op", "login"},
        {"args", json::array({
            {
                {"apiKey", api_key},
                {"passphrase", passphrase},
                {"timestamp", timestamp},
                {"sign", signature}
            }
        })}
    };
    
    // Send login message
    return websocket->send(loginMsg.dump());
}

bool OKXExchange::initialize(const std::string& api_key, const std::string& api_secret) {
    this->api_key = api_key;
    this->api_secret = api_secret;
    
    // Test connection by fetching server time
    std::string url = buildApiUrl("/api/v5/public/time");
    std::string response = makeRequest(url);
    
    if (!response.empty()) {
        try {
            json responseJson = json::parse(response);
            if (responseJson.contains("code") && responseJson["code"] == "0") {
                connected = true;
                std::cout << "Connected to OKX API successfully" << std::endl;
                return true;
            }
        } catch (const std::exception& e) {
            std::cerr << "Error parsing OKX response: " << e.what() << std::endl;
        }
    }
    
    std::cerr << "Failed to connect to OKX API" << std::endl;
    return false;
}
void OKXExchange::setPassphrase(const std::string& passphrase) {
    this->passphrase = passphrase;
}
std::vector<OHLCV> OKXExchange::fetchHistoricalData(const std::string& symbol, 
    const std::string& timeframe,
    const std::string& start_time,
    const std::string& end_time) {
    std::vector<OHLCV> result;

    // Format symbol for OKX (e.g., BTC-USDT)
    std::string formattedSymbol = formatSymbol(symbol);

    // Debug output
    std::cout << "Fetching data for instrument: " << formattedSymbol << std::endl;

    // Map timeframe to OKX format
    std::string okxTimeframe;
    if (timeframe == "1m") okxTimeframe = "1m";
    else if (timeframe == "5m") okxTimeframe = "5m";
    else if (timeframe == "15m") okxTimeframe = "15m";
    else if (timeframe == "30m") okxTimeframe = "30m";
    else if (timeframe == "1h") okxTimeframe = "1H";
    else if (timeframe == "4h") okxTimeframe = "4H";
    else if (timeframe == "1d") okxTimeframe = "1D";
    else {
        std::cerr << "Unsupported timeframe: " << timeframe << std::endl;
        return result;
    }

// Build API endpoint
    std::stringstream ss;
    ss << "/api/v5/market/candles?instId=" << formattedSymbol;
    ss << "&bar=" << okxTimeframe;

    if (!start_time.empty()) {
        ss << "&after=" << start_time;
    }

    if (!end_time.empty()) {
        ss << "&before=" << end_time;
    }

    ss << "&limit=100"; // OKX has a maximum limit of 100

    std::string url = buildApiUrl(ss.str());
    std::cout << "Request URL: " << url << std::endl; // Debug output

    std::string response = makeRequest(url);

    if (response.empty()) {
        std::cerr << "Empty response from OKX API" << std::endl;
        return result;
    }

    try {
        json responseJson = json::parse(response);

        if (responseJson.contains("code") && responseJson["code"] == "0" && responseJson.contains("data")) {
            for (const auto& item : responseJson["data"]) {
                if (item.size() < 6) continue;

                OHLCV candle;
                // OKX format: [0]=timestamp, [1]=open, [2]=high, [3]=low, [4]=close, [5]=volume
                candle.timestamp = std::stoll(item[0].get<std::string>()) / 1000; // Convert from ms to s
                candle.open = std::stod(item[1].get<std::string>());
                candle.high = std::stod(item[2].get<std::string>());
                candle.low = std::stod(item[3].get<std::string>());
                candle.close = std::stod(item[4].get<std::string>());
                candle.volume = std::stod(item[5].get<std::string>());

                result.push_back(candle);
            }
    } else {
            std::cerr << "Error in OKX API response: " << response << std::endl;
        }
    }catch (const std::exception& e) {
        std::cerr << "Error parsing OKX response: " << e.what() << std::endl;
    }

    return result;
}
double OKXExchange::getCurrentPrice(const std::string& symbol) {
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
        std::cerr << "Error parsing OKX response: " << e.what() << std::endl;
    }
    
    return 0.0;
}
bool OKXExchange::placeBuyOrder(const std::string& symbol, double quantity, double price) {
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
        std::cerr << "Error parsing OKX response: " << e.what() << std::endl;
    }
    return false;
}
bool OKXExchange::placeSellOrder(const std::string& symbol, double quantity, double price) {
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
        std::cerr << "Error parsing OKX response: " << e.what() << std::endl;
    }
    
    return false;
}
std::vector<Order> OKXExchange::getOpenOrders(const std::string& symbol) {
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
                
                // Map OKX order type to our enum
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
        std::cerr << "Error parsing OKX response: " << e.what() << std::endl;
    }
    return orders;
}
std::string OKXExchange::buildApiUrl(const std::string& endpoint) {
    return "https://www.okx.com" + endpoint;
}

std::string OKXExchange::signRequest(const std::string& data) {
    // OKX requires HMAC-SHA256 signing
    return generateHMAC(api_secret, data);
}
std::string OKXExchange::formatSymbol(const std::string& symbol) {
    // OKX uses specific format: BTC-USDT
    
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

std::string OKXExchange::generateHMAC(const std::string& key, const std::string& data) {
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
std::string OKXExchange::getTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto as_time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    
    std::tm tm = *std::gmtime(&as_time_t);
    char buffer[32];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%S", &tm);
    
    std::stringstream ss;
    ss << buffer << "." << std::setw(3) << std::setfill('0') << ms.count() << "Z";
    
    return ss.str();
}

size_t OKXExchange::WriteCallback(void* contents, size_t size, size_t nmemb, std::string* s) {
    size_t newLength = size * nmemb;
    try {
        s->append((char*)contents, newLength);
        return newLength;
    } catch (const std::bad_alloc& e) {
        // Handle memory problem
        return 0;
    }
}

std::string OKXExchange::makeRequest(const std::string& url, const std::string& method, 
                                     const std::string& data, const std::string& timestamp) {
    if (!curl) {
        std::cerr << "CURL not initialized" << std::endl;
        return "";
    }
    
    std::string responseString;
    std::string actualTimestamp = timestamp.empty() ? getTimestamp() : timestamp;
    
    curl_easy_reset(curl);
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseString);
    
    struct curl_slist* headers = NULL;
    
    // For authenticated requests
    if (!api_key.empty() && !api_secret.empty()) {
        // OKX authentication headers
        std::string signPath = url.substr(url.find("/api"));
        if (signPath.find('?') != std::string::npos) {
            signPath = signPath.substr(0, signPath.find('?'));
        }
        
        std::string signData = actualTimestamp + method + signPath;
        if (!data.empty()) {
            signData += data;
        }
        
        std::string signature = signRequest(signData);
        
        headers = curl_slist_append(headers, ("OK-ACCESS-KEY: " + api_key).c_str());
        headers = curl_slist_append(headers, ("OK-ACCESS-SIGN: " + signature).c_str());
        headers = curl_slist_append(headers, ("OK-ACCESS-TIMESTAMP: " + actualTimestamp).c_str());
        headers = curl_slist_append(headers, ("OK-ACCESS-PASSPHRASE: " + passphrase).c_str());
    }
    
    // Common headers
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    
    // Set HTTP method and data
    if (method == "POST") {
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        if (!data.empty()) {
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
        }
    } else if (method != "GET") {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method.c_str());
        if (!data.empty()) {
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
        }
    }
    
    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    
    if (res != CURLE_OK) {
        std::cerr << "CURL error: " << curl_easy_strerror(res) << std::endl;
        return "";
    }
    
    return responseString;
}