#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>

// Network
const char* ssid = "WiFI name";
const char* password = "WiFi password";

// Initialize Telegram BOT
#define BOTtoken "Bot token"
#define CHAT_ID "Chat ID"
WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

// Ultrasonic 
const int trigPin = 25; 
const int echoPin = 26;

long duration;
float distanceCm;

// Capacitive Touch Pin 
const int touchPin = 32;

int touchBaseline = 0;
int touchThreshold = 12; 

// State tracking
bool systemActive = false;   
bool lastTouchState = false; 

// Timers
unsigned long lastTouchCheck = 0;
const unsigned long touchInterval = 80; 

unsigned long lastSensorCheck = 0;
const unsigned long sensorInterval = 600; 

unsigned long lastTelegramAlertTime = 0;
const unsigned long alertCooldown = 6000; 

void setup() {
  delay(1000);
  // Ultrasonic
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  client.setInsecure(); 

  while (WiFi.status() != WL_CONNECTED) {delay(500);}
  
  // Dynamic Touch Auto-Calibration
  long sum = 0;
  for(int i = 0; i < 10; i++) {
    sum += touchRead(touchPin);
    delay(50);
  }
  touchBaseline = sum / 10;
  
  if (touchBaseline > 40) {
    touchThreshold = (touchBaseline * 65) / 100; 
  } else {
    touchThreshold = touchBaseline - 5; 
    if (touchThreshold < 1) touchThreshold = 1; 
  }
} 

void loop() {
  unsigned long currentMillis = millis();

  if (currentMillis - lastTouchCheck >= touchInterval) {
    lastTouchCheck = currentMillis;
    
    int touchValue = touchRead(touchPin);
    bool isCurrentlyTouched = (touchValue < touchThreshold);

    // Check if "Button" is being pressed
    if (isCurrentlyTouched && !lastTouchState) {
      systemActive = !systemActive; // Toggle system
      
      if (systemActive) {
        bot.sendMessage(CHAT_ID, "✅ Alarm system is active.", "");
      } else {
        bot.sendMessage(CHAT_ID, "❌ Alarm system is closed.", ""); 
      }
    }
    lastTouchState = isCurrentlyTouched;
  }
  
  // Ultrasonic readings
  if (systemActive && (currentMillis - lastSensorCheck >= sensorInterval)) {
    lastSensorCheck = currentMillis;

    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    
    duration = pulseIn(echoPin, HIGH, 30000); 
    distanceCm = duration * 0.0343 / 2;
    
    // Alert message
    if (distanceCm <= 30 && distanceCm > 0) {
      if (currentMillis - lastTelegramAlertTime >= alertCooldown) {
        bot.sendMessage(CHAT_ID, "🚨 ALERT!!! INTRUDER!!! ", "");
        lastTelegramAlertTime = currentMillis;
      }
    }
  }
}