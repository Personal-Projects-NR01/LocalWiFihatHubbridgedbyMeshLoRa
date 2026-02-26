#include <ArduinoJson.h>

void setup() {
  Serial.begin(115200); // For PC Debugging
  // UART1 Pins: RX=20, TX=21
    Serial1.begin(115200, SERIAL_8N1, 3, 1);
  Serial.println("Mesh Simulator Online...");
}

void loop() {
  if (Serial1.available()) {
    String incoming = Serial1.readStringUntil('\n');
    Serial.println("Received from Gateway: " + incoming);

    // Simulate LoRa Latency (1.5 seconds)
    delay(1500);

    // Build a Mock Response in Option B (Condensed JSON)
    JsonDocument doc;
    doc["s"] = "Mesh_Node_B";
    doc["m"] = "ACK: " + String(millis() % 1000); // Random tag
    doc["i"] = "remote_789";

    serializeJson(doc, Serial1);
    Serial1.println(); 
    
    Serial.println("Response sent back to Gateway.");
  }
}