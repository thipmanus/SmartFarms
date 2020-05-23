#include <FirebaseArduino.h>
#include <ESP8266WiFi.h>
#include <DHT.h>
#include <time.h>
#define LED_PIN 4

// Config DHT
#define DHTPIN 5
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// Config Firebase
#define FIREBASE_HOST "smartfarm-148.firebaseio.com"
#define FIREBASE_AUTH "***"

// Config connect WiFi
#define WIFI_SSID "eiei"
#define WIFI_PASSWORD "12345678"

// Config dateTime
int timezone = 7 * 3600;
int dst = 0;

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);
  
  // connect to wifi.
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("connecting");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.print("connected: ");
  Serial.println(WiFi.localIP());
  
  //setTime
  configTime(timezone, dst, "pool.ntp.org","time.nist.gov");
  Serial.println("\nWaiting for Internet time");

  while(!time(nullptr)){
     Serial.print("*");
     delay(1000);
  }
  Serial.println("\nTime response....OK"); 
  
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.setString("/Node1/switch/LED", "off");
  //Firebase.stream("/Node1/switch/LED");
  dht.begin();  
}

int i = 0;
String lastSW = "";
int hours = 0;
int minutes = 0;
int HH = 0;
int MM = 0;
int oldHH = 0;
int oldMM = 0;
int nD = 0;
int nM = 0;
int nY = 0;

void loop() {
  //showTime
  time_t now = time(nullptr);
  struct tm* p_tm = localtime(&now);
  if(i == 0){
    
    Serial.print(p_tm->tm_mday);
    Serial.print("/");
    Serial.print(p_tm->tm_mon + 1);
    Serial.print("/");
    Serial.print(p_tm->tm_year + 1900);
    
    Serial.print(" ");
    
    Serial.print(p_tm->tm_hour);
    Serial.print(":");
    Serial.print(p_tm->tm_min);
    Serial.print(":");
    Serial.println(p_tm->tm_sec);
    while ((p_tm->tm_year + 1900) == 1970) {
      time_t now = time(nullptr);
      struct tm* p_tm = localtime(&now);
      Serial.print(p_tm->tm_mday);
      Serial.print("/");
      Serial.print(p_tm->tm_mon + 1);
      Serial.print("/");
      Serial.print(p_tm->tm_year + 1900);
      
      Serial.print(" ");
      
      Serial.print(p_tm->tm_hour);
      Serial.print(":");
      Serial.print(p_tm->tm_min);
      Serial.print(":");
      Serial.println(p_tm->tm_sec);
    }
    oldHH = (p_tm->tm_hour+1);
    oldMM = (p_tm->tm_min+1);
  }
  HH = (p_tm->tm_hour);
  MM = (p_tm->tm_min);
  nD = (p_tm->tm_mday);
  nM = (p_tm->tm_mon + 1);
  nY = (p_tm->tm_year + 1900);
  
  // Read temp & Humidity for DHT22
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    delay(500);
    return;
  }
  
  if((i==0)||(oldMM == MM)){
    Firebase.setFloat("lastDHT/LHLF/humidity", h);
    if (Firebase.failed()) {
        Serial.print("set /humidity failed:");
        Serial.println(Firebase.error());  
        return;
    }
    //Serial.print("set /humidity to ");
    //Serial.println(Firebase.getFloat("lastDHT/LHLF/humidity"));
    
    Firebase.setFloat("lastDHT/LHLF/temperature", t);
    if (Firebase.failed()) {
        Serial.print("set /temperature failed:");
        Serial.println(Firebase.error());  
        return;
    }
    //Serial.print("set /temperature to ");
    //Serial.println(Firebase.getFloat("lastDHT/LHLF/temperature"));
    String tmp = String(HH)+":"+String(MM)+" น.";
    Firebase.setString("lastDHT/LHLF/timestamp", tmp);
    if (Firebase.failed()) {
        Serial.print("set /timestamp failed:");
        Serial.println(Firebase.error());  
        return;
    }
    oldMM = (p_tm->tm_min+1);
    //Serial.print("oldMM : ");
    //Serial.println(oldMM); 
  }
  if((i==0)||(oldHH == HH)){
    getDHTData(h,t,HH,MM);
    
    oldHH = (p_tm->tm_hour+1);
    //Serial.print("oldHH : ");
    //Serial.println(oldHH);
  }
  NewDoEvent();
  delay(1000);
  i=1;
  if(lastSW.indexOf("on") > -1){
    if(((p_tm->tm_hour) == hours)&&((p_tm->tm_min) == minutes)){
      Firebase.setString("/Node1/switch/LED", "off");
    }
  }
}
void getDHTData(float h,float t,int HH,int MM){
  String hf = String(h)+"%";
  String tf = String(t)+"°C";
  String tst = "";
  if(HH < 10){
    if(MM < 10){
      tst = "0"+String(HH)+":0"+String(MM)+" น.";
    }
    else{
      tst = "0"+String(HH)+":"+String(MM)+" น.";
    }
  }
  else{
    if(MM < 10){
      tst = String(HH)+":0"+String(MM)+" น.";
    }
    else{
      tst = String(HH)+":"+String(MM)+" น.";
    }
  }
  String nowSetDate = String(nD)+"/"+String(nM)+"/"+String(nY);
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["temperature"] = tf;
  root["humidity"] = hf;
  root["timestamp"] = tst;
  root["date"] = nowSetDate;
  
  // append a new value to /logDHT
  String name = Firebase.push("logDHT", root);
  // handle error
  if (Firebase.failed()) {
      Serial.print("pushing /logDHT failed:");
      Serial.println(Firebase.error());  
      return;
  }
  Serial.print("pushed: /logDHT/");
  Serial.println(name);
}
void NewDoEvent(){
  String Fg = Firebase.getString("/Node1/switch/LED");
  //Serial.println(TFg);
  if(Firebase.failed()){
    Serial.println(Firebase.error());
    return;
  } 
  else {
    if(lastSW != Fg){
      lastSW = Fg;
      Serial.println("SW: " + lastSW);
      //Serial.println("index of ON: " + String(lastSW.indexOf("on")));
      //Serial.println("index of OFF: " + String(lastSW.indexOf("off")));
      if(lastSW.indexOf("off") > -1){
        digitalWrite(LED_PIN, HIGH);
      } 
      else if(lastSW.indexOf("on") > -1){
        digitalWrite(LED_PIN, LOW);
        hours = Firebase.getInt("/Node1/switch/hours");
        minutes = Firebase.getInt("/Node1/switch/minutes");
      }
    }
  }
}
