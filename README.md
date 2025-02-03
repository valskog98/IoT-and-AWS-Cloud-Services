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

```
AWS Setup
Setting Up AWS Services
When creating an account with AWS, it's essential to select a region that supports all required services. In this project, the Ireland region was chosen, as it provides access to Timestream alongside other necessary AWS functionalities.

AWS IoT Core:

Created an IoT Thing in AWS IoT Core to manage the ESP32 device.
Generated device security certificates for secure communication.
Established a policy for the device to connect, publish, and receive messages.
Example Policy in JSON:

```json
Copy
{
    "Version": "2012-10-17",
    "Statement": [
        {
            "Effect": "Allow",
            "Action": "iot:Connect",
            "Resource": "*"
        },
        {
            "Effect": "Allow",
            "Action": "iot:Publish",
            "Resource": "arn:aws:iot:eu-west-1:YOUR_ACCOUNT_ID:topic/pullUpCounter/data"
        },
        {
            "Effect": "Allow",
            "Action": "iot:Subscribe",
            "Resource": "arn:aws:iot:eu-west-1:YOUR_ACCOUNT_ID:topicfilter/pullUpCounter/data"
        },
        {
            "Effect": "Allow",
            "Action": "iot:Receive",
            "Resource": "*"
        }
    ]
}
```
Amazon Timestream:

Created a Timestream database named PullUpCounterDB to store pull-up count records.
Created a table named PullUpData within the database to hold the data records.
AWS Lambda Function
To process incoming data and store it in Timestream, an AWS Lambda function was set up that is triggered by messages published to the IoT topic.

Example Lambda Function Code
```python
Copy
import json
import boto3
from datetime import datetime

timestream_client = boto3.client('timestream-write')
DATABASE_NAME = 'PullUpCounterDB'
TABLE_NAME = 'PullUpData'

def lambda_handler(event, context):
    print("Received event: ", json.dumps(event))
    
    for record in event['Records']:
        payload = json.loads(record['Sns']['Message'])
        count = payload.get('Count')

        if count is None:
            print("Error: Count not found in the payload.")
            return {
                'statusCode': 400,
                'body': json.dumps('Count not found.')
            }

        print(f"Processing Count: {count}")

        timestamp = int(datetime.now().timestamp() * 1000)  # Current time in milliseconds

        record_data = {
            'Dimensions': [
                {'Name': 'DeviceID', 'Value': 'YourDeviceID'},  # Modify as necessary
            ],
            'MeasureName': 'PullUpCount',
            'MeasureValue': str(count),
            'MeasureValueType': 'DOUBLE',
            'Time': str(timestamp)
        }

        try:
            response = timestream_client.write_records(
                DatabaseName=DATABASE_NAME,
                TableName=TABLE_NAME,
                Records=[record_data]
            )
            print(f"Successfully wrote record to Timestream: {response}")
        except Exception as e:
            print(f"Error writing to Timestream: {e}")

    return {
        'statusCode': 200,
        'body': json.dumps('Data processed successfully!')
    }
```
Data Visualization with AWS QuickSight
Using Amazon QuickSight, I created a dataset connected to the Timestream database. This allows for insightful visualizations related to pull-up counts.

Creating the Dataset
Connecting QuickSight to Timestream:

In QuickSight, create a new dataset by selecting Timestream as the data source.
Provide the database (PullUpCounterDB) and table (PullUpData) names.
Visualizing the Data:

Create visualizations (e.g., bar charts, line graphs) to analyze trends in pull-up performance over time.
Security and Scalability
Security Measures
IAM Policies:

Ensured that the appropriate IAM policies are attached to QuickSight and Lambda roles.
Implemented the principle of least privilege for all permissions.
Secure MQTT Connection:

Used TLS for secure communications between the ESP32 and AWS IoT.
Scalability
AWS provides a scalable architecture, enabling easy addition of multiple IoT devices without significant overhead. The architecture helps maintain performance even with increased data loads.

Conclusion
This project demonstrated the value of integrating IoT technology with cloud services, providing a framework for tracking and analyzing fitness metrics. Through the combination of the ESP32 microcontroller and the HC-SR04 ultrasonic sensor, I successfully created a pull-up counter that not only records the number of repetitions performed but also uploads this data to the AWS cloud for processing and analysis.

Using AWS IoT Core, I established a secure communication channel to transmit data from the ESP32 to an AWS Lambda function. The Lambda function processes incoming data and writes it to Amazon Timestream, enabling efficient time-series data management and analytics.

The integration with AWS QuickSight allowed for the creation of insightful visualizations, helping to monitor fitness progress over time. This project not only enhanced my understanding of cloud architecture and serverless computing but also illustrated the potential of real-time data analytics in fitness and health contexts.

Moving forward, I see opportunities to expand this project by incorporating additional sensors to track other fitness metrics, creating alerts for progress milestones, and developing a mobile application for real-time statistics. This project has provided a solid foundation for exploring the vast capabilities of IoT within the AWS ecosystem, and I look forward to enhancing this system in the future.

---
Projektanteckning:

På grund av oförutsedda omständigheter har jag stött på svårigheter med funktionaliteten hos min ESP32. Jag hade beställt en ny ESP32, men det har uppstått förseningar i leveransen, och jag har ännu inte fått den. Som en följd av detta kunde jag inte genomföra fullständiga tester av hela projektet. Jag har dock gjort det bästa av de resurser jag har och genomfört så mycket som möjligt med det jag hade tillgängligt.

Jag vill uttrycka mitt uppriktiga tack till Johan Holmberg för hans inspirerande och stödjande undervisning här på Nackademin. Din vägledning har varit ovärderlig för min utveckling.

Daniel Ericson
---
Bilder:
![Schematisk skiss](https://github.com/user-attachments/assets/b00220e7-0767-4e1d-bdc2-e3a378e69d9e)

![Histogram](https://github.com/user-attachments/assets/7c4ae468-4a69-477a-9af6-fd5ce9b55b95)

