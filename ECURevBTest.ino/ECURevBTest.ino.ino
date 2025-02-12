#include <DS18B20.h>
#include <SD.h>

File SDcard;
String Filename;
char filename[100];

//pin definitions
int pinRS41_ENABLE = 2;
int pin12V_ENABLE = 5;
int pinSW_I_HRES_EN = 6;
int pinSW_IMON_EN = 7;
int pinSW_FAULT = 8;
int pinLORA_CS = 10;
int pinV_ZEPHR_VMON = 20;
int pin12V_MON = 21;
int pinPCB_THERM = 22;
int pinSW_IMON = 23;
int pin5V_MON = 24;
int pinHEATER_DISABLE = 37;
float K_SNS1 = 800;  //Standard current sense ratio, see p.9 https://www.ti.com/lit/ds/symlink/tps281c100.pdf?ts=1704232917558&ref_url=https%253A%252F%252Fwww.ti.com%252Fproduct%252FTPS281C100
float K_SNS2 = 24;  //High accuracy current sense ratio
float R_SNS = 5000;
bool I_HRES = 0;

DS18B20 ds(pinPCB_THERM);

String DEBUG_Buff;  //buffer for the USB Serial monitor

//Resistance values in kOhms
float R5 = 499.0;
float R6 = 10.0;
float R13 = 30.0;
float R14 = 10.0;
float R15 = 48.7;
float R16 = 10.0;

void setup() {
  pinMode(pinRS41_ENABLE,OUTPUT);
  pinMode(pin12V_ENABLE,OUTPUT);
  pinMode(pinSW_I_HRES_EN,OUTPUT);
  pinMode(pinSW_IMON_EN,OUTPUT);
  pinMode(pinV_ZEPHR_VMON,INPUT);
  pinMode(pin12V_MON,INPUT);
  pinMode(pin5V_MON,INPUT);
  pinMode(pinHEATER_DISABLE,OUTPUT);
  
  if (!SD.begin(BUILTIN_SDCARD)) {
    Serial.println("SD card initialization failed!");
    while (1);
  }
  Serial.println("SD card initialized");
  Filename = nameFile();
  Filename.toCharArray(filename, 100);
  SerialUSB.print("Filename: ");
  SerialUSB.println(filename);
  SerialUSB.println(Filename);
  //SDcard = SD.open(filename, FILE_WRITE);
}

void loop() {
  checkCMD();

}

void checkCMD(){
  if (Serial.available()) {
   char DEBUG_Char = Serial.read();
   
   if((DEBUG_Char == '\n') || (DEBUG_Char == '\r'))
   {
    Serial.print("Received Command: ");
    Serial.println(DEBUG_Buff);
    parseCommand(DEBUG_Buff);
    DEBUG_Buff = "";
   } else
   {
    DEBUG_Buff += DEBUG_Char;
   }   
  }
}

void parseCommand(String commandToParse) {
  if(commandToParse.startsWith("#enable12V")) {
    digitalWrite(pin12V_ENABLE,HIGH);
    Serial.println("12V enabled");
  }
  if(commandToParse.startsWith("#disable12V")) {
    digitalWrite(pin12V_ENABLE,LOW);
    Serial.println("12V disabled");
  }
  if(commandToParse.startsWith("#enableHeater")) {
    digitalWrite(pinHEATER_DISABLE,LOW);
    Serial.println("Heater enabled");
  }
  if(commandToParse.startsWith("#disableHeater")) {
    digitalWrite(pinHEATER_DISABLE,HIGH);
    Serial.println("Heater disabled");
  }
  if(commandToParse.startsWith("#EnIHRes")) { //enable high resolution monitoring on current sensing for TSEN switch
    I_HRES = 1;
    Serial.println("High Accuracy Switch Load Current Enabled");
  }
  if(commandToParse.startsWith("#DisIHRes")) { //enable high resolution monitoring on current sensing for TSEN switch
    I_HRES = 0;
    Serial.println("High Accuracy Switch Load Current Disabled");
  }
  if(commandToParse.startsWith("#boardMon")) {
    boardMon();
  }
}

String nameFile() {
  int fileNum = 1;
  bool nameCheck = 0;
  char filename[20];
  String Filename;

  while (!nameCheck) {
    Filename = "";
    Filename = "ECUtest";
    Filename += fileNum;
    Filename.toCharArray(filename, 20);
    if (!SD.exists(filename)) {
      Serial.println(Filename);
      nameCheck = 1;
    } else {
      fileNum++;
    }
  }
  return filename;
}

void boardMon() {
  float K_SNS;
  if (I_HRES) {
    K_SNS = K_SNS2;
    digitalWrite(pinSW_I_HRES_EN,HIGH);
  }
  else {
    K_SNS = K_SNS1;
  }
  digitalWrite(pinSW_IMON_EN,HIGH);
  float V_ZEPHR = analogRead(pinV_ZEPHR_VMON) * (R5 + R6) * 3.3 / (1024.0  * R6);
  float V_5V = analogRead(pin5V_MON) * (R13 + R14) * 3.3 / (1024.0 * R14);
  float V_12V = analogRead(pin12V_MON) * (R15 + R16) * 3.3 / (1024.0 * R16);
  float I_SW = (analogRead(pinSW_IMON) / R_SNS) * K_SNS;

  String outputString = "5V:";
  outputString += V_5V;
  outputString += ", ";
  //outputString += "12V_I:";
  //outputString += analogRead(pin12V_IMON);
  //outputString += ", ";
  outputString += "12V:";
  outputString += V_12V;
  outputString += ", ";
  outputString += "PCB_THERM (C):";
  outputString += ds.getTempC();
  outputString += ", ";
  outputString += "ZEPHR_V:";
  outputString += V_ZEPHR;
  outputString += ", ";
  outputString += "I_SW:";
  outputString += I_SW;
  Serial.println(outputString);
  SDcard = SD.open(filename, FILE_WRITE);
  if (SDcard) {
    Serial.println("Writing to SD...");
    SDcard.println(outputString);
  }
  SDcard.close();
  outputString = "";
  digitalWrite(pinSW_IMON_EN,LOW);
  digitalWrite(pinSW_I_HRES_EN,LOW);
}

