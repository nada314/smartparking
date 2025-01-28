#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <MFRC522.h>
#include <ESP32Servo.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
#include <stdlib.h> 

// WiFi credentials
#define WIFI_SSID "Galaxy A20s1679"
#define WIFI_PASSWORD "oaam11.'"

// Firebase credentials
#define API_KEY "AIzaSyB1UzxVHzQtqLiazZ_0ySHneJyx2ZcxUzs"
#define DATABASE_URL "https://smartparking-1ba32-default-rtdb.europe-west1.firebasedatabase.app/"

// Hardware pins
#define SENSOR1 32
#define SS_PIN 5
#define RST_PIN 27
#define REDLED 33
#define GREENLED 25

// Initialize objects
LiquidCrystal_I2C lcd(0x21, 16, 2);
Servo myServo;
MFRC522 mfrc522(SS_PIN, RST_PIN);
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

bool locked = true;
bool signupOK = false;
bool isVide = true;
String lastCardId = "";
String currentExitLogKey = "";

void setup() {
  Serial.begin(115200);
  
  // Initialize LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Smart Parking");
  lcd.setCursor(0, 1);
  lcd.print("Connecting...");
  delay(2000);
  
  // Initialize pins
  pinMode(SENSOR1, INPUT_PULLUP);
  pinMode(REDLED, OUTPUT);
  pinMode(GREENLED, OUTPUT);
  
  // Initialize RFID
  SPI.begin();
  mfrc522.PCD_Init();
  
  // Initialize servo
  myServo.attach(2);
  myServo.write(0);
  
  // Connect to WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
  }
  
  // Configure Firebase
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  
  if (Firebase.signUp(&config, &auth, "", "")) {
    signupOK = true;
    lcd.clear();
    lcd.print("System Ready");
    delay(2000);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Parking Vide");
  }
  
  config.token_status_callback = tokenStatusCallback;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

void logAccessAttempt(String cardId, bool authorized, bool spaceAvailable, bool accessGranted, bool isExit = false, int duration = 0, int price = 0) {
  if (Firebase.ready() && signupOK) {
    // Create access log entry
    FirebaseJson json;
    json.set("timestamp/.sv", "timestamp");
    json.set("cardId", cardId);
    json.set("authorized", authorized);
    json.set("spaceAvailable", spaceAvailable);
    json.set("accessGranted", accessGranted);
    json.set("isExit", isExit);
    if (isExit) {
      json.set("duration", duration);
      json.set("price", price);
    }
    
    // Push to Firebase
    String path = "access_logs/";
    if (Firebase.RTDB.pushJSON(&fbdo, path.c_str(), &json)) {
      Serial.println("Access log saved");
      if (isExit) {
        // Store the key of the exit log for later update
        currentExitLogKey = fbdo.pushName();
      }
    }
    
    // Update parking status
    if (accessGranted && !isExit) {
      Firebase.RTDB.setBool(&fbdo, "parking_status/occupied", true);
      Firebase.RTDB.setString(&fbdo, "parking_status/last_card", cardId);
    }
  }
}

void updateExitLog(int duration, int price) {
  if (!currentExitLogKey.isEmpty()) {
    String path = "access_logs/" + currentExitLogKey;
    Firebase.RTDB.setInt(&fbdo, path + "/duration", duration);
    Firebase.RTDB.setInt(&fbdo, path + "/price", price);
    currentExitLogKey = ""; // Reset the key
  }
}

bool checkCardInParking(String cardId) {
  if (Firebase.RTDB.getString(&fbdo, "parking_status/last_card")) {
    String storedCard = fbdo.stringData();
    return storedCard == cardId;
  }
  return false;
}

void clearParkingStatus() {
  Firebase.RTDB.deleteNode(&fbdo, "parking_status");
}

/*
int calculateParkingDuration(String cardId) {
  Serial.println("Calculating duration...");
  if (Firebase.RTDB.getShallowData(&fbdo, "access_logs")) {
    Serial.println("in0");
    FirebaseJson json;
    json.setJsonData(fbdo.stringData());
    
    uint64_t latestEntryTime = 0;
    uint64_t currentExitTime = 0;

    // First, get the current exit time
    if (!currentExitLogKey.isEmpty()) {
      Serial.println(currentExitLogKey);
      String exitPath = "access_logs/" + currentExitLogKey + "/timestamp";
      if (Firebase.RTDB.getString(&fbdo, exitPath)) {
        Serial.println("in1");
        currentExitTime = strtoull(fbdo.stringData().c_str(), nullptr, 10);
      }
    }



    // Then find the latest entry
    size_t len = json.iteratorBegin();
    for (size_t i = 0; i < len; i++) {
      String key;
      String value;
      int type;
      json.iteratorGet(i, type, key, value);
      
      String entryPath = "access_logs/" + key;
      
      if (Firebase.RTDB.getString(&fbdo, entryPath + "/cardId")) {
        Serial.println("in2");
        String entryCardId = fbdo.stringData();
        
        if (entryCardId == cardId) {
          Serial.println("in3");
          if (Firebase.RTDB.getString(&fbdo, entryPath + "/timestamp")) {
            String timestampStr = fbdo.stringData();
            uint64_t timestamp = strtoull(timestampStr.c_str(), nullptr, 10);
            
            if (Firebase.RTDB.getBool(&fbdo, entryPath + "/isExit")) {
              bool isExit = fbdo.boolData();
              if (!isExit && timestamp > latestEntryTime) {
                latestEntryTime = timestamp;
              }
            }
          }
        }
      }
    }
    json.iteratorEnd();

    if (latestEntryTime > 0 && currentExitTime > 0) {
      return (currentExitTime - latestEntryTime) / 1000; // Convert to seconds
    }
  }
  return 0;
}
*/

int calculateParkingDuration(String cardId) {
  Serial.println("calcul...");
  if (Firebase.RTDB.getShallowData(&fbdo, "access_logs")) {
    Serial.println("in");
    FirebaseJson json;
    json.setJsonData(fbdo.stringData());
    
    FirebaseJsonData jsonData;
    String key;
    String value;
    int type;

    // Initialiser les variables pour stocker les timestamps
    uint64_t latestEntryTime = 0;
    uint64_t latestExitTime = 0;

    size_t len = json.iteratorBegin();
    for (size_t i = 0; i < len; i++) {
      json.iteratorGet(i, type, key, value);
      
      // Récupérer les données de chaque entrée avec la clé
      String entryPath = "access_logs/" + key;
      if (Firebase.RTDB.getString(&fbdo, entryPath + "/cardId")) {
        String entryCardId = fbdo.stringData();
        
        if (entryCardId == cardId) {
          // Récupérer le timestamp
          if (Firebase.RTDB.getString(&fbdo, entryPath + "/timestamp")) {
            String timestampStr = fbdo.stringData();
            uint64_t timestamp = strtoull(timestampStr.c_str(), nullptr, 10); // Conversion en uint64_t
            Serial.println("Timestamp : " + String(timestamp));
            
            // Vérifier si c'est une sortie
            if (Firebase.RTDB.getBool(&fbdo, entryPath + "/isExit")) {
              if (fbdo.boolData()) {
                // C'est une sortie
                if (timestamp > latestExitTime) {
                  latestExitTime = timestamp;
                }
              } else {
                // C'est une entrée
                if (timestamp > latestEntryTime) {
                  latestEntryTime = timestamp;
                }
              }
            }
          }
        }
      }
    }
    json.iteratorEnd();
    Serial.println(latestEntryTime);
    Serial.println(latestExitTime);

    // Calculer la durée
    if (latestEntryTime > 0 && latestExitTime > 0) {
      int duration = latestExitTime - latestEntryTime;
      Serial.println("Durée de stationnement : " + String(duration / 1000) + " secondes");
      return duration;
    } else {
      Serial.println("Impossible de calculer la durée pour cette carte.");
    }
  } else {
    Serial.println("Erreur : " + fbdo.errorReason());
  }
  return 0;
}
