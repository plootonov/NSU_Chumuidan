#include <UTFT.h>
#include "ConfigUI_UTFT.h"
#include "DisplayUI_UTFT.h"   // если используешь общий UI из прошлого шага
#include <EEPROM.h>


// ===== твой дисплей =====
UTFT myGLCD(TFT32MEGA /*или TFT32MEGA_2*/, 38,39,40,41);

// ===== пины кнопок (низкий уровень = нажато) =====
const uint8_t Butt_control_UP    = 8;
const uint8_t Butt_control_DOWN  = 10;
const uint8_t Butt_control_LEFT  = 6;
const uint8_t Butt_control_RIGHT = 7;
const uint8_t Butt_control_ENTER = 9;


// ===== таблицы строк (подставь свои) =====
const char* videoband[] = { "A", "B", "E", "F", "R", "L", "H" };
const char* videochan[] = { "1","2","3","4","5","6","7","8" };
const char* recTbl[]    = { "STOP","REC" };
const char* byTbl[]     = { "OFF","ON","AUTO" };

ConfigLabels cfgLabels = {
  videoband, (uint8_t)(sizeof(videoband)/sizeof(videoband[0])),
  videochan, (uint8_t)(sizeof(videochan)/sizeof(videochan[0])),
  recTbl,    (uint8_t)(sizeof(recTbl)/sizeof(recTbl[0])),
  byTbl,     (uint8_t)(sizeof(byTbl)/sizeof(byTbl[0]))
};

// ===== состояние =====
ConfigState cfg = {
  1,   // vrxMode: 1=5.8G
  0,   // vrxband
  0,   // vrxchan
  0,   // record
  0    // bypass
};

const unsigned long EN_HOLD_MS = 3000;   // 3 секунды

bool editMode = false;
unsigned long enPressStartMs = 0;
bool enPrev = false;

// ===== модули UI =====
ConfigUI_UTFT  cfgUI;
DisplayUI_UTFT mainUI;   // если используешь основной экран

// ===== колбэки =====
void reco(uint8_t r) { /* твоя логика включить/выключить запись */ }
void bypass_control(uint8_t b) { /* твоя логика bypass */ }

float readVoltage_V() {
  const int pin = A0;
  int raw = analogRead(pin);
  const float vref = 5.0;
  return (raw * vref) / 1023.0 * ((10000.0 + 2345.0) / 2345.0);
}

struct PersistCfg {
  uint8_t  version;    // 1
  uint8_t  vrxMode;
  uint8_t  vrxband;
  uint8_t  vrxchan;
  uint8_t  record;
  uint8_t  bypass;
  uint8_t  crc;
};

uint8_t calcCRC(const uint8_t* p, size_t n) {
  uint8_t c = 0;
  for (size_t i=0;i<n;i++) c ^= p[i];
  return c;
}

void saveConfigToEEPROM(const ConfigState& s) {
  PersistCfg pc{};
  pc.version = 1;
  pc.vrxMode = s.vrxMode;
  pc.vrxband = s.vrxband;
  pc.vrxchan = s.vrxchan;
  pc.record  = s.record;
  pc.bypass  = s.bypass;
  pc.crc     = 0;
  pc.crc     = calcCRC((uint8_t*)&pc, sizeof(pc));

  for (unsigned i=0;i<sizeof(pc);++i)
    EEPROM.update(i, ((uint8_t*)&pc)[i]);
}

bool loadConfigFromEEPROM(ConfigState& s) {
  PersistCfg pc{};
  for (unsigned i=0;i<sizeof(pc);++i)
    ((uint8_t*)&pc)[i] = EEPROM.read(i);

  if (pc.version != 1) return false;
  uint8_t crc = pc.crc; pc.crc = 0;
  if (calcCRC((uint8_t*)&pc, sizeof(pc)) != crc) return false;

  s.vrxMode = pc.vrxMode;
  s.vrxband = pc.vrxband;
  s.vrxchan = pc.vrxchan;
  s.record  = pc.record;
  s.bypass  = pc.bypass;
  return true;
}


void enterConfigMode() {
  editMode = true;
  cfgUI.resetCursor();
  cfgUI.forceRedraw();
  cfgUI.drawFrame("CONFIGURATION MODE");
  cfgUI.render(cfg);
}

void exitConfigModeAndSave() {
  editMode = false;
  saveConfigToEEPROM(cfg);
  mainUI.drawFrame();   // ← перерисуем основной экран (он сам зальёт фон)
}

void setup() {
  pinMode(Butt_control_ENTER, INPUT_PULLUP);
  myGLCD.InitLCD(LANDSCAPE);
  myGLCD.clrScr();
  myGLCD.setBackColor(VGA_TRANSPARENT);   // прозрачный фон текста
  if (!loadConfigFromEEPROM(cfg)) {
  // defaults уже в cfg
  }
  // основной UI (если нужен)
  mainUI.begin(myGLCD, /*landscape=*/1, /*cells=*/4);

  // конфиг-UI
  cfgUI.setScreenSize(480, 320);   // ландшафт
  cfgUI.begin(myGLCD,
              Butt_control_UP, Butt_control_DOWN, Butt_control_LEFT, Butt_control_RIGHT,
              cfgLabels,
              /*startY=*/180, /*blockW=*/448);   // можно подвинуть ниже/выше
  cfgUI.setCallbacks(reco, bypass_control);
  cfgUI.resetCursor();

}



void loop() {
  // --- удержание EN 3 сек ---
  bool enNow = (digitalRead(Butt_control_ENTER) == LOW);
  unsigned long now = millis();
  static unsigned long enPressStartMs = 0;
  static bool enPrev = false;

  if (enNow && !enPrev) enPressStartMs = now;
  if (!enNow && enPrev) enPressStartMs = 0;

  if (enNow && (now - enPressStartMs >= EN_HOLD_MS)) {
    enPressStartMs = now + 100000UL; // защита от повторных, пока держим
    if (!editMode) enterConfigMode();
    else           exitConfigModeAndSave();
  }
  enPrev = enNow;

  // --- отрисовка ---
  if (editMode) {
    cfgUI.tick(cfg, true);     // меняем значения на лету, меню само перерисует строки
  } else {
    UIData d{};
    d.voltage_V  = readVoltage_V();
    d.cells      = 4;
    d.freq_MHz   = 5800;
    d.bandChar   = (cfg.vrxMode==1)? videoband[cfg.vrxband][0] : '-';
    d.channel    = cfg.vrxchan+1;
    d.rssi_dB    = 52;
    d.control    = "ELRS";
    d.recording  = (cfg.record != 0);
    d.v_bypass   = (cfg.bypass != 0);
    d.azimuth_deg = 120;
    mainUI.render(d);
  }

  delay(10);
}
