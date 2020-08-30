#include <Arduino.h>
#include <Wire.h>
#include <LittleFS.h>
#include <Hash.h>

// ! Its BAD!!! #define INCBIN_OUTPUT_SECTION ".irom.text"
#include "incbin.h"
INCBIN(WebIndex, "E:/__BuildSource/__LIB__/Arduino/Sketch/MS-Code-Test/data/index.html");
// INCBIN(WebIndex, "../data/index.html");
/*
*/

#define DEBUG 1
const PROGMEM char *fname = "/index.html";

void setup() {
  Serial.begin(115200);
  LittleFS.begin();
  delay(1000);

  {
    fs::Dir dir = LittleFS.openDir("/");
    while (dir.next()) {
      Serial.printf("\tFILE: %s\n", dir.fileName().c_str());
    }
  }
  
  Serial.printf("-- begin, write file [%s] estimated size: %u ..\n", fname, gWebIndexSize);
  if (LittleFS.exists(fname)) {
    Serial.printf("-- file [%s] exist, overwrite..\n", fname);
  }
  fs::File f = LittleFS.open(fname, "w");
  if (f) {
    size_t sz = f.write(gWebIndexData, gWebIndexSize);
    f.close();
    Serial.printf("-- write bytes = %u/%u\n", sz, gWebIndexSize);
    if (sz != gWebIndexSize)
      Serial.printf("-- WARNING, file size missmatch!! %u\n", sz);
    else
      Serial.printf("-- file write OK [%s]\n", fname);
  }
}

void loop() {
}
