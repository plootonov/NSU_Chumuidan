#pragma once
#include <Arduino.h>
#include <UTFT.h>

// Встроенные шрифты UTFT
extern uint8_t SmallFont[];
extern uint8_t BigFont[];



// Что именно редактируем
enum ConfigItem : uint8_t {
  CFG_BAND = 0,   // видеополоса (band)
  CFG_CHAN,       // видеоканал
  CFG_RECORD,     // запись
  CFG_BYPASS,     // V_BYPASS
  CFG_ITEMS_COUNT
};

// Текущее состояние системы (то, что редактируем)
struct ConfigState {
  uint8_t vrxMode;    // 1 = 5.8G, 2 = 1.2G (как у тебя)
  uint8_t vrxband;    // 0..6 (для 5.8G), иначе игнор
  uint8_t vrxchan;    // 0..7 (или др. пределы)
  uint8_t record;     // 0/1
  uint8_t bypass;     // 0..2
};

// Таблицы строк для значений
struct ConfigLabels {
  // массивы строк и их длина — ты подаёшь свои (videoband, videochan, rec, Sbypass)
  const char* const* bands;   uint8_t bandsCount;   // например 7
  const char* const* chans;   uint8_t chansCount;   // например 8
  const char* const* rec;     uint8_t recCount;     // 2 ("STOP","REC")
  const char* const* bypass;  uint8_t bypassCount;  // 3 ("OFF","ON","AUTO")
};

// Колбэки на изменение (опционально)
typedef void (*OnRecordChanged)(uint8_t newVal);
typedef void (*OnBypassChanged)(uint8_t newVal);

// Модуль UI конфигурации
class ConfigUI_UTFT {
public:
  // Инициализация
  void begin(UTFT& lcd,
             uint8_t pinUp, uint8_t pinDown, uint8_t pinLeft, uint8_t pinRight,
             const ConfigLabels& labels,
             uint16_t startY = 64,   // геометрия блока меню
             uint16_t blockW = 440);

  // Установить внешние колбэки
  void setCallbacks(OnRecordChanged onRec, OnBypassChanged onByp) {
    onRecChanged_ = onRec; onBypassChanged_ = onByp;
  }
  void forceRedraw() { needFullRedraw_ = true; }
  // Полная перерисовка рамки
  void drawFrame(const char* title = "CONFIGURATION MODE");

  // Тик: читает кнопки, обновляет состояние, при изменениях — перерисовывает
  // Возвращает true, если были изменения (для твоей логики сохранений)
  bool tick(ConfigState& st, bool editMode);

  // Принудительно перерисовать текущий пункт и значения
  void render(const ConfigState& st);

  // Сброс курсора (например, при входе в editMode)
  void resetCursor() { cursor_ = 0; needFullRedraw_ = true; }
  void setScreenSize(uint16_t screenW, uint16_t screenH) { scrW_ = screenW; scrH_ = screenH; }

private:
  // Рисовалки
  void setColor(uint8_t r,uint8_t g,uint8_t b);
  void setBackTransparent();
  void fillRoundRect(int x,int y,int w,int h,int r,uint8_t rC,uint8_t gC,uint8_t bC);
  void drawTitle(const char* title);
  void drawRow(int y, const char* text, bool dim);
  void drawCurrentRow(int y, const char* text, const char* value);
  void computeCurrentStrings(const ConfigState& st, char* labelBuf, size_t lsz,
                             char* valueBuf, size_t vsz);

  
  
  // Хелперы логики
  void applyLeft(ConfigState& st);
  void applyRight(ConfigState& st);
  const char* valueForItem(const ConfigState& st, ConfigItem item);

private:
  UTFT* tft_ = nullptr;

  // GPIO
  uint8_t pinUp_ = 0, pinDown_ = 0, pinLeft_ = 0, pinRight_ = 0;
  unsigned long lastBtnMs_ = 0;
  uint16_t debounceMs_ = 120;
  uint16_t scrW_ = 480, scrH_ = 320;  // фактический размер экрана
uint16_t titleH_ = 40;              // высота ленты заголовка
uint16_t rowGap_ = 10;              // отступы между строками

  // Геометрия
  uint16_t X_ = 16;
  uint16_t Y_ = 64;      // верх блока
  uint16_t W_ = 440;
  uint16_t rowH_ = 48;

  // Данные/состояние
  ConfigLabels labels_{};
  int8_t cursor_ = 0;            // 0..CFG_ITEMS_COUNT-1
  bool needFullRedraw_ = true;

  // Колбэки
  OnRecordChanged  onRecChanged_  = nullptr;
  OnBypassChanged  onBypassChanged_ = nullptr;
};
