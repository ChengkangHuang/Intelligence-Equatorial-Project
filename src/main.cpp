#include <Arduino.h>
#include <SPI.h>
#include <BLE2902.h>
#include <BLEUtils.h>
#include <BLEDevice.h>
#include <TinyGPS++.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_HMC5883_U.h>

// The TinyGPS++ object
TinyGPSPlus gps;
// The Compass object
Adafruit_HMC5883_Unified compass = Adafruit_HMC5883_Unified(5883);
// The BLE Device
#define SERVICE_UUID        "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
BLECharacteristic *pCharacteristic;
bool deviceConnected = false;
char BLEBuffer[32];

// Server callback for BLE connection status
class MyServerCallbacks: public BLEServerCallbacks
{
	void onConnect(BLEServer* pServer)
	{
		deviceConnected = true;
		Serial.println("Connected");
	}

	void onDisconnect(BLEServer* pServer)
	{
		deviceConnected = false;
		Serial.println("Disconnected");
	}
};

// Characteristic callback for BLE data
class MyCallbacks: public BLECharacteristicCallbacks
{
	void onWrite(BLECharacteristic *pCharacteristic)
	{
		std::string rxValue = pCharacteristic->getValue();

		if (rxValue.length() > 0)
		{
			Serial.println("**********");
			Serial.print("Received Value: ");
			for (int i = 0; i < rxValue.length(); i++)
			{
				Serial.print(rxValue[i]);
			}
			Serial.println();
			Serial.println("**********");
		}
	}
};

// Initialize the BLE device
void BLESetup() {
	BLEDevice::init("ESP32 BLE Device");
	BLEServer *pServer = BLEDevice::createServer();
	pServer->setCallbacks(new MyServerCallbacks());

	BLEService *pService = pServer->createService(SERVICE_UUID);

	pCharacteristic = pService->createCharacteristic(
		CHARACTERISTIC_UUID,
		BLECharacteristic::PROPERTY_READ |
		BLECharacteristic::PROPERTY_WRITE |
		BLECharacteristic::PROPERTY_NOTIFY
	);
	pCharacteristic->addDescriptor(new BLE2902());
	pCharacteristic->setCallbacks(new MyCallbacks());

	BLEDevice::startAdvertising();
}

// BLE Task
void BLETask(void *parameter) {
	for (;;) {
		if (deviceConnected) {
			memset(BLEBuffer, 0, sizeof(BLEBuffer));
			memcpy(BLEBuffer, (char *)"Hello World!", sizeof(BLEBuffer));
			pCharacteristic->setValue(BLEBuffer);
			pCharacteristic->notify();
			Serial.println("Sent Value:");
			Serial.println(BLEBuffer);
			Serial.println();
		}
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
}

// GPS Task
void gpsTask(void *parameter)
{
	int satellites = 0;
	float lat = 0;
	float lng = 0;
	float alt = 0;
	char *gpsDate = "";
	char *gpsTime = "";
	TinyGPSDate date;
	TinyGPSTime time;
	
	while (Serial2.available())
	{
		gps.encode(Serial2.read());
	}

	for (;;)
	{
		if (gps.satellites.isValid() && gps.location.isValid() && gps.date.isValid() && gps.time.isValid())
		{
			satellites = gps.satellites.value();
			lat = gps.location.lat();
			lng = gps.location.lng();
			alt = gps.altitude.meters();
			char sz[32];
			date = gps.date;
			time = gps.time;
			sprintf(sz, "%02d-%02d-%02d", date.month(), date.day(), date.year());
			gpsDate = sz;
			sprintf(sz, "%02d:%02d:%02d", time.hour(), time.minute(), time.second());
			gpsTime = sz;
		}
		char s[128];
		sprintf(s, "Satellites: %d\nLat: %f\nLng: %f\nAlt: %f\nDate: %s\nTime: %s\n", satellites, lat, lng, alt, gpsDate, gpsTime);
		Serial.println(s);
		vTaskDelay(4000 / portTICK_PERIOD_MS);
	}
}

// Compass Task
void compassTask(void *parameter)
{
	float headingDegrees = 0;
	int runtimes = 1000; // Run 200 times to collect data
	for (;;) {
		if (compass.begin())
		{
			sensor_t sensor;
			compass.getSensor(&sensor);
			sensors_event_t event;
			compass.getEvent(&event);
			for (int i = 0; i < runtimes; i++)
			{
				float heading = atan2(event.magnetic.y, event.magnetic.x);
				float declinationAngle = -9.49;
				heading += declinationAngle;
				if (heading < 0)
				{
					heading += 2 * PI;
				}
				else if (heading > 0)
				{
					heading -= 2 * PI;
				}
				headingDegrees += heading * 180 / PI;
			}
			headingDegrees /= runtimes;
			Serial.print("Heading: ");
			Serial.println((int)headingDegrees);
		}
		vTaskDelay(4000 / portTICK_PERIOD_MS);
	}
}

// Location string formatter
String locationFormater(float location)
{
	String formatStr = location < 0 ? "-" : "";
	String originalStr = String(abs(location), 4U);
	formatStr += originalStr.substring(0, 2) + char(176) + originalStr.substring(3, 5) + "\"" + originalStr.substring(5, 7) + "\'";
	return formatStr;
}

void setup()
{
	// put your setup code here, to run once:
	Serial.begin(9600);
	BLESetup();
	xTaskCreatePinnedToCore(BLETask, "BLETask", 10000, NULL, 1, NULL, 0);
	xTaskCreatePinnedToCore(gpsTask, "gpsTask", 10000, NULL, 1, NULL, 1);
	xTaskCreatePinnedToCore(compassTask, "compassTask", 10000, NULL, 1, NULL, 1);
}

void loop()
{
	// put your main code here, to run repeatedly:
	// Leave this loop empty
}