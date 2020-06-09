#include "Elasticsearch.h"

ElasticsearchClient::ElasticsearchClient(String host, String index) {
  _host = host;
  _index = index;
}

void ElasticsearchClient::sendLog(String timestamp, String feature, String message) {
  _http.begin(_host + "/" + _index + "/_doc/");
  _http.addHeader("Content-Type", "application/json");
  int httpResponseCode = _http.POST("{\"@timestamp\":" + timestamp + ",\"feature\":\""+ feature +"\",\"message\":\"" + message + "\"}");
}

void ElasticsearchClient::sendMetric(String timestamp, String feature, String metric, int zone, String value, String index)
{
  _http.begin(_host + "/" + index + "/_doc/");
  _http.addHeader("Content-Type", "application/json");
  int httpResponseCode = _http.POST("{\"@timestamp\":" + timestamp + ",\"feature\":\""+ feature +"\",\"metric\":\"" + metric + "\",\"zone\":\"" + zone + "\",\"value\":\"" + value + "\"}");
}

void ElasticsearchClient::sendMetric(String timestamp, String feature, String metric, int zone, float value, String index)
{
  _http.begin(_host + "/" + index + "/_doc/");
  _http.addHeader("Content-Type", "application/json");
  int httpResponseCode = _http.POST("{\"@timestamp\":" + timestamp + ",\"feature\":\""+ feature +"\",\"metric\":\"" + metric + "\",\"zone\":\"" + zone + "\",\"value\":" + value + "}");
}
