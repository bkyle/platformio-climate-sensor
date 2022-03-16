#pragma once

#include "Sensors.h"
#include <PubSubClient.h>

class Connectivity {
  private:
    PubSubClient *_pubSubClient;
  public:
    bool begin(PubSubClient *pubSubClient);

    const bool hasNetworkConnection();
    const String getIPAddress();
    const bool hasMessagingConnection();
};