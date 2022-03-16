#include "Connectivity.h"
#include <WiFi.h>

bool Connectivity::begin(PubSubClient* pubSubClient) {
    _pubSubClient = pubSubClient;
    return true;
}

const bool Connectivity::hasNetworkConnection() {
    return WiFi.status() == WL_CONNECTED;
}

const bool Connectivity::hasMessagingConnection() {
    return _pubSubClient->connected();
}

const String Connectivity::getIPAddress() {
    return WiFi.localIP().toString();
}
