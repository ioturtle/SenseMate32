#include <HardwareSerial.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_AHTX0.h>

HardwareSerial ld2420(1);
String buffer = "";

// Web Server on port 80
WebServer server(80);

// AP Configuration
const char* ssid = "SenseMate32";
const char* password = "123456789";

// AHT20 Sensor
Adafruit_AHTX0 aht;

// Sensor data
struct SensorData {
  float distance = 0.0;
  String presence = "NONE";
  String status = "Disconnected";
  float temperature = 0.0;
  float humidity = 0.0;
  unsigned long lastUpdate = 0;
} sensorData;

void setup() {
  Serial.begin(115200);
  ld2420.begin(115200, SERIAL_8N1, 18, 19);
  
  Serial.println("SenseMate32 Web Interface");
  
  // Initialize AHT20 sensor
  Wire.begin(21, 22); // SDA, SCL pins for ESP32
  if (!aht.begin()) {
    Serial.println("Could not find AHT20? Check wiring");
    sensorData.status = "AHT20 Error";
  } else {
    Serial.println("AHT20 found");
  }
  
  // Start Access Point
  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  
  Serial.print("AP IP address: ");
  Serial.println(IP);
  Serial.print("SSID: ");
  Serial.println(ssid);
  
  // Setup web server routes
  setupWebServer();
  
  Serial.println("Web server started");
}

void setupWebServer() {
  // Serve main page
  server.on("/", HTTP_GET, []() {
    String html = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>SenseMate32 Dashboard</title>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }
        
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            padding: 20px;
        }
        
        .container {
            max-width: 1000px;
            margin: 0 auto;
            background: rgba(255, 255, 255, 0.95);
            border-radius: 20px;
            box-shadow: 0 20px 40px rgba(0, 0, 0, 0.1);
            overflow: hidden;
        }
        
        .header {
            background: linear-gradient(135deg, #2c3e50, #34495e);
            color: white;
            padding: 30px;
            text-align: center;
        }
        
        .header h1 {
            font-size: 2.5em;
            margin-bottom: 10px;
            background: linear-gradient(45deg, #00b4db, #0083b0);
            -webkit-background-clip: text;
            -webkit-text-fill-color: transparent;
            background-clip: text;
        }
        
        .header p {
            font-size: 1.1em;
            opacity: 0.9;
        }
        
        .dashboard {
            padding: 30px;
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 20px;
        }
        
        .card {
            background: white;
            padding: 25px;
            border-radius: 15px;
            box-shadow: 0 10px 20px rgba(0, 0, 0, 0.1);
            border-left: 5px solid #3498db;
            transition: transform 0.3s ease;
        }
        
        .card:hover {
            transform: translateY(-5px);
        }
        
        .card.presence {
            border-left-color: #e74c3c;
        }
        
        .card.distance {
            border-left-color: #2ecc71;
        }
        
        .card.temperature {
            border-left-color: #e67e22;
        }
        
        .card.humidity {
            border-left-color: #3498db;
        }
        
        .card.status {
            border-left-color: #f39c12;
            grid-column: span 2;
        }
        
        .card h3 {
            color: #2c3e50;
            margin-bottom: 15px;
            font-size: 1.2em;
            display: flex;
            align-items: center;
            gap: 10px;
        }
        
        .value {
            font-size: 2.5em;
            font-weight: bold;
            color: #2c3e50;
            margin: 10px 0;
        }
        
        .unit {
            font-size: 0.6em;
            color: #7f8c8d;
        }
        
        .presence-indicator {
            display: inline-block;
            width: 20px;
            height: 20px;
            border-radius: 50%;
            background-color: #95a5a6;
            margin-right: 10px;
            transition: background-color 0.3s ease;
        }
        
        .presence-detected {
            background-color: #e74c3c;
            box-shadow: 0 0 20px #e74c3c;
        }
        
        .sensor-row {
            display: flex;
            justify-content: space-between;
            align-items: center;
            margin: 10px 0;
        }
        
        .sensor-value {
            font-size: 2em;
            font-weight: bold;
            color: #2c3e50;
        }
        
        .controls {
            padding: 20px 30px;
            background: #f8f9fa;
            border-top: 1px solid #e9ecef;
            display: flex;
            justify-content: space-between;
            align-items: center;
        }
        
        .btn {
            padding: 12px 24px;
            border: none;
            border-radius: 25px;
            font-size: 1em;
            font-weight: 600;
            cursor: pointer;
            transition: all 0.3s ease;
            background: linear-gradient(135deg, #3498db, #2980b9);
            color: white;
        }
        
        .btn:hover {
            transform: translateY(-2px);
            box-shadow: 0 5px 15px rgba(0, 0, 0, 0.2);
        }
        
        .last-update {
            color: #7f8c8d;
            font-size: 0.9em;
        }
        
        @keyframes pulse {
            0% { transform: scale(1); }
            50% { transform: scale(1.05); }
            100% { transform: scale(1); }
        }
        
        .pulse {
            animation: pulse 2s infinite;
        }
        
        @media (max-width: 768px) {
            .dashboard {
                grid-template-columns: 1fr;
            }
            
            .card.status {
                grid-column: span 1;
            }
            
            .controls {
                flex-direction: column;
                gap: 15px;
            }
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>🌡️ SenseMate32 Dashboard</h1>
            <p>Real-time Environmental Monitoring & Presence Detection</p>
        </div>
        
        <div class="dashboard">
            <div class="card presence">
                <h3>
                    <span class="presence-indicator" id="presenceIndicator"></span>
                    Presence Status
                </h3>
                <div class="value" id="presenceValue">NONE</div>
                <p>Human presence detection</p>
            </div>
            
            <div class="card distance">
                <h3>📏 Distance</h3>
                <div class="value" id="distanceValue">0.00<span class="unit">m</span></div>
                <p>Measured distance to target</p>
            </div>
            
            <div class="card temperature">
                <h3>🌡️ Temperature</h3>
                <div class="value" id="temperatureValue">0.0<span class="unit">°C</span></div>
                <p>Ambient temperature</p>
            </div>
            
            <div class="card humidity">
                <h3>💧 Humidity</h3>
                <div class="value" id="humidityValue">0.0<span class="unit">%</span></div>
                <p>Relative humidity</p>
            </div>
            
            <div class="card status">
                <h3>📊 System Status</h3>
                <div class="value" id="statusValue">Connected</div>
                <p>Sensor connection status</p>
            </div>
        </div>
        
        <div class="controls">
            <div class="last-update">
                Last update: <span id="lastUpdate">Just now</span>
            </div>
            <button class="btn" onclick="refreshData()">🔄 Refresh Data</button>
        </div>
    </div>

    <script>
        function updateDashboard(data) {
            // Update presence
            const presenceValue = document.getElementById('presenceValue');
            const presenceIndicator = document.getElementById('presenceIndicator');
            
            presenceValue.textContent = data.presence;
            if (data.presence === 'DETECTED') {
                presenceIndicator.className = 'presence-indicator presence-detected pulse';
                presenceValue.style.color = '#e74c3c';
            } else {
                presenceIndicator.className = 'presence-indicator';
                presenceValue.style.color = '#2c3e50';
            }
            
            // Update distance
            document.getElementById('distanceValue').innerHTML = 
                data.distance.toFixed(2) + '<span class="unit">m</span>';
            
            // Update temperature
            document.getElementById('temperatureValue').innerHTML = 
                data.temperature.toFixed(1) + '<span class="unit">°C</span>';
            
            // Update humidity
            document.getElementById('humidityValue').innerHTML = 
                data.humidity.toFixed(1) + '<span class="unit">%</span>';
            
            // Update status
            document.getElementById('statusValue').textContent = data.status;
            
            // Update last update time
            const now = new Date();
            document.getElementById('lastUpdate').textContent = 
                now.toLocaleTimeString();
        }
        
        function refreshData() {
            fetch('/data')
                .then(response => response.json())
                .then(data => updateDashboard(data))
                .catch(error => console.error('Error:', error));
        }
        
        // Auto-refresh every 2 seconds
        setInterval(refreshData, 2000);
        
        // Initial load
        refreshData();
    </script>
</body>
</html>
    )rawliteral";
    
    server.send(200, "text/html", html);
  });
  
  // Serve JSON data
  server.on("/data", HTTP_GET, []() {
    StaticJsonDocument<300> doc;
    doc["distance"] = sensorData.distance;
    doc["presence"] = sensorData.presence;
    doc["status"] = sensorData.status;
    doc["temperature"] = sensorData.temperature;
    doc["humidity"] = sensorData.humidity;
    doc["timestamp"] = sensorData.lastUpdate;
    
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
  });
  
  // Start server
  server.begin();
}

void loop() {
  server.handleClient();
  
  // Read AT command responses
  while(ld2420.available()) {
    char c = ld2420.read();
    buffer += c;
    
    if(c == '\n') { // End of line
      buffer.trim();
      if(buffer.length() > 0) {
        parseATResponse(buffer);
        buffer = "";
      }
    }
  }
  
  // Read AHT20 sensor data
  static unsigned long lastSensorRead = 0;
  if(millis() - lastSensorRead > 3000) { // Read every 3 seconds
    readAHT20();
    lastSensorRead = millis();
  }
  
  // Send periodic status request to LD2420
  static unsigned long lastRequest = 0;
  if(millis() - lastRequest > 2000) {
    ld2420.println("AT+Q"); // Query status
    lastRequest = millis();
  }
  
  // Update sensor status based on last update time
  if(millis() - sensorData.lastUpdate > 5000) {
    sensorData.status = "No Data";
  } else {
    sensorData.status = "Connected";
  }
  
  delay(100);
}

void readAHT20() {
  sensors_event_t humidity, temp;
  if (aht.getEvent(&humidity, &temp)) {
    sensorData.temperature = temp.temperature;
    sensorData.humidity = humidity.relative_humidity;
    
    Serial.print("Temperature: ");
    Serial.print(sensorData.temperature);
    Serial.println(" °C");
    
    Serial.print("Humidity: ");
    Serial.print(sensorData.humidity);
    Serial.println(" %");
  } else {
    Serial.println("AHT20 read failed");
    sensorData.status = "AHT20 Error";
  }
}

void parseATResponse(String response) {
  Serial.print("AT Response: ");
  Serial.println(response);
  
  sensorData.lastUpdate = millis();
  
  if(response.startsWith("Range")) {
    // Parse distance: "Range 105" -> 1.05 meters
    int spaceIndex = response.indexOf(' ');
    if(spaceIndex > 0) {
      String distStr = response.substring(spaceIndex + 1);
      sensorData.distance = distStr.toInt() / 100.0;
      
      Serial.print("Distance: ");
      Serial.print(sensorData.distance);
      Serial.println(" m");
    }
  }
  else if(response == "ON") {
    sensorData.presence = "DETECTED";
    Serial.println("Presence: DETECTED");
  }
  else if(response == "OFF") {
    sensorData.presence = "NONE";
    Serial.println("Presence: NONE");
  }
  else if(response.startsWith("OK")) {
    Serial.println("Command accepted");
  }
}
