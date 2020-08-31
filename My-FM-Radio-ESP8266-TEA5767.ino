
#include "config.h"

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
#include "DEBUGSTRING.h"
#include "RADIOCTRL.h"

const PROGMEM char err_500_1[] = "500: not support this file type, JSON only..";
const PROGMEM char err_500_2[] = "500: couldn't create file";

TEA5767 radio{};
ESP8266WebServer server(80);
WebSocketsServer webSocket(81);

RDSParser rds{};
RADIOCTRL radioctrl(200);
fs::File fsu{};

void setup() {
  SERIAL_INIT();
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
             PRINTLN("SmartConfig Success");
             break;
           }
       }
    }
  }
  PRINTWIFI();
  MDNS.begin("fm-radio");
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

  PRINTLN("Radio...");
  radio.init();
  radioctrl.setRadio(&radio);
  radio.setBandFrequency(RADIO_BAND_FM, 9140);
  radio.setVolume(10);
  radio.setMono(false);
  radio.attachReceiveRDS(__impl_getRDS);
  rds.attachServicenNameCallback(__impl_getRDSName);

  PRINTF(UNIQUE_PROGMEM(2), WiFi.localIP().toString().c_str());
  if (LittleFS.exists(RADIO_SCAN_LIST_FILE)) {
    fs::File f = LittleFS.open(RADIO_SCAN_LIST_FILE, "r");
    if (f)
      radioctrl.load(f);
  }

# if (defined(DEBUG) && (DEBUG == 1))
  radio.debugEnable();
  radio.debugStatus();
  FSInfo fs_info;
  LittleFS.info(fs_info);
  PRINTF(UNIQUE_PROGMEM(3),
    fs_info.totalBytes,
    fs_info.usedBytes
  );
  __impl_getFileList();
# endif

}

void loop() {

  webSocket.loop();
  server.handleClient();

# if (defined(SERIAL_CMD_ENABLE) && (SERIAL_CMD_ENABLE == 1))
  if (Serial.available())
    __impl_serialCmdHandler(Serial.read());
# endif

  if (!radioctrl.isScan) {
    return;
  }
  if (!radioctrl.scan()) {
    radioctrl.save(LittleFS.open(RADIO_SCAN_LIST_FILE, "w"));
    webSocket.broadcastTXT("#e");
    return;
  }
}

void __impl_getRDS(uint16_t block_1, uint16_t block_2, uint16_t block_3, uint16_t block_4) {
  radioctrl.setRds(block_1);
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
# if (defined(DEBUG) && (DEBUG == 1))
  fs::Dir dir = LittleFS.openDir("/");
  while (dir.next()) {
    PRINTF(UNIQUE_PROGMEM(4), dir.fileName().c_str());
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
    PRINTF(UNIQUE_PROGMEM(5), fname.c_str());
    fsu = LittleFS.open(fname, "w");
    fname = String();
  
  } else if(upload.status == UPLOAD_FILE_WRITE) {
    if (fsu)
      fsu.write(upload.buf, upload.currentSize);
  } else if (upload.status == UPLOAD_FILE_END) {
    if (fsu) {
      fsu.close();
      PRINTF(UNIQUE_PROGMEM(6), upload.totalSize);
      server.sendHeader("Location","/index.html");
      server.send(303);
    } else {
      server.send(500, "text/plain", err_500_2);
    }
  }  
}

const char * __impl_getContentType(String filename){
       if(filename.endsWith(ext_type[0])) return mime_type[1];
  else if(filename.endsWith(ext_type[1])) return mime_type[1];
  else if(filename.endsWith(ext_type[2])) return mime_type[2];
  else if(filename.endsWith(ext_type[3])) return mime_type[3];
  else if(filename.endsWith(ext_type[4])) return mime_type[4];
  else if(filename.endsWith(ext_type[5])) return mime_type[5];
  else if(filename.endsWith(ext_type[6])) return mime_type[6];
  else if(filename.endsWith(ext_type[7])) return mime_type[7];
  else if(filename.endsWith(ext_type[8])) return mime_type[8];
  else if(filename.endsWith(ext_type[9])) return mime_type[9];
  else if(filename.endsWith(ext_type[10])) return mime_type[10];
  else if(filename.endsWith(ext_type[11])) return mime_type[11];
  else if(filename.endsWith(ext_type[12])) return mime_type[12];
  else if(filename.endsWith(ext_type[13])) return mime_type[13];
  return mime_type[0];
}

bool __impl_handleFileRead(String path) {
  PRINTF(UNIQUE_PROGMEM(7), path.c_str());
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
  PRINTF(UNIQUE_PROGMEM(8), path.c_str());
  return true;
}

void __impl_webSocketSendFreq(uint16_t f) {
  char *pdata = new char[FREQ_INFO_SIZE]{};
  char *odata = new char[((FREQ_INFO_SIZE * 2) + 3)]{};
  radio.formatFrequency(pdata, FREQ_INFO_SIZE);
  sprintf(odata, UNIQUE_PROGMEM(1), pdata, f);
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
      PRINTF(UNIQUE_PROGMEM(9), num);
      break;
    }
    case WStype_CONNECTED: {
        IPAddress ip = webSocket.remoteIP(num);
        PRINTF(UNIQUE_PROGMEM(10),
          num, ip[0], ip[1], ip[2], ip[3], payload
        );
        webSocket.broadcastTXT("#r");
        break;
    }
    case WStype_TEXT: {
      if ((lenght < 2) || (payload[0] != '#'))
        break;
      if (lenght == 2) {
        switch(payload[1]) {
          case 'f': {
            if (radioctrl.isScan)
              radioctrl.isScan = false;
            else
              radioctrl.isScan = true;
            break;
          }
          case 'w': {
            radioctrl.save(LittleFS.open(RADIO_SCAN_LIST_FILE, "w"));
            LittleFS.end();
            delay(1000);
            LittleFS.begin();
            break;
          }
          case 'd': {
            radioctrl.remove();
            break;
          }
          case 'u': {
            radioctrl.update();
            break;
          }
          case 'a': {
            radioctrl.setAutoTune();
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
            uint8_t x = radio.getVolume();
            (void) ((choice == 1U) ? ++x : --x);
            x = ((x >= 15) ? 15 : ((x <= 0) ? 0 : x));
            radio.setVolume((uint8_t)x);
            break;
          }
          case 'r': {
            (void)((choice == 1U) ? radioctrl.next() : radioctrl.prev());
            break;
          }
          case 's': {
            (void)((choice == 1U) ? radio.seekUp() : radio.seekDown());
            break;
          }
          case 't': {
            radioctrl.seek((choice == 1U));
            break;
          }
          case 'm': {
            radio.setMute((choice == 1U));
            break;
          }
          case 'M': {
            radio.setMono((choice == 1U));
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
            radioctrl.play(f);
            break;
          }
        }
      }
      ///
      break;
    }
  }
}

#if (defined(SERIAL_CMD_ENABLE) && (SERIAL_CMD_ENABLE == 1))
void __impl_serialCmdHandler(uint8_t c) {
    switch(c) {
      case '+': {
        radioctrl.seek(true);
        PRINTF(UNIQUE_PROGMEM(12), radioctrl.freq);
        break;
      }
      case '-': {
        radioctrl.seek(false);
        PRINTF(UNIQUE_PROGMEM(12), radioctrl.freq);
        break;
      }
      case '>': {
        radioctrl.next();
        PRINTF(UNIQUE_PROGMEM(13), radioctrl.freq);
        break;
      }
      case '<': {
        radioctrl.prev();
        PRINTF(UNIQUE_PROGMEM(14), radioctrl.freq);
        break;
      }
      case 'd': {
        radioctrl.remove();
        PRINTLN("-- Station deleted..");
        break;
      }
      case 'u': {
        radioctrl.update();
        PRINTF(UNIQUE_PROGMEM(15), radioctrl.freq);
        break;
      }
      case 'l': {
        radioctrl.printAll();
        radioctrl.toString();
        break;
      }
      case 'w': {
        radioctrl.save(
          LittleFS.open(RADIO_SCAN_LIST_FILE, "w")
        );
        LittleFS.end();
        delay(1000);
        LittleFS.begin();
        __impl_getFileList();
        break;
      }
      case 'p': {
        uint16_t f = Serial.parseInt();
        if ((f > 10800) || (f < 7600))
          break;
        radioctrl.play(f);
        break;
      }
      case 'f': {
        uint16_t f = radio.getFrequency();
        PRINTF(UNIQUE_PROGMEM(16),
          (f / 100), (f % 100) 
        );
        radioctrl.toString();
        break;
      }
      case 's': {
        PRINT("-- Scan ..");
        if (radioctrl.isScan) {
          radioctrl.isScan = false;
          PRINTLN(" STOP");
          __impl_serialCmdHandler('f');
        } else {
          PRINTLN(" START");
          radioctrl.isScan = true;
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
      case 't': {
        /*
        status = radio.getRegister(RDA5807M_REG_STATUS);
        PRINTLN("Status register {");
        if(status & RDA5807M_STATUS_RDSR)
            PRINTLN("* RDS Group Ready");
        if(status & RDA5807M_STATUS_STC)
            PRINTLN("* Seek/Tune Complete");
        if(status & RDA5807M_STATUS_SF)
            PRINTLN("* Seek Failed");
        if(status & RDA5807M_STATUS_RDSS)
            PRINTLN("* RDS Decoder Synchronized");
        if(status & RDA5807M_STATUS_BLKE)
            PRINTLN("* RDS Block E Found");
        if(status & RDA5807M_STATUS_ST)
            PRINTLN("* Stereo Reception");
        PRINTLN("}");
        */
        break;
      }
    }
}
#endif
