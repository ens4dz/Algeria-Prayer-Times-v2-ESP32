//working ntp, fixed without ntp lib, improve time at boot
//to do: improve http server (index html is inspired from ESP32-CAM example. )
// scrollup ok, and scroll right ok for no long text, font ok, .
//to do: add count down salat

  
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
#include <AsyncElegantOTA.h>
#include "index.h" //html page
#include <EEPROM.h> // to save city id and salat time shift config.

//for write in arabic
#include "prReshaper.h"
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
char Hadith[]= "قال رسول الله صلى الله عليه وسلم خير الأعمال أدومها وإن قل" ;
int left_offset_hadith=500;     // offset to draw
String s; // to save reshaped utf8 hadith from sql

//warning use this website:http://www.arabic-keyboard.org/photoshop-arabic/
const char Fajr_name[25]= " ﺮﺠﻔﻟﺍ"; // this is formated utf8 !
const char Shurooq_name[25]= " ﻕﻭﺮﺸﻟﺍ" ;
const char Dhuhr_name[25]= " ﺮﻬﻈﻟﺍ" ;
const char Asr_name[25]= " ﺮﺼﻌﻟﺍ" ;
const char Maghrib_name[25]= " ﺏﺮﻐﻤﻟﺍ" ;
const char Isha_name[25]= " ﺀﺎﺸﻌﻟﺍ" ;
static const char* const HijriMonthNames[]={"الشهر0","ﻡﺮﺤﻣ","ﺮﻔﺻ" ,"ﻝﻭﻷﺍ ﻊﻴﺑﺭ" ,"ﻲﻧﺎﺜﻟﺍ ﻊﻴﺑﺭ","ﻝﻭﻷﺍ ﻯﺩﺎﻤﺟ","ﻲﻧﺎﺜﻟﺍ ﻯﺩﺎﻤﺟ","ﺐﺟﺭ","ﻥﺎﺒﻌﺷ","ﻥﺎﻀﻣﺭ","ﻝﺍﻮﺷ","ﺓﺪﻌﻘﻟﺍﻭﺫ","ﺔﺠﺤﻟﺍ ﻭﺫ" } ;
static const char* const GeoMonthNames[]={"الشهر0","ﻲﻔﻧﺎﺟ","ﻱﺮﻔﻴﻓ","ﺱﺭﺎﻣ","ﻞﻳﺮﻓﺃ","ﻱﺎﻣ","ﻥﺍﻮﺟ","ﺔﻴﻠﻳﻮﺟ","ﺕﻭﺃ","ﺮﺒﻤﺘﺒﺳ","ﺮﺑﻮﺘﻛﺃ","ﺮﺒﻤﻓﻮﻧ","ﺮﺒﻤﺴﻳﺩ"} ;
static const char* const dayNames[]={"ﺖﺒﺴﻟﺍ","ﺪﺣﻷﺍ","ﻦﻴﻨﺛﻹﺍ","ﺀﺎﺛﻼﺜﻟﺍ","ﺀﺎﻌﺑﺭﻷﺍ","ﺲﻴﻤﺨﻟﺍ","ﺔﻌﻤﺠﻟﺍ"} ;

uint8_t salat2show=1 ; // show fajr first
int16_t  salat_top_offset[7]={200, 200, 200,200,200,200,200}; //outside of screen
  
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

const char* ssid = "........";  
const char* password = ".......";

//// Set your Static IP address
//IPAddress local_IP(192, 168, 43, 77);
//// Set your Gateway IP address
//IPAddress gateway(192, 168, 43, 1);
//IPAddress subnet(255, 255, 255, 0);

//vars to save sql request
char HijriDate[13],GeoDate[13],Fajr[6],Shurooq[6], Dhuhr[6], Asr[6], Maghrib[6],Isha[6]; //for printing2vga
unsigned int sqliteHijri; //to calcule H_year,H_month,H_day, 14420707 to 1442 07 07
uint16_t H_year ;
uint8_t H_month,H_day ;// hidjri date 
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
request->send(response);
}


void uptimer(AsyncWebServerRequest *request){
    static char json_response[128];
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

gfx.fillRect(0, 0,320, 50, WHITE); //WHITE
u8g2_for_adafruit_gfx.setForegroundColor(0); //black
u8g2_for_adafruit_gfx.setBackgroundColor(WHITE);   
u8g2_for_adafruit_gfx.drawUTF8(0,40,printTime(now).c_str()); //  for exp: 17:00

vga.setCursor(0, 0);
vga.setTextColor(vga.RGB(255,0,0));
char ip[20] ;
snprintf_P(ip,20,WiFi.localIP().toString().c_str());
vga.print(ip); //  for ip address

}

void Adan()
{
Serial.print("Adan time: ");
//Todo other cmd..
        if (Auto_fan) {
             digitalWrite(RELAY,HIGH) ;
             Serial.println("Fans turned on!");
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

  AsyncElegantOTA.begin(&server);    // Start ElegantOTA
  server.begin();
//  WiFi.disconnect(true);
//  WiFi.mode(WIFI_OFF);
//disconnect WiFi as it's no longer needed
  
////// end server


//read sqliteDB
sqlite3 *db1;
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
            
  String sql = "SELECT GeoDate,Fajr,Shurooq, Dhuhr, Asr, Maghrib,Isha FROM mawakit_midnight WHERE MADINA_ID = ";
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

  Serial.println("salat times for today:");
  String resp = "";
  while (sqlite3_step(res) == SQLITE_ROW) {
    FajrMinutes = sqlite3_column_int(res, 1)+Fajr_shift ;
    snprintf(Fajr,6, "%02d:%02d", FajrMinutes / 60, FajrMinutes % 60);
    
    ShurooqMinutes = sqlite3_column_int(res, 2) + Shurooq_shift ;
    snprintf(Shurooq,6, "%02d:%02d", ShurooqMinutes / 60, ShurooqMinutes % 60);
    
    DhuhrMinutes = sqlite3_column_int(res, 3) + Dhuhr_shift;
    snprintf(Dhuhr,6, "%02d:%02d", DhuhrMinutes / 60, DhuhrMinutes % 60);
    
    AsrMinutes = sqlite3_column_int(res, 4) + Asr_shift ;
    snprintf(Asr,6, "%02d:%02d", AsrMinutes / 60, AsrMinutes % 60);
    
    MaghribMinutes = sqlite3_column_int(res, 5) + Maghrib_shift;
    snprintf(Maghrib,6, "%02d:%02d", MaghribMinutes / 60, MaghribMinutes % 60);
    
    IshaMinutes = sqlite3_column_int(res, 6) + Isha_shift;
    snprintf(Isha,6, "%02d:%02d", IshaMinutes / 60, IshaMinutes % 60);    
    resp = "";
    resp += sqlite3_column_int(res, 1); //Fajr
    resp += " ";
    resp += sqlite3_column_int(res, 2); //Shurooq
    resp += " ";
    resp += sqlite3_column_int(res, 3); //Dhuhr
    resp += " ";
    resp += sqlite3_column_int(res, 4); //Asr
    resp += " ";
    resp += sqlite3_column_int(res, 5); //Maghrib
    resp += " ";
    resp += sqlite3_column_int(res, 6);//Isha
    Serial.println(resp);
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


//unsigned long startMillis;  //some global variables available anywhere in the program
//unsigned long currentMillis;
//const unsigned long period = 1000;  //the value is a number of milliseconds


short dv=1;
short wait=50;
void Printscrollup(const char* salat_name,char* times){
     u8g2_for_adafruit_gfx.setFont(ae_Dimnah36);
     vga.fillRect(0,150,LCDWidth, 50,vga.RGB(0,0,0));
     u8g2_for_adafruit_gfx.setForegroundColor(WHITE);
     u8g2_for_adafruit_gfx.drawUTF8(ALIGNE_RIGHT(salat_name)-10,salat_top_offset[salat2show],salat_name);

     u8g2_for_adafruit_gfx.setForegroundColor(RED);
     u8g2_for_adafruit_gfx.drawUTF8(10,salat_top_offset[salat2show],times);

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
    if (salat2show==6) salat2show=1;  //if ishaa show fajr
    else salat2show=salat2show+1;
  }

}


void loop() {
//startMillis = millis();
//vga.clear(0) ;
    
    now = Rtc.GetDateTime();
    if (!now.IsValid())
    {
        // Common Causes:
        //    1) the battery on the device is low or even missing and the power line was disconnected
        Serial.println("RTC lost confidence in the DateTime!");
        ESP.restart() ;
    }

  //toDo: use numbers better than chars
  //int TimeNoSec=now.Hour()*60+now.Minute() ; 
  
    char TimeNoSec[6];
    snprintf_P(TimeNoSec,countof(TimeNoSec),PSTR("%02u:%02u"),now.Hour(),now.Minute());
    if(now.Second()==0){
        Serial.print("now.Second()==0");
        //test if salat time Fajr,Shurooq, Dhuhr, Asr, Maghrib,Isha;      
        if(!strcmp(Fajr, TimeNoSec)) Adan();
        else if(!strcmp(Shurooq, TimeNoSec)) Adan();
        else if(!strcmp(Dhuhr,TimeNoSec)) Adan();
        else if(!strcmp(Asr, TimeNoSec)) Adan();
        else if(!strcmp(Maghrib, TimeNoSec)) Adan();
        else if(!strcmp(Isha, TimeNoSec)) Adan();
        
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


//u8g2_for_adafruit_gfx.setFontMode(1);                 // use u8g2 transparent mode (this is default)
//u8g2_for_adafruit_gfx.setFont(ae_Dimnah24);

u8g2_for_adafruit_gfx.setFont(watan); 
printClock(); //clock 09:00:00

char nextSalat[9];  // "00:00:00" 
int16_t nowMinutes=now.Hour()*60+now.Minute() ; 

if ((FajrMinutes - nowMinutes) >= 0) {
    nextSalatMinutes= FajrMinutes - nowMinutes ;
}
else if ((ShurooqMinutes - nowMinutes) >= 0) {
    nextSalatMinutes= ShurooqMinutes- nowMinutes ;
}
else if ((AsrMinutes - nowMinutes) >= 0) {
    nextSalatMinutes= AsrMinutes- nowMinutes ;
}
else if ((MaghribMinutes - nowMinutes) >= 0) {
    nextSalatMinutes= MaghribMinutes- nowMinutes ;
}
else if ((IshaMinutes - nowMinutes) >= 0) {
    nextSalatMinutes= IshaMinutes- nowMinutes ;
}
else if ((IshaMinutes - nowMinutes) < 0) {
    nextSalatMinutes= nowMinutes-FajrMinutes ;
}

if ((nextSalatMinutes / 60) > 0) 
    snprintf(nextSalat,9, "%02d:%02d:%02d", nextSalatMinutes / 60, nextSalatMinutes % 60 +1 ,59-now.Second());  
else   
    snprintf(nextSalat,9, "%02d:%02d", nextSalatMinutes % 60 +1 ,59-now.Second());  

u8g2_for_adafruit_gfx.drawUTF8(160,40,nextSalat);



/////////// print hadith
  u8g2_for_adafruit_gfx.setBackgroundColor(GREEN);   
  if(left_offset_hadith<LCDWidth )
  {
    gfx.fillRect(0, 49,LCDWidth, 50, GREEN);
    u8g2_for_adafruit_gfx.setForegroundColor(WHITE);   
    u8g2_for_adafruit_gfx.drawUTF8(left_offset_hadith,80,s.c_str());
    left_offset_hadith += SCROLL_DELTA;  
  }
  else {
    s = prReshaper123(Hadith); 
    left_offset_hadith= - u8g2_for_adafruit_gfx.getUTF8Width(s.c_str()) ;
  }



/////////////////////////////////////////////print date at bottom
char tmp[11];
int16_t len;
u8g2_for_adafruit_gfx.setFont(ae_Dimnah24);
u8g2_for_adafruit_gfx.setBackgroundColor(BLACK);   
gfx.fillRect(0, 100,LCDWidth, 50, 0);   

 
///////////////////hijri print:

if (salat2show!=6) { // print hijri most of time !
    u8g2_for_adafruit_gfx.setForegroundColor(WHITE);  
    u8g2_for_adafruit_gfx.drawUTF8(ALIGNE_RIGHT(dayNames[now.DayOfWeek()]), 137,dayNames[now.DayOfWeek()]);
         len=u8g2_for_adafruit_gfx.getUTF8Width(dayNames[now.DayOfWeek()]) +10 ;
    
    u8g2_for_adafruit_gfx.setForegroundColor(RED);    
    snprintf_P(tmp,countof(tmp),PSTR("%02u"),H_day);    
    u8g2_for_adafruit_gfx.drawUTF8(ALIGNE_RIGHT(tmp)-len, 137,tmp);
         len=len+u8g2_for_adafruit_gfx.getUTF8Width(tmp);   
         
    u8g2_for_adafruit_gfx.setForegroundColor(WHITE);  
    u8g2_for_adafruit_gfx.drawUTF8(ALIGNE_RIGHT(HijriMonthNames[H_month]) -len, 137,HijriMonthNames[H_month]);
         len=len+u8g2_for_adafruit_gfx.getUTF8Width(HijriMonthNames[H_month])+10 ;

    u8g2_for_adafruit_gfx.setForegroundColor(RED);    
    snprintf_P(tmp,countof(tmp),PSTR("%04u"),H_year);    
    u8g2_for_adafruit_gfx.drawUTF8(ALIGNE_RIGHT(tmp)-len, 137,tmp);
         len=len+u8g2_for_adafruit_gfx.getUTF8Width(tmp);  
}
else {
/////////// geo print                 
    u8g2_for_adafruit_gfx.setForegroundColor(WHITE);  
    u8g2_for_adafruit_gfx.drawUTF8(ALIGNE_RIGHT(dayNames[now.DayOfWeek()]), 137,dayNames[now.DayOfWeek()]);
         len=u8g2_for_adafruit_gfx.getUTF8Width(dayNames[now.DayOfWeek()])+10 ;
    
    u8g2_for_adafruit_gfx.setForegroundColor(RED);    
    snprintf_P(tmp,countof(tmp),PSTR("%02u"),now.Day());
    u8g2_for_adafruit_gfx.drawUTF8(ALIGNE_RIGHT(tmp)-len , 137,tmp);
         len=len+u8g2_for_adafruit_gfx.getUTF8Width(tmp);   
         
    u8g2_for_adafruit_gfx.setForegroundColor(WHITE);  
     u8g2_for_adafruit_gfx.drawUTF8(ALIGNE_RIGHT(GeoMonthNames[now.Month()])-len, 137,GeoMonthNames[now.Month()]);
        len=len+u8g2_for_adafruit_gfx.getUTF8Width(GeoMonthNames[now.Month()]);     

    u8g2_for_adafruit_gfx.setForegroundColor(RED);    
    snprintf_P(tmp,countof(tmp),PSTR("%02u"),now.Year());
    u8g2_for_adafruit_gfx.drawUTF8(ALIGNE_RIGHT(tmp)-len , 137,tmp);
}

//////////////////////   print salat times
switch (salat2show){
   case 1:
    Printscrollup(Fajr_name,Fajr);
     break;
   case 2:
     Printscrollup(Shurooq_name,Shurooq);  
     break;    
   case 3:
     Printscrollup(Dhuhr_name,Dhuhr);  
     break;    
   case 4:
     Printscrollup(Asr_name,Asr);  
     break;    
   case 5:
     Printscrollup(Maghrib_name,Maghrib);  
     break;    
   case 6:
     Printscrollup(Isha_name,Isha);  
   break;    
   //default:
     // default statements
}



vga.show();                // make everything visible
//delay(10);
AsyncElegantOTA.loop();
}
