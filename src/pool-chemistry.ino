// This #include statement was automatically added by the Particle IDE.
#include "DS18B20Minimal.h"
#include "MQTT.h"

// Change this pin to the one your D18B20 is connected too
#define ONE_WIRE_BUS D2

SYSTEM_MODE(SEMI_AUTOMATIC);
STARTUP(System.enableFeature(FEATURE_RETAINED_MEMORY));
STARTUP(disableStatusLED());

OneWire oneWire(ONE_WIRE_BUS);
DS18B20 sensor(&oneWire);

retained int cachedSampleCount = 0;
retained float lastTemperature;

bool messageAckd = false;
int iterations = 0;
bool waitingForAck = false;

int ordPower = D3;
int tdsPower = D4;
int tempPower = D5;
int phPower = D6;
int safeMode = D7;

byte myServer[] = {192, 168, 1, 125};
MQTT client(myServer, 1883, callback);

void disableStatusLED()
{
  RGB.control(true);
  RGB.color(0, 0, 0);
}

void setup(void)
{
  Serial.begin(115200);

  // bind following command to a pin, use a reed switch.  also bind a watchdog timer to it, and a reset counter
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

  Serial.printlnf("sampleCount = %d", cachedSampleCount++);
  sampleTemperature();
  maybePublish();
}

void loop(void)
{
  if ((++iterations == 10) || !waitingForAck || messageAckd)
  {
    // next will have to determine if no ack received; if so, the data needs to be saved for next time

    if (client.isConnected())
    {
      client.disconnect();
    }
    WiFi.off();
    Particle.process();

    System.sleep(A3, RISING, 1);
    System.sleep(SLEEP_MODE_DEEP, 5);
  }
  else
  {
    if (client.isConnected())
    {
      client.loop();
    }
    Particle.process();
    // burn some time until the ack comes back
    delay(1000);
  }
}

void sampleTemperature()
{
  digitalWrite(tempPower, HIGH);
  delay(250); // give device time to boot up and settle
  sensor.begin();
  sensor.setResolution(11);
  sensor.requestTemperatures();

  // delay is from table 2 in https://cdn-shop.adafruit.com/datasheets/DS18B20.pdf - depends upon resolution requested
  Serial.print("sampling temperature");
  int count = 0;
  while (!sensor.isConversionComplete())
  {
    delay(100);
    count++;
    Serial.print(".");
    if (count > 10)
    {
      Serial.println("conversion never completed");
      lastTemperature = 0;
      return;
    }
  }

  Serial.println("ready to read");
  Serial.print("Temp: ");
  Serial.print(count);
  Serial.print(" : ");
  lastTemperature = sensor.getCRCTempC();
  Serial.println(lastTemperature);

  digitalWrite(tempPower, LOW);
}

void callback(char *topic, byte *payload, unsigned int length)
{
}

// QOS ack callback.
// if application use QOS1 or QOS2, MQTT server sendback ack message id.
void qoscallback(unsigned int messageid)
{
  Serial.print("Ack Message Id:");
  Serial.println(messageid);
  messageAckd = true;
}

void maybePublish(void)
{
  if (cachedSampleCount % 2 == 0)
  {
    Serial.println("publishing data");
    WiFi.on();
    WiFi.connect();

    while (!WiFi.ready())
    {
      delay(250);
    }

    Serial.println("network is ready");

    //Particle.process();
    // connect to the server
    client.connect("sparkclient");
    Serial.println("connected to mq");
    // add qos callback. If don't add qoscallback, ACK message from MQTT server is ignored.
    client.addQosCallback(qoscallback);

    // publish/subscribe
    if (client.isConnected())
    {
      Serial.println("publishing to mq");
      // get the messageid from parameter at 4.
      uint16_t messageid;
      // QOS=2
      client.publish("outTopic/temperature", String(lastTemperature), MQTT::QOS2, &messageid);
      Serial.println(messageid);
      Serial.println("published!");
      waitingForAck = true;
    }

    Particle.process();
    client.loop();
    Serial.println("done with particle.process");
  }
}
