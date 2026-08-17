// Copy these settings into your Arduino libraries/TFT_eSPI/User_Setup.h
// or create an equivalent TFT_eSPI setup for the ESP32-2432S028 CYD.
#define USER_SETUP_LOADED
#define ILI9341_2_DRIVER
#define TFT_WIDTH 240
#define TFT_HEIGHT 320
#define TFT_MISO 12
#define TFT_MOSI 13
#define TFT_SCLK 14
#define TFT_CS 15
#define TFT_DC 2
#define TFT_RST -1
#define TFT_BL 21
#define TFT_BACKLIGHT_ON HIGH
#define TOUCH_CS 33
#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define SPI_FREQUENCY 40000000
#define SPI_READ_FREQUENCY 16000000
