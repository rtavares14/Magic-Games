#include "LCD.h"

LCD::LCD(uint8_t address, uint8_t columns, uint8_t rows) : lcd(address, columns, rows) {}

void LCD::begin() {
    lcd.init();
    lcd.backlight();
}

void LCD::clear() {
    lcd.clear();
}

void LCD::setCursor(uint8_t col, uint8_t row) {
    lcd.setCursor(col, row);
}

void LCD::print(const char* message) {
    lcd.print(message);
}

void LCD::printMessage(const char* line1, const char* line2, unsigned long duration) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(line1);
    lcd.setCursor(0, 1);
    lcd.print(line2);
    delay(duration);
}
