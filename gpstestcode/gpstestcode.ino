#include <TinyGPS++.h>
#include <HardwareSerial.h>
#define RX D7
#define GPS_BAUD 9600

// #include <WiFi.h>
// const char* ssid     = "";
// const char* password = "";
// WiFiServer server(80);
// String yazi;

TinyGPSPlus gps;

HardwareSerial gpsSerial(1);

void setup()
{
    Serial.begin(9600);

  gpsSerial.begin(GPS_BAUD, SERIAL_8N1, RX);
  Serial.println("Serial 2 started at 9600 baud rate");

}

void loop() {
  while (gpsSerial.available() > 0) {
    gps.encode(gpsSerial.read());
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



// void loop(){
//   smartdelay(1000);
//  WiFiClient client = server.available();    // listen for incoming clients
//     float flat, flon;
//   unsigned long age;
//    gps.f_get_position(&flat, &flon, &age);
  
//   if (client) {                             
//     Serial.println("new client");          
//     String currentLine = "";                // make a String to hold incoming data from the client
//     while (client.connected()) {            
//       if (client.available()) {             // if there's client data
//         char c = client.read();          // read a byte
//           if (c == '\n') {                      // check for newline character,
//           if (currentLine.length() == 0) {  // if line is blank it means its the end of the client HTTP request
      
// yazi="<!DOCTYPE html><html lang='en'><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'><meta http-equiv='X-UA-Compatible' content='ie=edge'><title>My Google Map</title><style>#map{height:400px;width:100%;}</style></head> <body><h1>My Google Map</h1><div id='map'></div><script>function initMap(){var options = {zoom:8,center:{lat:";
//     yazi+=flat;
//     yazi+=",lng:";
//     yazi+=flon;
//     yazi+="}};var map = new google.maps.Map(document.getElementById('map'), options);google.maps.event.addListener(map, 'click', function(event){addMarker({coords:event.latLng});});var markers = [{coords:{lat:";
// yazi+=flat;
// yazi+=",lng:";
// yazi+=flon;
// yazi+="}}];for(var i = 0;i < markers.length;i++){addMarker(markers[i]);}function addMarker(props){var marker = new google.maps.Marker({position:props.coords,map:map,});if(props.iconImage){marker.setIcon(props.iconImage);}if(props.content){var infoWindow = new google.maps.InfoWindow({content:props.content});marker.addListener('click', function(){infoWindow.open(map, marker);});}}}</script><script async defer src='https://maps.googleapis.com/maps/api/js?key=AIzaSyDHNUG9E870MPZ38LzijxoPyPgtiUFYjTM&callback=initMap'></script></body></html>";
 
 
//  client.print(yazi);

//             // The HTTP response ends with another blank line:
//             client.println();
//             // break out of the while loop:
//             break;
//           } else {   currentLine = ""; }
//         } else if (c != '\r') {  // if you got anything else but a carriage return character,
//           currentLine += c; // add it to the end of the currentLine
//         }
//          // here you can check for any keypresses if your web server page has any
//       }
//     }
//     // close the connection:
//     client.stop();
//     Serial.println("client disconnected");
//     }
// }
// static void smartdelay(unsigned long ms)
// {
//   unsigned long start = millis();
//   do 
//   {
//     while (ss.available())
//       gps.encode(ss.read());
//   } while (millis() - start < ms);
// }
