/**
   ESP32 + DHT22 Example for Wokwi
   
   https://wokwi.com/arduino/projects/322410731508073042
*/
#include <WiFi.h>
#include "DHTesp.h"
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

/************************* WiFi Access Point ***************************/
#define WLAN_SSID "Wokwi-GUEST"
#define WLAN_PASS ""
/************************* Adafruit.io Setup ***************************/
#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883                   // use 8883 for SSL
#define AIO_USERNAME  ""
#define AIO_KEY ""

/************ Global State (you don't need to change this!) ************/
#define DHT_PIN 0
#define LEDVERMELHO_PIN 1
#define LEDVERDE_PIN 2
#define LEDAMARELO_PIN 3
#define BTNVERMELHO_PIN 5
#define BTNVERDE_PIN 18
#define BTNAMARELO_PIN 19
DHTesp dhtSensor;

// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient client;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_USERNAME, AIO_KEY);

/****************************** Feeds ***************************************/

// Setup feeds 'temp' and 'hmdt' for publishing.
Adafruit_MQTT_Publish temp = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/temperatura");
Adafruit_MQTT_Publish hmdt = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/umidade");

// Setup a feed called 'onoff' for subscribing to changes to the ON/OFF button
Adafruit_MQTT_Subscribe onoffbtn = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/botao-on-slash-off", MQTT_QOS_1);

// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect() {
  int8_t ret;
  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }
  Serial.print("Connecting to MQTT... ");
  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
    Serial.println(mqtt.connectErrorString(ret));
    Serial.println("Retrying MQTT connection in 10 seconds...");
    mqtt.disconnect();
    delay(10000);  // wait 10 seconds
    retries--;
    if (retries == 0) {
      // basically die and wait for WDT to reset me
      while (1);
    }
  }
  Serial.println("MQTT Connected!");
}

// Function called back when an onoffbutton feed message arrives
void onoffcallback(char *data, uint16_t len) {
  Serial.print("Hey we're in an onoff callback, the button value is: ");
  Serial.println(data);
  if (strcmp(data, "ON") == 0) {
    Serial.println("Turning AC ON");
    digitalWrite(LED_PIN, HIGH);
  } else {
    Serial.println("Turning AC OFF");
    digitalWrite(LED_PIN, LOW);
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(BTN_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  dhtSensor.setup(DHT_PIN, DHTesp::DHT22);
  delay(10);

  Serial.println(F("Adafruit MQTT demo"));

  // Connect to WiFi access point.
  Serial.print("Connecting to ");
  Serial.println(WLAN_SSID);

  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(); Serial.println("WiFi connected");
  Serial.println("IP address: "); Serial.println(WiFi.localIP());

  // Set the callback function to be called when feed's message arrives
  onoffbtn.setCallback(onoffcallback);
  
  // Setup MQTT subscription for time feed.
  mqtt.subscribe(&onoffbtn);
}

unsigned long lastTime = 0;
int lastState = LOW;

void loop() {
  // Ensure the connection to the MQTT server is alive (this will make the first
  // connection and automatically reconnect when disconnected).  See the MQTT_connect
  // function definition further below.
  MQTT_connect();

  // this is our 'wait for incoming subscription packets and callback em' busy subloop
  // try to spend your time here:
  mqtt.processPackets(10000);
  
  // ping the server to keep the mqtt connection alive
  // NOT required if you are publishing once every KEEPALIVE seconds
  
  if(! mqtt.ping()) {
    mqtt.disconnect();
  }
  
  // Publish feeds
  unsigned long now = millis();
  if (now - lastTime > 10000) {
    lastTime = now;
    TempAndHumidity data = dhtSensor.getTempAndHumidity();
    Serial.println("Temperature: " + String(data.temperature, 1) + "°C");
    Serial.println("Humidity...: " + String(data.humidity, 0) + "%");
    Serial.println("---");
    char strTemperature[20], strHumidity[20];
    sprintf(strTemperature, "%.1f", data.temperature);
    sprintf(strHumidity, "%.0f", data.humidity);
    temp.publish(strTemperature);
    hmdt.publish(strHumidity);
  }
  if (digitalRead(BTN_PIN) == LOW) {
    lastState = !lastState;
    Serial.println("Button released");
    String txt = "Turning LED ";
    txt = lastState ? "ON" : "OFF";
    Serial.println(txt);
    digitalWrite(LED_PIN, lastState ? HIGH : LOW);
  }
}
