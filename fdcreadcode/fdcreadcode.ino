//////////////////////////////////////////////////////////////////////////////////////////
//
//    Demo code for the FDC1004 capacitance sensor breakout board
//
//    Author: Ashwin Whitchurch
//    Copyright (c) 2018 ProtoCentral
//
//    This example measures raw capacitance across CHANNEL0 and Gnd and
//		prints on serial terminal
//
//		Arduino connections:
//
//		Arduino   FDC1004 board
//		-------   -------------
//		5V - Vin
// 	 GND - GND
//		A4 - SDA
//		A5 - SCL
//
//    This software is licensed under the MIT License(http://opensource.org/licenses/MIT).
//
//   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
//   NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
//   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
//   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
//   SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
//   For information on how to use, visit https://github.com/protocentral/ProtoCentral_fdc1004_breakout
/////////////////////////////////////////////////////////////////////////////////////////

#include <Wire.h>
#include <Protocentral_FDC1004.h>
#include <SPI.h>
#include <SD.h>
#include <FS.h>
#include <WiFi.h>
#include <WebServer.h>

#define UPPER_BOUND  0X4000                 // max readout capacitance
#define LOWER_BOUND  (-1 * UPPER_BOUND)
#define CHANNEL0 0                          // channel to be read
#define MEASURMENT0 0                       // measurment channel
#define CHANNEL1 1                          // channel to be read
#define MEASURMENT1 1                       // measurment channel
#define CHANNEL2 2                          // channel to be read
#define MEASURMENT2 2                       // measurment channel
#define CHANNEL3 3                          // channel to be read
#define MEASURMENT3 3                       // measurment channel

#define SD_CS_PIN D1

const char* ssid = "Capacitance Readings";
const char* password = "12345678";

IPAddress local_ip(192,168,1,1);
IPAddress gateway(192,168,1,1);
IPAddress subnet(255,255,255,0);

WebServer server(80);

uint8_t capdac0 = 0;
uint8_t capdac1 = 0;
uint8_t capdac2 = 0;
uint8_t capdac3 = 0;

int32_t capacitance;
// int32_t capacitance0;
// int32_t capacitance1;
// int32_t capacitance2;
// int32_t capacitance3;

int cellCount = 1;

FDC1004 FDC;

RTC_DATA_ATTR int FileSuffix = 0;

String dataBuffer = "" ;
String fileName = String( "/" )+ "DATA" + String(FileSuffix) + ".csv" ;


void setup()
{
  Wire.begin();        //i2c begin
  Serial.begin(115200); // serial baud rate

  if(!SD.begin(SD_CS_PIN)) {
    Serial.println("Card Mount Failed");
    return;
  } else {
    Serial.println("Card Mount Passed");
    writeFile(fileName.c_str(), "DataLogging:");
  }

  writeFileTitle(fileName.c_str());

  createWebServer(ssid, password);

}

void loop()
{
  server.handleClient();

  fdcRead(MEASURMENT0, CHANNEL0, capdac0, capacitance);
  dataLogging();
  fdcRead(MEASURMENT0, CHANNEL0, capdac0, capacitance);
  dataLogging();
  fdcRead(MEASURMENT0, CHANNEL0, capdac0, capacitance);  
  dataLogging();
  fdcRead(MEASURMENT0, CHANNEL0, capdac0, capacitance);  
  dataLogging();

  
}

void fdcRead(uint8_t MEASURMENT, uint8_t CHANNEL, uint8_t &capdac, int32_t &capacitance) {

  FDC.configureMeasurementSingle(MEASURMENT, CHANNEL, capdac);
  FDC.triggerSingleMeasurement(MEASURMENT, FDC1004_100HZ);

  //wait for completion
  delay(15);
  uint16_t value[2];
  if (! FDC.readMeasurement(MEASURMENT, value))
  {
    int16_t msb = (int16_t) value[0];
    /*int32_t*/ capacitance = ((int32_t)457) * ((int32_t)msb); //in attofarads
    capacitance /= 1000;   //in femtofarads
    capacitance += ((int32_t)3028) * ((int32_t)capdac);

    // Serial.print("CIN");
    // Serial.print(MEASURMENT + 1);
    // Serial.print(": ");
    // Serial.print((((float)capacitance/1000)),4);
    // Serial.print("  pf, ");
    // Serial.println(capdac);

    if (msb > UPPER_BOUND)               // adjust capdac accordingly
	{
      if (capdac < FDC1004_CAPDAC_MAX)
	  capdac++;
    }
	else if (msb < LOWER_BOUND)
	{
      if (capdac > 0)
	  capdac--;
    }

  }
}

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
      String contentType = "text/csv"; // Default MIME type
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
      deleteFile(fileName.c_str());
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



void writeFileTitle(const char * path) {

  Serial. printf( "Writing file: %s\n", path);

  File file= SD.open(path, FILE_WRITE);
  if(!file) { 
    Serial.println ("Failed to open file for writing");
    return;
  }
  
  file.print("CIN1") ? : Serial.println ("Write failed");
  file.print(",") ? : Serial.println ("Write failed");
  file.print("CIN2") ? : Serial.println ("Write failed");
  file.print(",") ? : Serial.println ("Write failed");
  file.print("CIN3") ? : Serial.println ("Write failed");
  file.print(",") ? : Serial.println ("Write failed");
  file.println("CIN4") ? Serial.println ("File written") : Serial.println ("Write failed");

  file.close();
  
}

void writeFile (const char * path, const char * message) {
  Serial. printf( "Writing file: %s\n", path);

  File file= SD.open(path, FILE_WRITE);
  if(!file) { 
    Serial.println ("Failed to open file for writing");
    return;
  }

  if(file.print(message)) {
    Serial.println("File written");
  } else {
    Serial.println("Write failed");
  }

  file.close();
}

void appendFile(const char * path, const char * message) {
  Serial.printf("Appending to file: %s\n", path);

  File file = SD.open(path, FILE_APPEND);

  if(!file) {
    Serial.println("Failed to open file for appending");
    return;
  }
  
  if (cellCount != 4) {
    file.print(message) ? : Serial.println ("Append failed");
    file.print(",") ? Serial.println ("File appended") : Serial.println ("Append failed");
    cellCount++;
  } else {
    file.println(message) ? Serial.println ("File appended") : Serial.println ("Append failed");
    cellCount = 1;
  }


  file.close();
}

void deleteFile(const char * path) {
  Serial.printf("Deleting file: %s\n, path");

  if(SD.remove(path)) {
    Serial.println("File deleted");
  } else {
    Serial.println("Deleted Failed");
  }
}

void dataLogging(void) {
  dataBuffer += String((((float)capacitance/1000)),4);

  appendFile(fileName.c_str(), dataBuffer.c_str());
  dataBuffer = "";

}

String SendHTML(){
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
    if (!file.isDirectory()){
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
      }
      else if (fileSizeBytes < 1048576) {
        float fileSizeKB = fileSizeBytes / 1024.0;
        html += String(fileSizeKB);
        html += " KB\n";
      } 
      else {
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

    // Serial.print("CIN");
    // Serial.print(MEASURMENT + 1);
    // Serial.print(": ");
    // Serial.print((((float)capacitance/1000)),4);
    // Serial.print("  pf, ");
    // Serial.println(capdac);

    
