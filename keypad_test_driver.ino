#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include "SPIFFS.h"
// for exp32
#include <WiFi.h>
#include <AsyncTCP.h>
// for esp8266
//#include <ESP8266WiFi.h>
//#include <ESPAsyncTCP.h>

const char* ssid = "";  // Enter SSID here
const char* password = "";  //Enter Password here
char incomingbyte1;
int characterLineCount = 0;

// Set web server port number to 80
AsyncWebServer server(80);

// Variable to store the HTTP request
String header;

// for async string handling
String inputMessage;
String inputParam;

const int ROWS = 8; // rows
const int COLS = 7; // columns

void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

const char keys[ROWS][COLS] = {
  // ? is unknown
{'S','/','?','\a',' ','E','\b'}, // \a is alert bell E = word eraser
  {'S','z','x','c','v','b','n'}, //left shift
  {'L','a','s','d','f','g','h'}, //?
  {'T','q','w','e','r','t','y'}, //CODE
  {'M','1','2','3','4','5','6'}, //MARGIN
 {'7','8','9','0','-','=','\b'}, //\b = Backspace
 {'u','i','o','p','H','\r','j'}, //'H' = 1/2; ERASER
 {'k','l',';','\'','m',',','.'}  //CORRECT
};

//array of special characters that are reached by shift
const int SHKEYS = 13; // special keys
// euro becomes gulden xd
const char shiftkeys[SHKEYS] = {'!','@','€','$','%','^','&','*','(',')','_','+','?'};
const char shiftkeyskeys[SHKEYS] = {'1','2','3','4','5','6','7','8','9','0','-','=','/'};

// pin 34 35? led?
//                COLUMN   1  2  3  4  5  6  7
const int colPins[COLS] = {18,17,16,4 ,21,2 ,15};
//                   ROW   1  2  3  4  5  6  7  8
const int rowPins[ROWS] = {13,32,33,25,26,27,14,12};

void typeKey(char key) {
  for (int col = 0; col < COLS; col++) {
    for (int row = 0; row < ROWS; row++) {
      if (keys[row][col] == key) {
        Serial.print(key);
        Serial.print(" was found in row: ");
        Serial.print(row+1);
        Serial.print(" column: ");
        Serial.println(col+1);
        digitalWrite(rowPins[row], HIGH);
        digitalWrite(colPins[col], HIGH);
        delay(50);
        digitalWrite(rowPins[row], LOW);
        digitalWrite(colPins[col], LOW);
        delay(50); // delay between pressing keys
        characterLineCount++;
        if (key == '\r') {
            characterLineCount = 0;
        }
        return;
      }
    }
  }
  Serial.print(key);
  Serial.println(" was not found in lookup table");
  return;
}

void pressShift(int i) {
  if (i) {
    // shit does not seem to work so use the lock key
    typeKey('L');
    // if i = 1 press shift
    // Serial.println("Shift is pressed");
    // digitalWrite(rowPins[0], HIGH);
    // digitalWrite(colPins[2], HIGH);
    // delay(50);
  }
  else {
    typeKey('L');
    // if i = 0 release shift
    // Serial.println("shift is released");
    // digitalWrite(rowPins[0], LOW);
    // digitalWrite(colPins[2], LOW);
    // delay(50);
  }
}

void parseKey(char key) {
  if (isUpperCase(key)) {
    // key is uppercase
    pressShift(1);
    key = key + 32; //convert to lower case (ASCII)
    typeKey(key);
    pressShift(0);
    return;
  }
  else {
    // check if the key is under a special key
    for (int shkey = 0; shkey < SHKEYS; shkey++) {
      if (shiftkeys[shkey] == key) {
        pressShift(1);
        typeKey(shiftkeyskeys[shkey]);
        pressShift(0);
        return;
      }
    }
    // no special key, just type it
    typeKey(key);
    return;
  }
}

void setup() {
  Serial.begin(115200);
  // initialize the output pins:
  for (int i = 0; i < COLS; i++) {
    pinMode(colPins[i], OUTPUT);
    digitalWrite(colPins[i], LOW);
  }
  for (int i = 0; i < ROWS; i++) {
    pinMode(rowPins[i], OUTPUT);
    digitalWrite(rowPins[i], LOW);
  }
  // Initialize SPIFFS
  if(!SPIFFS.begin(true)){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  Serial.println("Ready");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("WiFi Failed!");
    return;
  }
  Serial.println();
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Send web page with input fields to client
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", "text/html");
  });
  // Route to load style.css file
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/style.css", "text/css");
  });
  // Send a GET request to <ESP_IP>/get?input1=<inputMessage>
  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
    // GET input1 value on <ESP_IP>/get?input1=<inputMessage>
    if (request->hasParam("text")) {
      inputMessage = request->getParam("text")->value();
      inputParam = "text";
    }
    else {
      inputMessage = "";
      inputParam = "none";
    }
    //Serial.println(inputMessage);
    // inputMessage was set, main loop will handle printing it
    // parseString(inputMessage);
    request->send(200, "text/html", "HTTP GET request sent to typewriter on input field (" 
                                     + inputParam + ") with value: " + inputMessage +
                                     "<br><a href=\"/\">Return to Home Page</a>");
  });
  server.onNotFound(notFound);
  server.begin();
}

void loop() {
  // in order to avoid watchdog messages,
  // don't print using function but print in main loop
  while (inputMessage.length() != 0) {
    // Serial.print(inputMessage.length());
    parseKey(inputMessage[0]);
    inputMessage.remove(0,1);
    if (characterLineCount >= 80) { 
        // for portrait A4 at space 12, should make this a setting on webpage
        typeKey('\r');
    }
  }
  delay(10);
}

//   if 
//   if(Serial.available() > 0){
//     incomingbyte1 = Serial.read();
//     parseKey(char(incomingbyte1));
//   }
//   delay(10);
