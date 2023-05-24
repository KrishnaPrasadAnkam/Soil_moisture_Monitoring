// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full
// license information.

#include <ESP8266WiFi.h>
#include "src/iotc/common/string_buffer.h"
#include "src/iotc/iotc.h"
#include "DHT.h"

#define DHTPIN 2  //D4

#define DHTTYPE DHT11   // DHT 11
int buzzer = D2;
int ir = D1;    
int pump = D8;  //Relay   
const int sensor_pin = A0;  /* Connect Soil moisture analog sensor pin to A0 of NodeMCU */


#define WIFI_SSID "Kittu"
#define WIFI_PASSWORD "asdfghjkl"
 
const char* SCOPE_ID = "0ne009F3F15";
const char* DEVICE_ID = "t979vl7ck3";
const char* DEVICE_KEY = "gRkwckoD7noxOhxJPqx9vB6qVao9L8xUNe8c7tOkNPY=";

DHT dht(DHTPIN, DHTTYPE);

void on_event(IOTContext ctx, IOTCallbackInfo* callbackInfo);
#include "src/connection.h"

void on_event(IOTContext ctx, IOTCallbackInfo* callbackInfo) {
  // ConnectionStatus
  if (strcmp(callbackInfo->eventName, "ConnectionStatus") == 0) {
    LOG_VERBOSE("Is connected ? %s (%d)",
                callbackInfo->statusCode == IOTC_CONNECTION_OK ? "YES" : "NO",
                callbackInfo->statusCode);
    isConnected = callbackInfo->statusCode == IOTC_CONNECTION_OK;
    return;
  }

  // payload buffer doesn't have a null ending.
  // add null ending in another buffer before print
  AzureIOT::StringBuffer buffer;
  if (callbackInfo->payloadLength > 0) {
    buffer.initialize(callbackInfo->payload, callbackInfo->payloadLength);
  }

  LOG_VERBOSE("- [%s] event was received. Payload => %s\n",
              callbackInfo->eventName, buffer.getLength() ? *buffer : "EMPTY");

  if (strcmp(callbackInfo->eventName, "Command") == 0) {
    LOG_VERBOSE("- Command name was => %s\r\n", callbackInfo->tag);
  }
}

void setup() {
  Serial.begin(9600);
  pinMode(ir, INPUT); 
  pinMode(buzzer, OUTPUT); 
  pinMode(pump, OUTPUT); 
  digitalWrite(pump, LOW);

  connect_wifi(WIFI_SSID, WIFI_PASSWORD);
  connect_client(SCOPE_ID, DEVICE_ID, DEVICE_KEY);

  if (context != NULL) {
    lastTick = 0;  // set timer in the past to enable first telemetry a.s.a.p
  }
   dht.begin();
}

void loop() {

float h = dht.readHumidity();
float t = dht.readTemperature();
int ir_data = digitalRead(ir);
float moisture_percentage;
digitalWrite(pump, 0);
digitalWrite(pump, LOW);

   if(ir_data!=HIGH){       //read sensor signal
    digitalWrite(buzzer, HIGH);   // if sensor is LOW, then turn on
    //digitalWrite(bulb, LOW);
  }
  else{
    digitalWrite(buzzer, LOW);   // if sensor is LOW, then turn on
    //digitalWrite(bulb, HIGH);
  }

  moisture_percentage = ( 100.00 - ( (analogRead(sensor_pin)/1023.00) * 100.00 ) );

  Serial.print("Soil Moisture(in Percentage) = ");
  Serial.print(moisture_percentage);
  Serial.println("%");

  if(moisture_percentage>40){       //read sensor signal
    digitalWrite(pump, LOW);   // if sensor is LOW, then turn on
    //digitalWrite(bulb, LOW);
  }
  else{
    digitalWrite(pump, HIGH);   // if sensor is LOW, then turn on
    //digitalWrite(bulb, HIGH);
  }

  if (isConnected) {

    unsigned long ms = millis();
    if (ms - lastTick > 10000) {  // send telemetry every 10 seconds
      char msg[64] = {0};
      int pos = 0, errorCode = 0;

      lastTick = ms;
      if (loopId++ % 2 == 0) {  // send telemetry
        pos = snprintf(msg, sizeof(msg) - 1, "{\"Temperature\": %f}",
                       t);
        errorCode = iotc_send_telemetry(context, msg, pos);
        
        pos = snprintf(msg, sizeof(msg) - 1, "{\"Humidity\":%f}",
                       h);
        errorCode = iotc_send_telemetry(context, msg, pos);

        pos = snprintf(msg, sizeof(msg) - 1, "{\"Moisture\":%f}",
                       moisture_percentage);
        errorCode = iotc_send_telemetry(context, msg, pos);
          
      } else {  // send property
        
      } 
  
      msg[pos] = 0;

      if (errorCode != 0) {
        LOG_ERROR("Sending message has failed with error code %d", errorCode);
      }
    }

    iotc_do_work(context);  // do background work for iotc
  } else {
    iotc_free_context(context);
    context = NULL;
    connect_client(SCOPE_ID, DEVICE_ID, DEVICE_KEY);
  }

}