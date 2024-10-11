////////////////////////////////////////////
////
////  Capacitance Sensor Data Logging File
////  Author: Zaid Mustafa
////  Acknowledgements: Ashwin Whitchurch, Hang Sung
////  Uses Edited Protocentral_FDC1004 Library
////
////  This code recieves capacitance data from up to 4 capictance sensors,
////  converts capacitance to water volumes, stores data on an SD card,
////  hosts a webserver and allows data to be downloaded or deleted from
////  webserver page. 
////////////////////////////////////////////


#include <Wire.h>
#include "Protocentral_FDC1004_EDITTED.h"
#include <SPI.h>
#include <SD.h>
#include <FS.h>
#include <WiFi.h>
#include <WebServer.h>
#include <TinyGPS++.h> // needs to be downloaded
#include <HardwareSerial.h>

#define RX D7 // ESP32S3 RX pin
#define GPS_BAUD 9600
#define UPPER_BOUND 0X4000  // max readout capacitance
#define LOWER_BOUND (-1 * UPPER_BOUND)
#define SD_CS_PIN D0  // SD card chip select pin

// Web server initialisation
// Directions for webserver use:
// 1. Connect to ESP32 wifi connection named as the ssid.
// 2. Enter the gateway address into a browser (e.g 192.168.1.1),
//    it will take you to the hosted webpage
const char* ssid = "Capacitance Readings"; // can be editted
const char* password = "12345678"; // can be editted
IPAddress local_ip(192, 168, 1, 1); // can be editted but must be same as gateway address
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
WebServer server(80);

// FDC 4 channel initialisation
uint8_t MEASUREMENT[] = { 0, 1, 2, 3 };
uint8_t CHANNEL[] = { 0, 1, 2, 3 };
uint8_t CAPDAC[] = { 0, 0, 0, 0 };
int32_t rawCapacitance[4];
float capacitance[4];
float avgCapacitance[4];
float waterVol[4];
FDC1004 FDC;

// file writing initialisation - change FileSuffix value to create a separate file
RTC_DATA_ATTR int FileSuffix = 0;
String dataBuffer = "";
String fileName = String("/") + "DATA" + String(FileSuffix) + ".csv";

// GPS setup - comment out if no gps is plugged in
TinyGPSPlus gps;
HardwareSerial gpsSerial(1);
float flat;
float flon;

//// Printing Test Values
// flat = -6.6255;
// flon = -76.6574;
////

int cellCount = 1;

void setup() {
  Wire.begin();          //i2c begin
  //// GPS communication to ESP32 - Comment out if no GPS
  gpsSerial.begin(GPS_BAUD, SERIAL_8N1, RX);
  ////
  // Serial.begin(115200);  // serial baud rate - can be commented out when not testing

  //// loops until fix on satellite - should be commented out when testing indoors
  // while (gpsSerial.available() > 0) {
  //   gps.encode(gpsSerial.read());
  // }
  //
  // while (!gps.location.isUpdated()) {
  //   while (gpsSerial.available() > 0) {
  //     gps.encode(gpsSerial.read());
  //   }
  // }
  // Serial.println("GPS FIXED")
  ////

  // SD Card Check
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("Card Mount Failed");
    return;
  } else {
    Serial.println("Card Mount Passed");
  }
  writeFileTitle(fileName.c_str());

  createWebServer(ssid, password);
}

void loop() {
  server.handleClient();

  //// Comment out if no GPS
  // GPS ping
  while (gpsSerial.available() > 0) {
    gps.encode(gpsSerial.read());
  }

  // GPS position update
  if (gps.location.isUpdated()) {
    flat = gps.location.lat();
    flon = gps.location.lng();
  }
  ////

  // Checks leaf moisture every 3 hours from 6am to 6pm 
  // If no GPS available, simply use dataLogging() alone 
  if ((gps.time.hour() == 6 && gps.time.minute() == 0 && (gps.time.second() % 10) == 0) || (gps.time.hour() == 9 && gps.time.minute() == 0 && (gps.time.second() % 10) == 0) || (gps.time.hour() == 12 && gps.time.minute() == 0 && (gps.time.second() % 10) == 0) || (gps.time.hour() == 15 && gps.time.minute() == 0 && (gps.time.second() % 10) == 0) || (gps.time.hour() == 18 && gps.time.minute() == 0 && (gps.time.second() % 10) == 0)) {
    dataLogging();
  //// Test code - prints values every ten seconds  
  } else if ((gps.time.second() % 10) == 0) {
    dataLogging();
  ////
  } else { // continues to read capacitance for capdac adjustment, delay/deepsleep could be added until a minute before print time
    fdcRead(MEASUREMENT, CHANNEL, CAPDAC, rawCapacitance);
  }
}

// FDC analogue to digital conversion code
// Modified for 4 inputs from Ashwin Whitchurch's FDC1004 demo code on the "ProtoCentral_fdc1004_breakout" github page
void fdcRead(uint8_t* MEASUREMENT, uint8_t* CHANNEL, uint8_t* CAPDAC, int32_t* rawCapacitance) {
  for (int i = 0; i < 4; i++) {
    uint8_t measurement = MEASUREMENT[i];
    uint8_t channel = CHANNEL[i];
    uint8_t capdac = CAPDAC[i];

    FDC.configureMeasurementSingle(measurement, channel, capdac);
    FDC.triggerSingleMeasurement(measurement, FDC1004_100HZ);

    //wait for completion
    delay(15);
    uint16_t value[2];
    if (!FDC.readMeasurement(measurement, value)) {
      int16_t msb = (int16_t)value[0];
      /*int32_t*/ rawCapacitance[i] = ((int32_t)457) * ((int32_t)msb);  //in attofarads
      rawCapacitance[i] /= 1000;                                        //in femtofarads
      rawCapacitance[i] += ((int32_t)3028) * ((int32_t)capdac);
      capacitance[i] = (float)rawCapacitance[i] / 1000; //in picofarads

      if (msb > UPPER_BOUND)  // adjust capdac accordingly
      {
        if (CAPDAC[i] < FDC1004_CAPDAC_MAX)
          CAPDAC[i]++;
      } else if (msb < LOWER_BOUND) {
        if (CAPDAC[i] > 0)
          CAPDAC[i]--;
      }
    }
  }
}

// Averages 10 capacitance readings and converts it to water volume
void fdcReadAverage() {

  float average[] = { 0, 0, 0, 0 };

  for (int i = 0; i < 10; i++) {
    fdcRead(MEASUREMENT, CHANNEL, CAPDAC, rawCapacitance);
    average[0] += capacitance[0];
    average[1] += capacitance[1];
    average[2] += capacitance[2];
    average[3] += capacitance[3];
  }

  for (int i = 0; i < 4; i++) {
    avgCapacitance[i] = average[i] /= 10;
    // converts capacitance to water vol with equation derived from sensor experiments
    waterVol[i] = avgCapacitance[i] * 0.7775 - 11.325; // volume in microlitre
  }
}

// Formatting csv file
void dataLogging(void) {
  fdcReadAverage();

  // Titles location and time based off of GPS data - comment out if no GPS is connected
  appendFileLocAndTime(fileName.c_str());

  // Appends to file the water volume values
  for (int i = 0; i < 4; i++) {
    dataBuffer += String(waterVol[i], 4);

    appendFile(fileName.c_str(), dataBuffer.c_str());
    dataBuffer = "";
  }
}

// Writes headers formatted for the csv file
void writeFileTitle(const char* path) {

  Serial.printf("Writing file: %s\n", path);

  File file = SD.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }

  file.print("LOCATION");
  file.print(",");
  file.print("TIME");
  file.print(",");
  file.print("SENSOR_1/microL");
  file.print(",");
  file.print("SENSOR_2/microL");
  file.print(",");
  file.print("SENSOR_3/microL");
  file.print(",");
  file.println("SENSOR_4/microL");


  file.close();
}

// Appends GPS info to file
void appendFileLocAndTime(const char* path) {
  Serial.printf("Writing file: %s\n", path);

  File file = SD.open(path, FILE_APPEND);
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }

  file.print(String(flat) + ":" + String(flon));
  file.print(","); 
  file.print(String(gps.time.hour()) + ":" + String(gps.time.minute()) + ":" + String(gps.time.second()) + " - " + String(gps.date.day()) + "/" + String(gps.date.month()) + "/" + String(gps.date.year()));
  file.print(",");

  file.close();
}

// Appends water volume values to file
void appendFile(const char* path, const char* message) {
  Serial.printf("Appending to file: %s\n", path);

  File file = SD.open(path, FILE_APPEND);

  if (!file) {
    Serial.println("Failed to open file for appending");
    return;
  }

  if (cellCount == 4) { // newline check
    file.println(message);
    cellCount = 1;
  } else {
    file.print(message) ?: Serial.println("Append failed");
    file.print(",");
    cellCount++;
  }


  file.close();
}

// file delete for resetting file on webserver request
void deleteFile(const char* path) {
  SD.remove(path);
}

//////// Web Server Handling
void createWebServer(const char* ssid, const char* password) {

  WiFi.softAP(ssid, password);
  WiFi.softAPConfig(local_ip, gateway, subnet);
  delay(100);

  server.on("/", HTTP_GET, handleOn);
  server.on("/download", HTTP_GET, handleDownload);
  server.on("/delete", HTTP_GET, handleDelete);

  server.begin();
  Serial.println("WIFI successful startup");
}

void handleOn() {
  server.send(200, "text/html", SendHTML());
}

void handleDownload() {
  String fileName = server.arg("file");
  Serial.println("Received file argument: " + fileName);
  if (fileName != "") {
    File file = SD.open("/" + fileName, FILE_READ);
    if (file) {
      server.sendHeader("Content-Disposition", "attachment; filename=\"" + fileName + "\"");
      String contentType = "text/csv";  // Default MIME type
      // Stream the file to the client
      server.streamFile(file, contentType);
      file.close();
    } else {
      server.send(404, "text/plain", "File not found");
    }
  } else {
    server.send(400, "text/plain", "Bad Request");
  }
}

void handleDelete() {
  String fileName = server.arg("file");
  Serial.println("Received file argument: " + fileName);
  if (fileName != "") {
    if (SD.exists("/" + fileName)) {
      fileName = "/" + fileName;
      SD.remove(fileName.c_str());
      writeFileTitle(fileName.c_str());
      server.sendHeader("Location", "/", true);
      server.send(303);
    } else {
      server.send(404, "text/plain", "File not found");
    }
  } else {
    server.send(400, "text/plain", "Bad Request");
  }
}

void handleNotFound() {
  server.send(404, "text/plain", "Not found");
}

String SendHTML() {
  File root = SD.open("/");
  String html = "<!DOCTYPE html><html>\n";
  html += "<html lang='en'><style>\n";
  html += "table{border: 1px solid black; border-collapse: collapse; width: 50%; margin-left: auto; margin-right: auto;}\n";
  html += "th, td {border: 1px solid black; padding: 8px; text-align: center; border-bottom: 1px solid #DDD;}\n";
  html += "tr:hover {background-color: #D6EEEE;} </style>\n";
  html += "<head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'>\n";
  html += "<title>File Download</title>\n";
  html += "</head>\n";
  html += "<body>\n";
  html += "<h1 style='background-color:powderblue; text-align:center;'>Data Logging - File Interface</h1>\n";
  html += "<h3 style='background-color:powderblue; text-align:center;'>You may <b>download</b> or <b>delete</b> files on this page</h3>\n";
  html += "<table>\n";

  while (File file = root.openNextFile()) {
    if (!file.isDirectory()) {
      // File name
      String fileName = file.name();
      html += "<tr><td>\n";
      html += fileName;
      html += "</td>\n";
      html += "<td>";

      // File size
      unsigned long fileSizeBytes = file.size();
      if (fileSizeBytes < 1024) {
        html += String(fileSizeBytes);
        html += " Bytes\n";
      } else if (fileSizeBytes < 1048576) {
        float fileSizeKB = fileSizeBytes / 1024.0;
        html += String(fileSizeKB);
        html += " KB\n";
      } else {
        float fileSizeMB = fileSizeBytes / 1048576.0;
        html += String(fileSizeMB);
        html += " MB\n";
      }

      // File Download
      html += "</td>\n";
      html += "<td>\n";
      html += " <a href=\"/download?file=\n";
      html += fileName;
      html += "\">Download</a>\n";
      html += "</td>\n";

      // File delete
      html += "<td>\n";
      html += " <a href=\"/delete?file=\n";
      html += fileName;
      html += "\" onclick=\"return confirm('Are you sure?')\">Delete</a>\n";
      html += "</td>\n";
      html += "</tr>\n";
    }
    file.close();
  }

  html += "</table>\n";
  html += "</body>\n";
  html += "</html>\n";

  return html;
}
