

// TO DO:
// Fixa till så allt lämplig data inhämtas (kanske kolla upp om det går att ansluta för mer info)? 
// kolla #define vs 'const char *' osv
// Kolla secrets för wifi creds


// BLE Reference:
//https://docs.espressif.com/projects/arduino-esp32/en/latest/api/ble.html
//https://www.iotwithesp.com/reference/protocols/ble/


#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <PicoSyslog.h>

#if defined(ESP32)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#else
#error "This board is not supported."
#endif

// WIFI CREDS
const char *ssid = "---";
const char *password = "---";

// BLUETOOTH
int scanTime = 5;  //In seconds

// SYSLOG
const char *esp32Hostname = "ESP32-C3-Zero-1";
const char *serverIP = "192.168.50.8";
const unsigned int syslogPort = 514;

// INIT PicoSyslog and BlueTooth
PicoSyslog::Logger syslog(
    "BluESPLog",
    esp32Hostname,
    PicoSyslog::LogLevel::information,
    nullptr,
    serverIP,
    syslogPort);

BLEScan *pBLEScan;


class AdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    if (advertisedDevice.haveName() == true){
      //syslog.printf("BOMBACLAD: %s \n", advertisedDevice.toString().c_str());
      BLEClient* client = BLEDevice::createClient();
      client->connect(advertisedDevice);
      BLERemoteService* remoteService = client->getService(SERVICE_UUID);
      BLERemoteCharacteristic* remoteChar = remoteService->getCharacteristic(CHAR_UUID);
      std::string value = remoteChar->readValue();
    }//else{
     // syslog.printf("Advertised Device: %s \n", advertisedDevice.toString().c_str());
    //}

    // JUST ADDED THIS BELOW V
    syslog.printf("Device: %s | Address: %s | RSSI: %d dBm",
      advertisedDevice.haveName() ? advertisedDevice.getName().c_str() : "(unnamed)",
      advertisedDevice.getAddress().toString().c_str(),
      advertisedDevice.getRSSI()
    );

    if (advertisedDevice.haveServiceUUID()) {
      syslog.printf(" | Service: %s", advertisedDevice.getServiceUUID().toString().c_str());
    }

    Serial.println();
  }
};

void setup() {
  Serial.begin(115200);
  delay(10);
  Serial.println();
  Serial.println();
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

  Serial.println("Scanning...");

  // INIT BLUETOOTH
  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan();  //create new scan
  pBLEScan->setAdvertisedDeviceCallbacks(new AdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);  //active scan uses more power, but get results faster
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);  // less or equal setInterval value

}

void loop() {
  // put your main code here, to run repeatedly:
  BLEScanResults *foundDevices = pBLEScan->start(scanTime, false);
  Serial.print("Devices found: ");
  Serial.println(foundDevices->getCount());
  Serial.println("Scan done!");
  pBLEScan->clearResults();  // delete results fromBLEScan buffer to release memory
  delay(2000);
}
