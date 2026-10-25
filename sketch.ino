#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>

// Hardware Pin Definitions
#define DHTPIN 15
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);
WebServer server(80);

// Wi-Fi Credentials (Wokwi Virtual Router)
const char* ssid = "Wokwi-GUEST";
const char* password = "";

// Global State Variables for Non-Blocking Logic
float currentTemp = 0.0;
float currentHum = 0.0;
unsigned long previousMillis = 0;
const long interval = 2000; // 2 seconds

// Custom Degree Symbol for LCD
byte degreeSymbol[8] = { 0b00111, 0b00101, 0b00111, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000 };

// Industrial Dashboard HTML (Embedded)
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Panam Group - IoT Monitor</title>
  <style>
    body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; background-color: #121212; color: #ffffff; text-align: center; margin: 0; padding: 50px 20px; }
    .container { max-width: 600px; margin: auto; background: #1e1e1e; padding: 30px; border-radius: 10px; box-shadow: 0 4px 15px rgba(0,0,0,0.5); }
    h1 { color: #4da8da; margin-bottom: 5px; }
    p.subtitle { color: #888; font-size: 14px; margin-bottom: 30px; }
    .sensor-box { background: #2a2a2a; padding: 20px; border-radius: 8px; margin: 15px 0; display: flex; justify-content: space-between; align-items: center; border-left: 5px solid #4da8da; }
    .sensor-box.humidity { border-left-color: #2ecc71; }
    .label { font-size: 1.2rem; color: #ccc; }
    .value { font-size: 2rem; font-weight: bold; }
    .val-temp { color: #4da8da; }
    .val-hum { color: #2ecc71; }
    .status { margin-top: 30px; font-size: 0.9rem; color: #555; }
  </style>
</head>
<body>
  <div class="container">
    <h1>Factory Edge Node Monitor</h1>
    <p class="subtitle">Live Telemetry Dashboard | Syncs with Local LCD</p>
    
    <div class="sensor-box">
      <div class="label">🌡️ Temperature</div>
      <div class="value val-temp"><span id="temp">--</span> &deg;C</div>
    </div>
    
    <div class="sensor-box humidity">
      <div class="label">💧 Relative Humidity</div>
      <div class="value val-hum"><span id="hum">--</span> %</div>
    </div>

    <div class="status">Live connection active. Hardware polling every 2000ms.</div>
  </div>

  <script>
    setInterval(function ( ) {
      fetch('/api/data')
        .then(response => response.json())
        .then(data => {
          document.getElementById("temp").innerText = data.temperature;
          document.getElementById("hum").innerText = data.humidity;
        })
        .catch(err => console.error(err));
    }, 2000);
  </script>
</body>
</html>
)rawliteral";

void setup() {
  Serial.begin(115200);
  
  // Init Hardware
  dht.begin();
  lcd.init();
  lcd.backlight();
  lcd.createChar(0, degreeSymbol);

  // Boot UI
  lcd.setCursor(0, 0);
  lcd.print("Connecting WiFi.");
  Serial.println("Initializing system... Connecting to WiFi.");
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  // Display Network Details on LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WiFi Connected!");
  lcd.setCursor(0, 1);
  lcd.print(WiFi.localIP().toString());
  
  Serial.println("\n✅ Wi-Fi Connected!");
  Serial.print("🌐 Dashboard IP Address: ");
  Serial.println(WiFi.localIP());
  
  delay(3000); // Hold IP on screen for 3 seconds before clearing
  lcd.clear();

  // Define Web Server Routes
  server.on("/", []() {
    server.send(200, "text/html", index_html);
  });

  server.on("/api/data", []() {
    // Web server instantly replies with the cached global variables
    String json = "{\"temperature\":\"" + String(currentTemp, 1) + "\", \"humidity\":\"" + String(currentHum, 1) + "\"}";
    server.send(200, "application/json", json);
  });

  server.begin();
  Serial.println("🚀 Web Server Started. Listening for connections...");
}

void loop() {
  // 1. Instantly handle incoming web traffic (No Delays!)
  server.handleClient(); 

  // 2. Non-blocking hardware timer
  unsigned long currentMillis = millis();
  
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    float t = dht.readTemperature();
    float h = dht.readHumidity();

    if (!isnan(t) && !isnan(h)) {
      currentTemp = t;
      currentHum = h;

      // Update Local LCD UI
      lcd.setCursor(0, 0);
      lcd.print("Temp: ");
      lcd.print(currentTemp, 1);
      lcd.write(0); 
      lcd.print("C  ");

      lcd.setCursor(0, 1);
      lcd.print("Hum:  ");
      lcd.print(currentHum, 1);
      lcd.print("%   ");

      // Log to CLI
      Serial.print("🌡️ Temp: ");
      Serial.print(currentTemp, 1);
      Serial.print(" °C  |  💧 Hum: ");
      Serial.print(currentHum, 1);
      Serial.println(" %");

    } else {
      lcd.setCursor(0, 0);
      lcd.print("Sensor Error!   ");
      Serial.println("⚠️ ERROR: Failed to read from DHT sensor!");
    }
  }
}