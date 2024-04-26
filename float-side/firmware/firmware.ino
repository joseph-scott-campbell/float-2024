// INPUTS FOR MD10A MOTOR DRIVER
//
// 12v OUT (Motor OUT)
//----------
// IN_2 HIGH
// IN_1 LOW
//
// -12v OUT (Motor IN)
//----------
// IN_2 LOW
// IN_1 HIGH
//
// 0v OUT (Stop)
//----------
// IN_2 HIGH
// IN_1 HIGH

// code is written in chronological order
// fairily massive libraries, but ESP32-S2 can handle it
#include <ArduinoWebsockets.h>
#include <WiFi.h>
#include <Adafruit_NeoPixel.h>

// motor driver pins
#define IN_1 9
#define IN_2 10

// hall effect sensor pins
#define HALL_EFFECT_1 5
#define HALL_EFFECT_2 6

// how long it will wait before reciving input from hall effect sensor
// will enter fail state if it waits past the timeout
#define TIMEOUT 30000

using namespace websockets;
WebsocketsServer server;

const char* ssid = "penis";         // put SSID here
const char* password = "balls123";  // put password here

hw_timer_t* recording_timer = NULL;
unsigned short depth_data[1000];  // array where depth data will be kept
uint8_t recording_cycle = 0;      // keep track of what index to write to

Adafruit_NeoPixel pixels(1, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);

void IRAM_ATTR record_depth() {
  // get depth data
  // still waiting for proper cable to arrive

  // depth data is stored in 1 dimension
  // data is sampled in 5 second increments
  depth_data[recording_cycle] = 35;
  recording_cycle++;
}

void setup() {
  // setting up interupts
  // running record_depth() every 5 seconds
  recording_timer = timerBegin(0, 80, true);
  timerAttachInterrupt(recording_timer, &record_depth, true);
  timerAlarmWrite(recording_timer, 5000000, true);
  timerAlarmEnable(recording_timer);

  pinMode(NEOPIXEL_POWER, OUTPUT);
  digitalWrite(NEOPIXEL_POWER, HIGH);

  pixels.begin();

  // Setting up GPIO
  pinMode(IN_1, OUTPUT);
  pinMode(IN_2, OUTPUT);

  // Setting up hall effect
  pinMode(HALL_EFFECT_1, INPUT);
  pinMode(HALL_EFFECT_2, INPUT);

  Serial.begin(115200);
  // Connect to wifi
  WiFi.setHostname("Sunk Robotics Float");
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
  pixels.fill(0x0000FF);
  pixels.show();

  bool looping = true;                        // allows clean break from loop
  WebsocketsClient client = server.accept();  // accepting websockets connection
  pixels.fill(0x00FF00);
  pixels.show();

  if (client.available()) {
    while (looping) {
      WebsocketsMessage msg = client.readBlocking();

      // parses commands being sent
      // uses if statement because switch statements want chars
      if (msg.data() == "profile") {
        client.send("going down");
        looping = false;
        client.close();  // closing client because connection will time out once it goes underwater
        profile();
      } else if (msg.data() == "break") {
        client.send("goodbye");
        looping = false;
      } else if (msg.data() == "get_data") {

        String message = "[";
        message = message + depth_data[0];
        for (int i = 1; i < recording_cycle; i++) {
          message = message + ",";
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
  // setting color to purple to indicate
  pixels.fill(0xFF00FF);
  pixels.show();

  // this function works because of hall_effect() interrupt
  // DON'T GET RID OF hall_effect() INTERRUPT
  // WILL CAUSE HARDWARE DAMMAGE
  descend();  // extending piston
  // possible future feature update: use depth rather than time for sinking
  delay(10000);  // giving float 10 seconds to sink
  ascend();      // retracting piston out
  delay(10000);
  descend();  // extending piston
}

void ascend() {
  digitalWrite(IN_1, HIGH);
  digitalWrite(IN_2, LOW);
  Serial.println("ascending");
  pixels.fill(0xFFFFFF);
  pixels.show();

  // continuing to move piston until hall effect or timeout
  unsigned short counter = 0;
  while (digitalRead(HALL_EFFECT_1) && counter < TIMEOUT) {
    counter++;
    delay(1);
  }
  digitalWrite(IN_1, HIGH);
  digitalWrite(IN_2, HIGH);

  if (counter >= TIMEOUT - 1) {
    fail_state();
  }
}

void descend() {
  digitalWrite(IN_1, LOW);
  digitalWrite(IN_2, HIGH);
  Serial.println("descend");
  pixels.fill(0x00FFFF);
  pixels.show();

  // continuing to move piston until hall effect or timeout
  unsigned short counter = 0;
  while (digitalRead(HALL_EFFECT_2) && counter < TIMEOUT) {
    counter++;
    delay(1);
  }
  digitalWrite(IN_1, HIGH);
  digitalWrite(IN_2, HIGH);
  if (counter >= TIMEOUT - 1) {
    fail_state();
  }
}

void fail_state() {
  // stops motor from moving and doing dammage
  digitalWrite(IN_1, HIGH);
  digitalWrite(IN_2, HIGH);

  // blinking indicator LED red
  while (true) {
    // setting color to red
    pixels.fill(0xFF0000);
    pixels.show();
    delay(500);
    // turning off LED
    pixels.fill(0x000000);
    pixels.show();
    delay(500);
  }
}
