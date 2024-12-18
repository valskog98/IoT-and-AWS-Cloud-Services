# Pull-Up Counter IoT Project

## Project Overview

This project leverages an **ESP32** microcontroller paired with an **HC-SR04** ultrasonic sensor to create a pull-up counter that records the number of pull-ups performed. The setup integrates AWS services to transmit, process, and visualize fitness data, providing insights into workout performance.

## Table of Contents

- [Hardware Components](#hardware-components)
- [Software and Tools](#software-and-tools)
- [Setup ESP32](#setup-esp32)
- [AWS Setup](#aws-setup)
- [AWS Lambda Function](#aws-lambda-function)
- [Data Visualization with AWS QuickSight](#data-visualization-with-aws-quicksight)
- [Security and Scalability](#security-and-scalability)
- [Conclusion](#conclusion)

## Hardware Components

- **ESP32 Development Board**
- **HC-SR04 Ultrasonic Sensor**
- **Jumper Wires**
- **Breadboard** (optional)

## Software and Tools

- **AWS Services**:
  - AWS Lambda
  - Amazon Timestream
  - AWS IoT Core
  - AWS QuickSight
  
- **Development Environment**:
  - Arduino IDE

## Setup ESP32

The ESP32 is programmed using the Arduino IDE with the following key libraries: HTTPClient, WiFiClientSecure, PubSubClient, and NewPing for handling MQTT communication, Wi-Fi connectivity, and distance measurement.

```cpp
#include "secrets_example.h" // AWS credentials
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <NewPing.h>

// Configuration for the ultrasonic sensor
#define TRIGGER_PIN  23
#define ECHO_PIN     22
#define COUNT_THRESHOLD 15 

NewPing sonar(TRIGGER_PIN, ECHO_PIN, 200); // Ultrasonic sensor setup

// Variables
int pullUpCount = 0; 
bool lastState = false;

WiFiClientSecure net;
PubSubClient client(net); // Create a PubSub client

void connectAWS() {
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connecting to Wi-Fi");
    
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("Connected to Wi-Fi");
    
    client.setServer(AWS_IOT_ENDPOINT, 8883); // Set the AWS IoT endpoint
    while (!client.connected()) {
        Serial.print("Connecting to AWS IoT...");
        if (client.connect("ESP32Client")) {
            Serial.println("connected");
        } else {
            Serial.print("Failed, rc=");
            Serial.print(client.state());
            Serial.println(" trying again in 5 seconds");
            delay(5000);
        }
    }
}

void publishTelemetry(String payload) {
    Serial.print("Publishing: ");
    Serial.println(payload);
    client.publish("pullUpCounter/data", payload.c_str());
}

void setup() {
    Serial.begin(115200);
    sonar.ping_cm();
    connectAWS();
}

void loop() {
    client.loop();
    unsigned int distance = sonar.ping_cm();
  
    if (distance < COUNT_THRESHOLD) {
        if (!lastState) {
            lastState = true; // Update state to 'close'
        }
    } else {
        if (lastState) {
            pullUpCount++;
            lastState = false;
            String payload = "{\"Records\":[{\"Sns\":{\"Message\":\"{\\\"Count\\\": " + String(pullUpCount) + "}\"}}]}";
            publishTelemetry(payload);
            Serial.print("Published: ");
            Serial.println(payload);
        }
    }

    delay(100); // Short delay
}
