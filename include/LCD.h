#ifndef LCD_H
#define LCD_H

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>

class LCD {
public:
    LCD(uint8_t address, uint8_t columns, uint8_t rows);
    void begin();
    void clear();
    void setCursor(uint8_t col, uint8_t row);
    void print(const char* message);
    void printMessage(const char* line1, const char* line2, unsigned long duration = 2000);

    void lcdShow(const char *line1, const char *line2);

    void printMessageNoTime(const char *line1, const char *line2);
    void updateLCD(const char* line1, const char* line2);

private:
    LiquidCrystal_I2C lcd;
};

#endif
