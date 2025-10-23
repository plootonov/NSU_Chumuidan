#include "DisplayUI_UTFT.h"
#include <math.h>

int DisplayUI_UTFT::imap(int x,int in_min,int in_max,int out_min,int out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

int DisplayUI_UTFT::voltageToPercent(float v, uint8_t cells) {
  if (!cells) cells = 1;
  float perCell = v / cells;
  float p = (perCell - 3.30f) / (4.20f - 3.30f) * 100.0f;
  if (p < 0) p = 0; if (p > 100) p = 100;
  return (int)(p + 0.5f);
}

void DisplayUI_UTFT::setColor(const RGB& c)     { tft_->setColor(c.r,c.g,c.b); }
void DisplayUI_UTFT::setBackColor(const RGB& c) { tft_->setBackColor(c.r,c.g,c.b); }

void DisplayUI_UTFT::fillRectR(int x,int y,int w,int h,const RGB& c) {
  setColor(c);
  tft_->fillRect(x, y, x+w-1, y+h-1);
}

void DisplayUI_UTFT::fillRoundRectR(int x,int y,int w,int h,int r,const RGB& c) {
  // Простейшая эмуляция: прямоугольник + 4 круга по углам
  fillRectR(x+r, y,   w-2*r, h, c);
  fillRectR(x,   y+r, r,     h-2*r, c);
  fillRectR(x+w-r, y+r, r,   h-2*r, c);
  setColor(c);
  tft_->fillCircle(x+r,   y+r,   r);
  tft_->fillCircle(x+w-r, y+r,   r);
  tft_->fillCircle(x+r,   y+h-r, r);
  tft_->fillCircle(x+w-r, y+h-r, r);
}

void DisplayUI_UTFT::drawRoundRectR(int x,int y,int w,int h,int r,const RGB& c) {
  setColor(c);
  // рамка закр. углами (окаймление)
  tft_->drawRoundRect(x, y, x+w-1, y+h-1); // у UTFT есть drawRoundRect — отлично!
}

void DisplayUI_UTFT::printAt(int x,int y,const char* s,const RGB& col,uint8_t* font, bool transparent) {
  tft_->setFont(font);
  setColor(col);
  //if (transparent) tft_->setBackColor(0,0,0); // "прозрачность" в UTFT = чёрный как отключённый фон
  if (transparent) {
    tft_->setBackColor(VGA_TRANSPARENT);          // ← было (0,0,0)
  } else {
    tft_->setBackColor(col.r, col.g, col.b);      // однородный фон, если вдруг понадобится
  }
  tft_->print((char*)s, x, y);
}

void DisplayUI_UTFT::begin(UTFT& lcd, uint8_t landscape, uint8_t cells) {
  tft_ = &lcd;

  // ИНИЦИАЛИЗАЦИЯ LCD как в UTFT (ты раньше так и делал)
  // Пример: myGLCD.InitLCD(LANDSCAPE);
  tft_->InitLCD(landscape ? LANDSCAPE : PORTRAIT);

  // Размеры (для LANDSCAPE — 480x320, для PORTRAIT — 320x480)
  if (landscape) { W_ = 480; H_ = 320; } else { W_ = 320; H_ = 480; }

  // Фон и первичный фрейм
  fillRectR(0,0,W_,H_, COL_BG);
  last_.cells = cells;
  tft_->setBackColor(VGA_TRANSPARENT);   // ← ВАЖНО: глобально прозрачный фон текста
  drawFrame();
}

void DisplayUI_UTFT::drawFrame() {
  // Полная подложка:
  fillRectR(0, 0, W_, H_, COL_BG);
  // Header плашка
  fillRoundRectR(headerX_, headerY_, W_-16, headerH_, 6, COL_CARD);
  printAt(20, headerY_ + 16, "UKROPCHIK NSU", COL_TEXT, BigFont);

  // Левая колонка — статические метки
  const char* labels[] = { "VIDEO","BAND","CHANNEL","RSSI","CONTROL","REC","V_BYPASS" };
  for (int i=0;i<7;i++) {
    int y = leftY0_ + i*(rowH_+rowGap_);
    fillRoundRectR(leftX_, y, leftW_, rowH_, 6, COL_CARD);
    printAt(leftX_+10, y+8, labels[i], COL_LABEL, SmallFont);
  }

  // Правый блок — рамка компаса
  fillRoundRectR(rightX_, rightY_, rightW_, rightH_, 6, COL_CARD);
  printAt(rightX_+10, rightY_+8, "AZIMUTH", COL_LABEL, SmallFont);
}

void DisplayUI_UTFT::drawHeaderBar(float voltage, int percent) {
  // Плашка заголовка
  fillRoundRectR(headerX_, headerY_, W_-16, headerH_, 6, COL_CARD);
  printAt(20, headerY_ + 16, "UKROPCHIK NSU", COL_TEXT, BigFont);

  // Батарейка справа
  int bx = W_ - 180, by = headerY_ + 8, bw = 150, bh = 36;
  fillRoundRectR(bx, by, bw, bh, 6, COL_BG);

  // Иконка батареи
  int ix = bx + 6, iy = by + 8, iw = 46, ih = 20, cap = 5;

  // Рамка батареи (контур)
  setColor(COL_BLACK);
  tft_->drawRect(ix-1, iy-1, ix+iw+cap, iy+ih+1);
  // “крышечка”
  tft_->fillRect(ix+iw, iy+5, ix+iw+cap, iy+ih-5);

  // Заполнение уровня
  int fillW = imap(percent, 0, 100, 0, iw);
  RGB lev = (percent >= 60) ? COL_OK : (percent >= 25 ? COL_WARN : COL_BAD);
  fillRectR(ix, iy, fillW, ih, lev);
  if (fillW < iw) fillRectR(ix+fillW, iy, iw-fillW, ih, COL_BG);

  // Надпись “XX.XXV  (YY%)”
  char vbuf[16]; dtostrf(voltage, 4, 2, vbuf);
  char line[32]; snprintf(line, sizeof(line), "%sV  (%d%%)", vbuf, percent);
  printAt(ix + iw + cap + 10, by + 12, line, COL_TEXT, SmallFont);
}

void DisplayUI_UTFT::drawLabelValueRow(int row, const char* label, const char* value, bool highlight) {
  const int y = leftY0_ + row*(rowH_+rowGap_);
  fillRoundRectR(leftX_, y, leftW_, rowH_, 6, COL_CARD);
  printAt(leftX_+10, y+8, label, COL_LABEL, SmallFont);

  if (highlight) {
  fillRoundRectR(leftX_+120, y+4, 160, rowH_-8, 6, COL_BLACK);
  setColor(COL_OK);
  tft_->setBackColor(VGA_TRANSPARENT);            // ← было (0,0,0)
  tft_->setFont(SmallFont);
  tft_->print((char*)(value ? value : "--"), leftX_+130, y+8);
} else {
  printAt(leftX_+130, y+8, value ? value : "--", COL_TEXT, SmallFont);
}
}

void DisplayUI_UTFT::drawAzimuthCard(int az) {
  fillRoundRectR(rightX_, rightY_, rightW_, rightH_, 6, COL_CARD);
  printAt(rightX_+10, rightY_+8, "AZIMUTH", COL_LABEL, SmallFont);

  int cx = rightX_ + rightW_/2;
  int cy = rightY_ + rightH_/2 + 10;
  int r  = 36;

  setColor(COL_TEXT);
  tft_->drawCircle(cx, cy, r);
  tft_->drawCircle(cx, cy, 2);

  float rad = az * 3.1415926f / 180.0f;
  int mx = cx + (int)(cos(rad) * (r-5));
  int my = cy - (int)(sin(rad) * (r-5));

  setColor(COL_OK);
  tft_->fillCircle(mx, my, 3);

  char d[12]; snprintf(d, sizeof(d), "%d%c", az, 176);
  printAt(cx-24, rightY_+rightH_-14, d, COL_TEXT, SmallFont);
}

void DisplayUI_UTFT::render(const UIData& d) {
  // Шапка (напряжение + %)
  if (fabs(d.voltage_V - last_.voltage_V) > 0.05f || d.cells != last_.cells) {
    last_.voltage_V = d.voltage_V;
    last_.cells     = d.cells;
    int percent = voltageToPercent(d.voltage_V, d.cells);
    drawHeaderBar(d.voltage_V, percent);
  }

  char buf[24];
  if (d.freq_MHz != last_.freq_MHz) {
    snprintf(buf, sizeof(buf), "%u MHz", (unsigned)d.freq_MHz);
    drawLabelValueRow(0, "VRX", buf);
    last_.freq_MHz = d.freq_MHz;
  }
  if (d.freq_MHz != last_.freq_MHz) {
    snprintf(buf, sizeof(buf), "%u MHz", (unsigned)d.freq_MHz);
    drawLabelValueRow(1, "VIDEO", buf);
    last_.freq_MHz = d.freq_MHz;
  }
  if (d.bandChar != last_.bandChar) {
    if (d.bandChar) { buf[0]=d.bandChar; buf[1]=0; }
    else strcpy(buf, "--");
    drawLabelValueRow(2, "BAND", buf);
    last_.bandChar = d.bandChar;
  }
  if (d.channel != last_.channel) {
    snprintf(buf, sizeof(buf), "%u", (unsigned)d.channel);
    drawLabelValueRow(3, "CHANNEL", buf);
    last_.channel = d.channel;
  }
  if (d.rssi_dB != last_.rssi_dB) {
    snprintf(buf, sizeof(buf), "%d dB", d.rssi_dB);
    bool poor = (d.rssi_dB < 30);
    drawLabelValueRow(4, "RSSI", buf, poor);
    last_.rssi_dB = d.rssi_dB;
  }
  if (d.control != last_.control) {
    drawLabelValueRow(5, "CONTROL", d.control ? d.control : "--");
    last_.control = d.control;
  }
  if (d.recording != last_.recording) {
    drawLabelValueRow(6, "REC", d.recording ? "REC" : "STOP", d.recording);
    last_.recording = d.recording;
  }
  if (d.v_bypass != last_.v_bypass) {
    drawLabelValueRow(6, "V_BYPASS", d.v_bypass ? "ON" : "OFF", d.v_bypass);
    last_.v_bypass = d.v_bypass;
  }
  if (d.azimuth_deg != last_.azimuth_deg) {
    drawAzimuthCard(d.azimuth_deg);
    last_.azimuth_deg = d.azimuth_deg;
  }
}
