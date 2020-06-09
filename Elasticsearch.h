#include <HTTPClient.h>

class ElasticsearchClient {
  private:
    String _host;
    String _index;
    HTTPClient _http;
  public:
    ElasticsearchClient(String host, String index);
    void sendLog(String timestamp, String feature, String message);
    void sendMetric(String timestamp, String feature, String counter, int zone, String value, String index);
    void sendMetric(String timestamp, String feature, String counter, int zone, float value, String index);
};
