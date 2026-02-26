#include <ArduinoJson.h>

void setup() {
  Serial.begin(115200);
  //Serial1.begin(115200, SERIAL_8N1, 3, 1);
  Serial1.begin(115200, SERIAL_8N1, 16, 17);
  delay(2000); // Wait for Gateway to boot
  announce();
}

void announce() {
  String names[] = {"Alpha", "Bravo", "Charlie"};
  String ids[] = {"u_1", "u_2", "u_3"};
  
  for(int i=0; i<3; i++) {
    JsonDocument doc;
    doc["type"] = "join";
    doc["id"] = ids[i];
    doc["n"] = names[i];
    serializeJson(doc, Serial1);
    Serial1.println();
    delay(500); 
  }
}

void loop() {
  // Announce every 10 seconds for testing stability
  static unsigned long last = 0;
  if (millis() - last > 10000) {
    announce();
    last = millis();
  }
}