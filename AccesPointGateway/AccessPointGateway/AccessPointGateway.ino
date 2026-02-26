#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    if (type == WS_EVT_DATA) {
        // --- WS to SERIAL (Outbound) ---
        // We take the JSON from the phone and dump it to the Simulator
        Serial1.write(data, len);
        Serial1.println(); 
        
        // Echo locally
        ws.textAll(data, len);
    }
}

void setup() {
    Serial.begin(115200);
    // UART1 Pins: RX=20, TX=21
    Serial1.begin(115200, SERIAL_8N1, 3, 1);

    WiFi.softAP("Mesh-Gateway-HIL", "");
    ws.onEvent(onEvent);
    server.addHandler(&ws);

    // Minimal HTML for testing
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "text/html", "<html><body style='background:#111;color:#eee'><h2>HIL Mesh Test</h2><div id='c'></div><input id='m'><button onclick='s()'>Send</button><script>var k=new WebSocket('ws://'+location.hostname+'/ws');k.onmessage=e=>document.getElementById('c').innerHTML+=e.data+'<br>';function s(){k.send(JSON.stringify({s:'User1',m:document.getElementById('m').value}));}</script></body></html>");
    });

    server.begin();
}

void loop() {
    ws.cleanupClients();

    // --- SERIAL to WS (Inbound) ---
    // Listen for the Simulator "Mesh" messages
    if (Serial1.available()) {
        String meshData = Serial1.readStringUntil('\n');
        Serial.println("Mesh Inbound: " + meshData);
        ws.textAll(meshData);
    }
}