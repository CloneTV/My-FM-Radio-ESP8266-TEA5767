#if !defined(__CONFIG_H)
# define __CONFIG_H 1

#define DEBUG 0
#define NODEBUG_WEBSOCKETS 1
#define EEPROM_STORE_ENABLE 0
#define SERIAL_CMD_ENABLE 0

#define STASSID "MYSSID"
#define STAPSK  "12345678"

#define AIO_SERVER     "io.adafruit.com"
#define AIO_SERVERPORT 1883 // use 8883 for SSL
#define AIO_USERNAME   "user"
#define AIO_KEY        "password"

#define RADIO_SCAN_LIST_FILE F("/json/AutoScanList.json")

#define FN__(A,B) A ## B
#define FN_(A,B) FN__(A,B)
#define UNIQUE_PROGMEM(A) FN_(prn_,A)

# if (defined(DEBUG) && (DEBUG == 1))
#  define SERIAL_INIT() Serial.begin(115200)
#  define PRINT(A) Serial.print(F(A))
#  define PRINTF(A, ...) Serial.printf(A, __VA_ARGS__); Serial.flush()
#  define PRINTLN(A) Serial.println(F(A)); Serial.flush()
#  define PRINTWIFI() WiFi.printDiag(Serial)
# else
#  define SERIAL_INIT()
#  define PRINT(A)
#  define PRINTF(A, ...)
#  define PRINTLN(A)
#  define PRINTWIFI()
# endif

#endif
