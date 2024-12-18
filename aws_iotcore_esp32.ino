#include "secrets_example.h" // Include your secrets with AWS credentials
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

WiFiClientSecure net; // Create a secure WiFi client
PubSubClient client(net); // Create a PubSub client

void connectAWS() {
    // Connect to Wi-Fi
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connecting to Wi-Fi");
    
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print("."); // Show connection progress
    }
    
    Serial.println("Connected to Wi-Fi");

    // Set MQTT broker details
    client.setServer(AWS_IOT_ENDPOINT, 8883); // Set the AWS IoT endpoint

    // Connect to AWS IoT
    while (!client.connected()) {
        Serial.print("Connecting to AWS IoT...");
        
        // Attempt to connect
        if (client.connect("ESP32Client")) {  // Use a unique client ID
            Serial.println("connected");
        } else {
            Serial.print("failed, rc=");
            Serial.print(client.state()); // Print the connection error code
            Serial.println(" trying again in 5 seconds");
            delay(5000); // Wait 5 seconds before retrying
        }
    }
}

void publishTelemetry(String payload) {
    Serial.print("Publishing: ");
    Serial.println(payload);
    client.publish("pullUpCounter/data", payload.c_str()); // Send the payload as const char*
}

void setup() {
    Serial.begin(115200);
    sonar.ping_cm(); // Initialize the ultrasonic sensor
    connectAWS(); // Connect to AWS IoT
}

void loop() {
    client.loop(); // Keep the MQTT connection alive
    
    // Read distance from the ultrasonic sensor
    unsigned int distance = sonar.ping_cm();
    Serial.print("Distance: ");
    Serial.println(distance); // Log the distance reading
  
    // Counting Logic
    if (distance < COUNT_THRESHOLD) { // Close to the sensor
        if (!lastState) {
            lastState = true; // Update state to 'close'
        }
    } else { // Far from the sensor
        if (lastState) { // Transitioning from 'close' to 'far'
            pullUpCount++; // Count the rep
            lastState = false;

            // Create the payload to be sent to AWS IoT
            String payload = "{\"Records\":[{\"Sns\":{\"Message\":\"{\\\"Count\\\": " + String(pullUpCount) + "}\"}}]}";
            publishTelemetry(payload); // Publish the payload

            Serial.print("Published: ");
            Serial.println(payload); // Log the published payload
        }
    }

    delay(100); // Short delay to avoid flooding MQTT
}