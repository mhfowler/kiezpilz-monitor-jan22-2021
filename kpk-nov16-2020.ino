/*
 * Telegram Bot api interface 
 * Uses an Arduino MKRWIFI1010 as a Telegram Bot 
 * A telegram Bot acts similar to an automatic answering machine, it accepts your commands, process them and, eventually, answer to you with a result.
 * You can send commands to your Bot from any smartphone (no matter the brand) with Telegram installed.
 * 
 * In the actual state the code is, more or less, a proof of concept.
 * It can be improved of course.
 * 
 * Changelog
 * Version 1.02 - 2020-06-02
 * Tested with MKRWIFI1010 FW 1.3.0 and updated certificates
 * you need to flash the latest firmware and then upload certificate from telegram.org:443
 * > Open Tools->Wifi101/wifinina updater
 * > Make sure your board is connected, check the ports on the left of the updater tool, the board must be there
 * > Click Open updater sketch and upload it to the board
 * > After upload, in the updater choose the latest firmware for your board and click update firmware
 * > After update, in the updater, section SSL certificates, click Add domain, in the dialog write telegram.org:443 then press OK
 * > Click Upload certificates to wifi module
 * > Upload this sketch to the board
 * Done, enjoy!
 * 
 * 
 * Version 1.01:
 * Added arduino_secrets.h for sensitive data.
 * Tested with MKRWIFI1010 FW 1.0.0 and 1.1.0 
 * 
 * 
 * Version 1.00 - First release
 * Available commands:
 * /start       Starts the Bot and answer with a welcome message 
 * help         Answer with some messages about available commands
 * sketch       Answer with the sketch informations at compiling time
 * time         Answer with the UTC time from Google server
 * led_on       Turns ON the built-in led (imagine it can be a relay instead...)
 * led_off      Turns OFF the built-in led 
 * led_status   Answer with a message about the built-in led status
 * 
 >Web:       https://dev.appsbydavidev.it/
 >Twitter:   https://twitter.com/appsbydavidev
 >Instagram: https://instagram.com/appsbydavidev/
 >Youtube:   https://www.youtube.com/channel/UCNY3KkW72IEFlLiBgTBdoKw
 >Email:     vdavidex@gmail.com
 */

#include <SPI.h>              // needs
#include <SD.h>
#include <WiFiNINA.h>         // New Arduino MKR WIFI 1010 library
#include <Arduino_MKRENV.h>
#include "ESP8266TelegramBOT.h"
#include <ArduinoJson.h>

// WiFi network and Telegram bot informations, V1.01 uses the "arduino_secrets.h" tab now, useful for code sharing... 
// Please enter your sensitive data in the Secret tab/arduino_secrets.h
#include "arduino_secrets.h"
char ssid[] = SECRET_SSID;          // network name
char password[] =SECRET_PASS;       // network password

// writing to sd card
Sd2Card card;
File sdFile;
String sdFilePath = "KPK.txt";

// for resetting arduino and reconnecting to wifi 
void(* resetFunc) (void) = 0;
int Reset = 1;
//int msUntilRestart = 120000;
int msUntilRestart = 1800000;

int status = WL_IDLE_STATUS;

void Bot_EchoMessages();
TelegramBOT bot(BOT_TOKEN, BOT_NAME, BOT_USERNAME);

int     Bot_mtbs = 5000; // messages polling time, milliseconds
long    Bot_lasttime; 
String  Sender;  

boolean withInternet = true;
int sensor_interval = 60000; // milliseconds
//int sensor_interval = 5000; // milliseconds
long sensor_lasttime;
File myFile;
int relayPin = 3;

void setup() {

  // pin for reset
  digitalWrite(Reset, HIGH);
  pinMode(Reset, OUTPUT); 
  
  //Initialize serial and wait for port to open, note that yor MKR will not continue 
  //with the execution until the serial is opened on the connected PC
  Serial.begin(9600);

  pinMode(6, OUTPUT);      // set the LED pin mode for the built-in led on MKRWiFi101
  pinMode(relayPin, OUTPUT);      // led for testing with breadboard

  bool usbMode = false;
  if (usbMode) {   
    while (!Serial) {
      ; // wait for serial port to connect. Needed for native USB port only
    }
  }

  const int CS=4;
  pinMode(CS, OUTPUT); // change this to 53 if you are using arduino mega
  digitalWrite(CS, HIGH); // Add this line

  Serial.println("Initializing SD card....");
//   if ( ! card.init(SPI_HALF_SPEED, CS)) {
  if (!SD.begin(CS)) {
    Serial.println("initialization failed.");
    return;
  } else {
    Serial.println("sd card initialized");
  }

  // initialize data file 
  //  sdFilePath = sdFilePath + '-' + String(random(100000)) + '.txt';  // for some reason arduino cant create files right now...
  sdFile = SD.open(sdFilePath, FILE_WRITE);
  if (sdFile) {
    sdFile.println("initialized at: " + String(millis()));
     sdFile.close();
    Serial.println("wrote initialization to " + sdFilePath);
  } else {
     Serial.println("error writing to " + sdFilePath);
  }


  if (!ENV.begin()) {
    Serial.println("Failed to initialize MKR ENV shield!");
    while (1);
  }

  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    while (true);
  }

  String fv = WiFi.firmwareVersion();
  //if (fv != "1.0.0") {
  Serial.println("Current WiFi fw version: "+fv);
  //}

  // attempt to connect to Wifi network:
  if (withInternet) {
    while (status != WL_CONNECTED) {
      Serial.print("Attempting to connect to SSID: ");
      Serial.println(ssid);
      // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
      status = WiFi.begin(ssid, password);
  
      // wait 10 seconds for connection:
      delay(5000);
    }
  
    // you're connected now, so print out the status:
    printWifiStatus();
  
    //init Telegram Bot
    bot.begin();      // launch Bot functionalities
  }
  else {
    Serial.println("++ running without internet");
  }

  Serial.println("End of setup");
  
}

void reconnectWifiAndBot() {
    if (withInternet) {
      while (status != WL_CONNECTED) {
        Serial.print("Attempting to connect to SSID: ");
        Serial.println(ssid);
        // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
        status = WiFi.begin(ssid, password);
    
        // wait 10 seconds for connection:
        delay(5000);
      }
      // you're connected now, so print out the status:
      printWifiStatus();
      
      //init Telegram Bot
      bot.begin();      // launch Bot functionalities
    }
}

String logRecipient = "643117986";
void log(String s) {
  Serial.println(s);
  if (withInternet) {
    bot.sendMessage(logRecipient, s, "");  
  }
  sdFile = SD.open(sdFilePath, FILE_WRITE);
  if (sdFile) {
    sdFile.println(s);
  }
  // close the file:
  sdFile.close();
}

void loop() { 

  bool blinkTest = false;
  if (blinkTest) {
    digitalWrite(relayPin, HIGH);   // turn the LED on (HIGH is the voltage level)
    delay(1000);                       // wait for a second
    digitalWrite(relayPin, LOW);    // turn the LED off by making the voltage LOW
    delay(1000);   
  }

  // check if internet connected and reset if not
  if (withInternet) {
    if (WiFi.status() != WL_CONNECTED) {
      log("++ disconnected from internet, restarting arduino");
      digitalWrite(Reset, LOW);
      log("++ this code should never get reached");
      resetFunc();
    }
    // also just reset every hour or so
    if (millis() > msUntilRestart) {
       log("++ periodic hourly reset");
       digitalWrite(Reset, LOW);
       log("++ this code should never get reached");
       resetFunc();
    }
  }
  
  //polling messages from telegram server
  if (withInternet) {
    if (millis() > Bot_lasttime + Bot_mtbs)  {
      // launch API GetUpdates up to xxx message
      if (bot.getUpdates(bot.message[0][1]) == true) {
        //Telegram is responding!
        Serial.println("Bot connected!");
        Bot_EchoMessages();                  // reply to message(s) with Echo
      } else {
        //Telegram is NOT responding!
        Serial.println("Bot not connected...");
        //Serial.println("Bot not connected, restarting the Bot...");
        //NVIC_SystemReset();      // processor software reset, just in case...
      }
      Bot_lasttime = millis();
    }
  }
  if (millis() > sensor_lasttime + sensor_interval) {
    float humidity = readSensors();
    sensor_lasttime = millis();
    float minHumidity = 85.0;
    if (humidity < minHumidity) {
      digitalWrite(relayPin, HIGH);
      log("++ humidifier ON (humidity: " + String(humidity) + ")");
    }
    else {
      digitalWrite(relayPin, LOW);
      log("++ humidifier OFF (humidity: " + String(humidity) + ")");
    }
  }
}


/********************************************
 * EchoMessages - function to Echo messages *
 ********************************************/
void Bot_EchoMessages() {
  for (int i = 1; i < bot.message[0][0].toInt() + 1; i++)      {
    Serial.println(bot.message[i][5]);
    
    //save the message's sender ID aka the user (not the username)
    //it's used later to answer to the correct user
    Sender=bot.message[i][4];
    //save the user message
    String userMsg=bot.message[i][5];
    userMsg.toLowerCase();    // this will recognize mixed lowercase and uppercase text
    //save the username
    String userName=bot.message[i][2];

    //check the user's command and take actions
    if (userMsg == "sketch") {
      //do something for this command
      display_Running_Sketch();
      bot.sendMessage(Sender, "Command done!", "");    //echo or send a result back
    }
    else if (userMsg == "whoami") {
      bot.sendMessage(Sender, String(Sender), "");    //echo or send a result back
    }
    else if (userMsg  == "time") {
      //do something for this command
      String tM=getTimeFromGoogle();
      bot.sendMessage(Sender, tM, "");    //echo or send a result back
      bot.sendMessage(Sender, "Command done!", "");    //echo or send a result back
    }
    else if (userMsg  == "help") {
      //do something for this command 
      getHelp();
      bot.sendMessage(Sender, "Command done!", "");    //echo or send a result back
    }

    else if (userMsg  == "led_on") {
      //do something for this command
      digitalWrite(6,HIGH);
      bot.sendMessage(Sender, "Command done!", "");    //echo or send a result back
    }

    else if (userMsg  == "led_off") {
      //do something for this command
      digitalWrite(6,LOW);
      bot.sendMessage(Sender, "Command done!", "");    //echo or send a result back
    }

    else if (userMsg  == "led_status") {
      //do something for this command
      if (digitalRead(6)==LOW) {
        bot.sendMessage(Sender, "Built-in led is OFF", "");    //echo or send a result back
      } else  {
        bot.sendMessage(Sender, "Built-in led is ON", "");    //echo or send a result back 
      }
      bot.sendMessage(Sender, "Command done!", "");    //echo or send a result back
    }

    else if (userMsg  == "/start") {
      //this is used usually to start the conversation
      String yourName = "hi " + userName;        //get the name so we can say Hello!
      String welcome = "this is the kiezpilz bot";
      //String welcome1 = "";
      //String welcome2 = "";
      //String welcome3 = "";
      bot.sendMessage(Sender, yourName, "");
      bot.sendMessage(Sender, welcome, "");
      //bot.sendMessage(Sender, welcome1, "");
      //bot.sendMessage(Sender, welcome2, "");
      //bot.sendMessage(Sender, welcome3, "");
      }
    else 
    {
      // unsupported command
      bot.sendMessage(Sender, "You wrote:", "");       //     
      bot.sendMessage(Sender, bot.message[i][5], "");  //Echo the unsupported command to the user
    }

    
  }
  bot.message[0][0] = "";                              // All messages have been replied - reset new messages
}

//print the WiFi informations on the serial connection
void printWifiStatus() {
  // print the SSID of the network you're attached to:
  log("SSID: " + String(WiFi.SSID()) + "\n");

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}

// Get the commands list
void getHelp() {
   bot.sendMessage(Sender, "/start Starts the Bot", "");
   bot.sendMessage(Sender, "help   Displays this message", "");
   bot.sendMessage(Sender, "sketch Displays the sketch version", "");
   bot.sendMessage(Sender, "time   Display the time retrieved from Google", "");
   bot.sendMessage(Sender, "led_on  Turns ON the builtin led", "");
   bot.sendMessage(Sender, "led_off Turns OFF the builtin led", "");
   bot.sendMessage(Sender, "led_status Reads the led status", "");
  
}

// Displays the Sketch running in the Arduino
void display_Running_Sketch (void){
  String the_path = __FILE__;
  int slash_loc = the_path.lastIndexOf('/');
  String the_cpp_name = the_path.substring(slash_loc+1);
  int dot_loc = the_cpp_name.lastIndexOf('.');
  String the_sketchname = the_cpp_name.substring(0, dot_loc);

  bot.sendMessage(Sender, "MKR1010 is running Sketch: " + the_sketchname, "");    //echo or send a result back
  bot.sendMessage(Sender, "Compiled on: ", "");    //echo or send a result back
  bot.sendMessage(Sender, __DATE__, "");    //echo or send a result back

}

float readSensors() {
    // read all the sensor values
  float temperature = ENV.readTemperature();
  float humidity    = ENV.readHumidity();
  float pressure    = ENV.readPressure();
  float illuminance = ENV.readIlluminance();
  float uva         = ENV.readUVA();
  float uvb         = ENV.readUVB();
  float uvIndex     = ENV.readUVIndex();
  String t = getTimeFromGoogle();
//  String t = String(millis());

  StaticJsonDocument<256> doc;

  doc["time"] = t;
  doc["temperature"] = String(temperature) + " °C";
  doc["humidity"] = String(humidity) + " %";
  doc["pressure"] = String(pressure) + " kPa";
  doc["illuminance"] = String(illuminance) + " lx";
  doc["uva"] = String(uva);
  doc["uvb"] = String(uvb);
  doc["uvindex"] = String(uvIndex);
  String sensorInfo;
  serializeJson(doc, sensorInfo);
  log(sensorInfo);
  return humidity;
}


// Get the UTC time from google server
String getTimeFromGoogle() {
  WiFiClient client;
  while (!!!client.connect("google.com", 80)) {
    Serial.println("connection failed, retrying...");
  }

  client.print("HEAD / HTTP/1.1\r\n\r\n");
 
  while(!!!client.available()) {
     yield();
  }

  while(client.available()){
    if (client.read() == '\n') {    
      if (client.read() == 'D') {    
        if (client.read() == 'a') {    
          if (client.read() == 't') {    
            if (client.read() == 'e') {    
              if (client.read() == ':') {    
                client.read();
                String theDate = client.readStringUntil('\r');
                client.stop();
                return theDate;
              }
            }
          }
        }
      }
    }
  }
}
