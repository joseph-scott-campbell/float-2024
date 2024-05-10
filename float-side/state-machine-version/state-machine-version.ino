#include <ArduinoWebsockets.h>
#include <WiFi.h>
#include "MS5837.h"
#include <Wire.h>
#include <Adafruit_NeoPixel.h>
#include <EEPROM.h>

#define EEPROM_SIZE 256
#define COUNTER_ADDR 0   // store where to write next data to
#define CURRENT_STATE 1  // store what state it is in for error recovery
#define DATA_START 2     // store data starting here and ending at end of EEPROM

#define COMPANY_NUMBER "1337"  // returned from
#define FLOAT_HEIGHT 61        // height of float in cm, used to convert depth from measuring at top to measuring at bottom

/* breadboard
#define M2 6
#define M1 5
#define SF 10
#define HT 11
#define HB 12
*/

#define M2 10
#define M1 9
#define HT 5
#define HB 6

#define UP 0
#define DOWN 1
#define STOP 2

#define SINK 0
#define FLOAT 1
#define WAIT 2
#define COMMUNICATE 3
#define HOME 4

#define TRIP 0
#define DEPTH_INTERVAL 5000
#define WAIT_INTERVAL 15000

using namespace websockets;
WebsocketsServer server;

const char* ssid = "sunk";          // put SSID here
const char* password = "sunksunk";  // put password here

Adafruit_NeoPixel pixels(1, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);
MS5837 sensor;

uint8_t state;
uint32_t time_now;
uint32_t depth_check;
uint32_t wait_check;
uint8_t depth_counter;
bool recovering_from_error = false;
unsigned long time_since_start;

void drive(uint8_t dir) {
  if (dir == DOWN) {
    digitalWrite(M2, HIGH);
    digitalWrite(M1, LOW);
  } else if (dir == UP) {
    digitalWrite(M2, LOW);
    digitalWrite(M1, HIGH);
  } else if (dir == STOP) {
    digitalWrite(M2, LOW);
    digitalWrite(M1, LOW);
  }
}

void connect_to_wifi() {
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

void store_depth() {
  sensor.read();
  float raw_data = sensor.depth();  // get data in cm
  if (raw_data < 0) {
    raw_data = 0;
  }
  uint8_t data = uint8_t(((raw_data * 100) + FLOAT_HEIGHT) / 5);
  Serial.println(raw_data * 5);
  Serial.println(data);

  EEPROM.write(depth_counter, data);
  depth_counter++;
  EEPROM.write(COUNTER_ADDR, depth_counter);
  EEPROM.commit();
}

void setup() {
  Serial.begin(115200);
  pinMode(M2, OUTPUT);
  pinMode(M1, OUTPUT);
  pinMode(HT, INPUT);
  pinMode(HB, INPUT);


  if (EEPROM.begin(EEPROM_SIZE)) {
    Serial.println("EEPROM Initalized");
  }

  depth_counter = EEPROM.read(COUNTER_ADDR);

  pixels.begin();
  // Connect to wifi
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

  time_since_start = millis();

  // if it crashes in a floating sinking or waiting state resume
  // otherwise home
  switch (EEPROM.read(CURRENT_STATE)) {
    case SINK:
      recovering_from_error = true;
      state = SINK;
      drive(UP);
      depth_check = millis();
      break;
    case FLOAT:
      recovering_from_error = true;
      state = FLOAT;
      drive(DOWN);
      depth_check = millis();
      break;
    case WAIT:
      recovering_from_error = true;
      state = WAIT;
      depth_check = millis();
      drive(STOP);
      break;
    default:
      recovering_from_error = false;
      connect_to_wifi();
      state = HOME;
      drive(DOWN);
      break;
  }
}


void loop() {
  if (EEPROM.read(CURRENT_STATE) != state) {
    EEPROM.write(CURRENT_STATE, state);
    EEPROM.commit();
  }

  time_now = millis();
  pixels.fill(0xFFFFF);
  pixels.show();
  switch (state) {
    case HOME:
      //Serial.println("Home");
      drive(DOWN);
      if (digitalRead(HB) == TRIP) {
        drive(STOP);
        state = COMMUNICATE;
      }
      break;
    case COMMUNICATE:
      {
        if (recovering_from_error) {
          connect_to_wifi();
          recovering_from_error = false;
        }

        //Serial.println("Communicate");
        pixels.fill(0x0000FF);
        pixels.show();

        WebsocketsClient client = server.accept();  // accepting websockets connection
        if (client.available()) {
          pixels.fill(0x00FF00);
          pixels.show();

          WebsocketsMessage msg = client.readBlocking();

          if (msg.data() == "profile") {
            client.send("going down");
            client.close();
            drive(UP);
            state = SINK;
          } else if (msg.data() == "get_depth") {
            String message = "[";
            depth_counter = EEPROM.read(COUNTER_ADDR);
            message = message + String(EEPROM.read(DATA_START) * 5);
            for (int i = DATA_START + 1; i < depth_counter; i++) {
              message = message + ",";
              message = message + String(EEPROM.read(i) * 5);
            }

            message = message + "]";

            client.send(message);
            client.close();
            state = COMMUNICATE;
          } else if (msg.data() == "get_pressure") {
            String message = "[";
            depth_counter = EEPROM.read(COUNTER_ADDR);
            message = message + String(float((EEPROM.read(DATA_START) * 5)) * 997.0474 * 9.80665);
            for (int i = DATA_START + 1; i < depth_counter; i++) {
              message = message + ",";
              message = message + String(float((EEPROM.read(i) * 5)) * 997.0474 * 9.80665);
            }

            message = message + "]";

            client.send(message);
            client.close();
          } else if (msg.data() == "clear_data") {
            depth_counter = DATA_START;
            EEPROM.write(COUNTER_ADDR, depth_counter);
            EEPROM.commit();
            client.close();
          } else if (msg.data() == "get_info") {
            String message = String("Company Number: RN1337");
            message = message + " | " + "Uptime (Seconds): " + String(millis() / 1000) + " | " + "Time Recording (Seconds): " + String(EEPROM.read(COUNTER_ADDR) * 5);
            client.send(message);
          } else if (msg.data() == "help") {
            client.send("JENA - the Sunk Robotics vertically profiling float\nCommands:\n\t* profile - float will do a vertical profile\n\t* get_info - returns uptime and company number\n\t* get_depth - Send depth data stored in EEPROM in centimeters (accurate to 5cm)\n\t* get_pressure - return pressure data stored in EEPROM\n\t* clear_data - Clear data in EEPROM and collect fresh data\n\t* break - exit gracefully without profile\n\t* help - print this message");
            client.close();
          } else if (msg.data() == "break") {
            client.send("goodbye");
            client.close();
          } else {
            client.send("Unknown command for a list of available commands type help");
            client.close();
          }
        }
        break;
      }
    case FLOAT:
      //Serial.println("Float");
      if (time_now - depth_check >= DEPTH_INTERVAL) {
        //read sensor
        store_depth();
        depth_check = time_now;
      }
      if (digitalRead(HB) == TRIP) {
        drive(STOP);
        state = COMMUNICATE;
        for (int i = 0; i < 5000; i++) {
          if (time_now - depth_check >= DEPTH_INTERVAL) {
            //read sensor
            store_depth();
            depth_check = time_now;
          }
          delay(1);
        }
      }
      break;
    case SINK:
      //Serial.println("Sink");
      if (time_now - depth_check >= DEPTH_INTERVAL) {
        //read sensor
        store_depth();
        depth_check = time_now;
      }
      if (digitalRead(HT) == TRIP) {
        drive(STOP);
        wait_check = time_now;
        state = WAIT;
      }
      break;
    case WAIT:
      //Serial.println("Wait");
      if (time_now - depth_check >= DEPTH_INTERVAL) {
        //read sensor
        store_depth();
        depth_check = time_now;
      }
      if (time_now - wait_check >= WAIT_INTERVAL) {
        drive(DOWN);
        state = FLOAT;
      }
      break;
  }
}
