#include <ArduinoBLE.h>
#define ENABLE_GxEPD2_GFX 0

#include <GxEPD2_BW.h>

#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>


GxEPD2_BW<GxEPD2_154_M09, GxEPD2_154_M09::HEIGHT> display(GxEPD2_154_M09(/*CS=D8*/ D2, /*DC=D3*/ D4, /*RST=D4*/ D5, /*BUSY=D2*/ D3)); // GDEW0154M09 200x200, JD79653A

// UUIDs used by the ESP32 BLE Server
const char* SERVICE_UUID = "d0000000-0000-1000-8000-00805f9b34fb";
const char* TEMP_UUID    = "2A6E";  // Temperature
const char* HUM_UUID     = "2A6F";  // Humidity
const char* VOC_UUID     = "d0000004-0000-1000-8000-00805f9b34fb";
const char* NOX_UUID     = "d0000005-0000-1000-8000-00805f9b34fb";
const char* PM25_UUID    = "2BD3";  // PM2.5

BLEDevice peripheral;
BLECharacteristic tempChar, humChar, vocChar, noxChar, pm25Char;

float decodeSFloat(const uint8_t* data) {
  uint16_t raw = data[1] << 8 | data[0];
  int16_t mantissa = raw & 0x0FFF;
  int8_t exponent = (raw >> 12) & 0x0F;
  if (mantissa & 0x0800) mantissa |= 0xF000; // sign extend
  return mantissa * pow(10, exponent);
}

bool connectToServer() {
  Serial.println("Scanning for BLE server...");

  BLE.scanForUuid(SERVICE_UUID);  // << simplified scan with filter

  BLEDevice dev;
  unsigned long timeout = millis() + 10000;
  while (millis() < timeout) {
    dev = BLE.available();
    if (dev) break;
  }
  BLE.stopScan();

  if (!dev) {
    Serial.println("No BLE server found.");
    return false;
  }

  Serial.print("Connecting to: ");
  Serial.println(dev.address());

  if (!dev.connect()) {
    Serial.println("Connection failed.");
    return false;
  }

  if (!dev.discoverAttributes()) {
    Serial.println("Service discovery failed.");
    dev.disconnect();
    return false;
  }

  peripheral = dev;

  tempChar  = peripheral.characteristic(TEMP_UUID);
  humChar   = peripheral.characteristic(HUM_UUID);
  vocChar   = peripheral.characteristic(VOC_UUID);
  noxChar   = peripheral.characteristic(NOX_UUID);
  pm25Char  = peripheral.characteristic(PM25_UUID);

  if (!(tempChar && humChar && vocChar && noxChar && pm25Char)) {
    Serial.println("One or more characteristics not found.");
    peripheral.disconnect();
    return false;
  }

  Serial.println("Connected to server.");
  return true;
}

void readAndPrint(BLECharacteristic& ch, const char* label) {
  uint8_t data[2];
  if (ch.readValue(data, 2)) {
    float value = decodeSFloat(data);
    Serial.print(label);
    Serial.print(": ");
    Serial.println(value);
    displayData(value);
  } else {
    Serial.print(label);
    Serial.println(": Read failed");
  }
}

void setup() {
  Serial.begin(115200);
  //while (!Serial);
  pinMode(D6, OUTPUT);
  digitalWrite(D6, LOW);

  display.init(115200);
  ble_init();
}

void loop() {
  ble_init();
  if (!peripheral || !peripheral.connected()) {
    if (!connectToServer()) {
      delay(90000);
      return;
    }
  }

  //readAndPrint(tempChar,  "Temperature");
  //readAndPrint(humChar,   "Humidity");
  //readAndPrint(vocChar,   "VOC Index");
  //readAndPrint(noxChar,   "NOx Index");
  readAndPrint(pm25Char,  "PM2.5");

  BLE.disconnect();
  BLE.end();
  Serial.println("---");
  delay(90000);
  delay(90000);
}

void displayData(float pm25)
{
  //Serial.println("helloWorld");
  display.setRotation(1);
  display.setFont(&FreeMonoBold12pt7b);
  display.setTextSize(2);
  if (display.epd2.WIDTH < 104) display.setFont(0);
  display.setTextColor(GxEPD_BLACK);
  int16_t tbx, tby; uint16_t tbw, tbh;
  //display.getTextBounds(HelloWorld, 0, 0, &tbx, &tby, &tbw, &tbh);
  // center bounding box by transposition of origin:
  //uint16_t x = ((display.width() - tbw) / 2) - tbx;
  //uint16_t y = ((display.height() - tbh) / 2) - tby;
  display.setFullWindow();
  display.firstPage();
  do
  {
    display.fillScreen(GxEPD_WHITE);
    display.setCursor(30, 60);
    display.print("PM2.5");
    display.setCursor(30, 120);
    display.print(pm25);
  }
  while (display.nextPage());
  //Serial.println("helloWorld done");
}

void ble_init() {
  if (!BLE.begin()) {
    Serial.println("Failed to initialize BLE!");
    while (1);
  }

  BLE.setLocalName("NanoClient");
  BLE.setDeviceName("NanoClient");

  if (!connectToServer()) {
    Serial.println("Retrying in loop...");
  }

}