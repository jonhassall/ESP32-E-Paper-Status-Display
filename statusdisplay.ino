//EInk
#include <SPI.h>
#include "epd2in7.h"
#include "imagedata.h"
#include "epdpaint.h"

#define COLORED     0
#define UNCOLORED   1

#include <WiFi.h>
#include <WiFiMulti.h>

#include <Arduino.h>
#include <HTTPClient.h>

#include <ArduinoJson.h>

int margin = 5; //Pixel margins

RTC_DATA_ATTR int bootCount = 0;
#define uS_TO_S_FACTOR 1000000ULL  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  30        /* Time ESP32 will go to sleep (in seconds) */

//Root Certificate Authority that signed the HTTPS server certificate
//This example is a certificate for LetsEncrypt
const char* root_ca = \
  "-----BEGIN CERTIFICATE-----\n" \
  "MIIEtjCCAp6gAwIBAgIRAOSLLZlzkiCW3s5KSK7GfFEwDQYJKoZIhvcNAQELBQAw\n" \
  "TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh\n" \
  "cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTIwMDQw\n" \
  "WhcNMjAwNjA0MTIwMDQwWjBUMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu\n" \
  "ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxGjAYBgNVBAMTEUlTUkcgUm9vdCBP\n" \
  "Q1NQIFgxMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAuw0cR9Li4+M+\n" \
  "aIJixENnV4PM9N8nAxwWsM/7PzV/766q/1PKA8jB4OykscNkK9XCblOElSzXSQJx\n" \
  "BrpckIquoydslakPvaB4HLj3cx8EJP4tEyXRDt415uZs9LWFSoplSLBFNC2gMfL7\n" \
  "WYxPqcoOagU+amCVSEDK85oILqnZ27FJrU2hQGOF/lWDa1y1YiIp9e2+ryFOUn1w\n" \
  "AVWQdnOyovh6suBnjCcR+269q6Xtf3/fUHjqnOgO7e8XMDy69MygLltOzDxI0/VA\n" \
  "21EL1kBoC2ckgorVASrKByaPS9o6p2bYcHZ3FC/3g+tv6pCiFZt+e4YMBnYVyAYC\n" \
  "miJVv7PFtQIDAQABo4GHMIGEMA4GA1UdDwEB/wQEAwIHgDAMBgNVHRMBAf8EAjAA\n" \
  "MB0GA1UdDgQWBBQfuyfyJnprzPH1dNmWK34Z7MMcIzATBgNVHSUEDDAKBggrBgEF\n" \
  "BQcDCTAfBgNVHSMEGDAWgBR5tFnme7bl5AFzgAiIyBpY9umbbjAPBgkrBgEFBQcw\n" \
  "AQUEAgUAMA0GCSqGSIb3DQEBCwUAA4ICAQB6baTUbmQI6I2/BPB2gDsxRMSeppGM\n" \
  "g5lcDQE4aP7oPDcRq+S+6LuuwVjj67UZfJqczCgcJRcG/6kMjBq7jcg6DmhVP8V6\n" \
  "9f47vri57ikQLQge4yibUkMpmaT+IaUKklSQhA2ZAsFRd638DW+wNlkp8FpORz0K\n" \
  "1BymHOYq6xpRyEW5piZhi7ZCrR4x1k7uGChpYP+X79xmht4FUpJRbVuHtnjjZTGH\n" \
  "BIrZj+n2iIPTrjBn5fpfmOb+ABlV9ovvxUYql6cpQmvYzelvoA7tZ0EJf0nwF+FS\n" \
  "orbSX1CiTLXGnvoGun84UXOLY2Thgf8SVGESaeR9cQJBHi8w1ksK++36SECauTNP\n" \
  "hcegMGEdxlHPc3yrP3EcC+rqNdLRWCNkSnnLXFxWlk9JJayzOuF/HDObtJB54AEp\n" \
  "M7ly58PdQD1MGQ4GBFTsXN28sqfzpwl/RQGmeHz7EVPb5hHoW8z+CwUtbTrNhYNc\n" \
  "FMnjGFnZCHsbTjuOMGN4koyZtpRnhpHs0LgmsYeofiECtbKUvSL1DB1DTkjitM7s\n" \
  "qjFfubGSKwAGJZFLi0EwLzUe+DZvYagZBT/Upp/Ush9ImwzKZirqVfA6/qgFOLST\n" \
  "NdCsat0uo+FX1klsPm6enps9WCOFpkIppbf6WAUv+myiDOY2jh6GpVsc07qDVZFG\n" \
  "89JKdHDn7q5TsQ==\n" \
  "-----END CERTIFICATE-----\n";

Epd epd; //e-Paper

const char* apiURL = "https://raw.githubusercontent.com/jonhassall/ESP32-E-Paper-Status-Display/master/sample.json";

void setup() {
  //Serial interface
  Serial.begin(115200);
  Serial.println("Starting up...");
  delay(500);

  //Increment boot number and print it every reboot
  ++bootCount;
  Serial.println("Boot number: " + String(bootCount));

  //Print the wakeup reason for ESP32
  print_wakeup_reason();

  //Initialize e-Paper
  Serial.println("Initializing e-Paper");
  if (epd.Init() != 0) {
    Serial.print("e-Paper init failed");
    return;
  }

  //WiFi network settings
  wifiMulti.addAP("SSID", "password");

  //Only clear screen on first boot
  if (bootCount <= 1) {
    //Clear the screen in partial filled rectangles
    unsigned char filledImage[1024];

    Paint paintFilled(filledImage, 176, 8);    //width should be the multiple of 8
    paintFilled.Clear(UNCOLORED);
    paintFilled.DrawFilledRectangle(0, 0, 176, 8, COLORED);

    int i = 0;
    while (i < 33) {
      epd.TransmitPartialData(paintFilled.GetImage(), 0, i * 8, paintFilled.GetWidth(), paintFilled.GetHeight());
      i++;
      Serial.println("Clearing step");
      Serial.println(i);
    }

    //Draw the combined partial data
    Serial.println("Draw");
    epd.DisplayFrame();
  }
}

void loop() {
  //Connect to WiFi
  Serial.println("Connecting to WiFi");

  if (wifiMulti.run() == WL_CONNECTED) {
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    long rssi = WiFi.RSSI();
    Serial.print("RSSI:");
    Serial.println(rssi);
  } else {
    Serial.println("WiFi problem");
    displayError("WiFi problem");
    delay(3000);
    return;
  }

  //Grab data from HTTPS API
  Serial.println("Getting from HTTPS server");
  HTTPClient http;

  //Allocate memory for JSON
  StaticJsonDocument<1000> doc;

  http.begin(apiURL, root_ca); //Specify the URL and certificate
  int httpCode = http.GET();                                                  //Make the request

  if (httpCode > 0) { //Check for the returning code
    Serial.println("HTTP OK");
    String payload = http.getString();
    Serial.println(httpCode);
    Serial.println(payload);

    //Disconnect from WiFi to save power
    Serial.println("Disconnecting from WiFi");
    WiFi.disconnect();
    http.end(); //Free resources
    WiFi.mode(WIFI_OFF);
    btStop();
    Serial.println("Disconnected from WiFi");

    //Deserialize the JSON document
    DeserializationError error = deserializeJson(doc, payload);

    //Test if parsing succeeds.
    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.c_str());
      displayError("JSON error");
      return;
    }

    JsonObject obj = doc.as<JsonObject>();

    //Get a reference to the root array
    JsonArray arr = doc.as<JsonArray>();
    //Get the number of elements in the array
    int count = arr.size();
    Serial.println("Number of elements:");
    Serial.println(count);

    //How many bytes are used in the document
    int memoryUsed = doc.memoryUsage();
    Serial.println("Memory used:");
    Serial.println(memoryUsed);

    //Walk the JsonArray efficiently, getting each heading and value, and drawing it
    unsigned char image[1024];
    int i = 0;
    for (JsonObject repo : arr) {
      const char* title = repo["heading"];
      const char* value = repo["value"];

      /**
          Due to RAM not enough in Arduino UNO, a frame buffer is not allowed.
          In this case, a smaller image buffer is allocated and you have to
          update a partial display several times.
          1 byte = 8 pixels, therefore you have to set 8*N pixels at a time.
      */
      //Fill in heading gap
      if (i == 0) {
        Paint paint(image, 176, 8);    //width should be the multiple of 8
        paint.Clear(UNCOLORED);
        paint.DrawFilledRectangle(0, 0, 176, 8, COLORED);
        epd.TransmitPartialData(paint.GetImage(), 0, 0, paint.GetWidth(), paint.GetHeight());
      }

      int x;
      x = margin;

      int y_heading;
      y_heading = (i * (16 + 24) ) + margin;
      int y_value;
      y_value = (y_heading + 16) + margin;

      Paint paint(image, 176, 24);    //width should be the multiple of 8
      paint.Clear(COLORED);
      paint.DrawStringAt(x, 3, title, &Font16, UNCOLORED);
      epd.TransmitPartialData(paint.GetImage(), 0, y_heading, paint.GetWidth(), paint.GetHeight());

      Paint paint2(image, 176, 24);    //width should be the multiple of 8
      paint2.Clear(UNCOLORED);
      //Choose a different font depending on the length of the string
      if (strlen(value) <= 10) {
        paint2.DrawStringAt(x, 0, value, &Font24, COLORED);
      } else if (strlen(value) <= 12) {
        paint2.DrawStringAt(x, 1, value, &Font20, COLORED);
      } else if (strlen(value) <= 16) {
        paint2.DrawStringAt(x, 2, value, &Font16, COLORED);
      } else {
        paint2.DrawStringAt(x, 3, value, &Font12, COLORED);
      }
      epd.TransmitPartialData(paint2.GetImage(), 0, y_value, paint2.GetWidth(), paint2.GetHeight());

      Serial.println("Partial draw");
      i++;
    }

    //Draw e-Paper data in memory
    Serial.println("Display data");
    epd.DisplayFrame();
  } else {
    //Handle HTTPS error
    Serial.println("Error on HTTP request");
    Serial.println(httpCode);
    displayError("HTTPS error");
    delay(15000);
    return;
  }

  /* This clears the SRAM of the e-paper display */
  //  epd.ClearFrame();
  //  Serial.println("Drawing3");

  /* This clears the SRAM of the e-paper display */
  //  epd.Reset();
  epd.ClearFrame();
  /* Display deep sleep */
  epd.Sleep();
  delay(100);

  Serial.println("Deep Sleeping...");
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  Serial.println("Setup ESP32 to sleep for every " + String(TIME_TO_SLEEP) +
                 " Seconds");
  Serial.flush();
  esp_deep_sleep_start();
}


void displayError(char* message) {
  //Display an error message on the e-Paper screen
  //Display this error in an area that will be redrawn on any later success
  Serial.println("display an error");
  unsigned char image[1024];
  Paint paint(image, 176, 16);    //width should be the multiple of 8
  paint.Clear(COLORED);
  paint.DrawFilledRectangle(0, 0, 176, 16, COLORED);
  paint.DrawStringAt(0, 0, message, &Font16, UNCOLORED);
  epd.TransmitPartialData(paint.GetImage(), 0, margin, paint.GetWidth(), paint.GetHeight());
  Serial.println("Draw error");
  epd.DisplayFrame();
}

/*
  Method to print the reason by which ESP32
  has been awaken from sleep
*/
void print_wakeup_reason() {
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch (wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n", wakeup_reason); break;
  }
}
