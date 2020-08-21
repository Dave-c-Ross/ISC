
void toogleSp(int pin, bool state);

void cloneWebInterface();
void initWebserver();

void writeLog(String feature, String message);
void writeMetric(String feature, String metric, float value, int zone);
void writeMetric(String feature, String metric, String value, int zone);

time_t getNtpTime();
void sendNTPpacket(IPAddress &address);
void sendRecuringMetric();
