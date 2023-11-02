// fairily massive libraries, but ESP32-S2 can handle it
#include <ArduinoWebsockets.h>
#include <WiFi.h>

#define PHASE 9
#define ENABLE 8
#define HALL_EFFECT 10

const char* ssid = "TP-Link_51CA";  // put SSID here
const char* password = "password";  // put password here

hw_timer_t* recording_timer = NULL;
unsigned short depth_data[1000];  // array where depth data will be kept
uint8_t recording_cycle = 0;      // keep track of what index to write to

void IRAM_ATTR record_depth() {
  // get depth data
  // still waiting for proper cable to arrive
  // placeholderch
  if (recording_cycle > 999) {  // preventing buffer overflow
    recording_cycle = 0;
  }
  depth_data[recording_cycle] = recording_cycle;
}

using namespace websockets;
WebsocketsServer server;

void setup() {
  // setting up interupts
  // running record_depth() every 5 seconds
  recording_timer = timerBegin(0, 80, true);
  timerAttachInterrupt(recording_timer, &record_depth, true);
  timerAlarmWrite(recording_timer, 1000000, true);
  timerAlarmEnable(recording_timer);
  // Setting up GPIO
  pinMode(ENABLE, OUTPUT);
  pinMode(PHASE, OUTPUT);

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
  Serial.println(WiFi.localIP());  //You can get IP address assigned to ESP

  server.listen(80);
  Serial.print("Is server live? ");
  Serial.println(server.available());
}


void loop() {
  bool looping = true;                        // allows clean break from loop
  WebsocketsClient client = server.accept();  // accepting websockets connection
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
        looping = false;
        client.close();  // closing client because connection will time out once it goes underwater
        // profile();
      } else if (msg.data() == "break") {
        client.send("goodbye");
        looping = false;
      } else if (msg.data() == "get_data") {
        String message = "[";
        for (int i = 0; i < recording_cycle; i++) {
          message = message + depth_data[recording_cycle];
          message = message + ", ";
        }
        message = message + "]";
        client.send(message);
      } else {
        client.send(msg.data());
      }
    }
    // close the connection
    client.close();
  }
}

void profile() {
  // placeholder
}
