#include "lib.h"
#include "ota.h"
#include "time.h"
#include "telnet.h"

const int relay = 16; // Pin D0 on Wemos d1 mini.
const int button = 2; // Pin D4 on Wemos d1 mini.

const char *mqtt_server = "192.168.0.200";
const char *mqtt_username = "mqtt";
const char *mqtt_password = "mqtt";
const char *mqtt_clientID = "ESP_Diffuser";
const char *availabilityTopic = "diffuser/availability";
const char *stateTopic = "diffuser/state";
const char *cmdTopic = "diffuser/cmd";
const char *Wifi_SSID = "GPN";
const char *Wifi_password = "water1672";
const int mqtt_port = 1883;

bool deviceIsOn = false;

WiFiClient espClient;
PubSubClient MQTTclient(espClient);

void MQTTaction(String MQTTmessage)
{
  if (MQTTmessage == "OFF")
  {
    digitalWrite(relay, LOW);
    MQTTclient.publish(stateTopic, "OFF");
    deviceIsOn = !deviceIsOn;
  }
  else if (MQTTmessage == "ON")
  {
    digitalWrite(relay, HIGH);
    MQTTclient.publish(stateTopic, "ON");
    deviceIsOn = !deviceIsOn;
  }
  else
    telnetPrintln("Error: illegal message " + MQTTmessage);
}

void callback(char *topic, byte *payload, unsigned int length)
{
  telnetPrintTime();
  telnetPrint("Message arrived in topic: ");
  telnetPrint(topic);

  String MQTTmessage;
  for (unsigned int i = (unsigned int)0; i < length; i++)
    MQTTmessage = MQTTmessage + (char)payload[i];

  telnetPrint(", Message: ");
  telnetPrint(MQTTmessage);
  telnetPrintEnter();

  MQTTaction(MQTTmessage);
}

const char *boolToCharArray(bool devicState)
{
  if (devicState == true)
    return "ON";
  else
    return "OFF";
}

String boolToString(bool devicState)
{
  if (devicState == true)
    return "ON";
  else
    return "OFF";
}

void printSizeOfAvailableMemory(int updateInterval)
{
  static const int seconds = 1000;
  unsigned static int lastAttempt = 0;
  unsigned const int interval = updateInterval * seconds; //seconds
  static const int kilobytes = 1024;

  if (millis() - lastAttempt > interval)
  {
    lastAttempt = millis();
    double heap = system_get_free_heap_size();
    double heapInKiloBytes = heap / kilobytes;

    telnetPrintTime();
    telnetPrint((String)heapInKiloBytes);
    telnetPrint("KB of available memory.");
    telnetPrintEnter();
  }
}

bool wifiConnection()
{
  if ((WiFi.status() != WL_CONNECTED))
  {
    WiFi.disconnect();
    WiFi.begin(Wifi_SSID, Wifi_password);
    telnetPrint("Wifi reconnecting...");
    while (WiFi.status() != WL_CONNECTED)
    {
      delay(500);
      telnetPrint(".");
    }
    telnetPrintln("Wifi reconnected!");
  }
  if ((WiFi.status() == WL_CONNECTED))
    return true;
  else
    return false;
}

bool mqttConnection()
{
  if (!MQTTclient.connected())
  {
    telnetPrintln("MQTT reconnecting...");
    if (MQTTclient.connect(mqtt_clientID, mqtt_username, mqtt_password))
    {
      MQTTclient.subscribe(cmdTopic);
      MQTTclient.connected();
      MQTTclient.publish(availabilityTopic, "online", true);
      MQTTclient.publish(stateTopic, boolToCharArray(deviceIsOn));
      telnetPrintln("MQTT reconnected!");
    }
  }
  if (MQTTclient.connected())
    return true;
  else
    return false;
}

void printConnectionState(bool wifiState, bool mqttState)
{
  telnetPrintTime();
  telnetPrint("Wifi - ");
  telnetPrint(boolToCharArray(wifiState));
  telnetPrint(", ");
  telnetPrint("MQTT - ");
  telnetPrint(boolToCharArray(mqttState));
  telnetPrintEnter();
}

void checkStateOfWireless(int updateInterval)
{
  const int seconds = 1000;
  unsigned static int lastAttempt = 0;
  unsigned const int interval = updateInterval * seconds; //seconds

  if (millis() - lastAttempt > interval)
  {
    lastAttempt = millis();

    bool wifiState = wifiConnection();
    bool mqttState = mqttConnection();
    printConnectionState(wifiState, mqttState);
  }
}

bool toggleDevice(byte device, bool status)
{
  MQTTclient.publish(stateTopic, status ? "ON" : "OFF");
  digitalWrite(device, status);
  return !status;
}

bool hasDebounceTimeLapsed()
{
  static int lastDebounceTime = 0;
  const int debounceDelay = 100;
  int now = millis();
  int deltaT = now - lastDebounceTime;
  bool hasLapsed = (deltaT > debounceDelay);
  if (hasLapsed)
    lastDebounceTime = now;
  return hasLapsed;
}

bool hasButtonStateChanged()
{
  bool buttonIsPushed = digitalRead(button);
  static bool buttonWasPushed = false;
  bool hasChanged = (buttonWasPushed != buttonIsPushed);
  buttonWasPushed = buttonIsPushed;
  return hasChanged;
}

bool isButtonReleased()
{
  return (digitalRead(button) == true); //pin is being pulled up. Off state is when pin is up, On is when it is grounded.
}

void listenToButton()
{
  if (hasDebounceTimeLapsed() && hasButtonStateChanged() && isButtonReleased())
  {
    telnetPrintln("Relay is: " + boolToString(deviceIsOn));
    deviceIsOn = toggleDevice(relay, deviceIsOn);
  }
}

void setup()
{
  delay(100); // first time boot delay.
  pinMode(relay, OUTPUT);
  pinMode(button, INPUT);

  Serial.begin(115200);

  setupOTA(mqtt_clientID);
  WiFi.begin(Wifi_SSID, Wifi_password);
  initializeTime();

#if defined(ESP8266)
  wifi_station_set_hostname(mqtt_clientID); //Only works with ESP8266. Need to fix this so it also works with ESP32
#elif defined(ESP32)
  //WiFi.setHostname(mqtt_clientID); dosent work, i do not know why. needs to be fixed.
#endif

  telnetPrint("Connecting to Wifi");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    telnetPrint(".");
  }
  telnetPrintln();
  telnetStart();

  MQTTclient.setServer(mqtt_server, mqtt_port);
  MQTTclient.setCallback(callback);

  if (MQTTclient.connect(mqtt_clientID, mqtt_username, mqtt_password, availabilityTopic, 1, true, "offline")) // clientID, Username, Password, QOS, retain Flag, Payload
  {
    telnetPrintln("MQTT - OK");
    MQTTclient.subscribe(cmdTopic);
    MQTTclient.publish(availabilityTopic, "online", true);
  }
  else
    telnetPrintln("MQTT - ERROR");

}

void loop()
{
  ArduinoOTA.handle(); // function called to check if ESP needs to be programmed OTA.
  telnetHandler(); //dose telnet stuff.
  timeClient.update(); //updates time from internet.
  //printSizeOfAvailableMemory(10); //use only when debugging!!! memcheck takes a long time and slows down over all performance by a lot. 
  checkStateOfWireless(30); //How often the internet connection will be checked in seconds.
  MQTTclient.loop(); //Handles MQTT.
  listenToButton(); //Checks curent state of button. 
}