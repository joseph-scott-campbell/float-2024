// fairily massive libraries, but ESP32-S2 can handle it
#include <ArduinoWebsockets.h>
#include <WiFi.h>

const char* ssid = "TP-Link_51CA"; // put SSID here
const char* password = "password"; // put password here

using namespace websockets;

WebsocketsServer server;

void profile(){
  // this will do a vertical profile
}

void setup() {
  Serial.begin(115200);
  // Connect to wifi
  WiFi.begin(ssid, password);

  // Wait some time to connect to wifi
  for (int i = 0; i < 15 && WiFi.status() != WL_CONNECTED; i++) {
    Serial.print(".");
    delay(1000);
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());   //You can get IP address assigned to ESP

  server.listen(80);
  Serial.print("Is server live? ");
  Serial.println(server.available());
}

void loop() {
  bool looping = true; // allows clean break from loop
  WebsocketsClient client = server.accept(); // accepting websockets connection
  if (client.available()) {
    while (looping) {
      WebsocketsMessage msg = client.readBlocking();
      // log
      Serial.print("Got Message: ");
      Serial.println(msg.data());

      // parses commands being sent
      // uses if statement because switch statements want chars
      if (msg.data() == "profile") {
        client.send("going down");
      } else if (msg.data() == "break") {
        client.send("goodbye");
        looping = false;
      } else {
        client.send(msg.data());
      }
    }
    // close the connection
    client.close();
  }
}
