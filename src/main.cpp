#include <Arduino.h>
#include <LovyanGFX.hpp>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>

// ===== WiFi Credentials =====
const char *ssid = "BazFPV";
const char *password = "AROSH2023";

// ===== Update Interval =====
const unsigned long UPDATE_INTERVAL = 8640000;
const unsigned long FLICK_INTERVAL = 1000;

// ===== Weather Variables =====
float temp = 16.0, wind = 6.0, hum = 60.0, vis = 10.0;
int sats = 21;

// ===== Timing =====
unsigned long lastUpdate = 0;
unsigned long lastFlick = 0;
bool flick = false;

// ===== Custom LGFX Class =====
class LGFX : public lgfx::LGFX_Device
{
  lgfx::Bus_SPI _bus;
  lgfx::Panel_ILI9341 _panel;

public:
  LGFX()
  {
    auto cfg = _bus.config();
    cfg.spi_host = SPI3_HOST;
    cfg.freq_write = 20000000;
    cfg.pin_mosi = 11;
    cfg.pin_miso = 13;
    cfg.pin_sclk = 12;
    cfg.pin_dc = 9;
    _bus.config(cfg);
    _panel.setBus(&_bus);

    auto panel_cfg = _panel.config();
    panel_cfg.pin_cs = 10;
    panel_cfg.pin_rst = 14;
    panel_cfg.panel_width = 240;
    panel_cfg.panel_height = 320;
    panel_cfg.memory_width = 240;
    panel_cfg.memory_height = 320;

    _panel.config(panel_cfg);
    setPanel(&_panel);
  }
};

LGFX tft;

// ===== Fetch Weather =====
void getWeather()
{
  if (WiFi.status() != WL_CONNECTED)
    return;

  HTTPClient http;
  http.begin("http://api.open-meteo.com/v1/forecast?latitude=24.0&longitude=89.0&current_weather=true&hourly=relative_humidity_2m,visibility");

  if (http.GET() > 0)
  {
    StaticJsonDocument<4096> doc;
    deserializeJson(doc, http.getString());

    temp = doc["current_weather"]["temperature"];
    wind = doc["current_weather"]["windspeed"];
    hum = doc["hourly"]["relative_humidity_2m"][0];
    vis = (float)doc["hourly"]["visibility"][0] / 1000.0;
  }

  http.end();
}

//////////////////////////////////////////////////
//////////////// BOOT SCREENS ////////////////////
//////////////////////////////////////////////////

void splashScreen()
{
  tft.fillScreen(TFT_BLACK);
  tft.setTextDatum(lgfx::middle_center);

  tft.setTextSize(3);
  tft.setTextColor(TFT_CYAN);
  tft.drawString("BazFPV", 120, 130);

  tft.setTextSize(2);
  tft.setTextColor(TFT_LIGHTGREY);
  tft.drawString("FPV WEATHER STATION", 120, 170);

  delay(1500);
}

void loadingBar(const char *msg)
{
  tft.fillScreen(TFT_BLACK);

  tft.setTextDatum(lgfx::middle_center);
  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE);
  tft.drawString(msg, 120, 120);

  tft.drawRect(40, 160, 160, 18, TFT_WHITE);

  for (int i = 0; i <= 156; i += 4)
  {
    tft.fillRect(42 + i, 162, 4, 14, TFT_GREEN);
    delay(35);
  }
}

void satelliteAnimation()
{
  tft.fillScreen(TFT_BLACK);

  tft.setTextDatum(lgfx::middle_center);
  tft.setTextSize(2);
  tft.setTextColor(TFT_CYAN);
  tft.drawString("ACQUIRING SATELLITES", 120, 80);

  for (int i = 0; i < 4; i++)
  {
    tft.fillCircle(120, 170, 5, TFT_WHITE);
    tft.drawCircle(120, 170, 20 + i * 20, TFT_GREEN);
    delay(350);
  }

  delay(700);
}

void wifiConnectScreen()
{
  tft.fillScreen(TFT_BLACK);

  tft.setTextDatum(lgfx::middle_center);
  tft.setTextSize(2);
  tft.setTextColor(TFT_YELLOW);
  tft.drawString("CONNECTING WIFI", 120, 130);

  WiFi.begin(ssid, password);

  int dots = 0;

  while (WiFi.status() != WL_CONNECTED)
  {
    tft.setCursor(95, 170);
    tft.print("Please wait");

    tft.setCursor(170, 170);

    for (int i = 0; i < dots; i++)
      tft.print(".");

    dots++;
    if (dots > 3)
      dots = 0;

    delay(500);
    tft.fillRect(170, 170, 40, 15, TFT_BLACK);
  }

  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_GREEN);
  tft.drawString("WIFI CONNECTED", 120, 150);

  delay(1200);
}

//////////////////////////////////////////////////
/////////////////// MAIN UI //////////////////////
//////////////////////////////////////////////////

void drawGrid()
{
  tft.fillScreen(TFT_BLACK);

  tft.setTextColor(TFT_PINK);
  tft.setTextDatum(lgfx::middle_center);
  tft.setTextSize(2);
  tft.setCursor(15, 12);
  tft.print("FPV WEATHER STATION");

  tft.drawFastHLine(0, 50, 240, TFT_YELLOW);
  tft.drawFastHLine(0, 120, 240, TFT_BLUE);
  tft.drawFastHLine(0, 190, 240, TFT_RED);
  tft.drawFastHLine(0, 260, 240, TFT_GREEN);

  tft.drawFastVLine(120, 50, 210, TFT_DARKGREY);
}

void drawFlyStatus()
{
  bool safe = (wind < 15.0);

  if (millis() - lastFlick > FLICK_INTERVAL)
  {
    flick = !flick;
    lastFlick = millis();
  }

  uint16_t statusBG = safe ? TFT_GREEN : TFT_RED;
  uint16_t textColor = flick ? TFT_BLACK : TFT_RED;

  tft.fillRect(0, 50, 240, 20, statusBG);

  tft.setTextDatum(lgfx::middle_center);
  tft.setTextSize(2);
  tft.setTextColor(textColor, statusBG);

  tft.drawString(safe ? "READY TO FLY" : "STAY GROUNDED", 120, 60);
}

void drawWeather()
{
  tft.setTextSize(1);
  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);

  tft.setCursor(15, 75);
  tft.print("TEMP");
  tft.setCursor(135, 75);
  tft.print("WIND SPEED");

  tft.setTextSize(2);

  tft.setTextColor(TFT_ORANGE);
  tft.setCursor(15, 90);
  tft.printf("%.1f C", temp);

  tft.setTextColor(TFT_CYAN);
  tft.setCursor(135, 90);
  tft.printf("%.1f kph", wind);

  tft.setTextSize(1);
  tft.setTextColor(TFT_LIGHTGREY);

  tft.setCursor(15, 140);
  tft.print("HUMIDITY");

  tft.setCursor(135, 140);
  tft.print("VISIBILITY");

  tft.setTextSize(2);

  tft.setTextColor(TFT_YELLOW);
  tft.setCursor(15, 155);
  tft.printf("%.0f %%", hum);

  tft.setTextColor(TFT_WHITE);
  tft.setCursor(135, 155);
  tft.printf("%.1f km", vis);

  // SAT & TIME at bottom
  tft.setTextSize(1);
  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft.setCursor(15, 200);
  tft.print("SAT:");
  tft.setCursor(135, 200);
  tft.print("TIME:");

  // Vertical separator between SAT and TIME
  tft.drawFastVLine(120, 195, 35, TFT_DARKGREY);

  tft.setTextSize(2);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setCursor(50, 200);
  tft.printf("%d", sats);

  time_t now = time(nullptr);
  struct tm *t = localtime(&now);
  int hour12 = t->tm_hour % 12;
  if (hour12 == 0)
    hour12 = 12;
  const char *ampm = t->tm_hour >= 12 ? "PM" : "AM";

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(170, 200);
  tft.printf("%02d:%02d %s", hour12, t->tm_min, ampm);

  // Horizontal line under SAT/TIME box
  tft.drawFastHLine(0, 230, 240, TFT_DARKGREY);
}

void drawWiFiStatus()
{
  tft.setTextSize(1);
  tft.setTextColor(WiFi.status() == WL_CONNECTED ? TFT_GREEN : TFT_RED, TFT_DARKGREY);
  tft.setCursor(135, 285);
  tft.print(WiFi.status() == WL_CONNECTED ? "WiFi: OK" : "WiFi: OFFLINE");
}

//////////////////////////////////////////////////
//////////////////// SETUP ///////////////////////
//////////////////////////////////////////////////

void setup()
{
  tft.init();
  tft.setRotation(7);
  tft.setSwapBytes(true);
  tft.setTextDatum(lgfx::top_left);

  splashScreen();
  loadingBar("SYSTEM BOOT");
  satelliteAnimation();
  wifiConnectScreen();

  configTime(21600, 0, "pool.ntp.org");

  getWeather();

  drawGrid();
}

//////////////////////////////////////////////////
//////////////////// LOOP ////////////////////////
//////////////////////////////////////////////////

void loop()
{
  unsigned long nowMillis = millis();

  if (nowMillis - lastUpdate > UPDATE_INTERVAL || lastUpdate == 0)
  {
    getWeather();
    lastUpdate = nowMillis;
  }

  drawFlyStatus();
  drawWeather();
  drawWiFiStatus();

  delay(100);
}