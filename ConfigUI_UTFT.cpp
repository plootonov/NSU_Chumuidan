#include "ConfigUI_UTFT.h"

// ===== ВНУТРЕННИЕ УТИЛИТЫ РИСОВАНИЯ (на UTFT) =====

void ConfigUI_UTFT::setColor(uint8_t r,uint8_t g,uint8_t b) {
  tft_->setColor(r,g,b);
}
void ConfigUI_UTFT::setBackTransparent() {
  tft_->setBackColor(VGA_TRANSPARENT);
}

static inline int imap_(int x,int in_min,int in_max,int out_min,int out_max){
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

// грубая закруглённая плашка
void ConfigUI_UTFT::fillRoundRect(int x,int y,int w,int h,int r,uint8_t rC,uint8_t gC,uint8_t bC) {
  setColor(rC,gC,bC);
  tft_->fillRect(x+r, y,   x+w-r-1, y+h-1);
  tft_->fillRect(x,   y+r, x+r-1,   y+h-r-1);
  tft_->fillRect(x+w-r, y+r, x+w-1, y+h-r-1);
  tft_->fillCircle(x+r,   y+r,   r);
  tft_->fillCircle(x+w-r, y+r,   r);
  tft_->fillCircle(x+r,   y+h-r, r);
  tft_->fillCircle(x+w-r, y+h-r, r);
}

// ===== ЖИЗНЕННЫЙ ЦИКЛ =====

void ConfigUI_UTFT::begin(UTFT& lcd,
                          uint8_t pinUp, uint8_t pinDown, uint8_t pinLeft, uint8_t pinRight,
                          const ConfigLabels& labels,
                          uint16_t startY, uint16_t blockW)
{
  tft_ = &lcd;
    // размеры с учётом выбранной ориентации в InitLCD()
  scrW_ = tft_->getDisplayXSize();
  scrH_ = tft_->getDisplayYSize();
   pinUp_ = pinUp; pinDown_ = pinDown; pinLeft_ = pinLeft; pinRight_ = pinRight;
  labels_ = labels;
  Y_ = startY; W_ = blockW;
  
  pinMode(pinUp_,    INPUT_PULLUP);
  pinMode(pinDown_,  INPUT_PULLUP);
  pinMode(pinLeft_,  INPUT_PULLUP);
  pinMode(pinRight_, INPUT_PULLUP);

  setBackTransparent();
  needFullRedraw_ = true;
}

void ConfigUI_UTFT::drawTitle(const char* title) {
  // Заголовок/лента
  fillRoundRect(X_, Y_, W_, 40, 6, 24,48,72); // COL_CARD
  tft_->setFont(BigFont);
  setColor(255,255,255);
  setBackTransparent();
  tft_->print((char*)title, X_ + 10, Y_ + 12);
}

void ConfigUI_UTFT::drawRow(int y, const char* text, bool dim) {
  fillRoundRect(X_, y, W_, rowH_, 6, 24,48,72); // COL_CARD
  tft_->setFont(SmallFont);
  if (dim) setColor(150,150,150); else setColor(255,255,255);
  setBackTransparent();
  tft_->print((char*)text, X_ + 10, y + 14);
}

void ConfigUI_UTFT::drawCurrentRow(int y, const char* text, const char* value) {
  // строка
  fillRoundRect(X_, y, W_, rowH_, 6, 24,48,72);
  tft_->setFont(SmallFont);
  setColor(255,255,255);
  setBackTransparent();
  tft_->print((char*)text, X_ + 10, y + 14);

  // “пилюля” справа + значение
  fillRoundRect(X_ + W_ - 180, y + 6, 160, rowH_ - 12, 8, 0,0,0); // чёрная подложка
  tft_->setFont(SmallFont);
  setColor(0,255,0); // зелёный текст
  setBackTransparent();
  tft_->print((char*)(value ? value : "--"), X_ + W_ - 170, y + 14);
}

void ConfigUI_UTFT::drawFrame(const char* title) {
  // 1) Полная подложка на весь экран
  setColor(8,16,24); // COL_BG
  tft_->fillRect(0, 0, scrW_-1, scrH_-1);

  // 2) Рассчитать центрирование по вертикали:
  //    Заголовок + 3 строки + два промежутка
  uint16_t totalH = titleH_ + 3*rowH_ + 2*rowGap_ + 12; // +12 небольшой нижний отступ
  if (totalH > scrH_) totalH = scrH_; // страховка
  Y_ = (scrH_ - totalH) / 2;          // верх блока по центру
  X_ =  (scrW_ - W_) / 2;              // по центру по ширине (если W_ < scrW_)

  // 3) Лента заголовка
  fillRoundRect(X_, Y_, W_, titleH_, 6, 24,48,72); // COL_CARD
  tft_->setFont(BigFont);
  setColor(255,255,255);
  setBackTransparent();
  tft_->print((char*)title, X_ + 10, Y_ + 12);

  // 4) Три строки: prev/cur/next
  const int y1 = Y_ + titleH_ + 12;
  const int y2 = y1 + rowH_ + rowGap_;
  const int y3 = y2 + rowH_ + rowGap_;

  drawRow(y1, "...", true);
  drawRow(y2, "...", false);
  drawRow(y3, "...", true);

  needFullRedraw_ = false;
}

void ConfigUI_UTFT::computeCurrentStrings(const ConfigState& st,
                                          char* labelBuf, size_t lsz,
                                          char* valueBuf, size_t vsz)
{
  static const char* const names[] = { "VIDEO BAND", "CHANNEL", "RECORDING", "V_BYPASS" };
  // Текст пункта
  strncpy(labelBuf, names[cursor_], lsz);
  labelBuf[lsz-1] = 0;

  // Значение
  const char* val = valueForItem(st, (ConfigItem)cursor_);
  if (!val) val = "--";
  strncpy(valueBuf, val, vsz);
  valueBuf[vsz-1] = 0;
}

const char* ConfigUI_UTFT::valueForItem(const ConfigState& st, ConfigItem it) {
  switch (it) {
    case CFG_BAND:
      if (st.vrxMode == 1) { // 5.8G
        uint8_t i = st.vrxband; if (i >= labels_.bandsCount) i = labels_.bandsCount-1;
        return labels_.bands ? labels_.bands[i] : nullptr;
      } else {
        return "N/A";
      }
    case CFG_CHAN: {
      uint8_t i = st.vrxchan; if (i >= labels_.chansCount) i = labels_.chansCount-1;
      return labels_.chans ? labels_.chans[i] : nullptr;
    }
    case CFG_RECORD: {
      uint8_t i = st.record; if (i >= labels_.recCount) i = labels_.recCount-1;
      return labels_.rec ? labels_.rec[i] : nullptr;
    }
    case CFG_BYPASS: {
      uint8_t i = st.bypass; if (i >= labels_.bypassCount) i = labels_.bypassCount-1;
      return labels_.bypass ? labels_.bypass[i] : nullptr;
    }
    default: return nullptr;
  }
}

void ConfigUI_UTFT::render(const ConfigState& st) {
  // координаты строк
  const int yPrev = Y_ + 52;
  const int yCur  = yPrev + rowH_ + 10;
  const int yNext = yCur  + rowH_ + 10;

  // Сформируем подписи для prev/cur/next
  auto nameOf = [](uint8_t idx)->const char* {
    static const char* const names[] = { "VIDEO BAND", "CHANNEL", "RECORDING", "V_BYPASS" };
    if (idx >= CFG_ITEMS_COUNT) return "";
    return names[idx];
  };

  // prev
  {
    uint8_t p = (cursor_ + CFG_ITEMS_COUNT - 1) % CFG_ITEMS_COUNT;
    drawRow(yPrev, nameOf(p), true);
  }
  // cur
  {
    char label[24]; char value[24];
    computeCurrentStrings(st, label, sizeof(label), value, sizeof(value));
    drawCurrentRow(yCur, label, value);
  }
  // next
  {
    uint8_t n = (cursor_ + 1) % CFG_ITEMS_COUNT;
    drawRow(yNext, nameOf(n), true);
  }
}

// ===== ЛОГИКА КНОПОК / ИЗМЕНЕНИЙ =====

void ConfigUI_UTFT::applyLeft(ConfigState& st) {
  switch (cursor_) {
    case CFG_BAND:
      if (st.vrxMode == 1) { // только для 5.8G
        if (st.vrxband > 0) st.vrxband--;
      }
      break;
    case CFG_CHAN:
      if (st.vrxchan > 0) st.vrxchan--;
      break;
    case CFG_RECORD:
      if (st.record > 0) {
        st.record--;
        if (onRecChanged_) onRecChanged_(st.record);
      }
      break;
    case CFG_BYPASS:
      if (st.bypass > 0) {
        st.bypass--;
        if (onBypassChanged_) onBypassChanged_(st.bypass);
      }
      break;
  }
}

void ConfigUI_UTFT::applyRight(ConfigState& st) {
  switch (cursor_) {
    case CFG_BAND:
      if (st.vrxMode == 1 && labels_.bandsCount) {
        if (st.vrxband + 1 < labels_.bandsCount) st.vrxband++;
      }
      break;
    case CFG_CHAN:
      if (labels_.chansCount) {
        if (st.vrxchan + 1 < labels_.chansCount) st.vrxchan++;
      }
      break;
    case CFG_RECORD:
      if (labels_.recCount) {
        if (st.record + 1 < labels_.recCount) st.record++;
        if (onRecChanged_) onRecChanged_(st.record);
      }
      break;
    case CFG_BYPASS:
      if (labels_.bypassCount) {
        if (st.bypass + 1 < labels_.bypassCount) st.bypass++;
        if (onBypassChanged_) onBypassChanged_(st.bypass);
      }
      break;
  }
}

bool ConfigUI_UTFT::tick(ConfigState& st, bool editMode) {
  bool changed = false;

  // Первичный фрейм, если нужно
  if (needFullRedraw_) {
    drawFrame("CONFIGURATION MODE");
    render(st);
    needFullRedraw_ = false;
  }

  // Только в режиме редактирования обрабатываем кнопки
  if (!editMode) return false;

  unsigned long now = millis();
  if (now - lastBtnMs_ < debounceMs_) return false;
  int up    = digitalRead(pinUp_)    == LOW;
  int down  = digitalRead(pinDown_)  == LOW;
  int left  = digitalRead(pinLeft_)  == LOW;
  int right = digitalRead(pinRight_) == LOW;

  if (up) {
    cursor_ = (cursor_ + CFG_ITEMS_COUNT - 1) % CFG_ITEMS_COUNT;
    render(st);
    changed = true;
  } else if (down) {
    cursor_ = (cursor_ + 1) % CFG_ITEMS_COUNT;
    render(st);
    changed = true;
  } else if (left) {
    applyLeft(st);
    render(st);
    changed = true;
  } else if (right) {
    applyRight(st);
    render(st);
    changed = true;
  }

  if (changed) lastBtnMs_ = now;
  return changed;
}
