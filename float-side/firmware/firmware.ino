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
#include <Wire.h>
#include "MS5837.h"

// motor driver pins
#define IN_1 5
#define IN_2 6

// hall effect sensor pins
#define HALL_EFFECT_1 12
#define HALL_EFFECT_2 11

// how long it will wait before reciving input from hall effect sensor
// will enter fail state if it waits past the timeout
#define TIMEOUT 30000
#define DEPTH_CHECK_INTERVAL 5000

// How long to float and how long to sink in ms
#define TIME_FLOATING 10000
#define TIME_SINKING 12000

using namespace websockets;
WebsocketsServer server;

const char* ssid = "TP-Link_51CA";  // put SSID here
const char* password = "password";  // put password here
// arrays where depth data will be kept
float depth_data[1000]; 
float pressure_data[1000];
unsigned short recording_cycle = 0;  // keep track of what index to write to

Adafruit_NeoPixel pixels(1, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);
MS5837 sensor;


// variables for storing time for depth sensor
unsigned long depth_starting_time;
unsigned long depth_current_time;

void setup() {
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

  Wire.begin();

  while (!sensor.init()) {
    Serial.println("Init failed!");
    Serial.println("Are SDA/SCL connected correctly?");
    Serial.println("Blue Robotics Bar30: White=SDA, Green=SCL");
    Serial.println("\n\n\n");
    delay(5000);
  }

  sensor.setModel(MS5837::MS5837_02BA);
  sensor.setFluidDensity(997);  // kg/m^3 (freshwater, 1029 for seawater)
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
  unsigned long starting_time = millis(); // time for delays in profile
  //depth_starting_time = millis(); // time for measuring depth
  // setting color to purple to indicate
  pixels.fill(0xFF00FF);
  pixels.show();

  move_piston_down();  // extending piston

  // waiting TIME_FLOATING milliseconds
  starting_time = millis();
  while (not(time_check(starting_time, millis(), TIME_FLOATING))) {
    check_depth();
  }

  move_piston_up();  // retracting piston

  starting_time = millis();
  while (not(time_check(starting_time, millis(), TIME_SINKING))) {
    check_depth();
  }

  move_piston_down();  // extending piston
}

void move_piston_up() {
  Serial.println("moving up");
  digitalWrite(IN_1, HIGH);
  digitalWrite(IN_2, LOW);
  pixels.fill(0xFFFFFF);
  pixels.show();

  // continuing to move piston until hall effect or timeout
  unsigned long piston_starting_time = millis();
  while (digitalRead(HALL_EFFECT_1)) {
    if (time_check(piston_starting_time, millis(), TIMEOUT)) {
      fail_state();  // indefinate blocking
    }
    check_depth();
  }

  digitalWrite(IN_1, HIGH);
  digitalWrite(IN_2, HIGH);
}

void move_piston_down() {
  Serial.println("moving down");
  digitalWrite(IN_1, LOW);
  digitalWrite(IN_2, HIGH);
  pixels.fill(0x00FFFF);
  pixels.show();

  // continuing to move piston until hall effect or timeout
  unsigned long piston_starting_time = millis();
  while (digitalRead(HALL_EFFECT_2)) {
    if (time_check(piston_starting_time, millis(), TIMEOUT)) {
      fail_state();  // indefinate blocking
    }
    check_depth();
  }

  Serial.println("hall effect detected");

  digitalWrite(IN_1, HIGH);
  digitalWrite(IN_2, HIGH);  
}

void check_depth() {
  if (time_check(depth_starting_time, depth_current_time, DEPTH_CHECK_INTERVAL)) {
    // restarting count for checking depth
    depth_starting_time = millis();

    // check to stop overflow
    //if (recording_cycle >= sizeof(depth_data) / sizeof(depth_data[1]) || recording_cycle >= sizeof(pressure_data) / sizeof(pressure_data[1])) {
    //  recording_cycle = 0;
    //}
    depth_data[recording_cycle] = sensor.depth();
    pressure_data[recording_cycle] = sensor.pressure();
    recording_cycle++;
    Serial.println("wrote");
  }
}

bool time_check(unsigned long starting_time, unsigned long current_time, unsigned long target_time) {
  if (current_time - starting_time >= target_time) {
    return (true);
  } else {
    return (false);
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
