#include <Arduino.h>
#include <SPI.h>
#include <HTTPClient.h>
#include "EPD_13in3e.h"
#include "FS.h"
#include <ArduinoJson.h>
// #include "SimpleWiFiManager.h"
#include <WiFiClientSecure.h>
#include "driver/rtc_io.h"
#include "config.h"
#include "button.h"
#include <Preferences.h>
#include <WifiCaptive.h>
#include <filesystem.h>

/* Pin Layout Description
DRIVER BOARD  <>  FireBeetle ESP32-C6
BUSY          <>  18  // E-paper busy signal input
RST           <>  14  // E-paper reset control
DC            <>  8   // Data/Command control
CS            <>  1   // Chip select control
SCLK          <>  23  // SPI clock
DIN           <>  22  // SPI data input
GND           <>  GND // Ground
VCC           <>  3V3 // Power supply
SETTING       <>  2  // Configuration mode trigger pin
*/

Preferences preferences;

class EpaperManager
{
private:
  // SimpleWiFiManager wifiManager;
  Epd epd;
  String imageUrl = "";

  bool downloadImage()
  {
    // preferences.begin("data, true");
    imageUrl = preferences.getString("SERVER_BASE_URL");
    Serial.print("nas url: ");
    Serial.println(imageUrl);
    bool isHttps = imageUrl.startsWith("https://");
    WiFiClient *basicClient = nullptr;
    WiFiClientSecure *secureClient = nullptr;
    HTTPClient http;
    HTTPClient sleepHttp; // New HTTP client for sleep request
    http.setTimeout(HTTP_TIMEOUT);

    // Parse base URL for sleep request
    String baseUrl = imageUrl;
    const char *downloadPath = "/download";
    const char *sleepPath = "/sleep";

    String sleepUrl = baseUrl + sleepPath;

    // Setup client for image download
    if (isHttps)
    {
      secureClient = new WiFiClientSecure;
      secureClient->setInsecure();
      if (!http.begin(*secureClient, imageUrl + downloadPath))
      {
        Serial.println("Failed to initialize HTTPS connection");
        delete secureClient;
        return false;
      }
    }
    else
    {
      basicClient = new WiFiClient;
      if (!http.begin(*basicClient, imageUrl + downloadPath))
      {
        Serial.println("Failed to initialize HTTP connection");
        delete basicClient;
        return false;
      }
    }

    // Add battery voltage to header
    analogReadResolution(12);
    int plusV = 0;
    for (int i = 0; i < 50; i++)
    {
      plusV += analogReadMilliVolts(0);
      delay(5);
    }
    int batteryVoltage = (plusV / 50) * 2;
    http.addHeader("batteryCap", String(batteryVoltage));

    // Download and process image
    bool success = false;
    int sleepDuration = 0;
    bool retryOnError = true; // Add retry flag

    while (retryOnError && !success)
    {                       // Add retry loop
      retryOnError = false; // Default to no retry

      for (uint8_t i = 0; i < MAX_RETRIES; i++)
      {
        int httpCode = http.GET();

        if (httpCode == HTTP_CODE_OK)
        {
          success = processImageData(&http);

          // After successful image download, get sleep duration
          if (success)
          {
            // Setup new client for sleep request
            WiFiClient *sleepBasicClient = nullptr;
            WiFiClientSecure *sleepSecureClient = nullptr;

            if (isHttps)
            {
              sleepSecureClient = new WiFiClientSecure;
              sleepSecureClient->setInsecure();
              sleepHttp.begin(*sleepSecureClient, sleepUrl);
            }
            else
            {
              sleepBasicClient = new WiFiClient;
              sleepHttp.begin(*sleepBasicClient, sleepUrl);
            }

            sleepHttp.addHeader("Accept", "application/json");
            int sleepHttpCode = sleepHttp.GET();

            if (sleepHttpCode == HTTP_CODE_OK)
            {
              String payload = sleepHttp.getString();
              StaticJsonDocument<200> doc;
              DeserializationError error = deserializeJson(doc, payload);

              if (!error)
              {
                sleepDuration = doc["sleep_duration"] | 0;
                if (sleepDuration > 0)
                {
                  sleepDuration /= 1000; // Convert to seconds
                }
              }
            }

            sleepHttp.end();
            if (sleepSecureClient)
              delete sleepSecureClient;
            if (sleepBasicClient)
              delete sleepBasicClient;
          }
          break;
        }
        else if (httpCode == HTTP_CODE_ACCEPTED)
        {
          Serial.println("Server processing, waiting...");
          delay(RETRY_DELAY);
        }
        else if (httpCode == HTTP_CODE_INTERNAL_SERVER_ERROR)
        {
          Serial.println("Server error (500), will retry once...");
          delay(RETRY_DELAY);
          retryOnError = true; // Enable one retry on 500 error
          break;               // Exit current retry loop
        }
        else
        {
          Serial.printf("%s GET failed: %s\n",
                        isHttps ? "HTTPS" : "HTTP",
                        http.errorToString(httpCode).c_str());
          break;
        }
      }
    }

    http.end();
    delay(10);
    if (secureClient)
      delete secureClient;
    if (basicClient)
      delete basicClient;

    // If we got a valid sleep duration, use it for hibernation
    if (success && sleepDuration > 0)
    {
      hibernate(sleepDuration);
    }
    else
    {
      // Use default sleep duration if server didn't provide one
      hibernate();
    }

    return success;
  }

  // check if https
  bool startsWith(const String &str, const char *prefix)
  {
    return str.substring(0, strlen(prefix)).equalsIgnoreCase(prefix);
  }

  // Checks if character is a valid delimiter in image data
  bool isDelimiter(char c)
  {
    return c == ',' || c == '\n' || c == '\r' || c == '\0';
  }

  // Process image data stream and update display
  bool processImageData(HTTPClient *http)
  {
    WiFiClient *stream = http->getStreamPtr();
    int contentLength = http->getSize();

    // Validate content length
    if (contentLength <= 0)
    {
      Serial.println("Invalid content length");
      return false;
    }
    Serial.printf("Content-Length: %d bytes\n", contentLength);
    Serial.println("Starting direct image processing...");

    epd.SendCommand(0x10);

    uint8_t *buffer = (uint8_t *)malloc(BUFFER_SIZE);
    if (buffer == NULL)
    {
      Serial.println("Buffer allocation failed");
      return false;
    }

    String hexBuffer;
    int totalBytesProcessed = 0;

    while (contentLength > 0 && http->connected())
    {
      int bytesToRead = min(contentLength, (int)sizeof(buffer));
      int bytesRead = stream->readBytes(buffer, bytesToRead);

      if (bytesRead > 0)
      {
        for (int i = 0; i < bytesRead; i++)
        {
          char c = (char)buffer[i];
          if (isDelimiter(c))
          {
            if (!hexBuffer.isEmpty())
            {
              uint8_t byteValue = (uint8_t)strtol(hexBuffer.c_str(), nullptr, 16);
              epd.SendData(byteValue);
              hexBuffer.clear();
            }
          }
          else
          {
            hexBuffer += c;
          }
        }

        totalBytesProcessed += bytesRead;
        contentLength -= bytesRead;
      }
      else
      {
        if (!http->connected())
        {
          Serial.println("HTTP connection lost!");
          free(buffer);
          return false;
        }
        delay(10);
      }
    }

    if (!hexBuffer.isEmpty())
    {
      uint8_t byteValue = (uint8_t)strtol(hexBuffer.c_str(), nullptr, 16);
      epd.SendData(byteValue);
    }

    free(buffer);
    Serial.println("Showing image");
    epd.TurnOnDisplay();
    epd.Sleep();

    return true;
  }

  // Enter deep sleep mode with calculated wake-up interval
  void hibernate(int sleepDuration = 0)
  {
    Serial.println("Preparing for deep sleep...");

    // Use provided sleep duration or get default from WiFi manager
    // int sleep_interval = sleepDuration > 0 ? sleepDuration : wifiManager.getServerSleepDuration();
    int sleep_interval = sleepDuration > 0 ? sleepDuration : 86400;

    // Disconnect WiFi and turn off radio
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    fs_deinit();
    delay(50);
    // Print sleep duration for debugging
    Serial.printf("Sleep interval: %d seconds\n", sleep_interval);

    // Convert sleep time to microseconds
    uint64_t sleep_time;
    if (sleep_interval > 0)
    {
      sleep_time = static_cast<uint64_t>(sleep_interval) * 1000000ULL;
    }
    else
    {
      sleep_time = static_cast<uint64_t>(SLEEP_INTERVAL) * 1000000ULL;
    }

    Serial.printf("Sleep time in microseconds: %llu\n", sleep_time);

    // Configure wake up sources
    esp_sleep_enable_timer_wakeup(sleep_time);

    // Configure GPIO wake up
    rtc_gpio_init(WAKEUP_PIN);
    rtc_gpio_set_direction(WAKEUP_PIN, RTC_GPIO_MODE_INPUT_ONLY);
    rtc_gpio_pullup_en(WAKEUP_PIN);
    rtc_gpio_pulldown_dis(WAKEUP_PIN);
    esp_sleep_enable_ext1_wakeup(1ULL << WAKEUP_PIN, ESP_EXT1_WAKEUP_ANY_LOW);

    // Wait for serial output to complete
    Serial.println("Entering deep sleep mode...");
    Serial.flush();

    // Add delay before sleep
    delay(50);

    // Enter deep sleep
    esp_deep_sleep_start();
  }

  static void resetDeviceCredentials(void)
  {
    WifiCaptivePortal.resetSettings();
    bool res = preferences.clear();
    preferences.end();
    ESP.restart();
  }

  // Check if configuration mode should be entered
  bool shouldEnterConfigMode()
  {
    // Check configuration pin with debounce
    // if (digitalRead(CONFIG_PIN) == LOW) {
    //   delay(BUTTON_DEBOUNCE);
    //   return digitalRead(CONFIG_PIN) == LOW;
    // }
    // return false;
    Button button(CONFIG_PIN);
    return button.result();
  }

public:
  bool begin()
  {
    Serial.begin(115200);
    delay(50);
    pinMode(CONFIG_PIN, INPUT_PULLUP);

    if (epd.Init() != 0)
    {
      Serial.println(F("e-Paper init failed"));
      return false;
    }
    Serial.println(F("e-Paper initialized successfully"));

    // initialize spiffs
    fs_init();

    // initialize preferences
    preferences.begin("data", true);

    WiFi.mode(WIFI_STA);

    // Check configuration button
    if (shouldEnterConfigMode())
    {
      Serial.println(F("Config button pressed, entering config mode..."));
      epd.Clear(EPD_7IN3E_WHITE);
      // epd.Sleep();

      bool res = WifiCaptivePortal.startPortal();
      if (res)
      {
        Serial.println(F("Config mode completed"));
        return true;
      }
      // else {
      //   epd.Clear(EPD_7IN3E_WHITE);
      //   epd.Sleep();
      //   return false;
      // }
    }

    // If button not pressed, try normal startup
    if (WifiCaptivePortal.isSaved())
    {
      int connection_res = WifiCaptivePortal.autoConnect();
      if (connection_res)
      {
        preferences.putInt(PREFERENCES_CONNECT_WIFI_RETRY_COUNT, 1);
        return true;
      }
      // else {
      //   epd.Clear(EPD_7IN3E_WHITE);
      //   epd.Sleep();
      // }
    }
    else
    {
      WifiCaptivePortal.setResetSettingsCallback(resetDeviceCredentials);
      bool res = WifiCaptivePortal.startPortal();
      if (res)
      {
        preferences.putInt(PREFERENCES_CONNECT_WIFI_RETRY_COUNT, 1);
        return true;
      }
      //   if (!res) {
      //     epd.Clear(EPD_7IN3E_WHITE);
      //     epd.Sleep();
    }
    // }
    Serial.println(F("No valid WiFi configuration found - main"));
    return false;
  }

  void update()
  {
    Serial.println(F("Update method called"));

    if (WiFi.status() == WL_CONNECTED)
    {
      Serial.println(F("WiFi Connected. Downloading image"));
      if (downloadImage())
      {
        Serial.println(F("Image download successful"));
      }
      else
      {
        Serial.println(F("Image download failed"));
      }
    }
    else
    {
      Serial.println(F("WiFi not connected. Cannot download image"));
    }

    Serial.println(F("Entering sleep mode"));
    hibernate();
  }

  // Check battery voltage level
  bool checkVoltage()
  {
    analogReadResolution(12);
    int analogVolts = analogReadMilliVolts(0);
    // Multiply by 2 due to voltage divider
    Serial.print("BAT millivolts value = ");
    Serial.print(analogVolts * 2);
    Serial.println("mV");
    delay(50);
    // Return false if battery voltage is below 3.05V
    if (analogVolts * 2 < 3050)
    {
      return false;
    }
    return true;
  }

  // Clear the e-paper display
  void clearScreen()
  {
    epd.Init();
    delay(1000);
    epd.Clear(EPD_7IN3E_WHITE);
    epd.Sleep();
  }
};

// Global instance
EpaperManager epaperManager;

void setup()
{
  // Determine wake up reason
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();

  if (wakeup_reason == ESP_SLEEP_WAKEUP_TIMER)
  {
    Serial.println("Wakeup caused by timer");
  }
  else if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT1)
  {
    Serial.println("Wakeup caused by external signal using RTC_GPIO");
  }
  else
  {
    Serial.println("First boot or reset");
  }

  if (!epaperManager.checkVoltage())
  {
    Serial.println(F("Battery low voltage (< 3.0V)"));
    Serial.println(F("Sleep for 24hr"));
    epaperManager.clearScreen();
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    delay(1000);
    esp_sleep_enable_timer_wakeup(86400 * 1000000ULL);
    esp_deep_sleep_start();
  }
  if (epaperManager.begin())
  {
    Serial.println(F("Begin successful, calling update"));
    epaperManager.update();
  }
  else
  {
    Serial.println(F("Begin failed"));
    epaperManager.clearScreen();

    delay(30000);
    ESP.restart();
  }
}

void loop()
{
  // deepsleep
}