
// #define DEBUG 1
#define NODEBUG_WEBSOCKETS 1

#include <Arduino.h>
#include <Wire.h>
#include <DNSServer.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266WebServer.h>
#include <WiFiClient.h>
#include <WebSocketsServer.h>
#include <radio.h>
#include <TEA5767.h>
#include <RDSParser.h>
#include <LittleFS.h>
#include <Hash.h>

// edit and rename config.default.h to config.h
#include "config.h"
#include "FREQJSON.h"

const PROGMEM char err_500_1[] = "500: not support this file type, JSON only..";
const PROGMEM char err_500_2[] = "500: couldn't create file";

TEA5767 radio{};
ESP8266WebServer server(80);
WebSocketsServer webSocket(81);

RDSParser rds{};
FREQJSON freqjson(200);
fs::File fsu{};

void setup() {
  Serial.begin(115200);
  LittleFS.begin();
  delay(1000);
  
  int cnt = 0; 
  WiFi.begin(STASSID, STAPSK);
  while(WiFi.status() != WL_CONNECTED) {
    delay(1000);
    if (cnt++ >= 30) {
       WiFi.beginSmartConfig();
       while(1){
           delay(500);
           if (WiFi.smartConfigDone()) {
             Serial.println("SmartConfig Success");
             break;
           }
       }
    }
  }
  WiFi.printDiag(Serial);
  MDNS.begin("radiofm");
  MDNS.addService("http", "tcp", 80);
  MDNS.addService("ws", "tcp", 81);

  server.onNotFound([]() {
    if (!__impl_handleFileRead(server.uri()))
      server.send(404, "text/plain", F("404: Not Found"));
  });
  server.on("/upload", HTTP_POST,
    [](){ server.send(200); },
    __impl_jsonUpload
  );  
  server.on("/radio/region.json", HTTP_GET,
    __impl_getPresetRadioLists
  );
  server.begin();
  webSocket.begin();
  webSocket.onEvent(__impl_webSocketEvent);

  Serial.println(F("Radio..."));
  radio.init();
  freqjson.setRadio(&radio);
  radio.setBandFrequency(RADIO_BAND_FM, 9140);
  radio.setVolume(10);
  radio.setMono(false);
  radio.attachReceiveRDS(__impl_getRDS);
  rds.attachServicenNameCallback(__impl_getRDSName);

  PRINTF("Begin, wait for you.. (%s)\n", WiFi.localIP().toString().c_str());
  if (LittleFS.exists(RADIO_SCAN_LIST_FILE)) {
    fs::File f = LittleFS.open(RADIO_SCAN_LIST_FILE, "r");
    if (f)
      freqjson.load(f);
  }

#if defined(DEBUG)
  radio.debugEnable();
  radio.debugStatus();
  FSInfo fs_info;
  LittleFS.info(fs_info);
 
  Serial.print("Total space:      ");
  Serial.print(fs_info.totalBytes);
  Serial.println("byte");
  Serial.print("Total space used: ");
  Serial.print(fs_info.usedBytes);
  Serial.println("byte");
  __impl_getFileList();
#endif

}

void loop() {

  webSocket.loop();
  server.handleClient();
  if(Serial.available())
    __impl_commandHandler(Serial.read());

  if (!freqjson.isScan) {
    delay(500);
    return;
  }
  if (!freqjson.scan()) {
    freqjson.save(LittleFS.open(RADIO_SCAN_LIST_FILE, "w"));
    webSocket.broadcastTXT("#e");
    return;
  }
}

void __impl_getRDS(uint16_t block_1, uint16_t block_2, uint16_t block_3, uint16_t block_4) {
  freqjson.setRds(block_1);
  rds.processData(block_1, block_2, block_3, block_4);
}

void __impl_getRDSName(char *name)
{
  for (uint8_t n = 0; n < 8; n++)
    if (name[n] == ' ') return;

  char *odata = new char[9]{};
  sprintf(odata, "#n%s", name);
  webSocket.broadcastTXT(odata);
  delete [] odata;
}

void __impl_getFileList() {
# if defined(DEBUG)
  fs::Dir dir = LittleFS.openDir("/");
  while (dir.next()) {
    PRINTF("FILE: %s\n", dir.fileName().c_str());
  }
# endif
}

void __impl_getPresetRadioLists() {
  fs::Dir dir = LittleFS.openDir("/json");
  String s("{\"lists\":[");
  while (dir.next()) {
    if (dir.fileName().endsWith(".json"))
      s += "\"/json/" + dir.fileName() + "\",";
  }
  s += "\"\"]}\n";
  server.send(200, "text/json", s);
}

void __impl_jsonUpload() {
  HTTPUpload& upload = server.upload();
  
  if (upload.status == UPLOAD_FILE_START) {
    String fname = upload.filename;
    if (!fname.endsWith(".json")) {
      server.send(500, "text/plain", err_500_1);
      return;
    }
    fname = "/json/" + fname;
    PRINTF("Upload: %s\n", fname.c_str());
    fsu = LittleFS.open(fname, "w");
    fname = String();
  
  } else if(upload.status == UPLOAD_FILE_WRITE) {
    if (fsu)
      fsu.write(upload.buf, upload.currentSize);
  } else if (upload.status == UPLOAD_FILE_END) {
    if (fsu) {
      fsu.close();
      PRINTF("Upload finish, size: %u\n", upload.totalSize);
      Serial.print("handleFileUpload Size: "); Serial.println(upload.totalSize);
      server.sendHeader("Location","/index.html");
      server.send(303);
    } else {
      server.send(500, "text/plain", err_500_2);
    }
  }  
}

const char * __impl_getContentType(String filename){
  if(filename.endsWith(".htm")) return "text/html";
  else if(filename.endsWith(".html")) return "text/html";
  else if(filename.endsWith(".css")) return "text/css";
  else if(filename.endsWith(".js")) return "application/javascript";
  else if(filename.endsWith(".png")) return "image/png";
  else if(filename.endsWith(".gif")) return "image/gif";
  else if(filename.endsWith(".jpg")) return "image/jpeg";
  else if(filename.endsWith(".webp")) return "image/webp";
  else if(filename.endsWith(".ico")) return "image/x-icon";
  else if(filename.endsWith(".xml")) return "text/xml";
  else if(filename.endsWith(".json")) return "text/json";
  else if(filename.endsWith(".pdf")) return "application/x-pdf";
  else if(filename.endsWith(".zip")) return "application/x-zip";
  else if(filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}

bool __impl_handleFileRead(String path) {
  PRINTF("REQUEST FILE: %s\n", path.c_str());
  if (path.endsWith("/"))
    path += "index.html";

  String contentType = __impl_getContentType(path);
  if (!LittleFS.exists(path)) {
    path += ".gz";
    if (!LittleFS.exists(path))
      return false;
  }
  File file = LittleFS.open(path, "r");
  (void) server.streamFile(file, contentType);
  file.close();
  PRINTF("SEND FILE: %s\n", path.c_str());
  return true;
}

void __impl_webSocketSendFreq(uint16_t f) {
  char *pdata = new char[FREQ_INFO_SIZE]{};
  char *odata = new char[((FREQ_INFO_SIZE * 2) + 3)]{};
  radio.formatFrequency(pdata, FREQ_INFO_SIZE);
  sprintf(odata, "#f%s|%u", pdata, f);
  webSocket.broadcastTXT(odata);
  delete [] pdata;
  delete [] odata;
}
void __impl_webSocketSendCmd(const char *cmd) {
  webSocket.broadcastTXT(cmd);
}

void __impl_webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t lenght) {
  switch (type) {
    case WStype_DISCONNECTED: {
      Serial.printf("[%u] Disconnected!\n", num);
      break;
    }
    case WStype_CONNECTED: {
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
        webSocket.broadcastTXT("#r");
        break;
    }
    case WStype_TEXT: {
      if ((lenght < 2) || (payload[0] != '#'))
        break;
      if (lenght == 2) {
        switch(payload[1]) {
          case 'f': {
            if (freqjson.isScan)
              freqjson.isScan = false;
            else
              freqjson.isScan = true;
            break;
          }
          case 'w': {
            freqjson.save(LittleFS.open(RADIO_SCAN_LIST_FILE, "w"));
            LittleFS.end();
            delay(1000);
            LittleFS.begin();
            break;
          }
          case 'd': {
            freqjson.remove();
            break;
          }
          case 'u': {
            freqjson.update();
            break;
          }
          case 'a': {
            freqjson.setAutoTune();
            break;
          }
          case 'q': {
            /// quit power .. todo
            break;
          }
        }
      }
      ///
      else if (lenght == 3) {
        uint8_t choice = 0U;
        switch(payload[2]) {
          case '+': {
            choice = 1U;
            break;
          }
          case '-': {
            choice = 2U;
            break;
          }
        }
        if (!choice)
          break;

        switch(payload[1]) {
          case 'v': {
            int8_t x = radio.getVolume();
            (void) ((choice == 1U) ? ++x : --x);
            x = ((x >= 15) ? 15 : ((x <= 0) ? 0 : x));
            radio.setVolume((uint8_t)x);
            break;
          }
          case 'r': {
            (void)((choice == 1U) ? freqjson.next() : freqjson.prev());
            break;
          }
          case 's': {
            (void)((choice == 1U) ? radio.seekUp() : radio.seekDown());
            break;
          }
          case 't': {
            (void)((choice == 1U) ? freqjson.seek(true) : freqjson.seek(false));
            break;
          }
          case 'm': {
            (void)((choice == 1U) ? radio.setMute(true) : radio.setMute(false));
            break;
          }
          case 'M': {
            (void)((choice == 1U) ? radio.setMono(true) : radio.setMono(false));
            break;
          }
        }
      }
      ///
      else if (lenght > 3) {
        switch(payload[1]) {
          case 'p': {
            payload += 2;
            uint16_t f = (uint16_t) atoi((const char*)payload);
            freqjson.play(f);
            break;
          }
        }
      }
      ///
      break;
    }
  }
}

void __impl_commandHandler(uint8_t c) {
    switch(c) {
      case '+': {
        freqjson.seek(true);
        PRINTF("Freq: %u\n", freqjson.freq);
        break;
      }
      case '-': {
        freqjson.seek(false);
        PRINTF("Freq: %u\n", freqjson.freq);
        break;
      }
      case '>': {
        freqjson.next();
        PRINTF("Freq >> %u\n", freqjson.freq);
        break;
      }
      case '<': {
        freqjson.prev();
        PRINTF("Freq << %u\n", freqjson.freq);
        break;
      }
      case 'd': {
        freqjson.remove();
        Serial.println(F("Station deleted.."));
        Serial.flush();
        break;
      }
      case 'u': {
        freqjson.update();
        PRINTF("Station updated.. %u\n", freqjson.freq);
        Serial.flush();
        break;
      }
      case 'l': {
        freqjson.printAll();
        freqjson.toString();
        Serial.flush();
        break;
      }
      case 'w': {
        freqjson.save(LittleFS.open(RADIO_SCAN_LIST_FILE, "w"));
        LittleFS.end();
        delay(1000);
        LittleFS.begin();
        __impl_getFileList();
        Serial.flush();
        break;
      }
      case 'p': {
        uint16_t f = Serial.parseInt();
        if ((f > 10800) || (f < 7600))
          break;
        freqjson.play(f);
        break;
      }
      case 'f': {
        uint16_t f = radio.getFrequency();
        Serial.print(F("Currently tuned to "));
        Serial.print(f / 100);
        Serial.print(".");
        Serial.print(f % 100);
        Serial.println(F(" MHz"));
        freqjson.toString();
        Serial.flush();
        break;
      }
      case 's': {
        Serial.print(F("Scan .."));
        if (freqjson.isScan) {
          freqjson.isScan = false;
          Serial.println(F(" STOP"));
          __impl_commandHandler('f');
        } else {
          Serial.println(F(" START"));
          freqjson.isScan = true;
          //freqjson.scan();
          //freqjson.save();
        }
        break;
      }
      case 'm': {
        radio.setMute(!radio.getMute());
        break;
      }
      case 'M': {
        radio.setMono(!radio.getMono());
        break;
      }
      case 'q': {
        /*
        Serial.print(F("RSSI = "));
        Serial.print(radio.getRSSI());
        Serial.println("dBuV");
        Serial.flush();
        */
        break;
      }
      case 't': {
        /*
        status = radio.getRegister(RDA5807M_REG_STATUS);
        Serial.println(F("Status register {"));
        if(status & RDA5807M_STATUS_RDSR)
            Serial.println(F("* RDS Group Ready"));
        if(status & RDA5807M_STATUS_STC)
            Serial.println(F("* Seek/Tune Complete"));
        if(status & RDA5807M_STATUS_SF)
            Serial.println(F("* Seek Failed"));
        if(status & RDA5807M_STATUS_RDSS)
            Serial.println(F("* RDS Decoder Synchronized"));
        if(status & RDA5807M_STATUS_BLKE)
            Serial.println(F("* RDS Block E Found"));
        if(status & RDA5807M_STATUS_ST)
            Serial.println(F("* Stereo Reception"));
        Serial.println("}");
        Serial.flush();
        */
        break;
      }
    }
}
