#pragma once

#define MQTT_MAX_RETRIES 5
#define WIFI_CONNECT_TIMEOUT 15000

#define I2C_SDA 2
#define I2C_SCL 1

#define MQTT_BROKER_HOST "broker.mqtthq.com"
#define MQTT_BROKER_PORT 1883
#define MQTT_TOPIC "foo/bar/baz/%s"

// Number of seconds between reports
#define REPORT_INTERVAL 5000