// Library
#include <ESP8266WiFi.h>    //Borad wifi lib
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>    // Including library for Json
#include <DHT.h>              // Including library for temp Sensor

// I/O PINS
#define DHTPIN D3          //Temp Sensor DHT11 At pin D3
#define DHTTYPE DHT11      // We are Using DHT11
#define lightSensor D6     //Light  Sensor LDR  At pin D6
#define Bulb D5            //Bulb At pin D5
#define moistureSensor A0  //Moisture Sensor At pin A0
#define relay D1           //Relay  At pin D1


// Moisture Sensor
int soilMoistureValue = 0;
int soilmoisturepercent = 0;

// Temp Sensor
float humidity;     // Reading Humidity Value
float temperature;  // Reading Temperature Value
DHT dht(DHTPIN, DHTTYPE);

// Light Sensor
bool intensityLevel;  //LDR 


//***********Wifi Credentials***********//
const char *ssid = "admin";          //SSID
const char *password = "123456789";  //PASSWORD

HTTPClient http;  //Declare an object of class HTTPClient
WiFiClient wifi;

void setup() {
  Serial.begin(9600);  // open serial port, set the baud rate to 9600 bps
  Serial.println("---------------------Monitoring Farm------------------------");
  dht.begin();

  pinMode(relay, OUTPUT);
  pinMode(lightSensor, INPUT);
  pinMode(Bulb, OUTPUT);
  digitalWrite(relay, HIGH);   //Initial pump off

  WiFi.mode(WIFI_OFF);  //Prevents reconnection issue (taking too long to connect)
  delay(100);
  WiFi.mode(WIFI_STA);  //Declare ESP8266 to Station Mode
  Serial.printf("Connecting to %s\n", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(200);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected, IP address: ");
  Serial.println(WiFi.localIP());
}


// Read Temperature and Humidity
void measureTemperature() {
  humidity = dht.readHumidity();        // Reading Humidity Value
  temperature = dht.readTemperature();  // Reading Temperature Value

  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.print(" degrees Celcius, Humidity: ");
  Serial.println(humidity);
}

// Read Soil Moistuer
void measureMoistuerLevel() {
  soilMoistureValue = analogRead(moistureSensor);  //put Sensor insert into soil
   soilmoisturepercent =  ( 100.00 - ( soilMoistureValue / 1023.00) * 100.00 ) ;
  // Serial.print("SoilMoistureValue: ");
  // Serial.print(soilmoisturepercent);
  // Serial.println("%");

  if (soilmoisturepercent > 100) {
     soilmoisturepercent = 100 ;  
    Serial.println("Soil moisture (in percent): 100 %");
  }

  else if (soilmoisturepercent < 0) {
    soilmoisturepercent = 0 ;
    Serial.println("Soil moisture (in percent): 0 %");
  }

   if (soilmoisturepercent >= 0 && soilmoisturepercent <= 100) {
    Serial.print("Soil moisture (in percent): ");
    Serial.print(soilmoisturepercent);
    Serial.println("%");
  }

  // If Moisture Level Drop then Motor (Pump) turn ON
  if (soilmoisturepercent >= 0 && soilmoisturepercent <= 30) {
    digitalWrite(relay, LOW);
    Serial.println("Dry land: Motor is ON for a while");
    
  } 
  else if (soilmoisturepercent > 30 && soilmoisturepercent <= 100) {
    digitalWrite(relay, HIGH);
    Serial.println("Wet land: Motor is OFF for a while");    
    
    
  }
}

// Detect SunLight
void detectSunLight() {
  intensityLevel = digitalRead(lightSensor);  //assign value of LDR sensor to a temporary variable
  Serial.print("Intensity = ");                 //print on serial monitor using ""
  Serial.println(intensityLevel);             //display output on serial monitor
  // delay(300);                                                                                                                                                                                                                                              
  if (intensityLevel == HIGH)  //HIGH means,light got blocked
  {
    digitalWrite(Bulb, LOW);  //if light is not present,LED on
    Serial.println("Bulb ON");
  } else {
    digitalWrite(Bulb, HIGH);  //if light is present,LED off
    Serial.println("Bulb OFF");
  }
}

// Send Data to Server
void sendToserver() {
  String data = "";  
  String payload = "";
  

  measureMoistuerLevel() ;
  measureTemperature();
  detectSunLight();

  //////////////////////////////////////////////////////////Json//////////////////
  http.begin(wifi, "http://mysterious-woodland-84868.herokuapp.com/user");  //initiate HTTP request, put your Website URL Specify request destination
  http.addHeader("Content-Type", "application/json");
  
  //JSON
  DynamicJsonDocument doc(1024);
  doc["moistureLevel"] = soilmoisturepercent;
  doc["temperature"] = temperature;
  doc["humidity"] = humidity;
  doc["light"] = lightSensor;
  serializeJson(doc, data);

  Serial.println(data);
  
  int httpCode = http.POST(data);
  // Serial.println(httpCode);  //Send the request

  if (httpCode > 0) {            //Check the returning code
    payload = http.getString();  //Get the request response payload
    Serial.println(payload);     //Print the response payload
  } 
  else {
    Serial.println("Error on sending POST: ");
    Serial.println(httpCode);
  }
  http.end();  //Close connection
}


void loop() {

  sendToserver();
  Serial.println("---------------------------------------------");
  Serial.println("");
  delay(5000);
}