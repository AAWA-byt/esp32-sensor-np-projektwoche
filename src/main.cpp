#ifdef CORE_DEBUG_LEVEL
#undef CORE_DEBUG_LEVEL
#endif

#define CORE_DEBUG_LEVEL 3
#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG

#include <Arduino.h>
#include <SPI.h>
#include <U8g2lib.h>
#include <Wire.h>
#include "HT_SSD1306Wire.h"
#include "ssd1366.h"
#include "font8x8_basic.h"
#include "esp32-hal-log.h"
#include <ArduinoLog.h>
#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define READINGS_NUMBER 10
#define DELAY_TIME 10

#define TAG "main"

#define uS_TO_S_FACTOR 1000000 /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP 30

// Define floats for the diffrent temperatures
float tmp1;
float tmp2;

uint32_t deviceID{0};
uint32_t chipId{0};

// Create object for oled display
SSD1306Wire display(OLED_I2C_ADDRESS, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED);

int red_led = 19;
int taster = 20;
int tasterstatus = 0;
int active = false;

// tmp sensors 1
#define ONE_WIRE_BUS 2
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// tmp sensors 2
#define TWO_WIRE_BUS 3
OneWire oneWire_2(TWO_WIRE_BUS);
DallasTemperature sensors_2(&oneWire_2);

void print_wakeup_reason()
{
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch (wakeup_reason)
  {
  case ESP_SLEEP_WAKEUP_EXT0:
    Serial.println("Wakeup caused by external signal using RTC_IO");
    break;
  case ESP_SLEEP_WAKEUP_EXT1:
    Serial.println("Wakeup caused by external signal using RTC_CNTL");
    break;
  case ESP_SLEEP_WAKEUP_TIMER:
    Serial.println("Wakeup caused by timer");
    break;
  case ESP_SLEEP_WAKEUP_TOUCHPAD:
    Serial.println("Wakeup caused by touchpad");
    break;
  case ESP_SLEEP_WAKEUP_ULP:
    Serial.println("Wakeup caused by ULP program");
    break;
  default:
    Serial.printf("Wakeup was not caused by deep sleep: %d\n", wakeup_reason);
    break;
  }
}

// Create display
void displayTask(void *)
{
  for (;;)
  {
    String title = String("Temperature Sensor");
    String masterID = String("masterID: ") + chipId;
    String TMP_1 = String("TMP_1: ") + tmp1 + String("°C"); /* String for tmp 1*/
    String TMP_2 = String("TMP_2: ") + tmp2 + String("°C"); /* String for tmp 2*/

    display.clear();
    display.drawString(0, 0, title.c_str());
    display.drawString(0, 10, masterID.c_str());
    display.drawString(0, 20, TMP_1.c_str()); /* Draw tmp 1 to display */
    display.drawString(0, 30, TMP_2.c_str()); /* Draw tmp 2 to display */
    display.display();
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

void createDisplayTask()
{
  int32_t res = xTaskCreate(
      displayTask,
      "displayTask",
      4096,
      (void *)1,
      tskIDLE_PRIORITY,
      NULL);
  if (res != pdPASS)
  {
    Log.errorln(F("displayTask creation gave error: %d"), res);
  }
}

void setup()
{
  Serial.begin(115200);

  Log.begin(LOG_LEVEL_TRACE, &Serial);

  print_wakeup_reason();

  display.init();

  for (int32_t i = 0; i < 17; i = i + 8)
  {
    chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
  }

  createDisplayTask();

  Serial.begin(9600);
  Serial.println("Dallas Temperature IC Control Library Demo");
  // Start up the library
  sensors.begin();
  sensors_2.begin();

  pinMode(red_led, OUTPUT);
  pinMode(taster, INPUT);
}

void loop()
{

  tasterstatus = digitalRead(taster);
  if (tasterstatus == HIGH && active == false) {
    active = true;

  } else if (tasterstatus == HIGH && active == true) {
    active = false;
    digitalWrite(red_led, LOW);
    tmp1 = 0;
    tmp2 = 0;
  }

  if (active == true)
  {
    digitalWrite(red_led, HIGH);
    Serial.print("Requesting temperatures...");
    sensors.requestTemperatures(); // Send the command to get temperatures
    sensors_2.requestTemperatures();
    Serial.println("DONE");
    // After we got the temperatures, we can print them here.
    // We use the function ByIndex, and as an example get the temperature from the first sensor only.
    tmp1 = sensors.getTempCByIndex(0);
    tmp2 = sensors_2.getTempCByIndex(0);
    // Check if reading was successful
    if (tmp1 != DEVICE_DISCONNECTED_C)
    {
      Serial.print("Temperature for the device 1 (index 0) is: ");
      Serial.println(tmp1);
    }
    else
    {
      Serial.println("Error: Could not read temperature data");
    }

    // Check if reading was successful
    if (tmp2 != DEVICE_DISCONNECTED_C)
    {
      Serial.print("Temperature for the device 2 (index 0) is: ");
      Serial.println(tmp2);
    }
    else
    {
      Serial.println("Error: Could not read temperature data");
    }
    
  }
  delay(1000);
}