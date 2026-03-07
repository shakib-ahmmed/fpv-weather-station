// Display driver
#define ILI9341_DRIVER

// Display size
#define TFT_WIDTH 240
#define TFT_HEIGHT 320

// ESP32-S3 pins
#define TFT_CS 10
#define TFT_DC 8
#define TFT_RST 9

#define TFT_MOSI 11
#define TFT_SCLK 12
#define TFT_MISO 13 // optional

// Backlight pin
#define TFT_BL 3V // or your GPIO if you want dim control

// No touch controller
#undef TOUCH_CS
