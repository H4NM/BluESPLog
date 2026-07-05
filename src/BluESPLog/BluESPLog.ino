

// TO DO:
// Fixa till så allt lämplig data inhämtas (kanske kolla upp om det går att ansluta för mer info)? 


// BLE Reference:
//https://docs.espressif.com/projects/arduino-esp32/en/latest/api/ble.html
//https://www.iotwithesp.com/reference/protocols/ble/


#include <BluetoothSerial.h>
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <PicoSyslog.h>

#include "SECRETS.h"

#if defined(ESP32)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#else
#error "This board is not supported."
#endif



#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

#if !defined(CONFIG_BT_SPP_ENABLED)
#error Serial Bluetooth not available or not enabled. It is only available for the ESP32 chip.
#endif


// DEV
bool dev = true;

// WIFI CREDS
const char *ssid = SECRET_SSID;
const char *password = SECRET_PASSWORD;

// BLE
// BOMBACLAD: Name: S36 01D6 LE, Address: e1:a1:8a:97:4c:1d, manufacturer data: a7053c3b582982583390e9, serviceUUID: 0000fe07-0000-1000-8000-00805f9b34fb, txPower: 0, rssi: -92 
char *BLEDeviceName = "";
char *BLEDeviceAddress = "";
char *BLEDeviceManufacturerData = "";
char *BLEDeviceServiceUUID = "";
int8_t *BLEDeviceTXPower = 0;
int *BLERSSI = 0;

// BLUETOOTH SCAN TIMES
int BLE_SCAN_TIME = 5;  //Seconds
int BC_SCAN_TIME = BLE_SCAN_TIME * 1000; //Miliseconds


// SYSLOG
const char *esp32Hostname = "ESP32";
const char *serverIP = "192.168.50.8";
const unsigned int syslogPort = 514;
String deviceDiscoveryText = "";

// INIT PicoSyslog and BLE and classic Bluetooth
PicoSyslog::Logger syslog(
    "BluESPLog",
    esp32Hostname,
    PicoSyslog::LogLevel::information,
    &Serial, //&Serial OR nullptr for no serial output
    serverIP,
    syslogPort);

BLEScan *pBLEScan;

BluetoothSerial SerialBT;


void connectWiFi() {
  if (WiFi.status() != WL_CONNECTED) {
    syslog.printf("WiFi disconnected. Reconnecting...\n");
    WiFi.reconnect();
  }
}

void logEvent(string text){
  if (dev){
    Serial.println(text);
  }else{
    syslog.printf(text); 
  }
}

void scanBC(){
    Serial.println("Scanning Bluetooth Classic... ");
    BTScanResults *foundDevices = SerialBT.discover(BC_SCAN_TIME);
    if (foundDevices) {
      
      int foundDeviceCount = foundDevices->getCount();
    
      for (int i = 0; i < foundDeviceCount; i++) {
          BTAdvertisedDevice *foundDevice = foundDevices->getDevice(i);
          Serial.printf("BC | %s\n", foundDevice->toString().c_str());
      }
    } else {
      Serial.println("Error on BT Scan, no result!");
    }
}

void scanBLE(){
    Serial.println("Scanning Bluetooth Low Energy... ");    
    BLEScanResults *foundDevices = pBLEScan->start(BLE_SCAN_TIME, false);

    int foundDeviceCount = foundDevices->getCount();
    
    for (int i = 0; i < foundDeviceCount; i++) {
        BLEAdvertisedDevice foundDevice = foundDevices->getDevice(i);
        Serial.printf("BLE | %s\n", foundDevice.toString().c_str());
    }

    pBLEScan->clearResults();
}

void btAdvertisedDeviceFound(BTAdvertisedDevice *pDevice) {
  if (dev == false){
    syslog.printf("Bluetooth | %s\n", pDevice->toString().c_str());  
  }else{
    Serial.printf("Bluetooth | %s\n", pDevice->toString().c_str()); 
  }
}

class AdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    if (dev == false){
    
      // JUST ADDED THIS BELOW V
      syslog.printf("BLE | Device: %s | Address: %s | RSSI: %d dBm",
        advertisedDevice.haveName() ? advertisedDevice.getName().c_str() : "(unnamed)",
        advertisedDevice.getAddress().toString().c_str(),
        advertisedDevice.getRSSI()
      );
  
      if (advertisedDevice.haveServiceUUID()) {
        syslog.printf(" | Service: %s", advertisedDevice.getServiceUUID().toString().c_str());
      }
  
      if (advertisedDevice.haveServiceData()) {
        syslog.printf(" | ServiceData: %s", advertisedDevice.getServiceData());
      }
  
      if (advertisedDevice.haveAppearance()) {
        syslog.printf(" | Appearance: %d", advertisedDevice.getAppearance());
      }
  
      Serial.println();
    }else{
      Serial.printf("BLE | Device: %s | Address: %s | RSSI: %d dBm",
        advertisedDevice.haveName() ? advertisedDevice.getName().c_str() : "(unnamed)",
        advertisedDevice.getAddress().toString().c_str(),
        advertisedDevice.getRSSI()
      );
      
      Serial.println();
    }
  }
};


void setup() {
  Serial.begin(115200);
  delay(10);
  Serial.println();
  Serial.println();
  Serial.printf("Starting - dev mode active: ");

  Serial.printf(dev ? "TRUE" : "FALSE");

  if (dev == false){
  
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
      Serial.print("..");
      delay(500);
    }
    Serial.println("");
    Serial.println("WiFi connected.");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  }

  // INIT ASYNC BLE
  Serial.println("Initiating BLE");
  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan();  //create new scan
  pBLEScan->setActiveScan(true);  //active scan uses more power, but get results faster
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);  // less or equal setInterval value
  
  // INIT CLASSIC BLUETOOTH
  Serial.println("Initiating Bluetooth classic");
  SerialBT.begin("ESP32test");  //Bluetooth device name
}

void loop() {
  if (dev == false){
    connectWiFi();  
  }
  scanBC();
  delay(350);
  scanBLE();
  delay(350);
}
