#include <time.h>

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

void initializeTime()
{
    timeClient.begin();
    timeClient.setTimeOffset(-21600); //mountain standered time offset in seconds.
}

unique_ptr<char> currentTimeStamp()
{
    String time = timeClient.getFormattedTime();
    String formatElements = " -> ";
    String formatedTime = time + formatElements;
    unique_ptr<char> curTime(new char[sizeof(formatedTime)+1]); 
    formatedTime.toCharArray(curTime.get(), sizeof(formatedTime)+1);
    return curTime;
}