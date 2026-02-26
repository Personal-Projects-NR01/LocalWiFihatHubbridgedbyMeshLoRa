#include <WiFi.h>
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <ESPmDNS.h>

const char* ssid = "Mesh-Gateway-Chat";
#define MIN_FREE_HEAP 40960 
//#define RX_PIN 3           
//#define TX_PIN 1            

#define RX_PIN 16
#define TX_PIN 17            

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
DNSServer dnsServer;

struct UserInfo { String id; String n; uint32_t socketId; };
std::vector<UserInfo> activeUsers;

// --- Helper: Send the list to ONE specific client ---
void sendUserListToClient(uint32_t client_id) {
    JsonDocument doc; 
    doc["type"] = "sys";
    JsonArray arr = doc.createNestedArray("users");
    for (auto const& u : activeUsers) {
        JsonObject o = arr.createNestedObject();
        o["id"] = u.id;
        o["n"] = u.n;
    }
    String out;
    serializeJson(doc, out);
    ws.text(client_id, out);
}

// --- Helper: Broadcast to everyone ---
void broadcastSystemStatus() {
    JsonDocument doc; 
    doc["type"] = "sys";
    JsonArray arr = doc.createNestedArray("users");
    for (auto const& u : activeUsers) {
        JsonObject o = arr.createNestedObject();
        o["id"] = u.id;
        o["n"] = u.n;
    }
    String out;
    serializeJson(doc, out);
    ws.textAll(out);
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    if (type == WS_EVT_CONNECT && ESP.getFreeHeap() < MIN_FREE_HEAP) client->close(503, "Full");
    
    if (type == WS_EVT_DISCONNECT) {
        activeUsers.erase(std::remove_if(activeUsers.begin(), activeUsers.end(), 
            [client](const UserInfo& u) { return u.socketId == client->id(); }), activeUsers.end());
        broadcastSystemStatus();
    }
    
    if (type == WS_EVT_DATA) {
        JsonDocument doc; 
        deserializeJson(doc, (char*)data);
        
        if (doc["type"] == "join") {
            // Remove duplicates
            activeUsers.erase(std::remove_if(activeUsers.begin(), activeUsers.end(), 
                [&](const UserInfo& u) { return u.id == doc["id"].as<String>(); }), activeUsers.end());
            
            activeUsers.push_back({doc["id"].as<String>(), doc["n"].as<String>(), client->id()});
            
            // CRITICAL: Send list to the person who just joined
            broadcastSystemStatus();
        } else {
            Serial1.println((char*)data);
            ws.textAll((char*)data, len);
        }
    }
}

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8"><meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Mesh Gateway</title>
    <style>
        :root { --bg:#0b141a; --header:#202c33; --accent:#00a884; --text:#e9edef; --border:#222d34; }
        * { box-sizing: border-box; margin: 0; padding: 0; font-family: sans-serif; }
        body { background: var(--bg); color: var(--text); height: 100dvh; display: flex; flex-direction: column; overflow: hidden; }
        #view-list { display: flex; flex-direction: column; height: 100%; }
        .header { height: 60px; background: var(--header); display: flex; align-items: center; padding: 0 15px; border-bottom: 1px solid var(--border); }
        .view { width: 100%; height: 100dvh; display: none; flex-direction: column; position: absolute; }
        .view.active { display: flex; }
        #users-list { flex:1; overflow-y: auto; background: var(--bg); }
        .user-item { padding: 15px; border-bottom: 1px solid var(--border); cursor: pointer; display: flex; align-items: center; gap: 10px; }
        .avatar { width: 35px; height: 35px; background: #444; border-radius: 50%; display: flex; align-items: center; justify-content: center; font-weight: bold; }
    </style>
</head>
<body>
    <div id="view-list" class="view active">
        <div class="header"><b>Mesh Gateway</b></div>
        <div style="padding: 10px; color: var(--accent); font-size: 12px;">MESH USERS</div>
        <div id="users-list"></div>
    </div>

    <div id="view-chat" class="view">
        <div class="header"><button onclick="location.reload()">←</button><b id="chat-title" style="margin-left:10px">Chat</b></div>
        <div id="msg-area" style="flex:1; overflow-y:auto; padding:15px;"></div>
    </div>

    <script>
        let socket, myId = localStorage.getItem('m_id') || "u"+Math.floor(Math.random()*1e6);
        localStorage.setItem('m_id', myId);
        let myNick = localStorage.getItem('m_nick');

        function connect() {
            socket = new WebSocket(`ws://${location.hostname}/ws`);
            socket.onopen = () => {
                console.log("Connected");
                socket.send(JSON.stringify({type:"join", id:myId, n:myNick}));
            };
            socket.onmessage = (e) => {
                const data = JSON.parse(e.data);
                if(data.type === 'sys') {
                    console.log("Users received:", data.users);
                    renderUsers(data.users);
                }
            };
        }

        function renderUsers(users) {
            const list = document.getElementById('users-list');
            list.innerHTML = users.map(u => `
                <div class="user-item">
                    <div class="avatar">${u.n[0]}</div>
                    <div>${u.n} ${u.id === myId ? '(Me)' : ''}</div>
                </div>
            `).join('');
        }

        window.onload = () => {
            if(!myNick) {
                myNick = prompt("Enter Nickname:");
                localStorage.setItem('m_nick', myNick);
            }
            connect();
        };
    </script>
</body>
</html>
)rawliteral";

void setup() {
    Serial.begin(115200);
    Serial1.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);
    WiFi.softAP(ssid);
    MDNS.begin("mesh");
    dnsServer.start(53, "*", WiFi.softAPIP());
    ws.onEvent(onEvent);
    server.addHandler(&ws);
    server.onNotFound([](AsyncWebServerRequest *request){ request->send_P(200, "text/html", index_html); });
    server.begin();
}

void loop() {
    dnsServer.processNextRequest();
    ws.cleanupClients();
    if (Serial1.available()) {
        String meshData = Serial1.readStringUntil('\n');
        ws.textAll(meshData);
    }
}