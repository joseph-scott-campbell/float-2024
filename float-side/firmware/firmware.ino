// code is written in chronological order
// fairily massive libraries, but ESP32-S2 can handle it
#include <ArduinoWebsockets.h>
#include <WiFi.h>
#include <Adafruit_NeoPixel.h>

// motor driver pins
#define PHASE 9
#define ENABLE 8

// hall effect sensor pins
#define HALL_EFFECT 18

// how long it will wait before reciving input from hall effect sensor
// will enter fail state if it waits past the timeout
#define TIMEOUT 30000

using namespace websockets;
WebsocketsServer server;

const char* ssid = "TP-Link_51CA";  // put SSID here
const char* password = "password";  // put password here

hw_timer_t* recording_timer = NULL;
unsigned short depth_data[1000];  // array where depth data will be kept
uint8_t recording_cycle = 0;      // keep track of what index to write to

bool hall_effect_triggered = false;


void IRAM_ATTR record_depth() {
  // get depth data
  // still waiting for proper cable to arrive
  
  // depth data is stored in 1 dimension
  // data is sampled in 5 second increments
  depth_data[recording_cycle] = 35;
  recording_cycle++;
}

void hall_effect() {
  // DO NOT REMOVE FUNCTION
  // WILL CAUSE HARDWARE DAMMAGE
  digitalWrite(ENABLE, LOW);
  hall_effect_triggered = true;
}


void setup() {
  // setting up interupts
  // running record_depth() every 5 seconds
  recording_timer = timerBegin(0, 80, true);
  timerAttachInterrupt(recording_timer, &record_depth, true);
  timerAlarmWrite(recording_timer, 5000000, true);
  timerAlarmEnable(recording_timer);

  // setting up hall effect sensor interrupt
  // DO NOT REMOVE, WILL CAUSE HARDWARE DAMMAGE
  attachInterrupt(digitalPinToInterrupt(HALL_EFFECT), hall_effect, FALLING);

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

      // parses commands being sent
      // uses if statement because switch statements want chars
      if (msg.data() == "profile") {
        client.send("going down");
        profile();
        looping = false;
        client.close();  // closing client because connection will time out once it goes underwater
        // profile();
      } else if (msg.data() == "break") {
        client.send("goodbye");
        looping = false;
      } else if (msg.data() == "get_data") {

        String message = "[";
        message = message + depth_data[0];
        for (int i = 1; i < recording_cycle; i++) {
          message = message + ", ";
          message = message + depth_data[i];
        }
        message = message + "]";
        client.send(message);
        client.send(String(recording_cycle));
      } else {
        client.send(msg.data());
      }
    }
    // close the connection
    client.close();
  }
}

void profile() {
  hall_effect_triggered = false;
  // this function works because of hall_effect() interrupt
  // DON'T GET RID OF hall_effect() INTERRUPT
  // WILL CAUSE HARDWARE DAMMAGE
  descend();  // retracting piston

  // possible future feature update: use depth rather than time for sinking
  delay(10000);  // giving float 10 seconds to sink

  ascend();  // pusing piston out
}

void ascend() {  
  hall_effect_triggered = false;
  digitalWrite(PHASE, LOW);
  digitalWrite(ENABLE, HIGH);
  Serial.println(hall_effect_triggered);

  // continuing to move piston until hall effect or timeout
  unsigned short counter = 0;
  while (not(hall_effect_triggered) && counter < TIMEOUT) {
    counter++;
    Serial.println(counter);
    delay(1);
  }
  if (counter >= TIMEOUT - 1) {
    fail_state();
  }
}

void descend() {
  hall_effect_triggered = false;
  digitalWrite(PHASE, HIGH);
  digitalWrite(ENABLE, HIGH);
  Serial.println("foo");
  Serial.println(hall_effect_triggered);

  // continuing to move piston until hall effect or timeout
  unsigned short counter = 0;
  while (not(hall_effect_triggered) && counter < TIMEOUT) {
    counter++;
    delay(1);
  }
  if (counter >= TIMEOUT - 1) {
    fail_state();
  }
}

void fail_state() {
  // stops motor from moving and doing dammage
  digitalWrite(ENABLE, LOW);

  Adafruit_NeoPixel pixels(1, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);
  pinMode(NEOPIXEL_POWER, OUTPUT);
  digitalWrite(NEOPIXEL_POWER, HIGH);

  pixels.begin();

  while (true) {
    // setting color to red
    pixels.fill(0xFF0000);
    pixels.show();
    delay(500);
    pixels.fill(0x000000);
    pixels.show();
    delay(500);
  }
}
