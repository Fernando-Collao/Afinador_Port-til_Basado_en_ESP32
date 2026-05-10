/*
 * PROGRAMA PRINCIPAL - MENÚ DE 3 FUNCIONES
 * - Opción 1: Afinador
 * - Opción 2: Metrónomo
 * - Opción 3: Selector de Tonalidad
 * 
 * Controles:
 * - Botón UP (GPIO13): Navegar hacia arriba
 * - Botón DOWN (GPIO12): Navegar hacia abajo
 * - Botón SELECT (GPIO14): Seleccionar función
 * - Botón BACK (GPIO27): Pulsación corta: función original del módulo
 *                        Pulsación larga (3s): Volver al menú principal
 */

#include "arduinoFFT.h"

// Pines para los pulsadores del menú
#define MENU_BTN_UP 13
#define MENU_BTN_DOWN 12
#define MENU_BTN_SELECT 14
#define MENU_BTN_BACK 27

// Estados de los botones del menú
bool lastMenuBtnUpState = HIGH;
bool lastMenuBtnDownState = HIGH;
bool lastMenuBtnSelectState = HIGH;
bool lastMenuBtnBackState = HIGH;

// Variables del menú
int menuIndex = 0; // 0: Afinador, 1: Metrónomo, 2: Selector de tonalidad
int currentMode = 0; // 0: Menú, 1: Afinador, 2: Metrónomo, 3: Selector de tonalidad

// Variable para anti-rebote
unsigned long lastDebounceTime = 0;
const unsigned long DEBOUNCE_DELAY = 50;

// Variable para detectar pulsación larga de BACK
unsigned long backPressStartTime = 0;
bool backPressed = false;
bool backLongPressHandled = false;
const unsigned long BACK_HOLD_TIME = 3000; // 3 segundos para volver al menú

// ==================== VARIABLES GLOBALES DEL AFINADOR ====================
#define AFINADOR_CHANNEL 4
const uint16_t afinador_samples = 1024;
const double afinador_samplingFrequency = 8000;
unsigned int afinador_sampling_period_us;
unsigned long afinador_microseconds;

double afinador_vReal[1024];
double afinador_vImag[1024];

int AFINADOR_LED_BEMOL = 15;
int AFINADOR_LED_SOS = 33;
int AFINADOR_LED_OK = 32;

ArduinoFFT<double> afinador_FFT = ArduinoFFT<double>(afinador_vReal, afinador_vImag, afinador_samples, afinador_samplingFrequency);

const char* afinador_nombresNotas[] = {
  "C", "C#", "D", "D#", "E", "F",
  "F#", "G", "G#", "A", "A#", "B"
};
const double afinador_A4_FREQ = 440.0;
const int afinador_A4_MIDI = 69;
const double afinador_MARGEN_AFINADO = 1.0;

// ==================== VARIABLES GLOBALES DEL METRÓNOMO ====================
const int METRO_SPEAKER_PIN = 25;
const int METRO_LED_PIN = 32;

const int METRO_BTN_UP = 13;
const int METRO_BTN_DOWN = 12;
const int METRO_BTN_SELECT = 14;
const int METRO_BTN_BACK = 27;
const int METRO_BTN_TAP = 19;

const int METRO_MIN_BPM = 40;
const int METRO_MAX_BPM = 250;

const int METRO_BPM_PRESETS[] = {60, 80, 120, 140, 180};
const int METRO_NUM_PRESETS = 5;
int metro_currentPresetIndex = 2;
bool metro_presetMode = false;

const int METRO_TAP_HISTORY_SIZE = 4;
unsigned long metro_tapTimes[4];
int metro_tapIndex = 0;
unsigned long metro_lastTapTime = 0;
const unsigned long METRO_TAP_TIMEOUT = 2000;

struct metro_TempoRange {
  const char* nombre;
  int minBPM;
  int maxBPM;
};

const metro_TempoRange metro_tempoTable[] = {
  {"Lento      ", 40, 60},
  {"Larghetto  ", 60, 66},
  {"Adagio     ", 66, 76},
  {"Andante    ", 76, 108},
  {"Moderato   ", 108, 120},
  {"Allegro    ", 120, 168},
  {"Vivace     ", 168, 176},
  {"Presto     ", 176, 200},
  {"Prestissimo", 200, 250}
};

const int METRO_TEMPO_COUNT = 9;

int metro_currentBPM = 120;
int metro_beatInterval;
int metro_beatCounter = 0;
int metro_timeSignature = 4;
unsigned long metro_lastBeatTime = 0;

unsigned long metro_lastSerialUpdate = 0;
const int METRO_SERIAL_UPDATE_INTERVAL = 500;

unsigned long metro_lastDebounceTime = 0;
const unsigned long METRO_DEBOUNCE_DELAY = 50;
int metro_lastBtnUpState = HIGH;
int metro_lastBtnDownState = HIGH;
int metro_lastBtnSelectState = HIGH;
int metro_lastBtnBackState = HIGH;
int metro_lastBtnTapState = HIGH;
int metro_btnUpState;
int metro_btnDownState;
int metro_btnSelectState;
int metro_btnBackState;
int metro_btnTapState;

// ==================== VARIABLES GLOBALES DEL SELECTOR DE TONALIDAD ====================
bool tonal_lastBtnUpState = HIGH;
bool tonal_lastBtnDownState = HIGH;
bool tonal_lastBtnSelectState = HIGH;
bool tonal_lastBtnBackState = HIGH;

struct tonal_Tonalidad {
  String nombre;
  String sostenidos[7];
  String bemoles[7];
  int numSostenidos;
  int numBemoles;
  String tipo;
};

tonal_Tonalidad tonal_tonalidadesMayores[12] = {
  {"C", {"", "", "", "", "", "", ""}, {"", "", "", "", "", "", ""}, 0, 0, "mayor"},
  {"G", {"F#", "", "", "", "", "", ""}, {"", "", "", "", "", "", ""}, 1, 0, "mayor"},
  {"D", {"F#", "C#", "", "", "", "", ""}, {"", "", "", "", "", "", ""}, 2, 0, "mayor"},
  {"A", {"F#", "C#", "G#", "", "", "", ""}, {"", "", "", "", "", "", ""}, 3, 0, "mayor"},
  {"E", {"F#", "C#", "G#", "D#", "", "", ""}, {"", "", "", "", "", "", ""}, 4, 0, "mayor"},
  {"B", {"F#", "C#", "G#", "D#", "A#", "", ""}, {"", "", "", "", "", "", ""}, 5, 0, "mayor"},
  {"F#/Gb", {"F#", "C#", "G#", "D#", "A#", "E#", ""}, {"Bb", "Eb", "Ab", "Db", "Gb", "Cb", ""}, 6, 6, "mayor"},
  {"Db", {"", "", "", "", "", "", ""}, {"Bb", "Eb", "Ab", "Db", "Gb", "", ""}, 0, 5, "mayor"},
  {"Ab", {"", "", "", "", "", "", ""}, {"Bb", "Eb", "Ab", "Db", "", "", ""}, 0, 4, "mayor"},
  {"Eb", {"", "", "", "", "", "", ""}, {"Bb", "Eb", "Ab", "", "", "", ""}, 0, 3, "mayor"},
  {"Bb", {"", "", "", "", "", "", ""}, {"Bb", "Eb", "", "", "", "", ""}, 0, 2, "mayor"},
  {"F", {"", "", "", "", "", "", ""}, {"Bb", "", "", "", "", "", ""}, 0, 1, "mayor"}
};

tonal_Tonalidad tonal_tonalidadesMenores[12] = {
  {"A", {"", "", "", "", "", "", ""}, {"", "", "", "", "", "", ""}, 0, 0, "menor"},
  {"E", {"F#", "", "", "", "", "", ""}, {"", "", "", "", "", "", ""}, 1, 0, "menor"},
  {"B", {"F#", "C#", "", "", "", "", ""}, {"", "", "", "", "", "", ""}, 2, 0, "menor"},
  {"F#", {"F#", "C#", "G#", "", "", "", ""}, {"", "", "", "", "", "", ""}, 3, 0, "menor"},
  {"C#", {"F#", "C#", "G#", "D#", "", "", ""}, {"", "", "", "", "", "", ""}, 4, 0, "menor"},
  {"G#", {"F#", "C#", "G#", "D#", "A#", "", ""}, {"", "", "", "", "", "", ""}, 5, 0, "menor"},
  {"D#/Eb", {"F#", "C#", "G#", "D#", "A#", "E#", ""}, {"Bb", "Eb", "Ab", "Db", "Gb", "Cb", ""}, 6, 6, "menor"},
  {"A#/Bb", {"", "", "", "", "", "", ""}, {"Bb", "Eb", "Ab", "Db", "Gb", "", ""}, 0, 5, "menor"},
  {"F", {"", "", "", "", "", "", ""}, {"Bb", "Eb", "Ab", "Db", "", "", ""}, 0, 4, "menor"},
  {"C", {"", "", "", "", "", "", ""}, {"Bb", "Eb", "Ab", "", "", "", ""}, 0, 3, "menor"},
  {"G", {"", "", "", "", "", "", ""}, {"Bb", "Eb", "", "", "", "", ""}, 0, 2, "menor"},
  {"D", {"", "", "", "", "", "", ""}, {"Bb", "", "", "", "", "", ""}, 0, 1, "menor"}
};

String tonal_notasNaturales[7] = {"C", "D", "E", "F", "G", "A", "B"};

int tonal_indiceActual = 0;
int tonal_modoMenu = 0;
int tonal_indiceSeleccionado = -1;
String tonal_modoActual = "mayor";

// ==================== PROTOTIPOS DE FUNCIONES ====================
// Menú
void mostrarMenu();
void manejarMenu();
void checkBackButtonForMenu();

// Afinador
int afinador_frecuenciaANota(double freq);
double afinador_notaAFrecuencia(int nota);
void setupAfinador();
void ejecutarAfinador();

// Metrónomo
void metro_calculateBeatInterval();
void metro_playBeat();
const char* metro_getTempoName(int bpm);
const char* metro_getTimeSignatureName();
void metro_handleTapTempo();
void metro_readButtons();
void metro_updateSerialDisplay();
void setupMetronomo();
void ejecutarMetronomo();

// Selector de tonalidad
void tonal_navegarArriba();
void tonal_navegarAbajo();
void tonal_seleccionarOpcion();
void tonal_retroceder();
void tonal_mostrarMenuPrincipal();
void tonal_mostrarListaEscalas();
void tonal_mostrarTonalidadDetalle();
String tonal_calcularRelativo(tonal_Tonalidad t);
void setupSelectorTonalidad();
void ejecutarSelectorTonalidad();

// ==================== FUNCIONES DEL MENÚ ====================
void setup() {
  Serial.begin(115200);
  while(!Serial);
  
  // Configurar pines de los botones del menú
  pinMode(MENU_BTN_UP, INPUT_PULLUP);
  pinMode(MENU_BTN_DOWN, INPUT_PULLUP);
  pinMode(MENU_BTN_SELECT, INPUT_PULLUP);
  pinMode(MENU_BTN_BACK, INPUT_PULLUP);
  
  // Inicializar currentMode en 0 para que siempre empiece en el menú principal
  currentMode = 0;
  
  // Mostrar menú principal
  Serial.println("\n\n\n========================================");
  Serial.println("     SISTEMA INTEGRADO MUSICAL");
  Serial.println("========================================\n");
  mostrarMenu();
}

void loop() {
  // Verificar pulsación larga de BACK para volver al menú (solo si no estamos en el menú)
  if (currentMode != 0) {
    checkBackButtonForMenu();
  }
  
  switch(currentMode) {
    case 0: // Menú principal
      manejarMenu();
      break;
      
    case 1: // Modo Afinador
      ejecutarAfinador();
      break;
      
    case 2: // Modo Metrónomo
      ejecutarMetronomo();
      break;
      
    case 3: // Modo Selector de Tonalidad
      ejecutarSelectorTonalidad();
      break;
  }
}

void checkBackButtonForMenu() {
  bool currentBackState = digitalRead(MENU_BTN_BACK);
  
  if (currentBackState == LOW && !backPressed) {
    // Botón BACK recién presionado
    backPressed = true;
    backPressStartTime = millis();
    backLongPressHandled = false;
  }
  else if (currentBackState == LOW && backPressed && !backLongPressHandled) {
    // Botón BACK mantenido presionado
    if (millis() - backPressStartTime >= BACK_HOLD_TIME) {
      // 3 segundos cumplidos - volver al menú
      currentMode = 0;
      Serial.println("\nVolviendo al menú principal...");
      mostrarMenu();
      backLongPressHandled = true;
      backPressed = false;
      while(digitalRead(MENU_BTN_BACK) == LOW); // Esperar a que suelte
    }
  }
  else if (currentBackState == HIGH && backPressed) {
    // Botón BACK liberado antes de los 3 segundos
    backPressed = false;
  }
}

void manejarMenu() {
  // Leer estado de los botones
  bool btnUpState = digitalRead(MENU_BTN_UP);
  bool btnDownState = digitalRead(MENU_BTN_DOWN);
  bool btnSelectState = digitalRead(MENU_BTN_SELECT);
  
  // Navegación UP
  if (btnUpState == LOW && lastMenuBtnUpState == HIGH) {
    delay(DEBOUNCE_DELAY);
    if (digitalRead(MENU_BTN_UP) == LOW) {
      menuIndex = (menuIndex - 1 + 3) % 3;
      mostrarMenu();
      while(digitalRead(MENU_BTN_UP) == LOW);
    }
  }
  
  // Navegación DOWN
  if (btnDownState == LOW && lastMenuBtnDownState == HIGH) {
    delay(DEBOUNCE_DELAY);
    if (digitalRead(MENU_BTN_DOWN) == LOW) {
      menuIndex = (menuIndex + 1) % 3;
      mostrarMenu();
      while(digitalRead(MENU_BTN_DOWN) == LOW);
    }
  }
  
  // Seleccionar función
  if (btnSelectState == LOW && lastMenuBtnSelectState == HIGH) {
    delay(DEBOUNCE_DELAY);
    if (digitalRead(MENU_BTN_SELECT) == LOW) {
      currentMode = menuIndex + 1;
      Serial.println("\n========================================");
      Serial.print("Iniciando: ");
      switch(currentMode) {
        case 1: 
          Serial.println("AFINADOR");
          setupAfinador();
          break;
        case 2: 
          Serial.println("METRÓNOMO");
          setupMetronomo();
          break;
        case 3: 
          Serial.println("SELECTOR DE TONALIDAD");
          setupSelectorTonalidad();
          break;
      }
      Serial.println("========================================\n");
      while(digitalRead(MENU_BTN_SELECT) == LOW);
    }
  }
  
  // Actualizar estados
  lastMenuBtnUpState = btnUpState;
  lastMenuBtnDownState = btnDownState;
  lastMenuBtnSelectState = btnSelectState;
  
  delay(10);
}

void mostrarMenu() {
  Serial.println("\n\n\n========================================");
  Serial.println("         MENÚ PRINCIPAL");
  Serial.println("========================================\n");
  
  const char* opciones[] = {"Afinador", "Metrónomo", "Selector de Tonalidad"};
  
  for(int i = 0; i < 3; i++) {
    if(i == menuIndex) {
      Serial.print(" > ");
    } else {
      Serial.print("   ");
    }
    Serial.println(opciones[i]);
  }
  
  Serial.println("\n----------------------------------------");
  Serial.println("UP/DOWN: Navegar | SELECT: Seleccionar");
  Serial.println("BACK: Mantener 3s para volver al menú (en cualquier modo)");
  Serial.println("========================================\n");
}

// ==================== FUNCIONES DEL AFINADOR ====================
int afinador_frecuenciaANota(double freq) {
  return round(12.0 * log2(freq / afinador_A4_FREQ) + afinador_A4_MIDI);
}

double afinador_notaAFrecuencia(int nota) {
  return afinador_A4_FREQ * pow(2.0, (nota - afinador_A4_MIDI) / 12.0);
}

void setupAfinador() {
  afinador_sampling_period_us = round(1000000*(1.0/afinador_samplingFrequency));
  pinMode(AFINADOR_LED_SOS, OUTPUT);
  pinMode(AFINADOR_LED_OK, OUTPUT);
  pinMode(AFINADOR_LED_BEMOL, OUTPUT);
}

void ejecutarAfinador() {
  // NOTA: La verificación de BACK para volver al menú se hace en checkBackButtonForMenu()
  
  afinador_microseconds = micros();
  for(int i=0; i<afinador_samples; i++)
  {
      afinador_vReal[i] = analogRead(AFINADOR_CHANNEL);
      afinador_vImag[i] = 0;
      while(micros() - afinador_microseconds < afinador_sampling_period_us){
    
      }
      afinador_microseconds += afinador_sampling_period_us;
  }

  afinador_FFT.windowing(FFTWindow::Hamming, FFTDirection::Forward);
  afinador_FFT.compute(FFTDirection::Forward);
  afinador_FFT.complexToMagnitude();
  double x = afinador_FFT.majorPeak();
  Serial.println(x, 6);

  if (x < 60 || x > 2000) {
    Serial.print("Frecuencia fuera de rango: ");
    Serial.print(x, 2);
    Serial.println(" Hz\n");
    digitalWrite(AFINADOR_LED_BEMOL, LOW);
    digitalWrite(AFINADOR_LED_SOS, LOW);
    digitalWrite(AFINADOR_LED_OK, LOW);
    delay(300);
    return;
  }
  
  int notaMidi = afinador_frecuenciaANota(x);
  double freqIdeal = afinador_notaAFrecuencia(notaMidi);
  double error = x - freqIdeal;

  int indiceNota = notaMidi % 12;
  int octava = (notaMidi / 12) - 1;
  
  Serial.println("--------------------------------------");
  Serial.print("Frecuencia detectada: ");
  Serial.print(x, 2);
  Serial.println(" Hz");

  Serial.print("Nota detectada: ");
  Serial.print(afinador_nombresNotas[indiceNota]);
  Serial.print(octava);

  Serial.print("  (");
  Serial.print(freqIdeal, 2);
  Serial.println(" Hz)");

  Serial.print("Desviacion: ");
  Serial.print(error, 2);
  Serial.println(" Hz");

  if (abs(error) <= afinador_MARGEN_AFINADO) {
    Serial.println("✔ AFINADO");
    digitalWrite(AFINADOR_LED_BEMOL, LOW);
    digitalWrite(AFINADOR_LED_SOS, LOW);
    digitalWrite(AFINADOR_LED_OK, HIGH);
  }
  else if (error > 0) {
    Serial.println("⬇ BAJA EL TONO");
    digitalWrite(AFINADOR_LED_BEMOL, LOW);
    digitalWrite(AFINADOR_LED_SOS, HIGH);
    digitalWrite(AFINADOR_LED_OK, LOW);
  }
  else {
    Serial.println("⬆ SUBE EL TONO");
    digitalWrite(AFINADOR_LED_BEMOL, HIGH);
    digitalWrite(AFINADOR_LED_OK, LOW);
    digitalWrite(AFINADOR_LED_SOS, LOW);
  }

  Serial.println("--------------------------------------\n");
  delay(100);
}

// ==================== FUNCIONES DEL METRÓNOMO ====================
void metro_calculateBeatInterval() {
  metro_beatInterval = 60000000L / metro_currentBPM;
}

void metro_playBeat() {
  digitalWrite(METRO_LED_PIN, HIGH);
  
  if (metro_beatCounter == 1) {
    tone(METRO_SPEAKER_PIN, 1000, 80);
  }
  else if (metro_timeSignature == 4 && metro_beatCounter == 3) {
    tone(METRO_SPEAKER_PIN, 800, 50);
  }
  else {
    tone(METRO_SPEAKER_PIN, 600, 30);
  }
}

const char* metro_getTempoName(int bpm) {
  for (int i = 0; i < METRO_TEMPO_COUNT; i++) {
    if (bpm >= metro_tempoTable[i].minBPM && bpm <= metro_tempoTable[i].maxBPM) {
      return metro_tempoTable[i].nombre;
    }
  }
  return "Fuera de rango";
}

const char* metro_getTimeSignatureName() {
  switch(metro_timeSignature) {
    case 2: return "2/4";
    case 3: return "3/4";
    case 4: return "4/4";
    default: return "4/4";
  }
}

void metro_handleTapTempo() {
  unsigned long currentTime = millis();
  
  if (currentTime - metro_lastTapTime > METRO_TAP_TIMEOUT) {
    metro_tapIndex = 0;
    for (int i = 0; i < METRO_TAP_HISTORY_SIZE; i++) {
      metro_tapTimes[i] = 0;
    }
  }
  
  metro_tapTimes[metro_tapIndex] = currentTime;
  metro_tapIndex = (metro_tapIndex + 1) % METRO_TAP_HISTORY_SIZE;
  metro_lastTapTime = currentTime;
  
  int validTaps = 0;
  for (int i = 0; i < METRO_TAP_HISTORY_SIZE; i++) {
    if (metro_tapTimes[i] != 0) validTaps++;
  }
  
  if (validTaps >= 2) {
    unsigned long totalInterval = 0;
    int intervalCount = 0;
    
    for (int i = 1; i < validTaps; i++) {
      int current = (metro_tapIndex - i + METRO_TAP_HISTORY_SIZE) % METRO_TAP_HISTORY_SIZE;
      int previous = (metro_tapIndex - i - 1 + METRO_TAP_HISTORY_SIZE) % METRO_TAP_HISTORY_SIZE;
      
      if (metro_tapTimes[current] != 0 && metro_tapTimes[previous] != 0) {
        unsigned long interval = metro_tapTimes[current] - metro_tapTimes[previous];
        totalInterval += interval;
        intervalCount++;
      }
    }
    
    if (intervalCount > 0) {
      unsigned long averageInterval = totalInterval / intervalCount;
      int calculatedBPM = 60000 / averageInterval;
      calculatedBPM = constrain(calculatedBPM, METRO_MIN_BPM, METRO_MAX_BPM);
      
      if (metro_timeSignature == 4) {
        metro_currentBPM = calculatedBPM;
        metro_calculateBeatInterval();
        metro_presetMode = false;
        
        Serial.print("Tap tempo: ");
        Serial.print(metro_currentBPM);
        Serial.print(" BPM (");
        Serial.print(validTaps);
        Serial.println(" taps)");
      } else {
        Serial.println("Tap tempo solo disponible en compás 4/4");
      }
    }
  }
}

void metro_readButtons() {
  int currentBtnUpState = digitalRead(METRO_BTN_UP);
  int currentBtnDownState = digitalRead(METRO_BTN_DOWN);
  int currentBtnSelectState = digitalRead(METRO_BTN_SELECT);
  int currentBtnBackState = digitalRead(METRO_BTN_BACK);
  int currentBtnTapState = digitalRead(METRO_BTN_TAP);
  
  if (currentBtnUpState != metro_lastBtnUpState) {
    metro_lastDebounceTime = millis();
  }
  
  if ((millis() - metro_lastDebounceTime) > METRO_DEBOUNCE_DELAY) {
    if (currentBtnUpState != metro_btnUpState) {
      metro_btnUpState = currentBtnUpState;
      
      if (metro_btnUpState == LOW) {
        if (metro_currentBPM < METRO_MAX_BPM) {
          metro_currentBPM++;
          metro_calculateBeatInterval();
          metro_presetMode = false;
          Serial.print("BPM aumentado a: ");
          Serial.println(metro_currentBPM);
        }
      }
    }
  }
  
  if (currentBtnDownState != metro_lastBtnDownState) {
    metro_lastDebounceTime = millis();
  }
  
  if ((millis() - metro_lastDebounceTime) > METRO_DEBOUNCE_DELAY) {
    if (currentBtnDownState != metro_btnDownState) {
      metro_btnDownState = currentBtnDownState;
      
      if (metro_btnDownState == LOW) {
        if (metro_currentBPM > METRO_MIN_BPM) {
          metro_currentBPM--;
          metro_calculateBeatInterval();
          metro_presetMode = false;
          Serial.print("BPM disminuido a: ");
          Serial.println(metro_currentBPM);
        }
      }
    }
  }
  
  if (currentBtnSelectState != metro_lastBtnSelectState) {
    metro_lastDebounceTime = millis();
  }
  
  if ((millis() - metro_lastDebounceTime) > METRO_DEBOUNCE_DELAY) {
    if (currentBtnSelectState != metro_btnSelectState) {
      metro_btnSelectState = currentBtnSelectState;
      
      if (metro_btnSelectState == LOW) {
        if (metro_timeSignature == 2) {
          metro_timeSignature = 3;
        } else if (metro_timeSignature == 3) {
          metro_timeSignature = 4;
        } else {
          metro_timeSignature = 2;
        }
        
        metro_beatCounter = 0;
        Serial.print("Compás cambiado a: ");
        Serial.println(metro_getTimeSignatureName());
      }
    }
  }
  
  if (currentBtnBackState != metro_lastBtnBackState) {
    metro_lastDebounceTime = millis();
  }
  
  if ((millis() - metro_lastDebounceTime) > METRO_DEBOUNCE_DELAY) {
    if (currentBtnBackState != metro_btnBackState) {
      metro_btnBackState = currentBtnBackState;
      
      if (metro_btnBackState == LOW) {
        metro_currentPresetIndex = (metro_currentPresetIndex + 1) % METRO_NUM_PRESETS;
        metro_currentBPM = METRO_BPM_PRESETS[metro_currentPresetIndex];
        metro_calculateBeatInterval();
        metro_presetMode = true;
        
        Serial.print("Preset ");
        Serial.print(metro_currentPresetIndex + 1);
        Serial.print(" seleccionado: ");
        Serial.print(metro_currentBPM);
        Serial.print(" BPM (");
        Serial.print(metro_getTempoName(metro_currentBPM));
        Serial.println(")");
      }
    }
  }
  
  if (currentBtnTapState != metro_lastBtnTapState) {
    metro_lastDebounceTime = millis();
  }
  
  if ((millis() - metro_lastDebounceTime) > METRO_DEBOUNCE_DELAY) {
    if (currentBtnTapState != metro_btnTapState) {
      metro_btnTapState = currentBtnTapState;
      
      if (metro_btnTapState == LOW) {
        metro_handleTapTempo();
      }
    }
  }
  
  metro_lastBtnUpState = currentBtnUpState;
  metro_lastBtnDownState = currentBtnDownState;
  metro_lastBtnSelectState = currentBtnSelectState;
  metro_lastBtnBackState = currentBtnBackState;
  metro_lastBtnTapState = currentBtnTapState;
}

void metro_updateSerialDisplay() {
  for (int i = 0; i < 20; i++) {
    Serial.println();
  }
  
  Serial.println("==========================================");
  Serial.println("          METRONOMO EN EJECUCION");
  Serial.println("==========================================");
  Serial.println();
  
  Serial.print("BPM ACTUAL: ");
  Serial.print(metro_currentBPM);
  if (metro_presetMode) {
    Serial.print(" [PRESET ");
    Serial.print(metro_currentPresetIndex + 1);
    Serial.print("]");
  }
  Serial.println();
  Serial.println();
  
  Serial.print("TEMPO: ");
  Serial.println(metro_getTempoName(metro_currentBPM));
  Serial.println();
  
  Serial.print("COMPAS: ");
  Serial.println(metro_getTimeSignatureName());
  Serial.println();
  
  Serial.print("TIEMPO ENTRE BEATS: ");
  Serial.print(metro_beatInterval / 1000.0);
  Serial.println(" ms");
  Serial.println();
  
  Serial.println("CONTROLES:");
  Serial.println("ARRIBA: Aumentar BPM (+1)");
  Serial.println("ABAJO:  Disminuir BPM (-1)");
  Serial.println("SELECT: Cambiar compás");
  Serial.println("BACK:   Cambiar preset BPM (pulsación corta)");
  Serial.println("TAP:    Tap tempo (solo 4/4)");
  Serial.println();
  
  int validTaps = 0;
  for (int i = 0; i < METRO_TAP_HISTORY_SIZE; i++) {
    if (metro_tapTimes[i] != 0) validTaps++;
  }
  
  Serial.print("TAP TEMPO: ");
  Serial.print(validTaps);
  Serial.print("/");
  Serial.print(METRO_TAP_HISTORY_SIZE);
  Serial.println(" taps registrados");
  Serial.println();
  
  Serial.print("BEATS (");
  Serial.print(metro_getTimeSignatureName());
  Serial.print("): ");
  
  if (metro_timeSignature == 2) {
    Serial.println("1(fuerte) 2");
    Serial.print("BEAT ACTUAL: ");
    switch(metro_beatCounter) {
      case 1: Serial.println("1 (FUERTE)    [X]"); break;
      case 2: Serial.println("2 (debil)     [ ]"); break;
      default: Serial.println("-");
    }
  }
  else if (metro_timeSignature == 3) {
    Serial.println("1(fuerte) 2 3");
    Serial.print("BEAT ACTUAL: ");
    switch(metro_beatCounter) {
      case 1: Serial.println("1 (FUERTE)    [X]"); break;
      case 2: Serial.println("2 (debil)     [ ]"); break;
      case 3: Serial.println("3 (debil)     [ ]"); break;
      default: Serial.println("-");
    }
  }
  else {
    Serial.println("1(fuerte) 2 3(medio) 4");
    Serial.print("BEAT ACTUAL: ");
    switch(metro_beatCounter) {
      case 1: Serial.println("1 (FUERTE)    [X]"); break;
      case 2: Serial.println("2 (debil)     [ ]"); break;
      case 3: Serial.println("3 (medio)     [*]"); break;
      case 4: Serial.println("4 (debil)     [ ]"); break;
      default: Serial.println("-");
    }
  }
  Serial.println();
  
  Serial.println("==========================================");
  Serial.println("PRESETS DISPONIBLES:");
  for (int i = 0; i < METRO_NUM_PRESETS; i++) {
    Serial.print("  ");
    if (i == metro_currentPresetIndex && metro_presetMode) {
      Serial.print(">> ");
    } else {
      Serial.print("   ");
    }
    Serial.print("Preset ");
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.print(METRO_BPM_PRESETS[i]);
    Serial.print(" BPM (");
    Serial.print(metro_getTempoName(METRO_BPM_PRESETS[i]));
    Serial.println(")");
  }
  
  if (metro_timeSignature != 4) {
    Serial.println("\n[INFO] Tap tempo solo funciona en compás 4/4");
  }
  
  Serial.println("\n[INFO] Mantener BACK 3 segundos para volver al menú principal");
  Serial.println("==========================================");
}

void setupMetronomo() {
  pinMode(METRO_SPEAKER_PIN, OUTPUT);
  pinMode(METRO_LED_PIN, OUTPUT);
  digitalWrite(METRO_LED_PIN, LOW);
  
  pinMode(METRO_BTN_UP, INPUT_PULLUP);
  pinMode(METRO_BTN_DOWN, INPUT_PULLUP);
  pinMode(METRO_BTN_SELECT, INPUT_PULLUP);
  pinMode(METRO_BTN_BACK, INPUT_PULLUP);
  pinMode(METRO_BTN_TAP, INPUT_PULLUP);
  
  for (int i = 0; i < METRO_TAP_HISTORY_SIZE; i++) {
    metro_tapTimes[i] = 0;
  }
  
  metro_calculateBeatInterval();
  
  Serial.println("\n==========================================");
  Serial.println("       METRONOMO DIGITAL - ESP32");
  Serial.println("==========================================");
  Serial.println("Compás actual: 4/4");
  Serial.println("Rango BPM: 40 - 250");
  Serial.println("\nCONTROLES:");
  Serial.println("Pulsador ARRIBA (GPIO13): Aumentar BPM");
  Serial.println("Pulsador ABAJO (GPIO12): Disminuir BPM");
  Serial.println("Pulsador SELECT (GPIO14): Cambiar compás");
  Serial.println("Pulsador BACK (GPIO27): Cambiar preset BPM (pulsación corta)");
  Serial.println("Pulsador TAP (GPIO19): Tap tempo (solo 4/4)");
  Serial.println("\nPRESETS DISPONIBLES:");
  for (int i = 0; i < METRO_NUM_PRESETS; i++) {
    Serial.print("  Preset ");
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.print(METRO_BPM_PRESETS[i]);
    Serial.print(" BPM (");
    Serial.print(metro_getTempoName(METRO_BPM_PRESETS[i]));
    Serial.println(")");
  }
  Serial.println("\nTAP TEMPO: Presiona el botón TAP al ritmo deseado");
  Serial.println("           (mínimo 4 taps, solo funciona en 4/4)");
  Serial.println("\n[INFO] Mantener BACK 3 segundos para volver al menú principal");
  Serial.println("==========================================\n");
}

void ejecutarMetronomo() {
  // NOTA: La verificación de BACK para volver al menú se hace en checkBackButtonForMenu()
  
  unsigned long currentTime = micros();
  
  metro_readButtons();
  
  if (currentTime - metro_lastBeatTime >= metro_beatInterval) {
    metro_playBeat();
    metro_beatCounter = (metro_beatCounter % metro_timeSignature) + 1;
    metro_lastBeatTime = currentTime;
  }
  
  if (millis() - metro_lastSerialUpdate >= METRO_SERIAL_UPDATE_INTERVAL) {
    metro_updateSerialDisplay();
    metro_lastSerialUpdate = millis();
  }
  
  static unsigned long lastLedOffTime = 0;
  if (millis() - lastLedOffTime > 50) {
    digitalWrite(METRO_LED_PIN, LOW);
  }
}

// ==================== FUNCIONES DEL SELECTOR DE TONALIDAD ====================
void tonal_navegarArriba() {
  if (tonal_modoMenu == 0) {
    if (tonal_indiceActual > 0) {
      tonal_indiceActual--;
    } else {
      tonal_indiceActual = 1;
    }
    tonal_mostrarMenuPrincipal();
  } 
  else if (tonal_modoMenu == 1 || tonal_modoMenu == 2) {
    if (tonal_indiceActual > 0) {
      tonal_indiceActual--;
    } else {
      tonal_indiceActual = 11;
    }
    tonal_mostrarListaEscalas();
  }
}

void tonal_navegarAbajo() {
  if (tonal_modoMenu == 0) {
    if (tonal_indiceActual < 1) {
      tonal_indiceActual++;
    } else {
      tonal_indiceActual = 0;
    }
    tonal_mostrarMenuPrincipal();
  } 
  else if (tonal_modoMenu == 1 || tonal_modoMenu == 2) {
    if (tonal_indiceActual < 11) {
      tonal_indiceActual++;
    } else {
      tonal_indiceActual = 0;
    }
    tonal_mostrarListaEscalas();
  }
}

void tonal_seleccionarOpcion() {
  if (tonal_modoMenu == 0) {
    if (tonal_indiceActual == 0) {
      tonal_modoMenu = 1;
      tonal_modoActual = "mayor";
    } else if (tonal_indiceActual == 1) {
      tonal_modoMenu = 2;
      tonal_modoActual = "menor";
    }
    tonal_indiceActual = 0;
    tonal_mostrarListaEscalas();
  } 
  else if (tonal_modoMenu == 1 || tonal_modoMenu == 2) {
    tonal_modoMenu = 3;
    tonal_mostrarTonalidadDetalle();
  }
}

void tonal_retroceder() {
  if (tonal_modoMenu == 1 || tonal_modoMenu == 2) {
    tonal_modoMenu = 0;
    tonal_indiceActual = 0;
    tonal_mostrarMenuPrincipal();
  }
  else if (tonal_modoMenu == 3) {
    tonal_modoMenu = (tonal_modoActual == "mayor") ? 1 : 2;
    tonal_mostrarListaEscalas();
  }
}

void tonal_mostrarMenuPrincipal() {
  Serial.println("\n=== MENÚ PRINCIPAL (TONALIDAD) ===");
  Serial.println("Use UP/DOWN para navegar, SELECT para seleccionar");
  Serial.println("----------------------------------------");
  
  if (tonal_indiceActual == 0) {
    Serial.println("> ESCALAS MAYORES");
    Serial.println("  ESCALAS MENORES");
  } else {
    Serial.println("  ESCALAS MAYORES");
    Serial.println("> ESCALAS MENORES");
  }
  
  Serial.println("----------------------------------------");
  Serial.println("SELECT: Seleccionar modo | BACK: Retroceder (pulsación corta)");
  Serial.println("BACK (mantener 3s): Volver al menú principal");
}

void tonal_mostrarListaEscalas() {
  Serial.println("\n=== " + String(tonal_modoActual == "mayor" ? "ESCALAS MAYORES" : "ESCALAS MENORES") + " ===");
  Serial.println("Use UP/DOWN para navegar, SELECT para ver detalles");
  Serial.println("BACK: Volver al menú anterior (pulsación corta)");
  Serial.println("----------------------------------------");
  
  tonal_Tonalidad* listaTonalidades;
  if (tonal_modoActual == "mayor") {
    listaTonalidades = tonal_tonalidadesMayores;
  } else {
    listaTonalidades = tonal_tonalidadesMenores;
  }
  
  for (int i = 0; i < 12; i++) {
    if (i == tonal_indiceActual) {
      Serial.print("> ");
    } else {
      Serial.print("  ");
    }
    
    Serial.print(listaTonalidades[i].nombre);
    
    if (listaTonalidades[i].numSostenidos > 0) {
      Serial.print(" (" + String(listaTonalidades[i].numSostenidos) + " sostenido(s))");
    } else if (listaTonalidades[i].numBemoles > 0) {
      Serial.print(" (" + String(listaTonalidades[i].numBemoles) + " bemol(es))");
    } else {
      Serial.print(" (Natural)");
    }
    
    Serial.println();
  }
  
  Serial.println("----------------------------------------");
  Serial.println("SELECT: Ver detalles | BACK: Menú anterior");
}

void tonal_mostrarTonalidadDetalle() {
  tonal_Tonalidad t;
  if (tonal_modoActual == "mayor") {
    t = tonal_tonalidadesMayores[tonal_indiceActual];
  } else {
    t = tonal_tonalidadesMenores[tonal_indiceActual];
  }
  
  Serial.println("\n========================================");
  Serial.println("        DETALLES DE TONALIDAD");
  Serial.println("========================================");
  Serial.println("Tonalidad: " + t.nombre + " " + (t.tipo == "mayor" ? "Mayor" : "Menor"));
  
  Serial.print("Posición en círculo de quintas: ");
  if (t.numSostenidos > 0) {
    Serial.println(String(t.numSostenidos) + " sostenidos");
  } else if (t.numBemoles > 0) {
    Serial.println(String(t.numBemoles) + " bemoles");
  } else {
    Serial.println("Sin alteraciones");
  }
  
  Serial.println("\n--- ALTERACIONES ---");
  
  if (t.numSostenidos > 0) {
    Serial.print("Sostenidos: ");
    for (int i = 0; i < t.numSostenidos && i < 7; i++) {
      if (t.sostenidos[i] != "") {
        Serial.print(t.sostenidos[i]);
        if (i < t.numSostenidos - 1) Serial.print(", ");
      }
    }
    Serial.println();
  }
  
  if (t.numBemoles > 0) {
    Serial.print("Bemoles: ");
    for (int i = 0; i < t.numBemoles && i < 7; i++) {
      if (t.bemoles[i] != "") {
        Serial.print(t.bemoles[i]);
        if (i < t.numBemoles - 1) Serial.print(", ");
      }
    }
    Serial.println();
  }
  
  String relativo = tonal_calcularRelativo(t);
  if (t.tipo == "mayor") {
    Serial.println("Relativo menor: " + relativo + " menor");
  } else {
    Serial.println("Relativo mayor: " + relativo + " Mayor");
  }
  
  Serial.println("\n========================================");
  Serial.println("Presione BACK para volver a la lista");
}

String tonal_calcularRelativo(tonal_Tonalidad t) {
  String grados[7] = {"C", "D", "E", "F", "G", "A", "B"};
  
  String tonica = t.nombre;
  if (tonica == "F#/Gb") tonica = "F#";
  if (tonica == "D#/Eb") tonica = "D#";
  if (tonica == "A#/Bb") tonica = "A#";
  
  char notaLetra = tonica.charAt(0);
  int indiceTonica = -1;
  
  for (int i = 0; i < 7; i++) {
    if (grados[i].charAt(0) == notaLetra) {
      indiceTonica = i;
      break;
    }
  }
  
  if (indiceTonica == -1) {
    return t.tipo == "mayor" ? "A" : "C";
  }
  
  if (t.tipo == "mayor") {
    int indiceRelativo = (indiceTonica + 5) % 7;
    return grados[indiceRelativo];
  } else {
    int indiceRelativo = (indiceTonica + 2) % 7;
    return grados[indiceRelativo];
  }
}

void setupSelectorTonalidad() {
  tonal_indiceActual = 0;
  tonal_modoMenu = 0;
  tonal_indiceSeleccionado = -1;
  tonal_modoActual = "mayor";
  
  Serial.println("\n====================================");
  Serial.println("   SELECTOR DE TONALIDADES MUSICALES");
  Serial.println("       CÍRCULO DE QUINTAS");
  Serial.println("====================================\n");
  
  tonal_mostrarMenuPrincipal();
}

void ejecutarSelectorTonalidad() {
  // Leer estado de los botones (incluyendo BACK para la función de retroceder)
  bool btnUpState = digitalRead(MENU_BTN_UP);
  bool btnDownState = digitalRead(MENU_BTN_DOWN);
  bool btnSelectState = digitalRead(MENU_BTN_SELECT);
  bool btnBackState = digitalRead(MENU_BTN_BACK);
  
  if (btnUpState == LOW && tonal_lastBtnUpState == HIGH) {
    delay(50);
    if (digitalRead(MENU_BTN_UP) == LOW) {
      tonal_navegarArriba();
      while(digitalRead(MENU_BTN_UP) == LOW);
    }
  }
  
  if (btnDownState == LOW && tonal_lastBtnDownState == HIGH) {
    delay(50);
    if (digitalRead(MENU_BTN_DOWN) == LOW) {
      tonal_navegarAbajo();
      while(digitalRead(MENU_BTN_DOWN) == LOW);
    }
  }
  
  if (btnSelectState == LOW && tonal_lastBtnSelectState == HIGH) {
    delay(50);
    if (digitalRead(MENU_BTN_SELECT) == LOW) {
      tonal_seleccionarOpcion();
      while(digitalRead(MENU_BTN_SELECT) == LOW);
    }
  }
  
  // Solo procesar BACK si NO estamos en medio de una pulsación larga
  // y si el botón está siendo presionado (flanco descendente)
  if (btnBackState == LOW && tonal_lastBtnBackState == HIGH) {
    // Pequeño delay para debounce
    delay(50);
    if (digitalRead(MENU_BTN_BACK) == LOW) {
      // Verificar que no sea una pulsación larga (menos de 200ms)
      if (!backPressed || (millis() - backPressStartTime) < 200) {
        tonal_retroceder();
        while(digitalRead(MENU_BTN_BACK) == LOW);
      }
    }
  }
  
  tonal_lastBtnUpState = btnUpState;
  tonal_lastBtnDownState = btnDownState;
  tonal_lastBtnSelectState = btnSelectState;
  tonal_lastBtnBackState = btnBackState;
  
  delay(10);
}
