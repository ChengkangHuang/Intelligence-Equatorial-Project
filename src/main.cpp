#include <Arduino.h>
#include <SPI.h>
#include <TinyGPS++.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_HMC5883_U.h>

// The TinyGPS++ object
TinyGPSPlus gps;

// The Compass object
Adafruit_HMC5883_Unified compass = Adafruit_HMC5883_Unified(5883);

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
	xTaskCreatePinnedToCore(gpsTask, "gpsTask", 10000, NULL, 1, NULL, 0);
	xTaskCreatePinnedToCore(compassTask, "compassTask", 10000, NULL, 1, NULL, 0);
	// xTaskcreate(gpsTask, "gpsTask", 1024, NULL, 1, NULL);
	// xTaskcreate(compassTask, "compassTask", 1024, NULL, 1, NULL);
}

void loop()
{
	// put your main code here, to run repeatedly:
	// Leave this loop empty
}