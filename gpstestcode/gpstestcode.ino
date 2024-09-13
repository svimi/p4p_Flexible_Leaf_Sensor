#include <TinyGPS++.h>
#include <HardwareSerial.h>
#define RX D7
#define GPS_BAUD 9600

TinyGPSPlus gps;

HardwareSerial gpsSerial(2);

void setup()
{
    Serial.begin(9600);

  gpsSerial.begin(GPS_BAUD, SERIAL_8N1, RX);
  Serial.println("Serial 2 started at 9600 baud rate");

}

void loop() {
  while (gpsSerial.available() > 0) {
    char gpsData = gpsSerial.read();
    gps.encode(gpsData);
    Serial.print(gpsData);
  }

  if (gps.location.isUpdated()) {
    Serial.print("Latitude: ");
    Serial.println(gps.location.lat(), 6); // Latitude in degrees
    Serial.print("Longitude: ");
    Serial.println(gps.location.lng(), 6); // Longitude in degrees
    Serial.print("Altitude: ");
    Serial.println(gps.altitude.meters()); // Altitude in meters
    Serial.print("Satellites: ");
    Serial.println(gps.satellites.value()); // Number of satellites in use
  }

  delay(1000);
}
