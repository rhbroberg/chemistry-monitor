// This #include statement was automatically added by the Particle IDE.
#include "DS18B20Minimal.h"

// Change this pin to the one your D18B20 is connected too
#define ONE_WIRE_BUS D2

//SYSTEM_MODE(SEMI_AUTOMATIC);
STARTUP(System.enableFeature(FEATURE_RETAINED_MEMORY));
retained int cachedSampleCount = 0;
retained float lastTemperature;

OneWire oneWire(ONE_WIRE_BUS);
DS18B20 sensor(&oneWire);

int ordPower = D3;
int tdsPower = D4;
int tempPower = D5;
int phPower = D6;
int safeMode = D7;

Timer timer(10000, sampleTemperature);

void disableStatusLED()
{
  RGB.control(true); 
  RGB.color(0, 0, 0);
}

STARTUP(disableStatusLED());

void setup(void)
{
  Serial.begin(115200);
  
  //System.buttonMirror(D7, RISING);
  // bind following command to a pin, use a reed switch.  also bind a watchdog timer to it, and a reset counter
  //System.enterSafeMode();
  attachInterrupt(safeMode, System.enterSafeMode, RISING);
  pinMode(safeMode, INPUT);

  Serial.println(__FILE__);
  Serial.print("DS18B20 Library version: ");
  Serial.println(DS18B20_LIB_VERSION);

  pinMode(tempPower, OUTPUT);
  pinMode(ordPower, OUTPUT);
  pinMode(tdsPower, OUTPUT); 
  pinMode(phPower, OUTPUT);
  
  digitalWrite(ordPower, LOW);
  digitalWrite(tdsPower, LOW);
  digitalWrite(phPower, LOW);

// timer.start();
}

void loop(void)
{
    delay(5000);
    sampleTemperature();
    maybePublish();
    Serial.printlnf("sampleCount = %d", cachedSampleCount++);
    delay(1000); // time to write out
//    System.sleep(SLEEP_MODE_DEEP, 5);
}

void sampleTemperature()
{
    digitalWrite(tempPower, HIGH);
    delay(250);  // give device time to boot up and settle
    sensor.begin();
    sensor.setResolution(11);
    sensor.requestTemperatures();

    // delay is from table 2 in https://cdn-shop.adafruit.com/datasheets/DS18B20.pdf - depends upon resolution requested
    Serial.print("sampling temperature");
    int count = 0;
    while (! sensor.isConversionComplete())
    {
        delay(100);
        count++;
        Serial.print(".");
    }

    Serial.println("ready to read");
    Serial.print("Temp: ");
    Serial.print(count);
    Serial.print(" : ");
    lastTemperature = sensor.getCRCTempC();
    Serial.println(lastTemperature);

    digitalWrite(tempPower, LOW);
}

void
maybePublish(void)
{
  if (cachedSampleCount % 2 == 0)
  {
    Serial.println("publishing data");
    WiFi.on();
    Particle.publish("temperature", String(lastTemperature), PUBLIC | WITH_ACK);
    Particle.publishVitals(particle::NOW);
    Particle.process();
    delay(5000);
    WiFi.off();
  }
}
