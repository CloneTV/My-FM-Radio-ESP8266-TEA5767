#include <EEPROM.h>
#define FREQ_MIN (uint16_t) 7600U
#define FREQ_MAX (uint16_t) 10800U
#define FREQ_INFO_SIZE 12

void __impl_webSocketSendFreq(uint16_t);
void __impl_webSocketSendCmd(const char*);

typedef struct FREQJSON {
  private:
    TEA5767  *radio = NULL;
    uint16_t *arr = NULL;
    uint16_t index = 0U, count, freq_last = 0U, rds_block = 0U;
    bool autotune = true;

    void send_client() {
      __impl_webSocketSendFreq(radio->getFrequency());
    }
    void seek_station(bool b) {
      cursor = ((b) ?
        ((cursor < index) ? ++cursor : 0U) :
        ((cursor > 0U) ? --cursor : index)
      );
      freq = arr[cursor];
      if (autotune)
        autoTune(freq);
      else
        radio->setFrequency(freq);
      send_client();
    }
    void seek_frequency(bool b) {
      freq = ((b) ? (freq + step) : (freq - step));
      if (!valid_frequency(freq))
        return;
      arr[cursor] = freq;
      radio->setFrequency(freq);
      send_client();
    }
    void seek_user(uint16_t f) {
      if (!valid_frequency(f))
        return;
      freq = f;
      radio->setFrequency(freq);
      send_client();
    }
    bool valid_frequency(uint16_t f) {
      return ((f >= FREQ_MIN) && (f <= FREQ_MAX));
    }

    /// Auto Tune

    uint16_t getRssi(uint16_t f) {
      RADIO_INFO ri{};
      radio->setFrequency(freq);
      delay(500);
      uint16_t wait_offset = millis();
      do {
        radio->getRadioInfo(&ri);
      } while ((!ri.tuned) && (wait_offset + 800 > millis()));
      return ri.rssi;
    }

    void autoTune(uint16_t f) {
      uint16_t rssc,
               rssi = getRssi(f),
               freq = (f + 5),
               cnt = 0;
      do {
        rssc = getRssi(freq);
        if (!valid_frequency(f))
          break;
        if (rssc > rssi) {
          rssi = rssc;
          f = freq;
          freq += 5;
        } else {
          break;
        }
      } while (true);

      freq = (f - 5);

      do {
        rssc = getRssi(freq);
        if (!valid_frequency(f))
          break;
        if (rssc > rssi) {
          rssi = rssc;
          f = freq;
          freq -= 5;
        } else {
          break;
        }
      } while (true);

      arr[cursor] = f;
      radio->setFrequency(f);
    }
  
  public:
    uint16_t step = 5U, freq = FREQ_MIN, cursor = 0U;
    bool isScan = false;
    
    ~FREQJSON() {
      clear();
    }
    FREQJSON(uint16_t cnt) {
      count = cnt;
      arr = new uint16_t[cnt];
    }
    void setAutoTune() {
      autotune = !autotune;
    }
    void setRadio(TEA5767 *r) {
      radio = r;
    }
    void setRds(uint16_t bl) {
      rds_block = bl;
    }
    void clear() {
      if (arr != NULL)
        delete [] arr;
    }
    bool add(uint16_t f) {
      if ((index >= count) || (!valid_frequency(f)))
        return ((isScan = false));
      arr[index] = freq = f;
      ++index;

#     if defined(DEBUG_ADDER)
      char *odata = new char[9]{};
      sprintf(odata, "#i%d|%d", index, f);
      __impl_webSocketSendCmd(odata);
      delete [] odata;
#     endif     

      return true;
    }
    void update() {
      arr[index] = freq = radio->getFrequency();
    }
    void remove() {
      freq_last = arr[index];
      for (uint16_t i = index; i < count; i++) {
        PRINTF("remove %u/%u (%u) [%u|%u]\n", index, count, i, arr[i], arr[(i + 1)]);
        arr[i] = arr[(i + 1)];
      }
      --count;
      freq = arr[index];
      radio->setFrequency(freq);
      send_client();
    }
    void prev() {
      seek_station(false);
    }
    void next() {
      seek_station(true);
    }
    void play(uint16_t f) {
      seek_user(f);
    }
    void seek(bool b) {
      seek_frequency(b);
    }
    void save(fs::File file) {
      if ((!file) || (index == 0)) {
        PRINTF("Saved failed.. %u\n", index);
        return;
      }
      file.print("{\"freqs\":[");
      for (uint16_t i = 0; i < index; i++) {
        file.print(arr[i]);
        if (i < (index - 1))
          file.print(',');
      }
      file.print("]}\n");
      file.close();
      EEPROM.begin(4096);
      EEPROM.put(0, freq);
      EEPROM.put(2, freq_last);
      EEPROM.commit();
      EEPROM.end();
    }
    void load(fs::File file) {
      if (!file) {
        PRINTF("Load failed.. %u\n", index);
        return;
      }
      size_t const len = file.size();
      if (len == 0)
        return;

      char *buff = new char[len + 1];
      while (file.available()) {
        size_t sz = file.readBytes(buff, len);
      }
      file.close();
      buff[len] = 0;

      char entry[6]{};
      bool isStart = false, isEntryBegin = false;
      for (size_t i = 0, n = 0; i < len; i++) {
        
        if (buff[i] == '[') {
          isStart = true;
          isEntryBegin = true;
          continue;
        }
        if ((isStart) && (buff[i] == ',')) {
          uint16_t f = (uint16_t) atoi(entry);
          if (f > 0U)
            add(f);
          n = 0;
          isEntryBegin = true;
          continue;
        }
        if (isEntryBegin) {
          if (n >= 5) {
            n = 0;
            isEntryBegin = false;
            continue;
          }
          entry[n++] = buff[i];
        }
      }
      delete [] buff;
      EEPROM.begin(4096);
      freq = EEPROM.get(0, freq);
      freq_last = EEPROM.put(2, freq_last);
      EEPROM.end();
      if (valid_frequency(freq)) {
        radio->setFrequency(freq);
        send_client();
      } else {
        freq = FREQ_MIN;
      }
    }
    bool scan() {
      if (!radio)
        return ((isScan = false));
        
      RADIO_INFO ri{};
      freq += step;
      if (!valid_frequency(freq))
        return ((isScan = false));

      radio->setFrequency(freq);
      send_client();
      delay(500);
      uint16_t wait_offset = millis();

      do {
        radio->getRadioInfo(&ri);
      } while ((!ri.tuned) && (wait_offset + 800 > millis()));

      if ((ri.rssi > 10) && (freq > (freq_last + 20))) {
        freq_last = freq;
        rds_block = 0U;
        
        do {
          radio->checkRDS();
        } while ((rds_block == 0U) && (wait_offset + 1200 > millis()));

#       if defined(DEBUG)
        char *pdata = new char[FREQ_INFO_SIZE]{};
        radio->formatFrequency(pdata, FREQ_INFO_SIZE);
        PRINTF("Station -> [%u] %s, RSSI: %u, SNR: %u, RDS: %s, %s [%s]\n",
          freq, pdata, ri.rssi, ri.snr,
          ((ri.rds) ? F("YES") : F("NO")),
          ((ri.tuned) ? F("TUNED") : F("-")),
          ((ri.mono) ? F("MONO") : ((ri.stereo) ? F("STEREO") : F("UNKNOWN")))
        );
        delete [] pdata;
#       endif
        return add(radio->getFrequency());
      }
      return true;
    }
    void printAll() {
      for (uint16_t i = 0; i < index; i++)
        PRINTF("Stored read = %u [%u]\n", arr[i], i);
    }
    void toString() {
      PRINTF("Debug: step=%u, index=%u, count=%u, cursor=%u, freq=%u\n",
        step, index, count, cursor, freq
      );
    }

} FREQJSON;
