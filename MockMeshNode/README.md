Mock MEsh Node

In this setup:

ESP32-C3 (The Gateway): Runs the WebServer and WebSocket logic.

ESP32 #2 (The Mesh Simulator): Acts exactly like the XIAO/Meshtastic node. It listens for your JSON and sends back "Mock Mesh" responses over the wire.


Step 1: The Hardware Connection

| ESP32-C3 (Gateway) |       ESP32 #2 (Simulator)      |     Function     |
--------------------------------------------------------------------------|
|    GPIO 21 (TX)    |  RX (Check your board's RX pin) | Gateway -> Mesh  |
|    GPIO 20 (RX)    |  TX (Check your board's TX pin) | Mesh -> Gateway  |
|        GND         |               GND               | Common Reference | 