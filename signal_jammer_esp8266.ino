#include <ESP8266WiFi.h>
#include <SPI.h>
#include <RF24.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

#define CE1_PIN 1
#define CSN1_PIN 3
#define CE2_PIN 0
#define CSN2_PIN 2

RF24 r1(CE1_PIN, CSN1_PIN);
RF24 r2(CE2_PIN, CSN2_PIN);
Adafruit_ST7789 tft = Adafruit_ST7789(15, 5, 4);

// Strictly BLE Advertising Channels
const uint8_t ble_channels[] = {2, 26, 80};
const uint8_t junk[32] = {0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55};

void setup() {
  WiFi.mode(WIFI_OFF);
  WiFi.forceSleepBegin();

  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV2); 

  tft.init(135, 240);
  tft.setRotation(3);
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_MAGENTA);
  tft.println("BLE_TARGET_MODE");

  if (r1.begin() && r2.begin()) {
    initRadio(&r1);
    initRadio(&r2);
    tft.setTextColor(ST77XX_GREEN);
    tft.println("LOCKED: CH 2, 26, 80");
  } else {
    tft.setTextColor(ST77XX_RED);
    tft.println("RADIO FAIL");
    while(1) yield();
  }
  delay(1000);
  tft.fillScreen(ST77XX_BLACK);
}

void initRadio(RF24* r) {
  r->setPALevel(RF24_PA_MAX); 
  r->setDataRate(RF24_2MBPS); // BLE only talks at 2MBPS or 1MBPS
  r->setAutoAck(false);
  r->setRetries(0, 0);
  r->stopListening();
  r->openWritingPipe(0x5555555555LL);
}

// Ultra-fast register write for frequency switching
void jump(int csn, uint8_t ch) {
  digitalWrite(csn, LOW);
  SPI.transfer(0x25); // RF_CH Register
  SPI.transfer(ch & 0x7F);
  digitalWrite(csn, HIGH);

  // Send the noise
  digitalWrite(csn, LOW);
  SPI.transfer(0xA0); // W_TX_PAYLOAD
  for(int i=0; i<32; i++) SPI.transfer(0xAA);
  digitalWrite(csn, HIGH);
  
  int ce = (csn == CSN1_PIN) ? CE1_PIN : CE2_PIN;
  digitalWrite(ce, HIGH);
  delayMicroseconds(10); 
  digitalWrite(ce, LOW);
}

void loop() {
  // Constant loop hitting ONLY BLE Advertising channels
  for (int i = 0; i < 3; i++) {
    jump(CSN1_PIN, ble_channels[i]);
    jump(CSN2_PIN, ble_channels[(i + 1) % 3]);
  }

  // Heartbeat every 500 cycles to keep ESP8266 stable
  static int cycles = 0;
  if (cycles++ > 500) {
    yield();
    cycles = 0;
    tft.drawPixel(230, 120, ST77XX_MAGENTA);
  }
}
