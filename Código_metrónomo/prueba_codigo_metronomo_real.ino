// Pines
const int SPEAKER_PIN = 25;  // Altavoz
const int LED_PIN = 32;       // LED interno del ESP32

// Pines para pulsadores
const int BTN_UP = 13;       // Pulsador arriba (aumentar BPM)
const int BTN_DOWN = 12;     // Pulsador abajo (disminuir BPM)
const int BTN_SELECT = 14;   // Pulsador select (cambiar compás)
const int BTN_BACK = 27;     // Pulsador back (cambiar preset BPM)
const int BTN_TAP = 19;      // Pulsador tap (tap tempo - solo 4/4)

// Parámetros del metrónomo
const int MIN_BPM = 40;
const int MAX_BPM = 250;

// Presets de BPM
const int BPM_PRESETS[] = {60, 80, 120, 140, 180};
const int NUM_PRESETS = sizeof(BPM_PRESETS) / sizeof(BPM_PRESETS[0]);
int currentPresetIndex = 2; // Empieza en 120 BPM (índice 2)
bool presetMode = false;

// Variables para tap tempo
const int TAP_HISTORY_SIZE = 4; // Número de taps a considerar
unsigned long tapTimes[TAP_HISTORY_SIZE];
int tapIndex = 0;
unsigned long lastTapTime = 0;
const unsigned long TAP_TIMEOUT = 2000; // 2 segundos para resetear

// Estructura para tempo musical
struct TempoRange {
  const char* nombre;
  int minBPM;
  int maxBPM;
};

// Tabla de tempo musical
const TempoRange tempoTable[] = {
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

const int TEMPO_COUNT = sizeof(tempoTable) / sizeof(tempoTable[0]);

// Variables del programa
int currentBPM = 120;
int beatInterval;  // en microsegundos
int beatCounter = 0;
int timeSignature = 4; // 2=2/4, 3=3/4, 4=4/4
unsigned long lastBeatTime = 0;

// Variables para control de actualización en Serial
unsigned long lastSerialUpdate = 0;
const int SERIAL_UPDATE_INTERVAL = 500; // ms

// Variables para anti-rebote de botones
unsigned long lastDebounceTime = 0;
const unsigned long DEBOUNCE_DELAY = 50;
int lastBtnUpState = HIGH;
int lastBtnDownState = HIGH;
int lastBtnSelectState = HIGH;
int lastBtnBackState = HIGH;
int lastBtnTapState = HIGH;
int btnUpState;
int btnDownState;
int btnSelectState;
int btnBackState;
int btnTapState;

void setup() {
  Serial.begin(115200);
  
  // Inicializar pines
  pinMode(SPEAKER_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  
  // Configurar pulsadores con pull-up interno
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_SELECT, INPUT_PULLUP);
  pinMode(BTN_BACK, INPUT_PULLUP);
  pinMode(BTN_TAP, INPUT_PULLUP);
  
  // Inicializar array de tap tempo
  for (int i = 0; i < TAP_HISTORY_SIZE; i++) {
    tapTimes[i] = 0;
  }
  
  // Calcular intervalo inicial
  calculateBeatInterval();
  
  // Mostrar información inicial
  Serial.println("\n==========================================");
  Serial.println("       METRONOMO DIGITAL - ESP32");
  Serial.println("==========================================");
  Serial.println("Compás actual: 4/4");
  Serial.println("Rango BPM: 40 - 250");
  Serial.println("\nCONTROLES:");
  Serial.println("Pulsador ARRIBA (GPIO15): Aumentar BPM");
  Serial.println("Pulsador ABAJO (GPIO16): Disminuir BPM");
  Serial.println("Pulsador SELECT (GPIO17): Cambiar compás");
  Serial.println("Pulsador BACK (GPIO18): Cambiar preset BPM");
  Serial.println("Pulsador TAP (GPIO19): Tap tempo (solo 4/4)");
  Serial.println("\nPRESETS DISPONIBLES:");
  for (int i = 0; i < NUM_PRESETS; i++) {
    Serial.print("  Preset ");
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.print(BPM_PRESETS[i]);
    Serial.print(" BPM (");
    Serial.print(getTempoName(BPM_PRESETS[i]));
    Serial.println(")");
  }
  Serial.println("\nTAP TEMPO: Presiona el botón TAP al ritmo deseado");
  Serial.println("           (mínimo 4 taps, solo funciona en 4/4)");
  Serial.println("==========================================\n");
  
  delay(1000);
}

void loop() {
  unsigned long currentTime = micros();
  
  // Leer estado de botones con anti-rebote
  readButtons();
  
  // Verificar si es tiempo para el siguiente beat
  if (currentTime - lastBeatTime >= beatInterval) {
    playBeat();
    beatCounter = (beatCounter % timeSignature) + 1;
    lastBeatTime = currentTime;
  }
  
  // Actualizar información en Serial periódicamente
  if (millis() - lastSerialUpdate >= SERIAL_UPDATE_INTERVAL) {
    updateSerialDisplay();
    lastSerialUpdate = millis();
  }
  
  // Apagar LED después de un breve periodo
  static unsigned long lastLedOffTime = 0;
  if (millis() - lastLedOffTime > 50) {
    digitalWrite(LED_PIN, LOW);
  }
}

void calculateBeatInterval() {
  beatInterval = 60000000L / currentBPM;
}

void playBeat() {
  digitalWrite(LED_PIN, HIGH);
  
  // Primer beat siempre fuerte
  if (beatCounter == 1) {
    tone(SPEAKER_PIN, 1000, 80);
  }
  // Para 4/4, el tercer beat es medio fuerte
  else if (timeSignature == 4 && beatCounter == 3) {
    tone(SPEAKER_PIN, 800, 50);
  }
  // Para 3/4 y 2/4, todos los demás beats son débiles
  else {
    tone(SPEAKER_PIN, 600, 30);
  }
}

const char* getTempoName(int bpm) {
  for (int i = 0; i < TEMPO_COUNT; i++) {
    if (bpm >= tempoTable[i].minBPM && bpm <= tempoTable[i].maxBPM) {
      return tempoTable[i].nombre;
    }
  }
  return "Fuera de rango";
}

const char* getTimeSignatureName() {
  switch(timeSignature) {
    case 2: return "2/4";
    case 3: return "3/4";
    case 4: return "4/4";
    default: return "4/4";
  }
}

void handleTapTempo() {
  unsigned long currentTime = millis();
  
  // Verificar si ha pasado mucho tiempo desde el último tap
  if (currentTime - lastTapTime > TAP_TIMEOUT) {
    // Resetear tap tempo si ha pasado mucho tiempo
    tapIndex = 0;
    for (int i = 0; i < TAP_HISTORY_SIZE; i++) {
      tapTimes[i] = 0;
    }
  }
  
  // Guardar el tiempo del tap actual
  tapTimes[tapIndex] = currentTime;
  tapIndex = (tapIndex + 1) % TAP_HISTORY_SIZE;
  lastTapTime = currentTime;
  
  // Solo calcular BPM si tenemos al menos 2 taps
  int validTaps = 0;
  for (int i = 0; i < TAP_HISTORY_SIZE; i++) {
    if (tapTimes[i] != 0) validTaps++;
  }
  
  if (validTaps >= 2) {
    // Calcular BPM basado en los intervalos entre taps
    // Usar los últimos validTaps-1 intervalos
    unsigned long totalInterval = 0;
    int intervalCount = 0;
    
    for (int i = 1; i < validTaps; i++) {
      int current = (tapIndex - i + TAP_HISTORY_SIZE) % TAP_HISTORY_SIZE;
      int previous = (tapIndex - i - 1 + TAP_HISTORY_SIZE) % TAP_HISTORY_SIZE;
      
      if (tapTimes[current] != 0 && tapTimes[previous] != 0) {
        unsigned long interval = tapTimes[current] - tapTimes[previous];
        totalInterval += interval;
        intervalCount++;
      }
    }
    
    if (intervalCount > 0) {
      unsigned long averageInterval = totalInterval / intervalCount;
      int calculatedBPM = 60000 / averageInterval;
      
      // Limitar al rango permitido
      calculatedBPM = constrain(calculatedBPM, MIN_BPM, MAX_BPM);
      
      // Solo actualizar si el compás es 4/4
      if (timeSignature == 4) {
        currentBPM = calculatedBPM;
        calculateBeatInterval();
        presetMode = false;
        
        Serial.print("Tap tempo: ");
        Serial.print(currentBPM);
        Serial.print(" BPM (");
        Serial.print(validTaps);
        Serial.println(" taps)");
      } else {
        Serial.println("Tap tempo solo disponible en compás 4/4");
      }
    }
  }
}

void readButtons() {
  // Leer estado actual de botones
  int currentBtnUpState = digitalRead(BTN_UP);
  int currentBtnDownState = digitalRead(BTN_DOWN);
  int currentBtnSelectState = digitalRead(BTN_SELECT);
  int currentBtnBackState = digitalRead(BTN_BACK);
  int currentBtnTapState = digitalRead(BTN_TAP);
  
  // Verificar cambios en botón UP
  if (currentBtnUpState != lastBtnUpState) {
    lastDebounceTime = millis();
  }
  
  if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
    if (currentBtnUpState != btnUpState) {
      btnUpState = currentBtnUpState;
      
      // Si el botón fue presionado (LOW porque es PULLUP)
      if (btnUpState == LOW) {
        if (currentBPM < MAX_BPM) {
          currentBPM++;
          calculateBeatInterval();
          presetMode = false;
          Serial.print("BPM aumentado a: ");
          Serial.println(currentBPM);
        }
      }
    }
  }
  
  // Verificar cambios en botón DOWN
  if (currentBtnDownState != lastBtnDownState) {
    lastDebounceTime = millis();
  }
  
  if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
    if (currentBtnDownState != btnDownState) {
      btnDownState = currentBtnDownState;
      
      // Si el botón fue presionado (LOW porque es PULLUP)
      if (btnDownState == LOW) {
        if (currentBPM > MIN_BPM) {
          currentBPM--;
          calculateBeatInterval();
          presetMode = false;
          Serial.print("BPM disminuido a: ");
          Serial.println(currentBPM);
        }
      }
    }
  }
  
  // Verificar cambios en botón SELECT
  if (currentBtnSelectState != lastBtnSelectState) {
    lastDebounceTime = millis();
  }
  
  if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
    if (currentBtnSelectState != btnSelectState) {
      btnSelectState = currentBtnSelectState;
      
      // Si el botón fue presionado (LOW porque es PULLUP)
      if (btnSelectState == LOW) {
        // Cambiar compás de forma circular: 2/4 -> 3/4 -> 4/4 -> 2/4
        if (timeSignature == 2) {
          timeSignature = 3;
        } else if (timeSignature == 3) {
          timeSignature = 4;
        } else {
          timeSignature = 2;
        }
        
        beatCounter = 0;
        Serial.print("Compás cambiado a: ");
        Serial.println(getTimeSignatureName());
      }
    }
  }
  
  // Verificar cambios en botón BACK (PRESET)
  if (currentBtnBackState != lastBtnBackState) {
    lastDebounceTime = millis();
  }
  
  if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
    if (currentBtnBackState != btnBackState) {
      btnBackState = currentBtnBackState;
      
      // Si el botón fue presionado (LOW porque es PULLUP)
      if (btnBackState == LOW) {
        currentPresetIndex = (currentPresetIndex + 1) % NUM_PRESETS;
        currentBPM = BPM_PRESETS[currentPresetIndex];
        calculateBeatInterval();
        presetMode = true;
        
        Serial.print("Preset ");
        Serial.print(currentPresetIndex + 1);
        Serial.print(" seleccionado: ");
        Serial.print(currentBPM);
        Serial.print(" BPM (");
        Serial.print(getTempoName(currentBPM));
        Serial.println(")");
      }
    }
  }
  
  // Verificar cambios en botón TAP
  if (currentBtnTapState != lastBtnTapState) {
    lastDebounceTime = millis();
  }
  
  if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
    if (currentBtnTapState != btnTapState) {
      btnTapState = currentBtnTapState;
      
      // Si el botón fue presionado (LOW porque es PULLUP)
      if (btnTapState == LOW) {
        handleTapTempo();
      }
    }
  }
  
  // Guardar estado actual para la siguiente lectura
  lastBtnUpState = currentBtnUpState;
  lastBtnDownState = currentBtnDownState;
  lastBtnSelectState = currentBtnSelectState;
  lastBtnBackState = currentBtnBackState;
  lastBtnTapState = currentBtnTapState;
}

void updateSerialDisplay() {
  for (int i = 0; i < 20; i++) {
    Serial.println();
  }
  
  Serial.println("==========================================");
  Serial.println("          METRONOMO EN EJECUCION");
  Serial.println("==========================================");
  Serial.println();
  
  Serial.print("BPM ACTUAL: ");
  Serial.print(currentBPM);
  if (presetMode) {
    Serial.print(" [PRESET ");
    Serial.print(currentPresetIndex + 1);
    Serial.print("]");
  }
  Serial.println();
  Serial.println();
  
  Serial.print("TEMPO: ");
  Serial.println(getTempoName(currentBPM));
  Serial.println();
  
  Serial.print("COMPAS: ");
  Serial.println(getTimeSignatureName());
  Serial.println();
  
  Serial.print("TIEMPO ENTRE BEATS: ");
  Serial.print(beatInterval / 1000.0);
  Serial.println(" ms");
  Serial.println();
  
  Serial.println("CONTROLES:");
  Serial.println("ARRIBA: Aumentar BPM (+1)");
  Serial.println("ABAJO:  Disminuir BPM (-1)");
  Serial.println("SELECT: Cambiar compás");
  Serial.println("BACK:   Cambiar preset BPM");
  Serial.println("TAP:    Tap tempo (solo 4/4)");
  Serial.println();
  
  // Mostrar estado del tap tempo
  int validTaps = 0;
  for (int i = 0; i < TAP_HISTORY_SIZE; i++) {
    if (tapTimes[i] != 0) validTaps++;
  }
  
  Serial.print("TAP TEMPO: ");
  Serial.print(validTaps);
  Serial.print("/");
  Serial.print(TAP_HISTORY_SIZE);
  Serial.println(" taps registrados");
  Serial.println();
  
  Serial.print("BEATS (");
  Serial.print(getTimeSignatureName());
  Serial.print("): ");
  
  // Mostrar patrón de beats según el compás
  if (timeSignature == 2) {
    Serial.println("1(fuerte) 2");
    Serial.print("BEAT ACTUAL: ");
    switch(beatCounter) {
      case 1: Serial.println("1 (FUERTE)    [X]"); break;
      case 2: Serial.println("2 (debil)     [ ]"); break;
      default: Serial.println("-");
    }
  }
  else if (timeSignature == 3) {
    Serial.println("1(fuerte) 2 3");
    Serial.print("BEAT ACTUAL: ");
    switch(beatCounter) {
      case 1: Serial.println("1 (FUERTE)    [X]"); break;
      case 2: Serial.println("2 (debil)     [ ]"); break;
      case 3: Serial.println("3 (debil)     [ ]"); break;
      default: Serial.println("-");
    }
  }
  else { // 4/4
    Serial.println("1(fuerte) 2 3(medio) 4");
    Serial.print("BEAT ACTUAL: ");
    switch(beatCounter) {
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
  for (int i = 0; i < NUM_PRESETS; i++) {
    Serial.print("  ");
    if (i == currentPresetIndex && presetMode) {
      Serial.print(">> ");
    } else {
      Serial.print("   ");
    }
    Serial.print("Preset ");
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.print(BPM_PRESETS[i]);
    Serial.print(" BPM (");
    Serial.print(getTempoName(BPM_PRESETS[i]));
    Serial.println(")");
  }
  
  // Mostrar advertencia si se intenta usar tap tempo en compás no 4/4
  if (timeSignature != 4) {
    Serial.println("\n[INFO] Tap tempo solo funciona en compás 4/4");
  }
  
  Serial.println("==========================================");
}