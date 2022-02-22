//
// SLIC (Simple Lossless Image Codec) decoding demo
// written by Larry Bank
//
// This sketch shows how the SLIC codec can decode images even on
// devices with very little RAM. The images chosen for the demo are
// very compressible due to areas of constant color and limited unique colors
// but still impressive that an 8-bit MCU with 2K of RAM can decode 150K
// 320x240x16-bit images.
// The RGB565 pixel type was specifically chosen over 8-bit palette images
// because the additional 768 bytes of RAM needed to hold the color palette
// would exceed the available RAM on AVR devices
//
// The GPIO pin numbers and LCD chosen are somewhat arbitrary. If needed
// This demo can work with any SPI LCD that's 240x320 or larger (e.g. ILI9341)
// 
#include <slic.h>
#include <bb_spi_lcd.h>

#include "homer_head_16.h"
#include "arduino_logo.h"

static SLIC slic;
static SPILCD lcd;
uint16_t usTemp[320];

#define TFT_CS 10
#define TFT_DC 9
#define TFT_RST 8
#define TFT_LED -1
#define TFT_MOSI 11
#define TFT_SCK 13
#define DISPLAY_WIDTH 320
#define DISPLAY_HEIGHT 240

void ShowImage(const uint8_t *pImage, int image_size)
{
int rc, xoff, yoff;

 spilcdFill(&lcd, 0, DRAW_TO_LCD);
  rc = slic.init_decode_flash((uint8_t *)pImage, image_size, NULL);
  xoff = (DISPLAY_WIDTH - slic.get_width())/2;
  yoff = (DISPLAY_HEIGHT - slic.get_height())/2;
  // set the memory window for the whole image (centered)
  spilcdSetPosition(&lcd, xoff, yoff, slic.get_width(), slic.get_height(), DRAW_TO_LCD);
  for (int y=0; y<slic.get_height() && rc == SLIC_SUCCESS; y++) {
    // decode one line's worth of pixels at a time
    rc = slic.decode((uint8_t *)usTemp, slic.get_width());
    for (int i=0; i<slic.get_width(); i++) { // byte reverse for the LCD (big endian)
      usTemp[i] = __builtin_bswap16(usTemp[i]);
    } // for i
    spilcdWriteDataBlock(&lcd, (uint8_t *)usTemp, slic.get_width()*sizeof(uint16_t), DRAW_TO_LCD);
  } // for y
} /* ShowImage() */

void setup() {
int rc;
  // init the LCD (ST7789 240x320)
  memset(&lcd, 0, sizeof(lcd));
  rc = spilcdInit(&lcd, LCD_ST7789, FLAGS_NONE, SPI_CLOCK_DIV2, TFT_CS, TFT_DC, TFT_RST, TFT_LED, -1, TFT_MOSI, TFT_SCK);
  spilcdSetOrientation(&lcd, LCD_ORIENTATION_90);
 
} /* setup() */

void loop() {
// Alternate between the 2 images
   ShowImage(arduino_logo, sizeof(arduino_logo));
   delay(2000);
   ShowImage(homer_head_16, sizeof(homer_head_16));
   delay(2000);
} /* loop() */
