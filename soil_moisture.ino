/* 
 * soil_moisture.ino An Arduino ESP8266 sample sketch.
 * A battery powered moisture sensor for a plant pot sending LINE notification.
 * LINE通知をするニッケル水素電池駆動の土壌水分センサ
 *
 * 3 x NiMH AAA ->Diode->VCC
 * IO16->RST->10k->VCC (Wakeup reset)
 * IO4->3.9k-> NPN transister (2SC1815) base to switch sensor power
 * IO14-> Soil moisture sensor digital output
 * IO13-> LED
 *
 * Add a header file named credentials.h by Ctrl+Shift+n.
 * Add the bellow lines in the header file.
 * #define WIFI_SSID "yourssid" 
 * #define WIFI_PASSWORD "wifipswd"
 * #define LINE_API_TOKEN "line_notification_api_token"
 * 
 * Copyright 2018 Masami Yamakawa (MONOxIT Inc.)
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */

#include "credentials.h"
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>

const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;
const char* lineApiToken = LINE_API_TOKEN;

const String lineNotifyMessage = "ベンジャミン喉乾いた！";
// https://developers.line.me/media/messaging-api/sticker_list.pdf
int stickerPackage = 1;
int stickerId = 401;

const char* host = "notify-api.line.me";
const String url = "/api/notify";
const int httpsPort = 443;

const int ledPin = 5;
const int moisturePowerPin = 4;
const int moistureInputPin = 14;

ADC_MODE(ADC_VCC);
WiFiClientSecure client;

void setup() {
  // put your setup code here, to run once:
  WiFi.mode(WIFI_STA);
  Serial.begin(115200);
  pinMode(ledPin,OUTPUT);
  pinMode(moisturePowerPin, OUTPUT);
  int v = ESP.getVcc();
  digitalWrite(moisturePowerPin, HIGH);
  digitalWrite(ledPin, HIGH);
  delay(500);
  int dry = digitalRead(moistureInputPin);
  digitalWrite(moisturePowerPin, LOW);
  digitalWrite(ledPin, LOW);
  Serial.println("");
  Serial.print("DRY:");
  Serial.print(dry);
  Serial.print(" BATT:");
  Serial.println(v);
  
  if(dry == HIGH || v < 3000){
    Serial.println();
    Serial.print("connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);
  
    for(int i = 0; WiFi.status() != WL_CONNECTED && i < 20; i++){
      digitalWrite(ledPin,HIGH);
      delay(250);
      digitalWrite(ledPin,LOW);
      delay(250);
      Serial.print(".");
    }

    if(WiFi.status() != WL_CONNECTED){
      Serial.println("");
      Serial.println("[Error] WiFi not connected. Sleep...");
      ESP.deepSleep((unsigned long)(5*60*1000*1000)); // Sleep 5 min
    }
    
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

    String message = "";
    if(dry == HIGH){
      message += lineNotifyMessage;
    }
    message += "電池：";
    message += v;
    lineNotify(message, stickerPackage, stickerId);
    delay(1000);
    Serial.println("Sleep...");
    ESP.deepSleep((unsigned long)(60*60*1000*1000)); //Sleep 1H
  }else{
    Serial.println("Sleep...");
    ESP.deepSleep((unsigned long)(30*60*1000*1000)); //Sleep 30 min
  }
}

void loop() {
  // put your main code here, to run repeatedly:
}

// if stkid or stkpkgid is 0, a sticker is not sent.
void lineNotify(String message, int stkpkgid, int stkid){
  digitalWrite(ledPin, HIGH);

  Serial.print("connecting to ");
  Serial.println(host);

  if (!client.connect(host, httpsPort)) {
    Serial.println("[Error] connection failed. sleep...");
    ESP.deepSleep((unsigned long)(60*1000*1000));
  }

  Serial.print("requesting URL: ");
  Serial.println(url);
  String lineMessage = "message=" + message;

  if((stkid != 0) && (stkpkgid != 0)){
    lineMessage += "&stickerPackageId=";
    lineMessage += stkpkgid;
    lineMessage += "&stickerId=";
    lineMessage += stkid;
  }

  String header = "POST " + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "User-Agent: ESP8266\r\n" +
               "Authorization: Bearer " + lineApiToken + "\r\n" +
               "Connection: close\r\n" +
               "Content-Type: application/x-www-form-urlencoded\r\n" +
               "Content-Length: " + lineMessage.length() + "\r\n";

  client.print(header);
  client.print("\r\n");
  client.print(lineMessage + "\r\n");
  
  Serial.print("request sent:");
  Serial.println(lineMessage);
  
  String res = client.readString();
  Serial.println(res);
  client.stop();
  digitalWrite(ledPin,LOW);
}
