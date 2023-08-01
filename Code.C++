#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>

#include <Adafruit_BMP085.h>
// #include <BH1750.h>
#include <Wire.h>

#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

#include <OneWire.h>
#include <DallasTemperature.h>


#define ONE_WIRE_BUS 6
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
#define sensorPin A0
#define DHTPIN 5
#define DHTTYPE DHT11  // DHT 22 (AM2302)

DHT_Unified dht(DHTPIN, DHTTYPE);

// BH1750 lightMeter;

Adafruit_BMP085 bmp;


// This EUI must be in little-endian format, so least-significant-byte
// first. When copying an EUI from ttnctl output, this means to reverse
// the bytes. For TTN issued EUIs the last bytes should be 0xD5, 0xB3,
// 0x70.
static const u1_t PROGMEM APPEUI[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
void os_getArtEui(u1_t* buf) {
  memcpy_P(buf, APPEUI, 8);
}

// This should also be in little endian format, see above.
static const u1_t PROGMEM DEVEUI[8] = { 0x8B, 0x00, 0x06, 0xD0, 0x7E, 0xD5, 0xB3, 0x70 };
void os_getDevEui(u1_t* buf) {
  memcpy_P(buf, DEVEUI, 8);
}

// This key should be in big endian format (or, since it is not really a
// number but a block of memory, endianness does not really apply). In
// practice, a key taken from ttnctl can be copied as-is.
static const u1_t PROGMEM APPKEY[16] = { 0xF4, 0x3D, 0x95, 0x7F, 0xC3, 0xA5, 0x92, 0x48, 0x98, 0xA5, 0x26, 0x84, 0x07, 0x80, 0xEC, 0xBA };
void os_getDevKey(u1_t* buf) {
  memcpy_P(buf, APPKEY, 16);
}

static osjob_t sendjob;

// Schedule TX every this many seconds (might become longer due to duty
// cycle limitations).
const unsigned TX_INTERVAL = 1;

// Pin mapping
const lmic_pinmap lmic_pins = {
  .nss = 10,
  .rxtx = LMIC_UNUSED_PIN,
  .rst = 4,
  .dio = { 2, 3, LMIC_UNUSED_PIN },
};

void onEvent(ev_t ev) {
  Serial.print(os_getTime());
  Serial.print(": ");
  switch (ev) {
    case EV_SCAN_TIMEOUT:
      Serial.println(F("EV_SCAN_TIMEOUT"));
      break;
    case EV_BEACON_FOUND:
      Serial.println(F("EV_BEACON_FOUND"));
      break;
    case EV_BEACON_MISSED:
      Serial.println(F("EV_BEACON_MISSED"));
      break;
    case EV_BEACON_TRACKED:
      Serial.println(F("EV_BEACON_TRACKED"));
      break;
    case EV_JOINING:
      Serial.println(F("EV_JOINING"));
      break;
    case EV_JOINED:
      Serial.println(F("EV_JOINED"));

      // Disable link check validation (automatically enabled
      // during join, but not supported by TTN at this time).
      LMIC_setLinkCheckMode(0);
      break;
    case EV_RFU1:
      Serial.println(F("EV_RFU1"));
      break;
    case EV_JOIN_FAILED:
      Serial.println(F("EV_JOIN_FAILED"));
      break;
    case EV_REJOIN_FAILED:
      Serial.println(F("EV_REJOIN_FAILED"));
      break;
      break;
    case EV_TXCOMPLETE:
      Serial.println(F("EV_TXCOMPLETE (includes waiting for RX windows)"));
      if (LMIC.txrxFlags & TXRX_ACK)
        Serial.println(F("Received ack"));
      if (LMIC.dataLen) {
        Serial.println(F("Received "));
        Serial.println(LMIC.dataLen);
        Serial.println(F(" bytes of payload"));
      }
      // Schedule next transmission
      os_setTimedCallback(&sendjob, os_getTime() + sec2osticks(TX_INTERVAL), do_send);
      break;
    case EV_LOST_TSYNC:
      Serial.println(F("EV_LOST_TSYNC"));
      break;
    case EV_RESET:
      Serial.println(F("EV_RESET"));
      break;
    case EV_RXCOMPLETE:
      // data received in ping slot
      Serial.println(F("EV_RXCOMPLETE"));
      break;
    case EV_LINK_DEAD:
      Serial.println(F("EV_LINK_DEAD"));
      break;
    case EV_LINK_ALIVE:
      Serial.println(F("EV_LINK_ALIVE"));
      break;
    default:
      Serial.println(F("Unknown event"));
      break;
  }
}

void do_send(osjob_t* j) {
  // Check if there is not a current TX/RX job running
  if (LMIC.opmode & OP_TXRXPEND) {
    Serial.println(F("OP_TXRXPEND, not sending"));
  } else {
    // Prepare upstream data transmission at the next possible time.
    Serial.println("Sending Data");

    sensors_event_t event;
    dht.humidity().getEvent(&event);
    int humidity = (event.relative_humidity) * 100;

    sensors.requestTemperatures();
    int external_temperature = int((double(sensors.getTempCByIndex(0))) * 100);

    // int lux = lightMeter.readLightLevel();

    int internal_temperature = bmp.readTemperature() * 100;
    uint32_t pressure = bmp.readPressure() * 100;
    //Voltage
    uint16_t voltage = getAccurateVcc();
    int SensorValue = analogRead(sensorPin);
    int outputValue = map(SensorValue, 0, 1023, 255, 0);  // map the 10-bit data to 8-bit data



    LMIC.frame[0] = external_temperature >> 8;
    LMIC.frame[1] = external_temperature;
    LMIC.frame[2] = humidity >> 8;
    LMIC.frame[3] = humidity;
    LMIC.frame[4] = outputValue >> 8;
    LMIC.frame[5] = outputValue;
    LMIC.frame[6] = internal_temperature >> 8;
    LMIC.frame[7] = internal_temperature;
    LMIC.frame[8] = pressure >> 24;
    LMIC.frame[9] = pressure >> 16;
    LMIC.frame[10] = pressure >> 8;
    LMIC.frame[11] = pressure;
    LMIC.frame[12] = voltage >> 8;
    LMIC.frame[13] = voltage;

    LMIC_setTxData2(1, LMIC.frame, 14, 0);
    Serial.println(F("Packet queued"));
    Serial.println(external_temperature);
  }
  // Next TX is scheduled after TX_COMPLETE event.
}
void setup() {

  Serial.begin(115200);
  Serial.println(1);
  pinMode(sensorPin, INPUT);
  dht.begin();
  Serial.println(1);
  Wire.begin();
  Serial.println(1);
  sensors.begin();
  Serial.println(1);
  // lightMeter.begin();
  Serial.println(1);
  bmp.begin();
  Serial.println(1);
  Serial.println(F("Starting"));

  // LMIC init
  os_init();
  // Reset the MAC state. Session and pending data transfers will be discarded.
  LMIC_reset();

  // Use with Arduino Pro Mini ATmega328P 3.3V 8 MHz
  // Let LMIC compensate for +/- 1% clock error
  LMIC_setClockError(MAX_CLOCK_ERROR * 1 / 100);

  // Start job (sending automatically starts OTAA too)
  do_send(&sendjob);
}

void loop() {
  os_runloop_once();
}

//Voltage
int getAccurateVcc() {
  getVcc();
  return getVcc();
}

int getVcc(void) {

  int Vcc;

  ADCSRA = (1 << ADEN);
  ADCSRA |= (1 << ADPS0) | (1 << ADPS1) | (1 << ADPS2);
  ADMUX = (1 << REFS0) | (1 << MUX3) | (1 << MUX2) | (1 << MUX1);
  delay(1);  // wait for ADC and VREF to settle

  ADCSRA |= (1 << ADSC);  // start conversion
  while (bit_is_set(ADCSRA, ADSC))
    ;  // wait until done
  Vcc = ADC;

  //1126400 = 1.1*1024*1000 = 1126400UL
  Vcc = 1126400UL / (unsigned long)Vcc;
  return Vcc;  // Vcc in millivolts
}
