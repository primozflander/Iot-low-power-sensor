/*
Project: Domoticz
Created: Primoz Flander 28.4.2019

v10: adc fix

v9: fixed Nan and zero hum and temp sending

v8: changed Areference to default, added air quality sensor, added luminosity sensor

v7: optimized power saving

v6: added IO pin for powering down sensors during sleep

v5: new board, battery powered

v4: added resistive moisture sensor

v3: added force refresh interval and sensor PA_LEVEL MAX

v2: added another two sensors

v1: demo

Board: Arduino Pro Mini 3.3 V / 8 MHz+ NRF24 Shield
Arduino pin   /      I/O:
DI2           ->     IRQ
DI3           ->     DHT
DI9           ->     CE
DI10          ->     CS
DI11          ->     MOSI
DI12          ->     MISO
DI13          ->     SCK
A0            ->     Battery level
A1            ->     Moisture sensor 1
A2            ->     Moisture sensor 2
A3            ->     Moisture sensor 3
A4            ->     Luminosity sensor BH1750 SDA
A5            ->     Luminosity sensor BH1750 SCL
A6            ->     Air quality sensor
*/

/*=======================================================================================
                                    Includes
========================================================================================*/
//#define MY_DEBUG
#define MY_RADIO_RF24
#define MY_RF24_PA_LEVEL RF24_PA_MAX
#define MY_RF24_DATARATE (RF24_250KBPS)
#include <MySensors.h>  
#include <SPI.h>
#include <DHT.h>
#include <Wire.h>
#include <math.h>
#include <BH1750.h>

/*=======================================================================================
                                    Definitions
========================================================================================*/

#define VCCSnz                          7
#define DHT_DATA_PIN                    3
#define SENSOR_TEMP_OFFSET              0
#define SENSOR_HUM_OFFSET               0
#define CHILD_ID_HUM0                   0
#define CHILD_ID_HUM1                   1
#define CHILD_ID_HUM2                   2
#define CHILD_ID_HUMS                   3
#define CHILD_ID_TEMP                   4
#define CHILD_ID_VOLTAGE                5
#define CHILD_ID_MQ                     6
#define CHILD_ID_LIGHT                  7
#define BATTERY_SENSE_PIN               A0
#define MOISTURE_SENSOR0_ANALOG_PIN     A1
#define MOISTURE_SENSOR1_ANALOG_PIN     A2
#define MOISTURE_SENSOR2_ANALOG_PIN     A3
#define AIRQUALITY_SENSOR_ANALOG_PIN    A6
uint32_t SLEEP_TIME = 300000UL;
//uint32_t SLEEP_TIME = 20000UL;

/*=======================================================================================
                                User Configurarations
========================================================================================*/

MyMessage msg0(CHILD_ID_HUM0, V_HUM);
MyMessage msg1(CHILD_ID_HUM1, V_HUM);
MyMessage msg2(CHILD_ID_HUM2, V_HUM);
MyMessage msg3(CHILD_ID_HUMS, V_HUM);
MyMessage msg4(CHILD_ID_TEMP, V_TEMP);
MyMessage msg5(CHILD_ID_VOLTAGE, V_VOLTAGE);
//MyMessage msg6(CHILD_ID_MQ, V_LEVEL);
MyMessage msg7(CHILD_ID_LIGHT, V_LIGHT_LEVEL);
DHT dht;
BH1750 lightMeter;

uint16_t lux;
uint16_t humidity0 = 0;
uint16_t humidity1 = 0;
uint16_t humidity2 = 0;
int lastHumLevel0 = 1;
int lastHumLevel1 = 1;
int lastHumLevel2 = 1;
int lastLux = 1;
int airQuality = 0;
int lastAirQuality = 1;
float humidity;
float temperature;
float lastTemp = 1;
float lastHum = 1;
float batteryLvl = 0;
float lastBatteryLvl = 1;
int forceRefreshSetCount = 6;
int forceRefreshCurrCount = 0;
bool powerDownFlag = false;

void presentation()  
{ 
  // Send the sketch version information to the gateway
  sendSketchInfo("BaterryMoistureSensor", "0.9");
  // Register all sensors to gw (they will be created as child devices)
  present(CHILD_ID_HUM0, S_HUM);
  present(CHILD_ID_HUM1, S_HUM);
  present(CHILD_ID_HUM2, S_HUM);
  present(CHILD_ID_HUMS, S_HUM);
  present(CHILD_ID_TEMP, S_TEMP);
  present(CHILD_ID_VOLTAGE, S_MULTIMETER);
  //present(CHILD_ID_MQ, S_AIR_QUALITY);
  present(CHILD_ID_LIGHT, S_LIGHT_LEVEL);
}

/*=======================================================================================
                                   Setup function
========================================================================================*/

void setup() {
  //Serial.begin(115200);
  pinMode(VCCSnz, OUTPUT);
  digitalWrite(VCCSnz, HIGH); // power ON
  delay(1000);
  dht.setup(DHT_DATA_PIN);
  sleep(dht.getMinimumSamplingPeriod());
  lightMeter.begin();
  //analogReference(INTERNAL);
}

/*=======================================================================================
                                            Loop
========================================================================================*/

void loop() { 

  if (powerDownFlag == false) { 
    digitalWrite(VCCSnz, HIGH);
    delay(1000);
    analogRead(BATTERY_SENSE_PIN); //dummy
    batteryLvl = analogRead(BATTERY_SENSE_PIN) / 55.4;
    if (batteryLvl != lastBatteryLvl) {
      send(msg5.set(batteryLvl, 2));
      lastBatteryLvl = batteryLvl;
      if ((batteryLvl < 3.3) && (lastBatteryLvl < 3.3)) {
        powerDownFlag = false;
      }
    }
    analogRead(MOISTURE_SENSOR0_ANALOG_PIN); //dummy
    humidity0 = map(analogRead(MOISTURE_SENSOR0_ANALOG_PIN),0,1023,100,0);
    if (humidity0 != lastHumLevel0) {
      send(msg0.set(humidity0));
      lastHumLevel0 = humidity0;
    }
    analogRead(MOISTURE_SENSOR1_ANALOG_PIN); //dummy
    humidity1 = map(analogRead(MOISTURE_SENSOR1_ANALOG_PIN),0,1023,100,0);
    if (humidity1 != lastHumLevel1) {
      send(msg1.set(humidity1));
      lastHumLevel1 = humidity1;
      delay(100);
    }
    analogRead(MOISTURE_SENSOR2_ANALOG_PIN); //dummy
    humidity2 = map(analogRead(MOISTURE_SENSOR2_ANALOG_PIN),0,1023,100,0);
    if (humidity2 != lastHumLevel2) {
      send(msg2.set(humidity2));
      lastHumLevel2 = humidity2;
    }
    lux = lightMeter.readLightLevel();
    if (lux != lastLux) {
      send(msg7.set(lux));
      lastLux = lux;
    }
//    airQuality = analogRead(AIRQUALITY_SENSOR_ANALOG_PIN);
//    if (airQuality != lastAirQuality) {
//      send(msg6.set(airQuality));
//      lastAirQuality = airQuality;
//    }
    dht.readSensor(true); // Force reading sensor, so it works also after sleep()
    temperature = dht.getTemperature();
    if ((temperature != lastTemp) && (isnan(temperature) == 0) && (temperature != 0)) {
      lastTemp = temperature;
      temperature += SENSOR_TEMP_OFFSET;
      send(msg4.set(temperature, 1));
    }
    humidity = dht.getHumidity();
    if ((humidity != lastHum) && (isnan(humidity) == 0) && (humidity != 0)) {
      lastHum = humidity;
      humidity += SENSOR_HUM_OFFSET;
      send(msg3.set(humidity, 1));
    }
    digitalWrite(VCCSnz, LOW);
    forceRefreshCurrCount++; 
    if  (forceRefreshCurrCount == forceRefreshSetCount)  {
      send(msg0.set(humidity0));
      send(msg1.set(humidity1));
      send(msg2.set(humidity2));
      if ((isnan(humidity) == 0) && (humidity != 0))  {
        send(msg3.set(humidity, 1));
      }
      if ((isnan(temperature) == 0) && (temperature != 0))  {
        send(msg4.set(temperature, 1));
      }
      send(msg5.set(batteryLvl, 1));
      //send(msg6.set(airQuality));
      forceRefreshCurrCount = 0;
    }
  }
  //debug();
  sleep(SLEEP_TIME);  // Sleep to save energy
}

/*=======================================================================================
                                         Functions
========================================================================================*/

void debug(void)  {

  Serial.println("-------------------------");
  Serial.print("batteryLvl:");
  Serial.println(batteryLvl);
  Serial.print("humidity0:");
  Serial.println(humidity0);
  Serial.print("humidity1:");
  Serial.println(humidity1);
  Serial.print("humidity2:");
  Serial.println(humidity2);
  Serial.print("humidity:");
  Serial.println(humidity);
  Serial.print("temperature:");
  Serial.println(temperature);
  Serial.print("airQuality:");
  Serial.println(airQuality);
  Serial.print("lux:");
  Serial.println(lux);
  delay(3000);
}
