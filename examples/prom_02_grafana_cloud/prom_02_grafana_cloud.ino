
#include <bearssl_x509.h>
#include "config.h"
#include "certificates.h"
#include <Prometheus.h>

Prometheus client;

// Create a write request for 2 series.
WriteRequest req(2);

// Check out https://prometheus.io/docs/practices/naming/ for metric naming and label conventions.
// This library does not currently create different metric types like gauges, counters, and histograms
// however internally Prometheus does not differentiate between these types, rather they are both
// a naming convention and a usage convention so it's possible to create any of these yourself.
// See the README at https://github.com/grafana/prometheus-arduino for more info.

// Define common labels.
LabelSet job = { "job", "esp32-test" };
LabelSet host = { "host","esp32" };

// Create a labelset array for the 2 labels.
LabelSet series1[] = { job, host };
// Define a TimeSeries which can hold up to 5 samples, has a name of `uptime_milliseconds`, uses the above labels of which there are 2
TimeSeries ts1(5, "uptime_milliseconds_total", series1, 2);

// Create a labelset array for the second timeseries but add a third label.
LabelSet series2[] = { job, host, {"foo","bar"} };
// Define a TimeSeries which can hold up to 5 samples, has a name of `heap_free_bytes`, uses the above labels which there are 3
TimeSeries ts2(5, "heap_free_bytes", series2, 3);

// Note, metrics with the same name and different labels are actually different series and you would need to define them separately
// LabelSet series3[] = { job, host, {"foo","bar2"} };
// TimeSeries ts3(5, "heap_free_bytes", series3, 3);

int loopCounter = 0;

void setup() {
    Serial.begin(115200);
    while (!Serial)
        delay(10);

    Serial.println("Starting");

    // Configure the client
    client.setUrl(GC_URL);
    client.setPath(GC_PATH);
    client.setPort(GC_PORT);
    client.setUser(GC_USER);
    client.setPass(GC_PASS);
    client.setUseTls(true);
    client.setCerts(TAs, TAs_NUM);
    client.setWifiSsid(WIFI_SSID);
    client.setWifiPass(WIFI_PASSWORD);
    client.setDebug(Serial);  // Remove this line to disable debug logging of the client.
    if (!client.begin()){
        Serial.println(client.errmsg);
    }

    // Add our TimeSeries to the WriteRequest
    req.addTimeSeries(ts1);
    req.addTimeSeries(ts2);
    req.setDebug(Serial);  // Remove this line to disable debug logging of the write request serialization and compression.

};



void loop() {
    int64_t time;
    time = client.getTimeMillis();
    Serial.println(time);

    // Efficiency in requests can be gained by batching writes so we accumulate 5 samples before sending.
    // This is not necessary however, especially if your writes are infrequent, but it's recommended to build batches when you can.
    if (loopCounter >= 5) {
        //Send
        loopCounter = 0;
        if (!client.send(req)) {
            Serial.println(client.errmsg);
            // Note: additional retries or error handling could be implemented here.
        }
        // Batches are not automatically reset so that additional retry logic could be implemented by the library user.
        // Reset batches after a succesful send.
        ts1.resetSamples();
        ts2.resetSamples();
    }
    else {
        if (!ts1.addSample(time, millis())) {
            Serial.println(ts1.errmsg);
        }
        if (!ts2.addSample(time, freeMemory())) {
            Serial.println(ts2.errmsg);
        }
        loopCounter++;
    }

    delay(500);

};