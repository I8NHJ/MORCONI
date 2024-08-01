// MORCONI 
// MOrse COde Network Interface for 6X00 and 8X00 Flex radio series.
// N5NHJ Ver 1.0 20240731
//

#define APP_VERSION 20240731

// #define CONFIG_DEBUG                                                   //Uncomment this for debugging the config file 

/* LIBRARIES INCLUSION */
#include <SD.h>
#include "NativeEthernet.h"
#include <Audio.h>

/* SIDETONE VARIABLE DEFINITIONS */
AudioSynthWaveform SideTone;
AudioOutputPWM Speaker (1,2);                                       
AudioConnection patchCord0 (SideTone, Speaker);

/* ETHERNET CHANNELS AND BUFFERS */
EthernetClient RadioTCPChannel;
String ConnectionHandle="";
String ClientHandle="";

/* GLOBAL VARIABLE DEFINITIONS */
const int chipSelect            = BUILTIN_SDCARD;
const int BuiltInLED            = LED_BUILTIN;
const int KeyInPin              = 0;
unsigned long CWIndex           = 1;
unsigned long SEQ               = 1;
String RadioCommand             = "";
bool KeyingStatus               = false;
bool PreviousKeying             = false;
bool FlexConnected              = false;

/* DEBOUNCE LOOP VARIABLES */
bool ThisLoopStatus             = HIGH;
bool LastLoopStatus             = HIGH;
unsigned long StatusChangedTime = 0;

/* TIMING VARIABLES */
unsigned int PingTimeInterval   = 1000;
unsigned long LastPingTime      = 0;
unsigned long ThisLoopTime      = 0;
unsigned long TimeIt            = 0;

/* SD Card parameters - Assign defaults in case some lines are missing */
File ConfigFile;
char Rchar;
String InBuf;
bool InSetup                    = true;
unsigned int StartUpDelay       = 250;
unsigned int Debounce           = 0;
bool SidetoneActive             = true;
float SidetoneFrequency         = 600.0;
float SidetoneVolume            = 0.5;
uint8_t FlexIP[4]               = {10, 1, 1, 1};
unsigned int FlexPort           = 4992;
unsigned int FlexDelay          = 3000;
bool TeensyDebug                = false;
bool StaticIP                   = false;
uint8_t CfgIP[4]                = {10, 1, 1, 2};
uint8_t CfgGateway[4]           = {10, 10, 10, 254};
uint8_t CfgMask[4]              = {255, 255, 255, 0};

/* ETHERNET CONFIGURATION */
byte MyMAC[6];
IPAddress MyIP;
IPAddress MyGateway;
IPAddress MyMask;
IPAddress MyDNS;
IPAddress RadioIP;

/* END GLOBAL VARIABLE DEFINITIONS  */

void setup() {
  pinMode(KeyInPin, INPUT_PULLUP);
  pinMode(BuiltInLED, OUTPUT);

  digitalWrite(BuiltInLED, HIGH);
  
  AudioMemory (8);
  SideTone.begin(0.0, SidetoneFrequency, WAVEFORM_SINE);               //Waveforms: SINE, PULSE, SAWTOOTH, SQUARE, TRIANGLE

  #ifdef CONFIG_DEBUG
    OpenSerialMonitor();
  #endif

  getConfigFile();

  /**********************************/
  /* Code debugging purpose         */
  // TeensyDebug = true;
  // Debounce = 5;
  // FlexDelay = 1000;
  /**********************************/

  if (TeensyDebug) {
    #ifndef CONFIG_DEBUG
      OpenSerialMonitor();
    #endif
  }

  getIpAddress();

  if (TeensyDebug) {
    Serial.printf("Teensy MAC: %02x:%02x:%02x:%02x:%02x:%02x\n", MyMAC[0], MyMAC[1], MyMAC[2], MyMAC[3], MyMAC[4], MyMAC[5]);
    Serial.printf("Teensy IP: %u.%u.%u.%u\n", MyIP[0], MyIP[1], MyIP[2], MyIP[3]);
    Serial.printf("Teensy MASK: %u.%u.%u.%u\n", MyMask[0], MyMask[1], MyMask[2], MyMask[3]);
    Serial.printf("Teensy GATEWAY: %u.%u.%u.%u\n", MyGateway[0], MyGateway[1], MyGateway[2], MyGateway[3]);
    Serial.printf("Teensy DNS: %u.%u.%u.%u\n", MyDNS[0], MyDNS[1], MyDNS[2], MyDNS[3]);
    Serial.printf("Flex IP: %u.%u.%u.%u\n", FlexIP[0], FlexIP[1], FlexIP[2], FlexIP[3]);
  }

  FlexConnect();

  FlexInit();

  if (TeensyDebug){
    Serial.println("Radio connected");
  }
}
/* END setup() */

void loop() {
  ThisLoopTime=millis();
  ThisLoopStatus=digitalRead(KeyInPin);

  if (Debounce != 0) {
    if (ThisLoopStatus != LastLoopStatus) {
      StatusChangedTime=millis();
    } 
    if ((millis()-StatusChangedTime) > Debounce) {
      if (ThisLoopStatus != KeyingStatus) {
        KeyingStatus = ThisLoopStatus;
        if (TeensyDebug){
          Serial.println("Debounce routine executed");
        }
      }
    }
  }
  else {
    KeyingStatus = ThisLoopStatus; 
  }
  
  // KeyingActions()

  if (KeyingStatus) {                                                  //It is high, I'm not transmitting
    if (PreviousKeying) {
      RadioCommand="C"+ String(SEQ).trim() + "|cw key 0 time=0x" + String(millis() % 0xFFFF, HEX).trim() + " index=" + String(CWIndex).trim() + " client_handle=0x" + ClientHandle + "\n";
      RadioTCPChannel.write(RadioCommand.c_str(), RadioCommand.length());
      CWIndex++;
      SEQ++;
      PreviousKeying = false;
      digitalWrite(BuiltInLED, LOW);                                  // LED off
      if (TeensyDebug) {
        Serial.println("RX");
      }
      if (SidetoneActive) {
        SideTone.amplitude(0);
      }
    }
//    else {
//    }
  }
  else {                                                               //It is low, I'm transmitting
    if (PreviousKeying) {
    }
    else {
      RadioCommand="C"+ String(SEQ).trim() + "|cw key 1 time=0x" + String(millis() % 0xFFFF, HEX).trim() + " index=" + String(CWIndex).trim() + " client_handle=0x" + ClientHandle + "\n";
      RadioTCPChannel.write(RadioCommand.c_str(), RadioCommand.length());
      CWIndex++;
      SEQ++;
      PreviousKeying = true;
      if (SidetoneActive) {
        SideTone.amplitude(SidetoneVolume);
      } 
      digitalWrite(BuiltInLED, HIGH);                                  // LED on
      if (TeensyDebug) {
        Serial.println("TX");
      }
    }
  }

  if (RadioTCPChannel.connected()) {
    if ( (ThisLoopTime-LastPingTime) > PingTimeInterval) {
      LastPingTime = ThisLoopTime;
      RadioCommand = "C" + String(SEQ).trim() + "|ping ms_timestamp=" + String(millis()).trim()+".0000\n";
      RadioTCPChannel.write(RadioCommand.c_str(), RadioCommand.length());
      SEQ++;
    }
  }
  else {
    RadioTCPChannel.stop();
    digitalWrite(BuiltInLED, HIGH);
    if (TeensyDebug) {
      Serial.println("Radio disconnected");
    }
//    FlexConnected=false;
    FlexConnect();
    ConnectionHandle="";
    ClientHandle="";
    FlexInit();
  }
  LastLoopStatus=ThisLoopStatus;
}
/* END loop() */

void FlexInit() {
/* MORE USEFULL API COMMANDS 
  RadioCommand="C1|client ip\n";
  RadioTCPChannel.write(RadioCommand.c_str(), RadioCommand.length());
  RadioCommand="C3|client start_persistence 0\n";
  RadioTCPChannel.write(RadioCommand.c_str(), RadioCommand.length());
  RadioCommand="C4|client bind client_ID\n";
  RadioTCPChannel.write(RadioCommand.c_str(), RadioCommand.length());
  RadioCommand="C5|info\n";
  RadioTCPChannel.write(RadioCommand.c_str(), RadioCommand.length());
  RadioCommand="C6|version\n";
  RadioTCPChannel.write(RadioCommand.c_str(), RadioCommand.length());
  RadioCommand="C7|ant list\n";
  RadioTCPChannel.write(RadioCommand.c_str(), RadioCommand.length());
  RadioCommand="C8|mic list\n";
  RadioTCPChannel.write(RadioCommand.c_str(), RadioCommand.length());
  RadioCommand="C9|profile global info\n";
  RadioTCPChannel.write(RadioCommand.c_str(), RadioCommand.length());
  RadioCommand="C10|profile tx info\n";
  RadioTCPChannel.write(RadioCommand.c_str(), RadioCommand.length());
  RadioCommand="C11|profile mic info\n";
  RadioTCPChannel.write(RadioCommand.c_str(), RadioCommand.length());
  RadioCommand="C12|profile displayinfo\n";
  RadioTCPChannel.write(RadioCommand.c_str(), RadioCommand.length());
  RadioCommand="C14|sub tx all\n";
  RadioTCPChannel.write(RadioCommand.c_str(), RadioCommand.length());
  RadioCommand="C15|sub atu all\n";
  RadioTCPChannel.write(RadioCommand.c_str(), RadioCommand.length());
  RadioCommand="C16|sub amplifier all\n";
  RadioTCPChannel.write(RadioCommand.c_str(), RadioCommand.length());
  RadioCommand="C17|sub meter all\n";
  RadioTCPChannel.write(RadioCommand.c_str(), RadioCommand.length());
  RadioCommand="C18|sub pan all\n";
  RadioTCPChannel.write(RadioCommand.c_str(), RadioCommand.length());
  RadioCommand="C19|sub slice all\n";
  RadioTCPChannel.write(RadioCommand.c_str(), RadioCommand.length());
  RadioCommand="C20|sub gps all\n";
  RadioTCPChannel.write(RadioCommand.c_str(), RadioCommand.length());
  RadioCommand="C21|sub audio_stream all\n";
  RadioTCPChannel.write(RadioCommand.c_str(), RadioCommand.length());
  RadioCommand="C22|sub cwx all\n";
  RadioTCPChannel.write(RadioCommand.c_str(), RadioCommand.length());
  RadioCommand="C23|sub xvtr all\n";
  RadioTCPChannel.write(RadioCommand.c_str(), RadioCommand.length());
  RadioCommand="C24|sub memories all\n";
  RadioTCPChannel.write(RadioCommand.c_str(), RadioCommand.length());
  RadioCommand="C25|sub daxiq all\n";
  RadioTCPChannel.write(RadioCommand.c_str(), RadioCommand.length());
  RadioCommand="C26|sub dax all\n";
  RadioTCPChannel.write(RadioCommand.c_str(), RadioCommand.length());
  RadioCommand="C27|sub usb_cable all\n";
  RadioTCPChannel.write(RadioCommand.c_str(), RadioCommand.length());
  RadioCommand="C28|sub spot all\n";
  RadioTCPChannel.write(RadioCommand.c_str(), RadioCommand.length());
  RadioCommand="C29|client set enforce_network_mtu=1 network_mtu=1500\n";
  RadioTCPChannel.write(RadioCommand.c_str(), RadioCommand.length());
  RadioCommand="C30|client set send_reduced_bw_dax=1\n";
  RadioTCPChannel.write(RadioCommand.c_str(), RadioCommand.length());
  RadioCommand="C31|client udpport 4995\n";
  RadioTCPChannel.write(RadioCommand.c_str(), RadioCommand.length());
  RadioCommand="C32|stream create netcw\n";
  RadioTCPChannel.write(RadioCommand.c_str(), RadioCommand.length());
  RadioCommand="C33|display panf rfgain_info 0x0\n";
  RadioTCPChannel.write(RadioCommand.c_str(), RadioCommand.length());
*/
  ConnectionHandle=getConnectionHandle();
  RadioCommand="C1|client program MORCONI\n";
  RadioTCPChannel.write(RadioCommand.c_str(), RadioCommand.length());
  while (ClientHandle=="") {
    if (TeensyDebug) {
      Serial.println("Waiting for SMARTSdr");
    }
    RadioCommand="C2|sub client\n";
    RadioTCPChannel.write(RadioCommand.c_str(), RadioCommand.length());
    ClientHandle=getClientHandle();
  }
  digitalWrite(BuiltInLED, LOW);
  SEQ=3;
  send_K();
}
/* END FlexInit() */

void FlexConnect() {
  FlexConnected=false;
  while (!FlexConnected) { 
    if (TeensyDebug) {                                                 //Retry connecting forever
      Serial.println("Connecting Radio");
    }
    FlexConnected=connect(RadioIP, FlexPort);
    if (!FlexConnected) {
      delay(FlexDelay);                           
    }
  }
}
/* END FlexConnect() */

void send_K() {                                                        // _._ at speed of 40WPM
  SideTone.frequency(600.0);

  SideTone.amplitude(SidetoneVolume);
  delay (90);
  SideTone.amplitude(0.0);
  delay (30);
  SideTone.amplitude(SidetoneVolume);
  delay (30);
  SideTone.amplitude(0.0);
  delay (30);
  SideTone.amplitude(SidetoneVolume);
  delay (90);
  SideTone.amplitude(0.0);

  SideTone.frequency(SidetoneFrequency);
}
/* END send_K() */

bool connect(IPAddress IP, uint16_t Port) {
  if (RadioTCPChannel.connect(IP, Port)) {
    return true;
  }
  else {
    return false;
  }
}
/* END connect() */

String getConnectionHandle () {
  delay (FlexDelay);                                                   //Give the radio some time to sync
  String buffer;
  String Handle;
  while (RadioTCPChannel.available()) {
    buffer=RadioTCPChannel.readStringUntil('\n');
    if (TeensyDebug) {
      Serial.println(buffer);
    }

    if (buffer.substring(0,1)=="H") {
      Handle=buffer.substring(1,8);
    }
  }
  if (TeensyDebug){
    Serial.println(Handle);
  }
  return Handle;
}
/* END getConnectionHandle () */

String getClientHandle () {
  delay (FlexDelay);                           //Give the radio some time to sync
  int index;
  String buffer;
  String Handle;
  while (RadioTCPChannel.available()) {
    buffer=RadioTCPChannel.readStringUntil('\n');
    if (TeensyDebug) {
      Serial.println(buffer);
    }
    index = buffer.indexOf(F("|client 0x"));
    if (index>=0) {
      Handle=buffer.substring(index+10,index+18);
    }
  }
  if (TeensyDebug){
    Serial.println(Handle);
  }
  return Handle;
}
/* END getClientHandle () */

void OpenSerialMonitor() {
  Serial.begin(115200);
  while (!Serial) { 
    if (millis() > 2000) { 
      break;
    } 
  }
  Serial.print(F("MORCONI for Flex 6/8X00 series radios V."));
  Serial.print((APP_VERSION));
  Serial.println(F(" by N5NHJ"));
  Serial.println(F("----------------------"));
}
/* END OpenSerialMonitor() */

//---------------------------------------------
// Element (dot or space) length = 60000 (milliseconds in a minute) / (50 (elements of the world PARIS) * Speed (word per minute))
// @20WMP: 60000 / (50*20) = 60ms
// @30WPM: 60000 / (50*30) = 40ms
// @40WPM: 60000 / (50*40) = 30ms
// void send_dot(int speed) {}
// void send_dash(int speed) {}
// void send_element_space(int speed) {}
// void send_letter_space (int speed){}
// void send_word_space (int speed){}
