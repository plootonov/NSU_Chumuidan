#pragma once
#include <Arduino.h>
#include <UTFT.h>

// Эти шрифты есть в UTFT
extern uint8_t SmallFont[];
extern uint8_t BigFont[];

struct UIData {
  float   voltage_V;
  uint8_t cells;       // 3S/4S/6S...
  const char* vrx;
  uint16_t freq_MHz;
  char    bandChar;    // 'A','B','E','F','R'... или 0
  uint8_t channel;     // 1..8
  int16_t rssi_dB;
  const char* control; // "ELRS"/"CRSF"/"SBUS"...
  bool    recording;   // REC/STOP
  bool    v_bypass;    // ON/OFF
  int16_t azimuth_deg; // 0..359
};

class DisplayUI_UTFT {
public:
  // Инициализация. Передаём твой уже созданный myGLCD и ориентацию (0=PORTRAIT, 1=LANDSCAPE)
  void begin(UTFT& lcd, uint8_t landscape = 1, uint8_t cells = 4);

  // Полная первичная отрисовка рамки/плашек
  void drawFrame();

  // Обновление значений (перерисовывает только изменившиеся части)
  void render(const UIData& d);

  void setCells(uint8_t cells) { last_.cells = cells; }

private:
  UTFT* tft_ = nullptr;
  int16_t W_ = 480, H_ = 320;  // под ILI9481 в LANDSCAPE

  // Геометрия
  const int headerX_ = 6, headerY_ = 6, headerH_ = 52;
  const int leftX_ = 16, leftY0_ = 72, leftW_ = 300, rowH_ = 28, rowGap_ = 6; //const int leftX_ = 16, leftY0_ = 72, leftW_ = 300, rowH_ = 28, rowGap_ = 6;
  const int rightX_ = 340, rightY_ = 72, rightW_ = 124, rightH_ = 120;

  // Кэш для частичной перерисовки
  UIData last_ = { -999.f, 4, "",0, 0, 0, -999, "", false, false, -999 };

  // Палитра (RGB)
  struct RGB { uint8_t r,g,b; };
  const RGB COL_BG    {  8, 16, 24 };   // фон (тёмный)
  const RGB COL_CARD  { 24, 48, 72 };   // плашки
  const RGB COL_TEXT  {255,255,255};    // белый
  const RGB COL_LABEL {0,0,0};    // подписи//180,180,180
  const RGB COL_OK    {  0,255,  0};    // зелёный
  const RGB COL_WARN  {255,255,  0};    // жёлтый
  const RGB COL_BAD   {255,  0,  0};    // красный
  const RGB COL_BLACK {  0,  0,  0};

  // Утилиты рисования (UTFT)
  void setColor(const RGB& c);
  void setBackColor(const RGB& c);
  void fillRectR(int x,int y,int w,int h,const RGB& c);           // прямоугольник
  void fillRoundRectR(int x,int y,int w,int h,int r,const RGB& c); // закруглённый (эмуляция)
  void drawRoundRectR (int x,int y,int w,int h,int r,const RGB& c);
  void printAt(int x,int y,const char* s,const RGB& col,uint8_t* font, bool transparent=true);

  // Конкретные блоки UI
  void drawHeaderBar(float voltage, int percent);
  void drawLabelValueRow(int row, const char* label, const char* value, bool highlight=false);
  void drawAzimuthCard(int az);

  // Логика
  static int  voltageToPercent(float v, uint8_t cells);
  static int  imap(int x,int in_min,int in_max,int out_min,int out_max);
  static void clamp(int& v,int lo,int hi) { if(v<lo)v=lo; if(v>hi)v=hi; }
};
