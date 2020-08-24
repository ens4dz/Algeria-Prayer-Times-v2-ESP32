// scrollup ok, and scroll right ok for short text, font ok, .
//to do: improve http server (index html is inspired from ESP32-CAM example. ) ,
//to do: add hadith db
#define RELAY 19
  
// CONNECTIONS:
// DS1302 CLK/SCLK --> 5 
// DS1302 DAT/IO --> 4
// DS1302 RST/CE --> 2
// DS1302 VCC --> 3.3v - 5v
// DS1302 GND --> GND
// vga bluePin = 32; 
// vga  greenPin = 33; 
// vga  redPin = 25;
// vga  hsyncPin = 26;
// vga  vsyncPin = 27;
// RELAY 19

//libs for sqlite db
#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <FS.h>
#include "SPIFFS.h"
// This file should be compiled with 'Partition Scheme' (in Tools menu)
// set to 'NO OTA (2MB APP/2MB FATFS)' 

//fot vga
#include "Adafruit_GFX.h"
#include <WiFi.h>
#include "time.h"
#include <ESP32Lib.h>
#include <Ressources/Font6x8.h> //to print ip address
#include <GfxWrapper.h>

//clock rtc "Rtc_by_Makuna"
#include <ThreeWire.h>  
#include <RtcDS1302.h> 

//Html server:
#include <WiFi.h>
#include <Hash.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "index.h" //html page
#include <EEPROM.h> // to save city id and salat time shift config.

//for write in arabic
//#include "prReshaper.h"
#include <U8g2_for_Adafruit_GFX.h>
#include "fonts.c"

//----------------------------------------------------------------------------
#define DEBUG                            1
#define LCDWidth                         320
#define LCDHeight                        200
#define ALIGNE_CENTER(t)                 ((LCDWidth - (u8g2_for_adafruit_gfx.getUTF8Width(t))) / 2)
#define ALIGNE_RIGHT(t)                  (LCDWidth - u8g2_for_adafruit_gfx.getUTF8Width(t))
#define ALIGNE_LEFT                      0
#define IS_UNICODE(c)                   (((c) & 0xc0) == 0xc0)
#define VERSION       1
#define BLACK    0x0000
#define BLUE     0x001F
#define RED      0xf800
#define GREEN    0x07E0
#define CYAN     0x07FF
#define MAGENTA  0xF81F
#define YELLOW   0xFFE0 
#define WHITE    0xFFFF
//----------------------------------------------------------------------------
U8G2_FOR_ADAFRUIT_GFX u8g2_for_adafruit_gfx;
#define FONT watan

//for scrollRight
#define SCROLL_DELTA 2
#define SCROLL_DELAY 0

String Hadith_string="" ;

//warning use this website:http://www.arabic-keyboard.org/photoshop-arabic/
static const char* const salat_name_tab[6]={" ﺮﺠﻔﻟﺍ"," ﻕﻭﺮﺸﻟﺍ"," ﺮﻬﻈﻟﺍ" , " ﺮﺼﻌﻟﺍ" ," ﺏﺮﻐﻤﻟﺍ" ," ﺀﺎﺸﻌﻟﺍ" } ;
char** salat_times_list ; //for printing2vga (04:15)

static const char* const HijriMonthNames[13]={"الشهر0","ﻡﺮﺤﻣ","ﺮﻔﺻ" ,"ﻝﻭﻷﺍ ﻊﻴﺑﺭ" ,"ﻲﻧﺎﺜﻟﺍ ﻊﻴﺑﺭ","ﻝﻭﻷﺍ ﻯﺩﺎﻤﺟ","ﻲﻧﺎﺜﻟﺍ ﻯﺩﺎﻤﺟ","ﺐﺟﺭ","ﻥﺎﺒﻌﺷ","ﻥﺎﻀﻣﺭ","ﻝﺍﻮﺷ","ﺓﺪﻌﻘﻟﺍﻭﺫ","ﺔﺠﺤﻟﺍ ﻭﺫ" } ;
static const char* const GeoMonthNames[13]={"الشهر0","ﻲﻔﻧﺎﺟ","ﻱﺮﻔﻴﻓ","ﺱﺭﺎﻣ","ﻞﻳﺮﻓﺃ","ﻱﺎﻣ","ﻥﺍﻮﺟ","ﺔﻴﻠﻳﻮﺟ","ﺕﻭﺃ","ﺮﺒﻤﺘﺒﺳ","ﺮﺑﻮﺘﻛﺃ","ﺮﺒﻤﻓﻮﻧ","ﺮﺒﻤﺴﻳﺩ"} ;
static const char* const dayNames[7]={"ﺪﺣﻷﺍ","ﻦﻴﻨﺛﻹﺍ","ﺀﺎﺛﻼﺜﻟﺍ","ﺀﺎﻌﺑﺭﻷﺍ","ﺲﻴﻤﺨﻟﺍ","ﺔﻌﻤﺠﻟﺍ","ﺖﺒﺴﻟﺍ"} ;

uint8_t salat2show=0 ; // show fajr first
int16_t  salat_top_offset[6]={LCDHeight, LCDHeight, LCDHeight,LCDHeight,LCDHeight,LCDHeight}; //outside of screen
  
//VGA Device
#define  bluePin 32
#define  greenPin  33
#define  redPin  25
#define  hsyncPin 26
#define  vsyncPin 27
VGA3Bit vga;
GfxWrapper<VGA3Bit> gfx(vga, 320, 200); //screen resolution

//for ntp time 
struct tm timeinfo; 
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600; //+1GMT
const int8_t   daylightOffset_sec = 0; 

const char* ssid = "m";  
const char* password = "12345678+";

//// Set your Static IP address
//IPAddress local_IP(192, 168, 43, 77);
//// Set your Gateway IP address
//IPAddress gateway(192, 168, 43, 1);
//IPAddress subnet(255, 255, 255, 0);

//vars to save the sql request
char HijriDate[13],GeoDate[13]; 
unsigned int sqliteHijri; //to calcule H_year,H_month,H_day, 14420707 to 1442 07 07
uint16_t H_year ;
uint8_t H_month,H_day ;// hidjri date 
uint8_t G_Day ; // to reboot if this changed
int16_t nextSalatMinutes,FajrMinutes,ShurooqMinutes,DhuhrMinutes,AsrMinutes,MaghribMinutes,IshaMinutes ;

// store config in EEPROM.
//see this:https://roboticsbackend.com/arduino-store-int-into-eeprom/
//warning int size is 4 bytes in esp32, https://github.com/espressif/arduino-esp32/issues/1745 
//this vars to be saved in EEPROM
int8_t  Auto_fan,city_id,hijri_shift,Fajr_shift,Shurooq_shift,Dhuhr_shift,Asr_shift,Maghrib_shift,Isha_shift ;

AsyncWebServer server(80);
bool Auto_fan_Test=false ;
#define RELAY 19

RtcDateTime ntp_time,now;
ThreeWire myWire(4,5,2); // IO, SCLK, CE - RTC clock
RtcDS1302<ThreeWire> Rtc(myWire);

//for sqlite functions
/* You only need to format SPIFFS the first time you run a
   test or else use the SPIFFS plugin to create a partition
   https://github.com/me-no-dev/arduino-esp32fs-plugin */
#define FORMAT_SPIFFS_IF_FAILED false
const char* data = "Callback function called";
static int callback(void *data, int argc, char **argv, char **azColName) {
   int i;
   Serial.printf("%s: ", (const char*)data);
   for (i = 0; i<argc; i++){
       Serial.printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
   }
   Serial.printf("\n");
   return 0;
}

int db_open(const char *filename, sqlite3 **db) {
   int rc = sqlite3_open(filename, db);
   if (rc) {
       Serial.printf("Can't open database: %s\n", sqlite3_errmsg(*db));
       return rc;
   } else {
       Serial.printf("Opened database successfully\n");
   }
   return rc;
}

char *zErrMsg = 0;
int db_exec(sqlite3 *db, const char *sql) {
   Serial.println(sql);
   long start = micros();
   int rc = sqlite3_exec(db, sql, callback, (void*)data, &zErrMsg);
   if (rc != SQLITE_OK) {
       Serial.printf("SQL error: %s\n", zErrMsg);
       sqlite3_free(zErrMsg);
   } else {
       Serial.printf("Operation done successfully\n");
   }
   Serial.print(F("Time taken:"));
   Serial.println(micros()-start);
   return rc;
}
//end sqlite functions


// http server:
void save_config(AsyncWebServerRequest *request){

   EEPROM.write(0, Auto_fan);
   EEPROM.write(1, city_id); 
   EEPROM.write(2, hijri_shift);
   EEPROM.write(3, Fajr_shift);
   EEPROM.write(4, Shurooq_shift);
   EEPROM.write(5, Dhuhr_shift);
   EEPROM.write(6, Asr_shift);
   EEPROM.write(7, Maghrib_shift);
   EEPROM.write(8, Isha_shift);   
   if( EEPROM.commit() )   Serial.print("config saved!");


    AsyncWebServerResponse * response = request->beginResponse(200, "text/plain", "تم حفظ البيانات يرجى اعادة التشغيل");
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
}

void Status(AsyncWebServerRequest *request){
    static char json_response[1024];
    char * p = json_response;
    *p++ = '{';
    
    p+=sprintf(p, "\"Auto_fan\":%d,", Auto_fan);
    p+=sprintf(p, "\"city\":%2d,", city_id);
    p+=sprintf(p, "\"hijri_shift\":%d,", hijri_shift);
    p+=sprintf(p, "\"Fajr_shift\":%d,", Fajr_shift);
    p+=sprintf(p, "\"Shurooq_shift\":%d,", Shurooq_shift);
    p+=sprintf(p, "\"Dhuhr_shift\":%d,", Dhuhr_shift);
    p+=sprintf(p, "\"Asr_shift\":%d,", Asr_shift);
    p+=sprintf(p, "\"Maghrib_shift\":%d,", Maghrib_shift);
    p+=sprintf(p, "\"Isha_shift\":%d", Isha_shift);
    

    *p++ = '}';
    *p++ = 0;
    AsyncWebServerResponse * response = request->beginResponse(200, "application/json", json_response);
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
}

void  getClockIndex(AsyncWebServerRequest *request){
AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", index_html_gz, index_html_gz_len);
response->addHeader("Content-Encoding", "gzip");
response->addHeader("Access-Control-Max-Age", "5");

request->send(response);
}


void uptimer(AsyncWebServerRequest *request){
    static char json_response[200];
    int sec = millis() / 1000;
    int min = sec / 60;
    int hr = min / 60;
    int dy = hr / 24;

    char * p = json_response;
    *p++ = '{';
    
    p+=sprintf(p, "\"uptime\":\"");
    p+=sprintf(p," مشتغل منذ ");
    p+=sprintf(p,"%02d ", dy);
    p+=sprintf(p,"يوم و ");
    p+=sprintf(p,"%02d-%02d-%02d ",sec %60, min % 60,hr % 24);
    p+=sprintf(p,"الساعة حاليا: ");
    p+=sprintf(p,"%02d:%02d:%02d",now.Hour(),now.Minute(),now.Second());
    p+=sprintf(p," التاريخ الهجري: ");
    p+=sprintf(p,"%s", HijriDate);
    p+=sprintf(p," التاريخ الميلادي: ");
    p+=sprintf(p,"%s", GeoDate);    
    p+=sprintf(p,"\""); //end of json
    
    *p++ = '}';
    *p++ = 0;              
    AsyncWebServerResponse * response = request->beginResponse(200, "application/json", json_response);
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
}

void setClockVar(AsyncWebServerRequest *request){
    if(!request->hasArg("var") || !request->hasArg("val")){
        request->send(404);
        return;
    }
    String var = request->arg("var");
    const char * variable = var.c_str();
    int val = atoi(request->arg("val").c_str());
    int res = 0;
  
    if(!strcmp(variable, "Auto_fan")) Auto_fan=val;
    
      else if(!strcmp(variable, "city")) city_id=val;    
      else if(!strcmp(variable, "hijri_shift")) hijri_shift=val;
      else if(!strcmp(variable, "Fajr_shift")) Fajr_shift=val;
      else if(!strcmp(variable, "Shurooq_shift")) Shurooq_shift=val;
      else if(!strcmp(variable, "Dhuhr_shift")) Dhuhr_shift=val;
      else if(!strcmp(variable, "Asr_shift")) Asr_shift=val;
      else if(!strcmp(variable, "Maghrib_shift")) Maghrib_shift=val;
      else if(!strcmp(variable, "Isha_shift")) Isha_shift=val;

     else {
        request->send(404);
        return;
    }

    AsyncWebServerResponse * response = request->beginResponse(200);
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
}

///zzz

#define countof(a) (sizeof(a) / sizeof(a[0]))
String printDateTime(const RtcDateTime& dt)
{
    char datestring[20];
    snprintf_P(datestring, 
            countof(datestring),
            PSTR("%02u/%02u %02u:%02u:%02u"),
            dt.Month(),
            dt.Day(),
            dt.Hour(),
            dt.Minute(),
            dt.Second() );
    return datestring;
}


String printTime(const RtcDateTime& dt)
{
    char datestring[7];
    snprintf_P(datestring, 
            countof(datestring),
            PSTR("%02u:%02u"),
            dt.Hour(),
            dt.Minute());
    return datestring;
}


void updateFromNTP(){
  //need to add real code here !!!
    while ( WiFi.status() != WL_CONNECTED ) {
      Serial.println("connecting wifi");
      delay ( 500 );
      Serial.print ( "connecting to WiFi." );
    }
  
    while ( !getLocalTime(&timeinfo) ){
    Serial.println("updating time..");
    Serial.println("updating time..");
    delay ( 1000 );
    }
    
    Serial.print("Use this URL to connect: ");
    Serial.print("http://");
    Serial.print(WiFi.localIP());
    Serial.println("/");
    time_t timeSinceEpoch = mktime(&timeinfo);
    ntp_time.InitWithEpoch32Time(timeSinceEpoch+gmtOffset_sec);
    Rtc.SetDateTime(ntp_time);
}

void printClock()
{
u8g2_for_adafruit_gfx.setFont(watan); 
gfx.fillRect(0, 0,LCDWidth, 50, BLACK); //WHITE
u8g2_for_adafruit_gfx.setForegroundColor(WHITE); //black
u8g2_for_adafruit_gfx.setBackgroundColor(BLACK);   
u8g2_for_adafruit_gfx.drawUTF8(0,40,printTime(now).c_str()); //  for exp: 17:00

vga.setCursor(0, 0);
vga.setTextColor(vga.RGB(255,0,0));
char ip[20] ;
snprintf_P(ip,20,WiFi.localIP().toString().c_str());
vga.print(ip); //  for ip address

}

void Adan(short i)
{
Serial.print("Adan time: ");
    if (Auto_fan) {
         digitalWrite(RELAY,HIGH) ;
         Serial.println("Fans turned on!");
    } 
//Todo other cmd..

   unsigned long iqama_start_Millis=millis(); 
   char tmpp[]= "ﻥﺍﺫﺃ ﺖﻗﻭ ﻥﺎﺣ" ;
   u8g2_for_adafruit_gfx.setFont(ae_Dimnah36);
   bool color=true;    
while (millis() - iqama_start_Millis <60*1000){   
     if(color){
     vga.clear(WHITE) ;
     u8g2_for_adafruit_gfx.setFont(ae_Dimnah36);
     u8g2_for_adafruit_gfx.setBackgroundColor(WHITE);
     u8g2_for_adafruit_gfx.setForegroundColor(BLACK);  
     }
     else
     {
     vga.clear(BLACK) ;
     u8g2_for_adafruit_gfx.setFont(ae_Dimnah36);
     u8g2_for_adafruit_gfx.setBackgroundColor(BLACK);
     u8g2_for_adafruit_gfx.setForegroundColor(WHITE);      
     }
     
     u8g2_for_adafruit_gfx.drawUTF8(ALIGNE_CENTER(tmpp),50,tmpp);
     u8g2_for_adafruit_gfx.drawUTF8(ALIGNE_CENTER(salat_name_tab[i]),130,salat_name_tab[i]);
     vga.show() ;
     color=!color;
     delay(5000);
}

}

void setup(){
  Serial.begin(115200);
  EEPROM.begin(512);
    
  Auto_fan = EEPROM.read(0);
  city_id = EEPROM.read(1);
  hijri_shift = EEPROM.read(2);

  Fajr_shift = EEPROM.read(3);
  Shurooq_shift = EEPROM.read(4);  
  Dhuhr_shift = EEPROM.read(5);
  Asr_shift = EEPROM.read(6);  
  Maghrib_shift = EEPROM.read(7);
  Isha_shift = EEPROM.read(8);
  
  pinMode(RELAY, OUTPUT);
  digitalWrite(RELAY,LOW);

//for manual ip:  
//     if (!WiFi.config(local_IP, gateway, subnet)) {
//    Serial.println("WiFi Failed to configure");}

    WiFi.begin(ssid, password);
    delay(3000);  
    Serial.println(WiFi.localIP());
    configTime(gmtOffset_sec, daylightOffset_sec,  ntpServer);    
    Serial.print("compiled: ");
    Serial.print(__DATE__);
    Serial.print(" ");
    Serial.println(__TIME__);

    Rtc.Begin();
    RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);

    if (!Rtc.IsDateTimeValid()) 
    {
        // Common Causes:
        //    1) first time you ran and the device wasn't running yet
        //    2) the battery on the device is low or even missing
        Serial.println("RTC lost confidence in the DateTime!");
        updateFromNTP();
    }

    if (Rtc.GetIsWriteProtected())
    {
        Serial.println("RTC was write protected, enabling writing now");
        Rtc.SetIsWriteProtected(false);
    }

    if (!Rtc.GetIsRunning())
    {
        Serial.println("RTC was not actively running, starting now");
        Rtc.SetIsRunning(true);
    }
    
  time_t timeSinceEpoch=0;
  time_t timeSinceEpochRtc=0;
  
  if ( getLocalTime(&timeinfo) ){        //get time from ntp test
    timeSinceEpoch = mktime(&timeinfo);
    ntp_time.InitWithEpoch32Time(timeSinceEpoch+gmtOffset_sec);
    updateFromNTP();
  }

    now = Rtc.GetDateTime();
    Serial.print("time from NTP:" ); 
    Serial.println(printDateTime(ntp_time)); //print ntp time
    Serial.print("time from RTC:" );     
    Serial.println(printDateTime(now)); //print rtc time

    if (now < compiled) 
    {
        Serial.println("RTC is older than compiled time!  (Updating DateTime)");
        updateFromNTP();
    }
    
  //Serial.print("FreeMem before http:") ;   //FreeMem before http:255988
  //Serial.println(ESP.getFreeHeap() ) ; 

/////  http server
    server.on("/save_config", HTTP_GET, save_config);
    server.on("/control", HTTP_GET, setClockVar);
    server.on("/status", HTTP_GET, Status);
    server.on("/uptime", HTTP_GET, uptimer);
   server.on("/Auto_fan_Test", HTTP_GET, [](AsyncWebServerRequest *request){  Auto_fan_Test=true ; });
    server.on("/", HTTP_GET, getClockIndex);
   server.on("/restart", HTTP_GET, [](AsyncWebServerRequest *request){  ESP.restart();  });

  server.begin();

  
////// end server


//read sqliteDB
sqlite3 *db1,*db2;
int rc;
sqlite3_stmt *res;
const char *tail;
   
   if (!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)) {
       Serial.println("Failed to mount file system");
       return;
   }

   // list SPIFFS contents
   File root = SPIFFS.open("/");
   if (!root) {
       Serial.println("- failed to open directory");
       return;
   }
   
   if (!root) {
       Serial.println("- failed to open directory");
       return;
   }
   if (!root.isDirectory()) {
       Serial.println(" - not a directory");
       return;
   }
   File file = root.openNextFile();
   while (file) {
       if (file.isDirectory()) {
           Serial.print("  DIR : ");
           Serial.println(file.name());
       } else {
           Serial.print("  FILE: ");
           Serial.print(file.name());
           Serial.print("\tSIZE: ");
           Serial.println(file.size());
       }
       file = root.openNextFile();
   }

  Serial.print("FreeMem before sqlite:") ;  
  Serial.println(ESP.getFreeHeap() ) ; 
//  FreeMem before sqlite:231816

   sqlite3_initialize();

   if (db_open("/spiffs/Algeria.db", &db1))
       return;
  
//  // for test only     
//   rc = db_exec(db1, "SELECT GeoDate,Fajr,Shurooq, Dhuhr, Asr, Maghrib,Isha FROM mawakit_midnight WHERE MADINA_ID = 1 AND GeoDate = 20200101");
//   if (rc != SQLITE_OK) {
//       sqlite3_close(db1);
//       return;
//   }
//  //end test

    //char ntp_time_as_str[10];
    //sprintf(ntp_time_as_str, "%d", rtc_time_as_int);
    //rtc_time_as_int=(now.tm_year+1900)*10000+(now.tm_mon+1)*100+now.tm_mday ;

   char dateForSQL[11];
    snprintf_P(dateForSQL,countof(dateForSQL),
            PSTR("%04u%02u%02u"),now.Year(),now.Month(),now.Day());
            //String asString(dateForSQL)
    snprintf_P(GeoDate,countof(GeoDate),
            PSTR("%04u-%02u-%02u"),now.Year(),now.Month(),now.Day());  // only for print in uptime func
  
  G_Day= now.Day() ; // save this day to reboot if now.Day() get changed
  
  String sql = "SELECT Fajr,Shurooq, Dhuhr, Asr, Maghrib,Isha FROM mawakit_midnight WHERE MADINA_ID = ";
  sql += city_id;
  sql += " AND GeoDate = ";
  //sql += "20200714";
  //sql +=ntp_time_as_str ;
  sql +=dateForSQL ;
  Serial.println(sql);
  rc = sqlite3_prepare_v2(db1, sql.c_str(), 1000, &res, &tail);
  if (rc != SQLITE_OK) {
    String resp = "Failed to fetch data: ";
    resp += sqlite3_errmsg(db1);
    Serial.println(resp.c_str());
    return;
  }


    // make salat_times_list[X]
    salat_times_list = (char **)malloc(6 * sizeof(char *));
    // make salat_times_list[X][X]
    for(size_t i = 0; i < 6; i++)
    {
        salat_times_list[i] = (char *)malloc(6 * sizeof(char));
    }

  Serial.println("salat times for today:");
  String resp = "";
  while (sqlite3_step(res) == SQLITE_ROW) {
    resp = "";
    resp += sqlite3_column_int(res, 0); //Fajr
    resp += " ";
    resp += sqlite3_column_int(res, 1); //Shurooq
    resp += " ";
    resp += sqlite3_column_int(res, 2); //Dhuhr
    resp += " ";
    resp += sqlite3_column_int(res, 3); //Asr
    resp += " ";
    resp += sqlite3_column_int(res, 4); //Maghrib
    resp += " ";
    resp += sqlite3_column_int(res, 5);//Isha
    Serial.println(resp);
    
    FajrMinutes = sqlite3_column_int(res, 0)+ Fajr_shift ;
    snprintf(salat_times_list[0],6, "%02d:%02d", FajrMinutes / 60, FajrMinutes % 60);
    
    ShurooqMinutes = sqlite3_column_int(res, 1) + Shurooq_shift ;
    snprintf(salat_times_list[1],6, "%02d:%02d", ShurooqMinutes / 60, ShurooqMinutes % 60);
    
    DhuhrMinutes = sqlite3_column_int(res, 2) + Dhuhr_shift;
    snprintf(salat_times_list[2],6, "%02d:%02d", DhuhrMinutes / 60, DhuhrMinutes % 60);
    
    AsrMinutes = sqlite3_column_int(res, 3) + Asr_shift ;
    snprintf(salat_times_list[3],6, "%02d:%02d", AsrMinutes / 60, AsrMinutes % 60);
    
    MaghribMinutes = sqlite3_column_int(res, 4) + Maghrib_shift;
    snprintf(salat_times_list[4],6, "%02d:%02d", MaghribMinutes / 60, MaghribMinutes % 60);
    
    IshaMinutes = sqlite3_column_int(res, 5) + Isha_shift;
    snprintf(salat_times_list[5],6, "%02d:%02d", IshaMinutes / 60, IshaMinutes % 60);    

    Serial.println("salat_times_list is saved !");
  }
sqlite3_finalize(res);


  Serial.print("FreeMem before get hijri date:") ;  
  Serial.println(ESP.getFreeHeap() ) ; 
  //FreeMem before get hijri date:190972

  //get hijri date
  resp="";
  sql = "SELECT HijriDate FROM itc_tab_hijri_geo_date WHERE GeoDate = ";
  sql +=dateForSQL ;
  Serial.println(sql);
  rc = sqlite3_prepare_v2(db1, sql.c_str(), 1000, &res, &tail);
  if (rc != SQLITE_OK) {
    resp = "Failed to fetch data of hijri time: ";
    resp += sqlite3_errmsg(db1);
    Serial.println(resp.c_str());
    return;
  }


  Serial.println("hijri_shift: ");
  Serial.println(hijri_shift);
  while (sqlite3_step(res) == SQLITE_ROW) {
    sqliteHijri=sqlite3_column_int(res, 0) ; //14391120
    H_year=sqliteHijri/10000;               // 1439
    sqliteHijri=sqliteHijri%10000  ;              //1120
    H_month=sqliteHijri/100;           //11
    H_day=sqliteHijri%100+hijri_shift ;           //20
      if(H_day==31) {
        H_day=1 ; 
        if(H_month!=12) H_month=H_month+1 ;
          else {
            H_month=1;
            H_year =H_year+1;
            }
        }
        else if (H_day==0) {
          H_day=30 ;
          if(H_month!=1) H_month=H_month-1 ;
          else {
            H_month=12;
            H_year =H_year-1;
            }
        }
      
    snprintf(HijriDate,11, "%04d-%02d-%02d", H_year, H_month,H_day);  
    Serial.print("today HijriDate: ");
    Serial.println(HijriDate);
  }
  sqlite3_finalize(res);
  
  sqlite3_close(db1);
  //end sqlitedb read



//Hadith_string
   if (db_open("/spiffs/hadith.db", &db2))
       return;
  sql = "SELECT HADITH  FROM ahadith WHERE id=";
  sql += 1;
  Serial.println(sql);
  rc = sqlite3_prepare_v2(db2, sql.c_str(), 1000, &res, &tail);
  if (rc != SQLITE_OK) {
    String resp = "Failed to fetch data: ";
    resp += sqlite3_errmsg(db2);
    Serial.println(resp.c_str());
    return;
  }

  Serial.println("HADITH FOR today:");
  resp = "";
  while (sqlite3_step(res) == SQLITE_ROW) {
    Hadith_string = "";
    Hadith_string += (const char *) sqlite3_column_text(res, 0); //HADITH
    Serial.println(Hadith_string);
        
  }
  sqlite3_finalize(res);
  sqlite3_close(db2);

  Serial.print("FreeMem before vga init:") ;  
  Serial.println(ESP.getFreeHeap() ) ; 
  //FreeMem before vga init: 231588
  //it's crached if FreeMem less than:190000 ,229k  
  //vga.init(vga.MODE640x400, redPin, greenPin, bluePin, hsyncPin, vsyncPin);

  //need double buffering reduce flickering
  vga.setFrameBufferCount(2);
  vga.init(vga.MODE320x200, redPin, greenPin, bluePin, hsyncPin, vsyncPin);
  vga.setFont(Font6x8); // small font ot print ip address

 u8g2_for_adafruit_gfx.begin(gfx);                 // connect u8g2 procedures to Adafruit GFX
//end setup
} 



short dv=1;
short wait=50;
void Printscrollup(const char* salat_name,const char* times){
     u8g2_for_adafruit_gfx.setFont(ae_Dimnah36);
     vga.fillRect(0,150,LCDWidth, 50,vga.RGB(0,0,0));
//     u8g2_for_adafruit_gfx.setBackgroundColor(GREEN);      
//     u8g2_for_adafruit_gfx.setForegroundColor(BLACK);
     u8g2_for_adafruit_gfx.setForegroundColor(GREEN);      
     u8g2_for_adafruit_gfx.setBackgroundColor(BLACK);
     u8g2_for_adafruit_gfx.drawUTF8(ALIGNE_RIGHT(salat_name)-20,salat_top_offset[salat2show],salat_name);

     u8g2_for_adafruit_gfx.setFont(watan); 
     //u8g2_for_adafruit_gfx.setForegroundColor(RED);
     u8g2_for_adafruit_gfx.drawUTF8(20,salat_top_offset[salat2show]+5,times);

  if(salat_top_offset[salat2show]>185 ){ //يستمر في الصعود أو النزول حسب الحالة
     salat_top_offset[salat2show] -= dv;  
  }
  else      { 
      if(wait>0) wait-=1 ; // ثبات
      else{ // بداية النزول
        wait=50 ; // للعرض القادم
        dv=-4 ; // يصبح التغير في العرض الى الاسفل
        salat_top_offset[salat2show] -= dv; // مكان العرض التالي تحدث مرة واحدة
        }   
    }

  if(salat_top_offset[salat2show]>250 ) { //outside of screen
    dv=2 ;
    salat_top_offset[salat2show]= 200 ; //fot the next show
    if (salat2show==5) salat2show=0;  //if ishaa show fajr
    else salat2show=salat2show+1;
  }

}

unsigned long hadith_update=0;  //some global variables available anywhere in the program
unsigned long startMillis;  //some global variables available anywhere in the program
unsigned long check_wifi = 30000;

String hadith_tmp="";
String hadith_test="";
String hadith_tmp2="";
String hadith_test2="";

int hadith_tmp_len;
int ii,hadith_pos=Hadith_string.length(),count = 0; 


void printHadith() {
  if(millis()>hadith_update){
    hadith_update=millis()+5000;      
    Serial.println(hadith_pos);
    hadith_tmp="";
    hadith_test="";
    
    for (ii = hadith_pos;  ii>=0;  ii--)
    {
        if (Hadith_string[ii] == ' ' || ii == 0){
            count++; 
            //snprintf(hadith_test,ii-hadith_pos,Hadith_string.substring(hadith_pos,ii).c_str());
            hadith_test=Hadith_string.substring(ii,hadith_pos) + hadith_test ;
            int hadith_test_len=u8g2_for_adafruit_gfx.getUTF8Width( hadith_test.c_str() ) ; 
            if  (hadith_test_len<LCDWidth) {
                //snprintf(hadith_tmp,ii-hadith_pos,Hadith_string.substring(hadith_pos,ii).c_str());
                hadith_tmp=Hadith_string.substring(ii,hadith_pos) + hadith_tmp;
                Serial.println(hadith_tmp);
                hadith_pos=ii;
              }  
              else break ;                    
        }
    }
  

    if(ii <0) {
      hadith_pos=Hadith_string.length() ;
      hadith_tmp2="";
    }
    else{
/////// second line
    hadith_tmp2="";
    hadith_test2="";
    
    for (ii = hadith_pos;  ii>=0;  ii--){
        if (Hadith_string[ii] == ' ' || ii == 0){
            count++; 
            //snprintf(hadith_test,ii-hadith_pos,Hadith_string.substring(hadith_pos,ii).c_str());
            hadith_test2=Hadith_string.substring(ii,hadith_pos) + hadith_test2 ;
            int hadith_test_len=u8g2_for_adafruit_gfx.getUTF8Width( hadith_test2.c_str() ) ; 
            if  (hadith_test_len<LCDWidth) {
                //snprintf(hadith_tmp,ii-hadith_pos,Hadith_string.substring(hadith_pos,ii).c_str());
                hadith_tmp2=Hadith_string.substring(ii,hadith_pos) + hadith_tmp2;
                Serial.println(hadith_tmp2);
                hadith_pos=ii;
              }  
              else break ;                    
        }
    }
  
    if(ii <0) {
      hadith_pos=Hadith_string.length() ;
    }
    
}

}
    
    gfx.fillRect(0, 50,LCDWidth, 50, WHITE);
    gfx.fillRect(0, 100,LCDWidth, 50, WHITE);
    u8g2_for_adafruit_gfx.setBackgroundColor(WHITE);      
    u8g2_for_adafruit_gfx.setForegroundColor(RED);   
    u8g2_for_adafruit_gfx.drawUTF8(ALIGNE_CENTER(hadith_tmp.c_str()),80,hadith_tmp.c_str());   
   
//    u8g2_for_adafruit_gfx.setBackgroundColor(GREEN);      
//    u8g2_for_adafruit_gfx.setForegroundColor(RED);       
    u8g2_for_adafruit_gfx.drawUTF8(ALIGNE_CENTER(hadith_tmp2.c_str()),130,hadith_tmp2.c_str()); //second line
}

void loop() {
//startMillis = millis();
    
    now = Rtc.GetDateTime();
    if (!now.IsValid())
    {
        // Common Causes:
        //    1) the battery on the device is low or even missing and the power line was disconnected
        Serial.println("RTC lost confidence in the DateTime!");
        ESP.restart() ;
    }

    char TimeNoSec[6];
    snprintf_P(TimeNoSec,countof(TimeNoSec),PSTR("%02u:%02u"),now.Hour(),now.Minute());
    if(now.Second()==0){
        Serial.print("now.Second()==0");
        if(!strcmp(salat_times_list[0], TimeNoSec)) Adan(0); //Fajr
        //else if(!strcmp(salat_times_list[1], TimeNoSec)) Adan(1); //Shurooq
        else if(!strcmp(salat_times_list[2],TimeNoSec)) Adan(2); //Dhuhr
        else if(!strcmp(salat_times_list[3], TimeNoSec)) Adan(3); //Asr
        else if(!strcmp(salat_times_list[4], TimeNoSec)) Adan(4); //Maghrib
        else if(!strcmp(salat_times_list[5], TimeNoSec)) Adan(5); //Isha
        
        if (now.DayOfWeek() == 6 ) {
          // todo: Djomoaa addons
        }                        
    } //end if now.second==0 
     
    if (Auto_fan_Test) {
        Serial.println("fans turned on !");
        digitalWrite(RELAY,HIGH) ;
        Auto_fan_Test=false;
        }
  
//  Serial.print("FreeMem:") ;  
//  Serial.println(ESP.getFreeHeap() ) ;   //wifi disconnected: FreeMem=90152


printClock(); //clock 09:00:00

//char nextSalat[9];  // "00:00:00" 
//int16_t nowMinutes=now.Hour()*60+now.Minute() ; 
//
//if ((FajrMinutes - nowMinutes) >= 0) {
//    nextSalatMinutes= FajrMinutes - nowMinutes ;
//}
//else if ((ShurooqMinutes - nowMinutes) >= 0) {
//    nextSalatMinutes= ShurooqMinutes- nowMinutes ;
//}
//else if ((AsrMinutes - nowMinutes) >= 0) {
//    nextSalatMinutes= AsrMinutes- nowMinutes ;
//}
//else if ((MaghribMinutes - nowMinutes) >= 0) {
//    nextSalatMinutes= MaghribMinutes- nowMinutes ;
//}
//else if ((IshaMinutes - nowMinutes) >= 0) {
//    nextSalatMinutes= IshaMinutes- nowMinutes ;
//}
//else if ((IshaMinutes - nowMinutes) < 0) {
//    nextSalatMinutes= (24*60-nowMinutes) + FajrMinutes ;
//}
//
//if (now.Second()==0){
//    if ((nextSalatMinutes / 60) > 0)  // 65-->01:05:00
//        snprintf(nextSalat,9, "%02d:%02d:00", nextSalatMinutes / 60, nextSalatMinutes % 60);  
//    else                              //    30:44
//        snprintf(nextSalat,9, "%02d:00", nextSalatMinutes % 60);  
//}
//else{ //!0
//      if ((nextSalatMinutes / 60) > 0)  // 02:05:17
//      snprintf(nextSalat,9, "%02d:%02d:%02d", nextSalatMinutes / 60, nextSalatMinutes % 60 -1 ,60-now.Second());  
//      else                              //    30:44
//      snprintf(nextSalat,9, "%02d:%02d", nextSalatMinutes % 60 -1 ,60-now.Second());  
//}
//
//u8g2_for_adafruit_gfx.drawUTF8(ALIGNE_RIGHT(nextSalat)-5,40,nextSalat);


/////////// print hadith
printHadith();

/////////////////////////////////////////////print date at bottom
char tmp[13];
int16_t len;
//u8g2_for_adafruit_gfx.setFont(ae_Dimnah24);
u8g2_for_adafruit_gfx.setFont(watan20); 
u8g2_for_adafruit_gfx.setBackgroundColor(BLACK);   
//gfx.fillRect(0, 100,LCDWidth, 50, YELLOW);   

///////////////////hijri print:

if (salat2show!=5) { // this will print hijri most of time !
//    u8g2_for_adafruit_gfx.setForegroundColor(WHITE);  
//    u8g2_for_adafruit_gfx.drawUTF8(ALIGNE_RIGHT(dayNames[now.DayOfWeek()]), 32,dayNames[now.DayOfWeek()]);
//         len=u8g2_for_adafruit_gfx.getUTF8Width(dayNames[now.DayOfWeek()]) +5 ;
    
    u8g2_for_adafruit_gfx.setForegroundColor(RED);    
    snprintf_P(tmp,countof(tmp),PSTR("%02u"),H_day);    
    u8g2_for_adafruit_gfx.drawUTF8(ALIGNE_RIGHT(tmp)-3, 32,tmp);
//        len=len+u8g2_for_adafruit_gfx.getUTF8Width(tmp);   
          len=u8g2_for_adafruit_gfx.getUTF8Width(tmp)+10;           

    u8g2_for_adafruit_gfx.setForegroundColor(WHITE);  
    u8g2_for_adafruit_gfx.drawUTF8(ALIGNE_RIGHT(HijriMonthNames[H_month]) -len, 32,HijriMonthNames[H_month]);
         len=len+u8g2_for_adafruit_gfx.getUTF8Width(HijriMonthNames[H_month])+10 ;

    u8g2_for_adafruit_gfx.setForegroundColor(RED);    
    snprintf_P(tmp,countof(tmp),PSTR("%04u"),H_year);    
    u8g2_for_adafruit_gfx.drawUTF8(ALIGNE_RIGHT(tmp)-len, 32,tmp);
         //len=len+u8g2_for_adafruit_gfx.getUTF8Width(tmp);  
}
else {
/////////// geo print                 
    u8g2_for_adafruit_gfx.setForegroundColor(WHITE);  
    u8g2_for_adafruit_gfx.drawUTF8(ALIGNE_RIGHT(dayNames[now.DayOfWeek()])-3, 32,dayNames[now.DayOfWeek()]);
         len=u8g2_for_adafruit_gfx.getUTF8Width(dayNames[now.DayOfWeek()]) +10;
    
    u8g2_for_adafruit_gfx.setForegroundColor(RED);    
    snprintf_P(tmp,countof(tmp),PSTR("%02u"),now.Day());
    u8g2_for_adafruit_gfx.drawUTF8(ALIGNE_RIGHT(tmp)-len , 32,tmp);
         len=len+u8g2_for_adafruit_gfx.getUTF8Width(tmp)+10;   
         
    u8g2_for_adafruit_gfx.setForegroundColor(WHITE);  
     u8g2_for_adafruit_gfx.drawUTF8(ALIGNE_RIGHT(GeoMonthNames[now.Month()])-len, 32,GeoMonthNames[now.Month()]);
        //len=len+u8g2_for_adafruit_gfx.getUTF8Width(GeoMonthNames[now.Month()]);     

//    u8g2_for_adafruit_gfx.setForegroundColor(RED);    
//    snprintf_P(tmp,countof(tmp),PSTR("%02u"),now.Year());
//    u8g2_for_adafruit_gfx.drawUTF8(ALIGNE_RIGHT(tmp)-len , 32,tmp);
}

//////////////////////   print salat times

  Printscrollup(salat_name_tab[salat2show],salat_times_list[salat2show]);


  // if Wifi is down, try reconnecting every 30 seconds
  if (millis() > check_wifi) {
    if (WiFi.status() != WL_CONNECTED){
    Serial.println("Reconnecting to WiFi...");
    WiFi.disconnect();
    WiFi.begin(ssid, password);
    check_wifi = millis() + 30000;
  }
  }
  

if (G_Day!=now.Day()) ESP.restart(); //reboot every new day


vga.show();     // make everything visible
 
//Serial.print("time for 1 loop:") ;  
//Serial.println(millis() - startMillis) ; //60
}
