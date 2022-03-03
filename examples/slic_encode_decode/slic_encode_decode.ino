//
// SLIC encode+decode example
// written by Larry Bank
//
// A sketch to demonstrate how to encode and decode
// SLIC data on a resource constrained device
// This code was tested on the Arduino Nano Every (ATMega4809 MCU)
//
#include <slic.h>
#include <bb_spi_lcd.h>

static SLIC slic;
static SPILCD lcd;
uint16_t usTemp[240];
uint8_t ucImage[2048];
#define TFT_CS 10
#define TFT_DC 9
#define TFT_RST 8
#define TFT_LED -1
#define TFT_MOSI 11
#define TFT_SCK 13
#define DISPLAY_WIDTH 240
#define DISPLAY_HEIGHT 240

//
// Generate an image dynamically and
// compress it with the SLIC codec
// into a RAM buffer
//
int CreateImage(void)
{
  int rc, x, y;
  char szTemp[32];
  // Initialize the encoder to output to our RAM buffer
  rc = slic.init_encode_ram(128, 128, 16, NULL, ucImage, sizeof(ucImage));
  if (rc == SLIC_SUCCESS) {
    // Create a 128x128xRGB565 image
    for (y=0; y<128; y++) {
      if (y == 0 || y == 127) { // solid lines on top/bottom
        for (x=0; x<128; x++) {
          usTemp[x] = 0xffff; // white border
        }
      } else {
        usTemp[0] = usTemp[127] = 0xffff; // white edges
        // blue background
        for (x=1; x<127; x++) {
          usTemp[x] = 0x1f;
        }
        usTemp[y] = usTemp[127-y] = 0x7e0; // green X down the middle
      }
      slic.encode((uint8_t *)usTemp, 128); // encode 128 pixels (this line)
    } // for y
  spilcdWriteString(&lcd, 0,0,(char *)"SLIC encoding produced", 0xffe0,0,FONT_8x8, DRAW_TO_LCD);
  sprintf(szTemp, "%d bytes of compressed data", slic.get_output_size());
  spilcdWriteString(&lcd, 0,10,szTemp, 0xffe0,0,FONT_8x8, DRAW_TO_LCD);
  sprintf(szTemp, "(A ~%d:1 compression ratio)", (128*128*2)/slic.get_output_size());
  spilcdWriteString(&lcd, 0,20,szTemp, 0xffe0,0,FONT_8x8, DRAW_TO_LCD);
  return slic.get_output_size();
  } else {
    Serial.print("SLIC encoder returned error - ");
    Serial.println(rc, DEC);
    return 0;
  }
} /* CreateImage() */

void ShowImage(uint8_t *pImage, int image_size)
{
int rc, xoff, yoff;

  rc = slic.init_decode_ram((uint8_t *)pImage, image_size, NULL);
  Serial.print("Decoding a ");
  Serial.print(slic.get_width(), DEC);
  Serial.print(" x ");
  Serial.print(slic.get_height(), DEC);
  Serial.println(" image");
  xoff = (DISPLAY_WIDTH - slic.get_width())/2;
  yoff = (DISPLAY_HEIGHT - slic.get_height())/2;
  spilcdSetPosition(&lcd, xoff, yoff, slic.get_width(), slic.get_height(), DRAW_TO_LCD);
  for (int y=0; y<slic.get_height() && rc == SLIC_SUCCESS; y++) {
    rc = slic.decode((uint8_t *)usTemp, slic.get_width());
    for (int i=0; i<slic.get_width(); i++) { // byte reverse
      usTemp[i] = __builtin_bswap16(usTemp[i]);
    } // for i
    spilcdWriteDataBlock(&lcd, (uint8_t *)usTemp, slic.get_width()*2, DRAW_TO_LCD);
  } // for y
} /* ShowImage() */

void setup() {
int iLen;
  Serial.begin(115200);
  Serial.println("Starting...");
  // init the LCD
  memset(&lcd, 0, sizeof(lcd));
  spilcdInit(&lcd, LCD_ST7789_240, FLAGS_NONE, 10000000, TFT_CS, TFT_DC, TFT_RST, TFT_LED, -1, TFT_MOSI, TFT_SCK);
  spilcdFill(&lcd, 0, DRAW_TO_LCD);
  // Create a compressed image
  iLen = CreateImage();
  // Display it on the LCD
  ShowImage(ucImage, iLen);
} /* setup() */

void loop() {
} /* loop() */
