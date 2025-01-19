#include <Arduino.h>
#include <Wire.h>
#include <BluetoothSerial.h>

const long CI_V_BAUD = 9600;    // CI-V baud rate for IC-705
const int CI_V_BUFFER_SIZE = 11;  // Size of CI-V message buffer
const char* APP_NAME = "XPA125B BT";
const float dacConversion = 254/3;
BluetoothSerial SerialBT;

unsigned long frequency = 0;
float voltage = 0.0;
int analogValue = 0;

struct BandVoltage {
  unsigned long lowerBound;
  unsigned long higherBound;
  float voltage;
};

BandVoltage mappings[] = {
  { 800000, 2999999, 0.23  },     // 160m band, 1.8 MHz to 2 MHz
  { 3000000, 4499999, 0.46 },    // 80m band, 3.5 MHz to 4 MHz
  { 4500000, 5999999, 0.69 },    // 60m band, 14 MHz to 14.35 MHz
  { 6000000, 8999999, 0.92 },    // 40m band, 7 MHz to 7.3 MHz
  { 9000000, 12999999, 1.15 },   // 30m band, 10.1 to 10.15 Mhz
  { 13000000, 16999999, 1.38 },  // 20m band, 14 MHz to 14.35 MHz
  { 17000000, 19999999, 1.61 },  // 17m band, 18.068 MHz to 18.168 MHz
  { 20000000, 22999999, 1.84 },  // 15m band, 21 MHz to 21.45 MHz
  { 23000000, 26999999, 2.07 },  // 12m band, 24.89 to 24.99 MHz
  { 27000000, 39999999, 2.30 },  // 10m band, 28 MHz to 29.7 MHz
  { 40000000, 60000000, 2.53 }   // 6m band, 51 Mhz to 54.0 Mhz
};

// Function to convert BCD to decimal
unsigned long bcdToDecimal(byte *bcdBytes, int length) {
  unsigned long result = 0;
  for (int i = 0; i < length; i++) {
    result = result * 100 + ((bcdBytes[i] >> 4) * 10) + (bcdBytes[i] & 0x0F);
  }
  return result;
}

float getVoltage(unsigned long frequency) {
  for (int i = 0; i < sizeof(mappings) / sizeof(BandVoltage); i++) {
    if (frequency >= mappings[i].lowerBound && frequency <= mappings[i].higherBound) {
      return mappings[i].voltage;
    }
  }
  return 0.0;
}

void setup() {
  Serial.begin(CI_V_BAUD);
  SerialBT.begin(APP_NAME);
}

void loop() {
  byte buffer[CI_V_BUFFER_SIZE];
  int available = SerialBT.available();

  if (available >= CI_V_BUFFER_SIZE) {
    SerialBT.readBytesUntil(0xFD, buffer, available);

    if (buffer[3] == 0xA4) {  // 0xA4 indicates a frequency message
      byte freqBytes[] = { buffer[8], buffer[7], buffer[6], buffer[5] };
      frequency = bcdToDecimal(freqBytes, 4);
      analogValue = (int) (getVoltage(frequency) * dacConversion);
      Serial.println(frequency);
      Serial.println(analogValue);
      dacWrite(25, analogValue * dacConversion);
    }else {
      Serial.println("Skipping non-frequency message type:");
      Serial.write(buffer, available);
    }
  }

}