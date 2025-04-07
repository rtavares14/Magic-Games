#include "LCD.h"

LCD::LCD(uint8_t address, uint8_t columns, uint8_t rows) : lcd(address, columns, rows) {}

void LCD::begin()
{
    lcd.init();
    lcd.backlight();
}

void LCD::clear()
{
    lcd.clear();
}

void LCD::setCursor(uint8_t col, uint8_t row)
{
    lcd.setCursor(col, row);
}

void LCD::print(const char *message)
{
    lcd.print(message);
}

void LCD::printMessage(const char *line1, const char *line2, unsigned long duration)
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(line1);
    lcd.setCursor(0, 1);
    lcd.print(line2);
    delay(duration);
}

void LCD::lcdShow(const char *line1, const char *line2) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(line1);
    lcd.setCursor(0, 1);
    lcd.print(line2);
}

// Helper function to update the LCD only when content changes.
void LCD::updateLCD(const char* line1, const char* line2) {
    static char lastLine1[17] = "";
    static char lastLine2[17] = "";
    if (strcmp(line1, lastLine1) != 0 || strcmp(line2, lastLine2) != 0) {
      lcdShow(line1, line2);
      strncpy(lastLine1, line1, 16);
      lastLine1[16] = '\0';
      strncpy(lastLine2, line2, 16);
      lastLine2[16] = '\0';
    }
  }