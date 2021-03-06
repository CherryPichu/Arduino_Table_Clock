#include "DHT.h"
#include <SD.h>
#include <SPI.h>
#include <Wire.h>
#include <OzOLED.h>
#include <math.h>
#include <WiFiNINA.h>

int CS_PIN = 10;
int Sensor0 = 0;
String Data = "";
int count_down = 0; // sd카드 저장 주기 시간 계산해주는 변수(20)
int screen_change = 0; // 스크린 바뀔때 시간 세주는 변수
String last_save = ""; // 마지막으로 저장 시기
String last_wifi = ""; // 마지막으로 와이파이 통신시기

char ssid[] = "8-1324";        // your network SSID (name)  8-1324  Cherry
char pass[] = "";    // your network password (use for WPA, or use as key for WEP)
char server[] = "api.thingspeak.com";
int keyIndex = 0;
int status = WL_IDLE_STATUS; // wifi 추가
WiFiSSLClient client;

#define DHTPIN 2        // SDA 핀의 설정
#define DHTTYPE DHT22   // DHT22 (AM2302) 센서종류 설정
DHT dht(DHTPIN, DHTTYPE);

#include <Wire.h>
 
#define DS3231_I2C_ADDRESS 104
short seconds, minutes, hours, day, date, month, year;
char weekDay[4];
 
short tMSB, tLSB;
float temp3231;

File myFile;
#define LIGHTPIN A0 //Ambient light sensor reading
void setup() {
  
//  send_api();
  Serial.begin(115200); // 115200 9600
  delay(2000); // 버그 수정 딜레이를 줘야만 전원을 동시에 공급했을때 작동하는 듯?
  pinMode(4, OUTPUT); // 스위치(버튼)
  pinMode(5, INPUT_PULLUP);
  pinMode(LIGHTPIN, INPUT); //조도 센서
  OzOled.init();  //initialze Eduino OLED display
  OzOled.printString("Hello!");
  // put your setup code here, to run once:
  wifi_init();
  
  pinMode(CS_PIN, OUTPUT);
  dht.begin();
  Wire.begin();
  initalizeSD();
  OzOled.clearDisplay();
}

void initalizeSD(){
  Serial.println("Interalizing SD card..");
  pinMode(CS_PIN,OUTPUT);
  if(SD.begin()){
    Serial.println("SD Card is ready to use.");
  }else {
    Serial.println("SD Card initalization failed");
    OzOled.clearDisplay();
    OzOled.printString("SD Card", 0, 0);
    OzOled.printString("initalization failed", 0, 1);
    delay(3000);
    return;
  }
  return;
}
float get_light(){
  float reading = analogRead(LIGHTPIN); 
  return reading;
}
void wifi_init(){ // wifi 추가
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    OzOled.printString("Communication with WiFi module failed!", 0,1);
    // don't continue
    while (true);
  }
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    OzOled.printString("Attempting to connect to SSID: ",0,2);
    Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);

    // wait 10 seconds for connection:
    delay(10000);
  }
  Serial.println("Connected to wifi");
  OzOled.printString("Connected to wifi",0,4);
  printWiFiStatus();
   String fv = WiFi.firmwareVersion();
    if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Please upgrade the firmware");
  }
  Serial.println("\nStarting connection to server...");
  OzOled.printString("Starting connection to server...",0,5);
  // if you get a connection, report back via serial:
  OzOled.clearDisplay();
}
void send_api(int temp, int humidity, float light) { // wifi 추가

  if (client.connect(server, 443)) {
    Serial.println("connected to server");
//    OzOled.printString("connected to server",0,6);
    // Make a HTTP request:
    client.println("GET /update?api_key=I1IPO6UY6WQG4IN8&field1="+String(temp)+"&field2="+String(humidity)+"&field3=" + String(light));
    client.println("Host: api.thingspeak.com");
    client.println("Connection: close");
    client.println();
    last_wifi = String(hours) + ":" +String(minutes) + ":" + String(seconds);
  }
}
void TestWiFiConnection()
//test if always connected
{
 int StatusWiFi=WiFi.status();
 if(StatusWiFi==WL_CONNECTION_LOST || StatusWiFi==WL_DISCONNECTED || StatusWiFi==WL_SCAN_COMPLETED) //if no connection
 {
  WiFi.end(); // WiFi를 종료한다.
  WiFiConnect(); //if my SSID is present, connect
 }
}
void WiFiConnect()
//connect to my SSID
{
 status= WL_IDLE_STATUS;
 while(status!=WL_CONNECTED)
 {
    OzOled.printString("try to connect..",1, 0, 15);
   status = WiFi.begin(ssid,pass);
   delay(2000);
  }
  OzOled.printString("               ",1, 0, 20);
}


void loop() {
  float light  = get_light();
  float humidity = dht.readHumidity();
  float temp = dht.readTemperature();
  String light_status = "";
  if(light > 140) {
      light_status = "Very bright"; 
    }else if(light > 80 && light <= 140) {
      light_status = "Bright"; 
    }else if(light > 60 && light <= 80) {
      light_status = "Slight darkness";
    }else if(light > 30 && light <= 60) {
      light_status = "Darkness";
    }else if(light > 0 && light <= 30) {
      light_status = "Very dark";
    }
   TestWiFiConnection();
  // put your main code here, to run repeatedly:
  watchConsole();
  get3231Date();
  Data =  String(weekDay) + ", 20" + String(year) + "/" + String(month) + "/" + String(date) + "," + String(hours) + ":" + String(minutes) + ":" + 
       String(seconds) + " , Temp :" + String(temp) + " , Humidity :" + String(humidity)+", light :" + String(light);
  if(count_down == 20){
  //("20" + String(year) + "/" + String(month)+"/"+String(date) +"-info")
      String file_name = String(month)+"M"+String(date)+"D.txt";
      Serial.println(file_name);
      myFile = SD.open(file_name, FILE_WRITE);
      send_api(temp, humidity, light);
      if(myFile){
        Serial.println("File created successfully");
        last_save = String(hours) + ":" +String(minutes) + ":" + String(seconds);
      }else {
        Serial.println("Error while creating file.");
        OzOled.clearDisplay();
        OzOled.printString("Error while", 0, 0);
        OzOled.printString("creating file", 0, 1);
        delay(2000);
        OzOled.clearDisplay();
      }
    myFile.println(Data);
    myFile.close();
    count_down = 0;
  }else{
    count_down++;
  }
  
  
  Serial.println(Data);
  // 아래 코드는 기본 화면 구성 출력
  if(digitalRead(5) == LOW){ // 스위치가 눌렸을 경우
    screen_change = 5;
    OzOled.clearDisplay();
  }
  if(screen_change > 1){ // 버튼이 눌린 경우에 온도와 습도창을 보여준다.
    OzOled.printString(("temp :"+String(temp,2)).c_str(), 0 ,0,10);
    OzOled.printString(("Humidity :" + String(humidity,2)).c_str(), 0 ,1,16);
    OzOled.printString(("Light :" + String(light, 2)).c_str(), 0 ,2,16);
    OzOled.printString((String(weekDay)+" "+ "20" + String(year) + "/" + String(month)+"/"+String(date)).c_str(), 0 ,3,16);
    OzOled.printString(("Status :" + light_status).c_str(), 0 ,4,16);
    OzOled.printString(("save :" + last_save).c_str(), 0 ,5,16);
    OzOled.printString(("wifi :" + last_wifi).c_str(), 0 ,6,16);
    screen_change--;
    if(screen_change == 1){
      screen_change = 0;
      OzOled.clearDisplay();
    }
  }else{
    String str_hours = String(hours);
    String str_minutes = String(minutes);
    String str_seconds = String(seconds) + "s ";
    const char * time_hours = str_hours.c_str(); // 한글 출력 방법
    const char * time_minutes = str_minutes.c_str(); 
    const char * time_seconds = str_seconds.c_str(); 
    
    OzOled.printBigNumber(time_hours, 0, 1,3);
    OzOled.printBigNumber(":",6,1, 10);
    OzOled.printBigNumber(time_minutes, 9, 1,3);
    OzOled.printString(time_seconds,11,6, 3); //x위치 y위치 글자 수
    
    OzOled.printString((String(weekDay)+" "+ String(month)+"/"+String(date)).c_str(),1,6, 10); 

  }
  


  
  delay(1000);
}


// 10진수를 2진화 10진수인 BCD 로 변환 (Binary Coded Decimal)
short decToBcd(short val)
{
  return ( (val/10*16) + (val%10) );
}
 
void watchConsole()
{
  if (Serial.available()) {      // Look for char in serial queue and process if found
    if (Serial.read() == 84) {   //If command = "T" Set Date
//      set3231Date();
      get3231Date();
      Serial.println(" ");
    }
  }
}
 
//시간설정
// T(설정명령) + 년(00~99) + 월(01~12) + 일(01~31) + 시(00~23) + 분(00~59) + 초(00~59) + 요일(1~7, 일1 월2 화3 수4 목5 금6 토7)
// 예: T1605091300002 (2016년 5월 9일 13시 00분 00초 월요일)
void set3231Date()
{
  year    = (short) ((Serial.read() - 48) *10 +  (Serial.read() - 48));
  month   = (short) ((Serial.read() - 48) *10 +  (Serial.read() - 48));
  date    = (short) ((Serial.read() - 48) *10 +  (Serial.read() - 48));
  hours   = (short) ((Serial.read() - 48) *10 +  (Serial.read() - 48));
  minutes = (short) ((Serial.read() - 48) *10 +  (Serial.read() - 48));
  seconds = (short) ((Serial.read() - 48) * 10 + (Serial.read() - 48));
  day     = (short) (Serial.read() - 48);
 
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  Wire.write(0x00);
  Wire.write(decToBcd(seconds));
  Wire.write(decToBcd(minutes));
  Wire.write(decToBcd(hours));
  Wire.write(decToBcd(day));
  Wire.write(decToBcd(date));
  Wire.write(decToBcd(month));
  Wire.write(decToBcd(year));
  Wire.endTransmission();
}
 
 
void get3231Date()
{
  // send request to receive data starting at register 0
  Wire.beginTransmission(DS3231_I2C_ADDRESS); // 104 is DS3231 device address
  Wire.write(0x00); // start at register 0
  Wire.endTransmission();
  Wire.requestFrom(DS3231_I2C_ADDRESS, 7); // request seven shorts
 
  if(Wire.available()) {
    seconds = Wire.read(); // get seconds
    minutes = Wire.read(); // get minutes
    hours   = Wire.read();   // get hours
    day     = Wire.read();
    date    = Wire.read();
    month   = Wire.read(); //temp month
    year    = Wire.read();
       
    seconds = (((seconds & B11110000)>>4)*10 + (seconds & B00001111)); // convert BCD to decimal
    minutes = (((minutes & B11110000)>>4)*10 + (minutes & B00001111)); // convert BCD to decimal
    hours   = (((hours & B00110000)>>4)*10 + (hours & B00001111)); // convert BCD to decimal (assume 24 hour mode)
    day     = (day & B00000111); // 1-7
    date    = (((date & B00110000)>>4)*10 + (date & B00001111)); // 1-31
    month   = (((month & B00010000)>>4)*10 + (month & B00001111)); //msb7 is century overflow
    year    = (((year & B11110000)>>4)*10 + (year & B00001111));
  }
  else {
    //oh noes, no data!
  }
 
  switch (day) {
    case 1:
      strcpy(weekDay, "Sun");
      break;
    case 2:
      strcpy(weekDay, "Mon");
      break;
    case 3:
      strcpy(weekDay, "Tue");
      break;
    case 4:
      strcpy(weekDay, "Wed");
      break;
    case 5:
      strcpy(weekDay, "Thu");
      break;
    case 6:
      strcpy(weekDay, "Fri");
      break;
    case 7:
      strcpy(weekDay, "Sat");
      break;
  }
}
 
//float get3231Temp()
//{
//  //temp registers (11h-12h) get updated automatically every 64s
//  Wire.beginTransmission(DS3231_I2C_ADDRESS);
//  Wire.write(0x11);
//  Wire.endTransmission();
//  Wire.requestFrom(DS3231_I2C_ADDRESS, 2);
// 
//  if(Wire.available()) {
//    tMSB = Wire.read(); //2's complement int portion
//    tLSB = Wire.read(); //fraction portion
//   
//    temp3231 = (tMSB & B01111111); //do 2's math on Tmsb
//    temp3231 += ( (tLSB >> 6) * 0.25 ); //only care about bits 7 & 8
//  }
//  else {
//    //error! no data!
//  }
//  return temp3231;
//}

void printWiFiStatus() { // wifi 추가 부분
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}
