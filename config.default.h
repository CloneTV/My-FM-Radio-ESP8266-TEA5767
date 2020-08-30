#define DBG_PRINT 1
#define BMP280_ENABLE 1
#define SSD1306_ENABLE 1

#define STASSID "MYSSID"
#define STAPSK  "12345678"

#define AIO_SERVER     "io.adafruit.com"
#define AIO_SERVERPORT 1883 // use 8883 for SSL
#define AIO_USERNAME   "user"
#define AIO_KEY        "password"

#define RADIO_SCAN_LIST_FILE F("/json/AutoScanList.json")

#define FN__(A,B) A ## B
#define FN_(A,B) FN__(A,B)
#define UNIQUE_STRING(A) PROGMEM const char FN_(A,__LINE__)[]

#if defined(DEBUG)
#  define PRINTF(A, ...) Serial.printf(A, __VA_ARGS__)
#else
#  define PRINTF(A, ...)
#endif

  
