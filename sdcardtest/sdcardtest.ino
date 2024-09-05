#include <SD.h>
#define SD_CS_PIN D0

void setup() {
  // put your setup code here, to run once:

  Serial.begin(115200);

  if(!SD.begin(SD_CS_PIN)) {
      Serial.println("Card Mount Failed");
      return;
    } else {
      Serial.println("Card Mount Passed");
      
  }

}

void loop() {
  // put your main code here, to run repeatedly:

}
