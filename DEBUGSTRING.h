
#if !defined(__DEBUGSTRING_H)
# define __DEBUGSTRING_H 1

  const PROGMEM char prn_1[] = "#f%s|%u";

# if (defined(DEBUG) && (DEBUG == 1))
  const PROGMEM char prn_2[] = "-- Begin, wait for you.. (%s)\n";
  const PROGMEM char prn_3[] = "-- RAM space: total=%u byte, used=%u byres\n";
  const PROGMEM char prn_4[] = "\tFILE: %s\n";
  const PROGMEM char prn_5[] = "-- Upload: %s\n";
  const PROGMEM char prn_6[] = "-- Upload finish, size: %u\n";
  const PROGMEM char prn_7[] = "\tREQUEST FILE: %s\n";
  const PROGMEM char prn_8[] = "\tSEND FILE: %s\n";
  const PROGMEM char prn_9[] = "-- [%u] Disconnected!\n";
  const PROGMEM char prn_10[] = "-- [%u] Connected from %d.%d.%d.%d url: %s\n";
# endif

# if (defined(SERIAL_CMD_ENABLE) && (SERIAL_CMD_ENABLE == 1))
  const PROGMEM char prn_12[] = "-- Freq: %u\n";
  const PROGMEM char prn_13[] = "-- Freq >> %u\n";
  const PROGMEM char prn_14[] = "-- Freq << %u\n";
  const PROGMEM char prn_15[] = "-- Station updated.. %u\n";
  const PROGMEM char prn_16[] = "-- Currently tuned to %u.%u MHz\n";
# endif

  const PROGMEM char *mime_type[] = {
    "text/plain", "text/html", "text/css", "application/javascript",
    "image/png", "image/gif", "image/jpeg", "image/webp", "image/x-icon",
    "text/xml", "text/json", 
  };
  const PROGMEM char *ext_type[] = {
    ".htm", ".html", ".css", ".js", ".png", ".gif", ".jpg", ".webp", ".ico",
    ".xml", ".json", ".pdf", ".zip", ".gz", "application/x-pdf",
    "application/x-zip", "application/x-gzip"
  };

#endif

