#include "lcd.h"

#include <SPI.h>

#include "board_config.h"

namespace {

SPISettings lcdSpi(40000000, MSBFIRST, SPI_MODE0);

void lcdSelect() {
  SPI.beginTransaction(lcdSpi);
  digitalWrite(PIN_LCD_CS, LOW);
}

void lcdDeselect() {
  digitalWrite(PIN_LCD_CS, HIGH);
  SPI.endTransaction();
}

void lcdWriteCommand(uint8_t command) {
  lcdSelect();
  digitalWrite(PIN_LCD_DC, LOW);
  SPI.transfer(command);
  lcdDeselect();
}

void lcdWriteData(const uint8_t* data, size_t len) {
  lcdSelect();
  digitalWrite(PIN_LCD_DC, HIGH);
  SPI.writeBytes(data, len);
  lcdDeselect();
}

void lcdWriteData8(uint8_t value) {
  lcdWriteData(&value, 1);
}

void lcdWriteData16(uint16_t value) {
  uint8_t data[] = {static_cast<uint8_t>(value >> 8), static_cast<uint8_t>(value)};
  lcdWriteData(data, sizeof(data));
}

void lcdSetAddrWindow(int x, int y, int w, int h) {
  const int x0 = x + LCD_X_OFFSET;
  const int x1 = x + w - 1 + LCD_X_OFFSET;
  const int y0 = y + LCD_Y_OFFSET;
  const int y1 = y + h - 1 + LCD_Y_OFFSET;

  lcdWriteCommand(0x2a);
  lcdWriteData16(x0);
  lcdWriteData16(x1);
  lcdWriteCommand(0x2b);
  lcdWriteData16(y0);
  lcdWriteData16(y1);
  lcdWriteCommand(0x2c);
}

void lcdPushColor(uint16_t color, uint32_t count) {
  const uint8_t hi = color >> 8;
  const uint8_t lo = color & 0xff;
  uint8_t buffer[128];
  for (size_t i = 0; i < sizeof(buffer); i += 2) {
    buffer[i] = hi;
    buffer[i + 1] = lo;
  }

  lcdSelect();
  digitalWrite(PIN_LCD_DC, HIGH);
  while (count > 0) {
    const uint32_t pixels = count < sizeof(buffer) / 2 ? count : sizeof(buffer) / 2;
    SPI.writeBytes(buffer, pixels * 2);
    count -= pixels;
  }
  lcdDeselect();
}

const uint8_t* glyphFor(char ch) {
  static const uint8_t space[] = {0, 0, 0, 0, 0};
  static const uint8_t dash[] = {0x08, 0x08, 0x08, 0x08, 0x08};
  static const uint8_t dot[] = {0, 0, 0x40, 0, 0};
  static const uint8_t colon[] = {0, 0x14, 0, 0x14, 0};
  static const uint8_t slash[] = {0x40, 0x20, 0x10, 0x08, 0x04};
  static const uint8_t zero[] = {0x3e, 0x51, 0x49, 0x45, 0x3e};
  static const uint8_t one[] = {0x00, 0x42, 0x7f, 0x40, 0x00};
  static const uint8_t two[] = {0x62, 0x51, 0x49, 0x49, 0x46};
  static const uint8_t three[] = {0x22, 0x49, 0x49, 0x49, 0x36};
  static const uint8_t four[] = {0x18, 0x14, 0x12, 0x7f, 0x10};
  static const uint8_t five[] = {0x2f, 0x49, 0x49, 0x49, 0x31};
  static const uint8_t six[] = {0x3e, 0x49, 0x49, 0x49, 0x32};
  static const uint8_t seven[] = {0x01, 0x71, 0x09, 0x05, 0x03};
  static const uint8_t eight[] = {0x36, 0x49, 0x49, 0x49, 0x36};
  static const uint8_t nine[] = {0x26, 0x49, 0x49, 0x49, 0x3e};
  static const uint8_t a[] = {0x7e, 0x11, 0x11, 0x11, 0x7e};
  static const uint8_t b[] = {0x7f, 0x49, 0x49, 0x49, 0x36};
  static const uint8_t c[] = {0x3e, 0x41, 0x41, 0x41, 0x22};
  static const uint8_t d[] = {0x7f, 0x41, 0x41, 0x22, 0x1c};
  static const uint8_t e[] = {0x7f, 0x49, 0x49, 0x49, 0x41};
  static const uint8_t f[] = {0x7f, 0x09, 0x09, 0x09, 0x01};
  static const uint8_t g[] = {0x3e, 0x41, 0x49, 0x49, 0x7a};
  static const uint8_t h[] = {0x7f, 0x08, 0x08, 0x08, 0x7f};
  static const uint8_t i[] = {0x00, 0x41, 0x7f, 0x41, 0x00};
  static const uint8_t j[] = {0x20, 0x40, 0x41, 0x3f, 0x01};
  static const uint8_t k[] = {0x7f, 0x08, 0x14, 0x22, 0x41};
  static const uint8_t l[] = {0x7f, 0x40, 0x40, 0x40, 0x40};
  static const uint8_t m[] = {0x7f, 0x02, 0x0c, 0x02, 0x7f};
  static const uint8_t n[] = {0x7f, 0x04, 0x08, 0x10, 0x7f};
  static const uint8_t o[] = {0x3e, 0x41, 0x41, 0x41, 0x3e};
  static const uint8_t p[] = {0x7f, 0x09, 0x09, 0x09, 0x06};
  static const uint8_t q[] = {0x3e, 0x41, 0x51, 0x21, 0x5e};
  static const uint8_t r[] = {0x7f, 0x09, 0x19, 0x29, 0x46};
  static const uint8_t s[] = {0x46, 0x49, 0x49, 0x49, 0x31};
  static const uint8_t t[] = {0x01, 0x01, 0x7f, 0x01, 0x01};
  static const uint8_t u[] = {0x3f, 0x40, 0x40, 0x40, 0x3f};
  static const uint8_t v[] = {0x1f, 0x20, 0x40, 0x20, 0x1f};
  static const uint8_t w[] = {0x7f, 0x20, 0x18, 0x20, 0x7f};
  static const uint8_t x[] = {0x63, 0x14, 0x08, 0x14, 0x63};
  static const uint8_t y[] = {0x07, 0x08, 0x70, 0x08, 0x07};
  static const uint8_t z[] = {0x61, 0x51, 0x49, 0x45, 0x43};

  if (ch >= 'a' && ch <= 'z') ch -= 32;
  switch (ch) {
    case '0': return zero;
    case '1': return one;
    case '2': return two;
    case '3': return three;
    case '4': return four;
    case '5': return five;
    case '6': return six;
    case '7': return seven;
    case '8': return eight;
    case '9': return nine;
    case 'A': return a;
    case 'B': return b;
    case 'C': return c;
    case 'D': return d;
    case 'E': return e;
    case 'F': return f;
    case 'G': return g;
    case 'H': return h;
    case 'I': return i;
    case 'J': return j;
    case 'K': return k;
    case 'L': return l;
    case 'M': return m;
    case 'N': return n;
    case 'O': return o;
    case 'P': return p;
    case 'Q': return q;
    case 'R': return r;
    case 'S': return s;
    case 'T': return t;
    case 'U': return u;
    case 'V': return v;
    case 'W': return w;
    case 'X': return x;
    case 'Y': return y;
    case 'Z': return z;
    case '-': return dash;
    case '.': return dot;
    case ':': return colon;
    case '/': return slash;
    default: return space;
  }
}

void drawChar(int x, int y, char ch, uint16_t color, uint16_t bg, int scale) {
  const uint8_t* glyph = glyphFor(ch);
  for (int col = 0; col < 5; col += 1) {
    for (int row = 0; row < 7; row += 1) {
      const uint16_t pixel = glyph[col] & (1 << row) ? color : bg;
      lcdFillRect(x + col * scale, y + row * scale, scale, scale, pixel);
    }
  }
}

}  // namespace

uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
  return ((r & 0xf8) << 8) | ((g & 0xfc) << 3) | (b >> 3);
}

void lcdFillRect(int x, int y, int w, int h, uint16_t color) {
  if (x >= LCD_WIDTH || y >= LCD_HEIGHT || w <= 0 || h <= 0) return;
  if (x < 0) {
    w += x;
    x = 0;
  }
  if (y < 0) {
    h += y;
    y = 0;
  }
  if (x + w > LCD_WIDTH) w = LCD_WIDTH - x;
  if (y + h > LCD_HEIGHT) h = LCD_HEIGHT - y;
  lcdSetAddrWindow(x, y, w, h);
  lcdPushColor(color, static_cast<uint32_t>(w) * h);
}

void lcdDrawHLine(int x, int y, int w, uint16_t color) {
  lcdFillRect(x, y, w, 1, color);
}

void lcdDrawVLine(int x, int y, int h, uint16_t color) {
  lcdFillRect(x, y, 1, h, color);
}

void lcdDrawRect(int x, int y, int w, int h, uint16_t color) {
  lcdDrawHLine(x, y, w, color);
  lcdDrawHLine(x, y + h - 1, w, color);
  lcdDrawVLine(x, y, h, color);
  lcdDrawVLine(x + w - 1, y, h, color);
}

void lcdSetOn(bool on) {
  digitalWrite(PIN_LCD_BL, on ? HIGH : LOW);
}

void lcdInit(bool on) {
  pinMode(PIN_LCD_CS, OUTPUT);
  pinMode(PIN_LCD_DC, OUTPUT);
  pinMode(PIN_LCD_RST, OUTPUT);
  pinMode(PIN_LCD_BL, OUTPUT);
  digitalWrite(PIN_LCD_CS, HIGH);
  digitalWrite(PIN_LCD_DC, HIGH);
  lcdSetOn(false);

  SPI.begin(PIN_LCD_SCLK, -1, PIN_LCD_MOSI, PIN_LCD_CS);
  digitalWrite(PIN_LCD_RST, HIGH);
  delay(20);
  digitalWrite(PIN_LCD_RST, LOW);
  delay(20);
  digitalWrite(PIN_LCD_RST, HIGH);
  delay(120);

  lcdWriteCommand(0x11);
  delay(120);
  lcdWriteCommand(0x3a);
  lcdWriteData8(0x55);
  lcdWriteCommand(0x36);
  lcdWriteData8(0x08);
  lcdWriteCommand(0x21);
  lcdWriteCommand(0x13);
  delay(10);
  lcdWriteCommand(0x29);
  delay(120);
  lcdSetOn(on);
}

void drawText(int x, int y, const char* text, uint16_t color, uint16_t bg, int scale) {
  while (*text) {
    drawChar(x, y, *text, color, bg, scale);
    x += 6 * scale;
    text += 1;
  }
}
