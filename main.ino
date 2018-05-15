#include <Servo.h>
#include <SPI.h>
#include <WiFi.h>
#include <TimerOne.h>
#include "rgb_lcd.h"
#include <stdlib.h>
#include <string.h>
#include <TH02_dev.h>

//RTOS Var
int ms10=0;
int ms50=0;
int ms100=0;
int ms500=0;
int s1=0;
int s5=0;

//sensor value
int sensorValue[5];
const char* sensorList[] = { "temp", "humi", "light", "uv" ,"Moistrue Sensor"};
enum SENSOR {TEMPER=0,HUMI,LIGHT,UV,MS};

//PIN Definition
const int pinSound = A0;
const int pinMoistrue = A1;
const int pinLight = A2;
const int pinUV = A3;
const int pinButton =0;
const int pinEncoder1 = 2;
const int pinEncoder2 = 3;
const int pinBuzzer = 4;
const int pinRelay = 5;
const int pinPIR = 7;
const int pinServo = 6;

//alarm threshold
int alarmTemp=30;
int alarmHumi=60;
int alarmUv=20;
int alarmLight=390;
int alarmMs=30;

//LCD color
unsigned char BLColorRGB[]={0x00, 0x00, 0x00};

//server
char ssid[48] = "iPhone";           // your network SSID (name) 
char pass[48] = "99998888";       // your network password
boolean isSSIDreconfiged = false;
int keyIndex = 0;                // your network key Index number (needed only for WEP)
int status = WL_IDLE_STATUS;
int serverconnected = 0;



//Flags and control variable
int lcdCount=1;//1-4
int lcdBackground=0;//0-light 1-dark
int LEDEnable=0;//0-blink
int alarmEnable=1;//0-alarm
int shouldAlarm=0;//0-should alarm

//Lcd and Server Instantiation
rgb_lcd lcd;
Servo myservo;
WiFiServer server(88);
WiFiClient client;

//change screen to Red
void LCDtoRed();

//Blink LED
void blinkLED();

//Update Sensor Value
void updateSensor();

//Updata LCD screen
void updateLCD();

//Updata server
void updateServer();

//init all devices;
void initDevice();

//init web server
void initServer();

//organize webpage
void sendHtml();

//Serial command process
void commandProcess();

void setup() 
{
  initDevice();
  initServer();
  // set a timer of length 1000(1ms) microseconds
  Timer1.initialize(1000); 
  // TickTimer Interrupt
  Timer1.attachInterrupt( timerIsr ); 

}

void loop()
{
  //task will run every 10ms
  if(ms10>10){

    ms10=0;
  }
  //task will run every 50ms
  if(ms50>50){
    updateSensor();
    ms50=0;
  }
  //task will run every 100ms
  if(ms100>100){
    updateServer();
    ms100=0;
  }
  //task will run every 500ms
  if(ms500>500){
    commandProcess();
    ms500=0;
  }
  //task will run every 1s
  if(s1>1000){
    blinkLED();
    updateLCD();
    s1=0;
  }
  //task will run every 5s
  if(s5>5000){

    s5=0;
  }
}

//init all devices
void initDevice(){
  //set up LCD
  lcd.begin(16, 2);
  //setup Sensor Pin
  pinMode(13, OUTPUT);//LED
  pinMode(pinButton,INPUT);
  pinMode(pinRelay,OUTPUT);
  pinMode(pinBuzzer,OUTPUT);
  pinMode(pinEncoder1,INPUT);
  pinMode(pinEncoder2,INPUT);
  //start USART
  Serial.begin(115200);
}

//init the Server
void initServer(){
    while ( status != WL_CONNECTED) { 
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:    
    status = WiFi.begin(ssid, pass);

    // wait 10 seconds for connection:
    delay(10000);
  } 
  Serial.print("connected!! \n");
  server.begin();
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
}

//RTOS core part
void timerIsr(){
    ms10++;
    ms50++;
    ms100++;
    ms500++;
    s1++;
    s5++;
}

//update all sensor value
void updateSensor(){
  sensorValue[TEMPER] = (int)TH02.ReadTemperature();
  sensorValue[HUMI] = (int)TH02.ReadHumidity();
  sensorValue[MS] = analogRead(pinMoistrue);
  sensorValue[LIGHT] = analogRead(pinLight);
  sensorValue[UV] = analogRead(pinUV);
  return;
}

////Serial command process
void commandProcess(){
  int i=0;
  char buf[80];
  int count=0;
  if(Serial.peek()==-1)
     return;
  while(1){
    if(count>50)
      break;
    count++;
    char res =Serial.read();
    if(res != '\n')
      buf[i++] = res;
    else
      break;
  }
  if(buf[0]=='T'){
    alarmTemp=(buf[2]-0x30)*10+(buf[3]-0x30);
    Serial.print(alarmTemp);
  }
  if(buf[0]=='H'){
    alarmHumi=(buf[2]-0x30)*10+(buf[3]-0x30);
    Serial.print(alarmHumi);
  }
  if(buf[0]=='U'){
    alarmUv=(buf[2]-0x30)*10+(buf[3]-0x30);
    Serial.print(alarmUv);
  }
  if(buf[0]=='L'){
    alarmLight=(buf[2]-0x30)*100+(buf[3]-0x30)*10+(buf[4]-0x30);
    Serial.print(alarmLight);
  }
  if(buf[0]=='M'){
    alarmMs=(buf[2]-0x30)*10+(buf[3]-0x30);
    Serial.print(alarmMs);
  }
}


//update the LCD screen to show sensor value
void updateLCD(){
  lcd.clear();
  lcd.setRGB(225,225,225);
  char line1[16];
  char line2[16];
  switch (lcdCount){
    case 1:
      if(sensorValue[TEMPER]>alarmTemp||sensorValue[HUMI]>alarmHumi&&alarmEnable==0)
        LCDtoRed();
      sprintf(line1,"  Temp&Humidity ");
      sprintf(line2,"<    %dC  %d%    >",sensorValue[TEMPER],sensorValue[HUMI]);
      lcd.print(line1);
      lcd.setCursor(0,1);
      lcd.print(line2);
      lcdCount++;
    break;
    case 2:
      if(sensorValue[MS]>alarmMs&&alarmEnable==0)
        LCDtoRed();
      sprintf(line1,"Moistrue Sensor ");
      sprintf(line2,"<      %d      >",sensorValue[MS]);
      lcd.print(line1);
      lcd.setCursor(0,1);
      lcd.print(line2);
      lcdCount++;
    break;
    case 3:
      if(sensorValue[LIGHT]>alarmLight&&alarmEnable==0)
        LCDtoRed();
      sprintf(line1,"  Light Sensor  ");
      sprintf(line2,"<     %d      >",sensorValue[LIGHT]);
      lcd.print(line1);
      lcd.setCursor(0,1);
      lcd.print(line2);
      lcdCount++;
    break;
    case 4:
      if(sensorValue[UV]>alarmUv&&alarmEnable==0)
        LCDtoRed();
      sprintf(line1,"   UV Sensor    ");
      sprintf(line2,"<      %d      >",sensorValue[UV]);
      lcd.print(line1);
      lcd.setCursor(0,1);
      lcd.print(line2);
      lcdCount=1;
    break;
  }
}
//Blink LED
void blinkLED(){
  if(LEDEnable==0)
    digitalWrite( 13, digitalRead( 13 ) ^ 1 );
}

//LCD to Red
void LCDtoRed()
{
  BLColorRGB[0]=0xFF; 
  BLColorRGB[1]=0x00;
  BLColorRGB[2]=0x00;
  lcd.setRGB(BLColorRGB[0],BLColorRGB[1],BLColorRGB[2]);
}


//listen to the cilent connection
void updateServer(){
  String readString=""; 
  client = server.available();
  if (client) {
    Serial.println("new client");
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        readString += c;
        if (c == '\n' && currentLineIsBlank) {
          if(readString.indexOf("?lo") >0) {
            LEDEnable=LEDEnable^1;
            Serial.write("got request!\n");
            break;
          }
          if(readString.indexOf("?ao") >0) {
            alarmEnable=alarmEnable^1;
            Serial.write("got request!\n");
            break;
          }
          sendHtml();
          break;
        }
        if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
        } else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }
    // give the web browser time to receive the data
    delay(1);
    
    // close the connection:
    client.stop();
    Serial.println("client disonnected");
    readString=""; 
  }
}
void sendHtml(){
   client.println("HTTP/1.1 200 OK");
   client.println("Content-Type: text/html");
   client.println("Connection: close");  // the connection will be closed after completion of the response
   client.println("Refresh: 5");         // refresh the page automatically every 2 sec
   client.println();
   client.println("<!DOCTYPE HTML>");
   client.println("<html>");
   client.println("<title>XB Yin</title>"); 
   client.println("<font face=\"Microsoft YaHei\" color=\"#0071c5\"/>");
   client.println("<h1 align=\"center\">Enviroment Warder</h1>");
   client.print("<br />");
   for(int i=0; i < 5; i++){
    client.print("<h2 align=\"center\"><big>");
    client.print(sensorList[i]);
    client.print(" = ");
    client.print(sensorValue[i]); 
    client.println("</big></h2>"); 
    //client.print("<br />");
  }
  client.println("<div align=\"center\"><button id=\"control\" type=\"button\" onclick=\"sendLED()\">Lumos</button><button id=\"alarm\" type=\"button\" onclick=\"sendalarm()\">Start ALarm</button></div>");
  client.println("</html>");
  client.println("<script type=\"text/javascript\">function sendLED(){var xmlhttp;xmlhttp=new XMLHttpRequest();xmlhttp.open(\"GET\",\"?lo\",true);xmlhttp.send();}function sendalarm(){xmlhttp=new XMLHttpRequest();xmlhttp.open(\"GET\",\"?ao\",true);xmlhttp.send();}</script>");
}

