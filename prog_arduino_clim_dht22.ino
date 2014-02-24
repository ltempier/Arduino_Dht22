#include <EEPROM.h>
#include "DHT22.h"

#define DHT22_PIN 3
#define TIME_BEFORE_MESURE 10
#define MIN_INTERVAL_MEASURES 2000
#define MAX_INTERVAL_MEASURES 86400000 //1 day

DHT22 myDHT22(DHT22_PIN);

unsigned long lastTimeMesure = millis(); //reset all 49 days
unsigned long intervalMeasures = MIN_INTERVAL_MEASURES;
float lastTemperature, lastHumidity;
float calibrationValueTemperature = 0, calibrationValueHumidity = 0;

void setup(void)
{
  Serial.begin(9600);
  Serial.flush();
  Serial.println("Power turn on");
  Serial.println("-------------------------------------------------------");
  Serial.println("Serial commands infos: ");
  Serial.println("get value: return actual value");
  Serial.println("get last value: return last stocked value");
  Serial.println("get delay: return delay time between two measures");
  Serial.println("set delay xxxx: set delay between two measures (ms)");
  Serial.println("get actual time: return the number of milliseconds since the arduino began running");
  Serial.println("get last time mesure: return last number of milliseconds");
  Serial.println("calibrate temperature xxx.xx:");
  Serial.println("calibrate humidity xxx.xx: ");
  Serial.println("calibratation reset: ");
  Serial.println("get last time mesure: return last number of milliseconds");
  Serial.println("trame form: Txx.xxCHyy.yy%");
  Serial.println("-------------------------------------------------------");
  delay(MIN_INTERVAL_MEASURES);
  readEEPROM();
}

void loop(void)
{
  if(Serial.available())
    trtSerialRead(serialRead());
    
  if((millis()-lastTimeMesure) > (intervalMeasures-TIME_BEFORE_MESURE)){
    if(intervalMeasures-(millis()-lastTimeMesure) < TIME_BEFORE_MESURE)
      delay(intervalMeasures-(millis()-lastTimeMesure));
    if(getSensorData())
      dataTransmission();
  }
}

String serialRead(){
  String receiptString = "";
  while(Serial.available() > 0){
    receiptString.concat(char(Serial.read()));
    delay(5);
  }
  receiptString.concat(char('\0'));
  return receiptString;
}
void trtSerialRead(String receiptString){
  if(receiptString.startsWith("get value",0) || receiptString.startsWith("get data",0)){
    getSensorData();
  }
  else if(receiptString.startsWith("get last value",0) || receiptString.startsWith("get last data",0)){
    dataTransmission();
  }
  else if(receiptString.startsWith("get actual time",0)){
    Serial.print("actual time (ms): ");
    Serial.println(millis());
  }
  else if(receiptString.startsWith("get last time mesure",0)){
    Serial.print("last time measure (ms): ");
    Serial.println(lastTimeMesure);
  }
  else if(receiptString.startsWith("get delay",0)){
    Serial.print("interval measures (ms): ");
    Serial.println(intervalMeasures);
  }
  else if(receiptString.startsWith("set delay",0)){
    intervalMeasures = stringToUnsignedLong(receiptString);
    checkVariable();
    Serial.print("new interval measures (ms): ");
    Serial.println(intervalMeasures);
  }
  else if(receiptString.startsWith("calibrate temperature",0)){
    getSensorData();
    calibrationValueTemperature = lastTemperature - stringToFloat(receiptString);
    Serial.print("new linear calibration temperature: T=sensorValue-(");
    Serial.print(calibrationValueTemperature);
    Serial.println(")");
    writeEEPROM(receiptString);
  }
  else if(receiptString.startsWith("calibrate humidity",0)){
    getSensorData();
    calibrationValueHumidity = lastHumidity - stringToFloat(receiptString);
    Serial.print("new linear calibration humidity: H=sensorValue-(");
    Serial.print(calibrationValueHumidity);
    Serial.println(")");
    writeEEPROM(receiptString);
  }
  else if(receiptString.startsWith("calibration reset",0)){
    calibrationValueTemperature=0;
    calibrationValueHumidity = 0;
    clearEEPROM();
    Serial.println("reset calibratation");
  }
}

void checkVariable(){
  if(intervalMeasures < MIN_INTERVAL_MEASURES)
    intervalMeasures = MIN_INTERVAL_MEASURES;
  if(intervalMeasures > MAX_INTERVAL_MEASURES)
    intervalMeasures = MAX_INTERVAL_MEASURES;
}

boolean getSensorData(){
  if(lastTimeMesure-millis() < MIN_INTERVAL_MEASURES)
    delay(MIN_INTERVAL_MEASURES-(lastTimeMesure-millis()));
  DHT22_ERROR_t errorCode = myDHT22.readData();
  lastTimeMesure=millis();
    switch(errorCode)
  {
    case DHT_ERROR_NONE:
      lastTemperature = myDHT22.getTemperatureC();
      lastHumidity = myDHT22.getHumidity();
      return true;
      break;
    case DHT_ERROR_CHECKSUM:
      Serial.print("Warning: check sum error");
      lastTemperature = myDHT22.getTemperatureC();
      lastHumidity = myDHT22.getHumidity();
      break;
    case DHT_BUS_HUNG:
      Serial.println("Error: BUS Hung");
      break;
    case DHT_ERROR_NOT_PRESENT:
      Serial.println("Error: Sensor Not Connect");
      break;
    case DHT_ERROR_ACK_TOO_LONG:
      Serial.println("Error: ACK time out");
      break;
    case DHT_ERROR_SYNC_TIMEOUT:
      Serial.println("Error: Sync Timeout");
      break;
    case DHT_ERROR_DATA_TIMEOUT:
      Serial.println("Error: Data Timeout");
      break;
    case DHT_ERROR_TOOQUICK:
      //Serial.println("Error: Polled to quick");
      break;
  }
  return false;
}

void dataTransmission(){
  Serial.print('T');
  Serial.print(lastTemperature-calibrationValueTemperature);
  Serial.print('C');
  Serial.print('H');
  Serial.print(lastHumidity-calibrationValueHumidity);
  Serial.println('%');
}

unsigned long stringToUnsignedLong(String myString){
  unsigned long result = 0;
  for(int i=0 ; i<myString.length() ; i++){
    if(int(myString.charAt(i)) > 47 && int(myString.charAt(i)) < 58){
      result = result*10;
      result += (int(myString.charAt(i))-48);
    }
    else if(int(myString.charAt(i)) == 46 || myString.charAt(i) == char('\0'))
      break;
  }
  return result;
}

float stringToFloat(String myString){
  float result = 0.0;
  boolean havefractions = false;
  int fractions = 0;  
  for(int i=0 ; i<myString.length() ; i++){
    if(int(myString.charAt(i)) > 47 && int(myString.charAt(i)) < 58){
      result = result*10;
      result += (int(myString.charAt(i))-48);
      if(havefractions)
        fractions++;
    }
    else if(int(myString.charAt(i)) == 46){
      havefractions=true;
    }
    else if(myString.charAt(i) == char('\0'))
      break;
  }
  return result/pow(10, fractions);
}

void writeEEPROM(String saveString){
  int lenght = saveString.length();
  if(lenght>100)
    return;
  int adr = 200;
  if(saveString.startsWith("calibrate temperature",0))
    adr = 0;
  else if(saveString.startsWith("calibrate humidity",0))
    adr = 100;
  for(int i=0; i<lenght; i++){
    EEPROM.write(adr, int(saveString.charAt(i)));
    adr++;
  }
}

void readEEPROM(){
  for(int j=0; j<512; j+=100){
    String savedString = "";
    for(int adr=j; adr<j+100; adr++){
      if(EEPROM.read(adr)==255)
        break;
      char value = char(EEPROM.read(adr));
      savedString.concat(value);
      if(value == char('\0')){
        trtSerialRead(savedString);
        break;
      }
    }
  }
}

void clearEEPROM(){
  for (int i = 0; i < 512; i++)
    EEPROM.write(i, 255);
}


