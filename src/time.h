#include "lib.h"

extern WiFiUDP ntpUDP;
extern NTPClient timeClient;

void initializeTime();

unique_ptr<char> currentTimeStamp();
