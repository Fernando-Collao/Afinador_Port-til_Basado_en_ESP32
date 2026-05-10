/*
 * SELECTOR DE TONALIDADES MUSICALES - CÍRCULO DE QUINTAS
 * Control con 4 pulsadores:
 * - UP: Subir en la lista
 * - DOWN: Bajar en la lista
 * - SELECT: Seleccionar
 * - BACK: Volver al menú principal
 */

// Definición de pines para los pulsadores
#define BTN_UP 13
#define BTN_DOWN 12
#define BTN_SELECT 14
#define BTN_BACK 27

// Estados de los botones
bool lastBtnUpState = HIGH;
bool lastBtnDownState = HIGH;
bool lastBtnSelectState = HIGH;
bool lastBtnBackState = HIGH;

// Estructura para representar una tonalidad
struct Tonalidad {
  String nombre;
  String sostenidos[7];
  String bemoles[7];
  int numSostenidos;
  int numBemoles;
  String tipo; // "mayor" o "menor"
};

// Array de tonalidades MAYORES (Círculo de Quintas)
Tonalidad tonalidadesMayores[12] = {
  // 0 sostenidos/bemoles
  {"C", {"", "", "", "", "", "", ""}, {"", "", "", "", "", "", ""}, 0, 0, "mayor"},
  
  // 1 sostenido
  {"G", {"F#", "", "", "", "", "", ""}, {"", "", "", "", "", "", ""}, 1, 0, "mayor"},
  
  // 2 sostenidos
  {"D", {"F#", "C#", "", "", "", "", ""}, {"", "", "", "", "", "", ""}, 2, 0, "mayor"},
  
  // 3 sostenidos
  {"A", {"F#", "C#", "G#", "", "", "", ""}, {"", "", "", "", "", "", ""}, 3, 0, "mayor"},
  
  // 4 sostenidos
  {"E", {"F#", "C#", "G#", "D#", "", "", ""}, {"", "", "", "", "", "", ""}, 4, 0, "mayor"},
  
  // 5 sostenidos
  {"B", {"F#", "C#", "G#", "D#", "A#", "", ""}, {"", "", "", "", "", "", ""}, 5, 0, "mayor"},
  
  // 6 sostenidos / 6 bemoles (Enarmonía)
  {"F#/Gb", {"F#", "C#", "G#", "D#", "A#", "E#", ""}, {"Bb", "Eb", "Ab", "Db", "Gb", "Cb", ""}, 6, 6, "mayor"},
  
  // 5 bemoles
  {"Db", {"", "", "", "", "", "", ""}, {"Bb", "Eb", "Ab", "Db", "Gb", "", ""}, 0, 5, "mayor"},
  
  // 4 bemoles
  {"Ab", {"", "", "", "", "", "", ""}, {"Bb", "Eb", "Ab", "Db", "", "", ""}, 0, 4, "mayor"},
  
  // 3 bemoles
  {"Eb", {"", "", "", "", "", "", ""}, {"Bb", "Eb", "Ab", "", "", "", ""}, 0, 3, "mayor"},
  
  // 2 bemoles
  {"Bb", {"", "", "", "", "", "", ""}, {"Bb", "Eb", "", "", "", "", ""}, 0, 2, "mayor"},
  
  // 1 bemol
  {"F", {"", "", "", "", "", "", ""}, {"Bb", "", "", "", "", "", ""}, 0, 1, "mayor"}
};

// Array de tonalidades MENORES naturales (relativas de las mayores)
Tonalidad tonalidadesMenores[12] = {
  // La menor (relativa de Do Mayor)
  {"A", {"", "", "", "", "", "", ""}, {"", "", "", "", "", "", ""}, 0, 0, "menor"},
  
  // Mi menor (relativa de Sol Mayor)
  {"E", {"F#", "", "", "", "", "", ""}, {"", "", "", "", "", "", ""}, 1, 0, "menor"},
  
  // Si menor (relativa de Re Mayor)
  {"B", {"F#", "C#", "", "", "", "", ""}, {"", "", "", "", "", "", ""}, 2, 0, "menor"},
  
  // Fa# menor (relativa de La Mayor)
  {"F#", {"F#", "C#", "G#", "", "", "", ""}, {"", "", "", "", "", "", ""}, 3, 0, "menor"},
  
  // Do# menor (relativa de Mi Mayor)
  {"C#", {"F#", "C#", "G#", "D#", "", "", ""}, {"", "", "", "", "", "", ""}, 4, 0, "menor"},
  
  // Sol# menor (relativa de Si Mayor)
  {"G#", {"F#", "C#", "G#", "D#", "A#", "", ""}, {"", "", "", "", "", "", ""}, 5, 0, "menor"},
  
  // Re# menor / Mib menor (Enarmonía)
  {"D#/Eb", {"F#", "C#", "G#", "D#", "A#", "E#", ""}, {"Bb", "Eb", "Ab", "Db", "Gb", "Cb", ""}, 6, 6, "menor"},
  
  // La# menor / Sib menor
  {"A#/Bb", {"", "", "", "", "", "", ""}, {"Bb", "Eb", "Ab", "Db", "Gb", "", ""}, 0, 5, "menor"},
  
  // Fa menor (relativa de Lab Mayor)
  {"F", {"", "", "", "", "", "", ""}, {"Bb", "Eb", "Ab", "Db", "", "", ""}, 0, 4, "menor"},
  
  // Do menor (relativa de Mib Mayor)
  {"C", {"", "", "", "", "", "", ""}, {"Bb", "Eb", "Ab", "", "", "", ""}, 0, 3, "menor"},
  
  // Sol menor (relativa de Sib Mayor)
  {"G", {"", "", "", "", "", "", ""}, {"Bb", "Eb", "", "", "", "", ""}, 0, 2, "menor"},
  
  // Re menor (relativa de Fa Mayor)
  {"D", {"", "", "", "", "", "", ""}, {"Bb", "", "", "", "", "", ""}, 0, 1, "menor"}
};

// Escalas naturales (sin alteraciones)
String notasNaturales[7] = {"C", "D", "E", "F", "G", "A", "B"};

// Variables del sistema
int indiceActual = 0;
int modoMenu = 0; // 0: menú principal, 1: escalas mayores, 2: escalas menores, 3: modo detalle
int indiceSeleccionado = -1;
String modoActual = "mayor"; // "mayor" o "menor"

void setup() {
  Serial.begin(115200);
  
  // Configurar pines de los botones
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_SELECT, INPUT_PULLUP);
  pinMode(BTN_BACK, INPUT_PULLUP);
  
  // Inicializar pantalla serial
  Serial.println("\n====================================");
  Serial.println("   SELECTOR DE TONALIDADES MUSICALES");
  Serial.println("       CÍRCULO DE QUINTAS");
  Serial.println("====================================\n");
  
  // Mostrar el menú inicial
  mostrarMenuPrincipal();
}

void loop() {
  // Leer estado de los botones
  bool btnUpState = digitalRead(BTN_UP);
  bool btnDownState = digitalRead(BTN_DOWN);
  bool btnSelectState = digitalRead(BTN_SELECT);
  bool btnBackState = digitalRead(BTN_BACK);
  
  // Detectar flanco descendente (pulsación)
  if (btnUpState == LOW && lastBtnUpState == HIGH) {
    delay(50); // Debounce
    if (digitalRead(BTN_UP) == LOW) {
      navegarArriba();
    }
  }
  
  if (btnDownState == LOW && lastBtnDownState == HIGH) {
    delay(50); // Debounce
    if (digitalRead(BTN_DOWN) == LOW) {
      navegarAbajo();
    }
  }
  
  if (btnSelectState == LOW && lastBtnSelectState == HIGH) {
    delay(50); // Debounce
    if (digitalRead(BTN_SELECT) == LOW) {
      seleccionarOpcion();
    }
  }
  
  if (btnBackState == LOW && lastBtnBackState == HIGH) {
    delay(50); // Debounce
    if (digitalRead(BTN_BACK) == LOW) {
      retroceder();
    }
  }
  
  // Actualizar estados anteriores
  lastBtnUpState = btnUpState;
  lastBtnDownState = btnDownState;
  lastBtnSelectState = btnSelectState;
  lastBtnBackState = btnBackState;
  
  delay(10); // Pequeña pausa para no saturar el procesador
}

void navegarArriba() {
  if (modoMenu == 0) { // Menú principal
    if (indiceActual > 0) {
      indiceActual--;
    } else {
      indiceActual = 1; // Solo 2 opciones en menú principal
    }
    mostrarMenuPrincipal();
  } 
  else if (modoMenu == 1 || modoMenu == 2) { // Lista de escalas
    if (indiceActual > 0) {
      indiceActual--;
    } else {
      indiceActual = 11; // Volver al final (circular)
    }
    mostrarListaEscalas();
  }
  // En modoMenu == 3 (modo detalle), navegación deshabilitada
}

void navegarAbajo() {
  if (modoMenu == 0) { // Menú principal
    if (indiceActual < 1) {
      indiceActual++;
    } else {
      indiceActual = 0; // Solo 2 opciones en menú principal
    }
    mostrarMenuPrincipal();
  } 
  else if (modoMenu == 1 || modoMenu == 2) { // Lista de escalas
    if (indiceActual < 11) {
      indiceActual++;
    } else {
      indiceActual = 0; // Volver al inicio (circular)
    }
    mostrarListaEscalas();
  }
  // En modoMenu == 3 (modo detalle), navegación deshabilitada
}

void seleccionarOpcion() {
  if (modoMenu == 0) { // Menú principal
    if (indiceActual == 0) {
      modoMenu = 1; // Escalas mayores
      modoActual = "mayor";
    } else if (indiceActual == 1) {
      modoMenu = 2; // Escalas menores
      modoActual = "menor";
    }
    indiceActual = 0;
    mostrarListaEscalas();
  } 
  else if (modoMenu == 1 || modoMenu == 2) { // Lista de escalas
    // Entrar en modo detalle
    modoMenu = 3;
    mostrarTonalidadDetalle();
  }
  // En modoMenu == 3 (modo detalle), SELECT no hace nada
}

void retroceder() {
  if (modoMenu == 1 || modoMenu == 2) { // Si está en lista de escalas
    modoMenu = 0; // Volver al menú principal
    indiceActual = 0;
    mostrarMenuPrincipal();
  }
  else if (modoMenu == 3) { // Si está en modo detalle
    // Regresar a la lista de escalas
    modoMenu = (modoActual == "mayor") ? 1 : 2;
    mostrarListaEscalas();
  }
  // Si está en modo 0 (menú principal), BACK no hace nada
}

void mostrarMenuPrincipal() {
  Serial.println("\n=== MENÚ PRINCIPAL ===");
  Serial.println("Use UP/DOWN para navegar, SELECT para seleccionar");
  Serial.println("----------------------------------------");
  
  if (indiceActual == 0) {
    Serial.println("> ESCALAS MAYORES");
    Serial.println("  ESCALAS MENORES");
  } else {
    Serial.println("  ESCALAS MAYORES");
    Serial.println("> ESCALAS MENORES");
  }
  
  Serial.println("----------------------------------------");
  Serial.println("SELECT: Seleccionar modo | BACK: N/A en este menú");
}

void mostrarListaEscalas() {
  Serial.println("\n=== " + String(modoActual == "mayor" ? "ESCALAS MAYORES" : "ESCALAS MENORES") + " ===");
  Serial.println("Use UP/DOWN para navegar, SELECT para ver detalles");
  Serial.println("BACK: Volver al menú principal");
  Serial.println("----------------------------------------");
  
  Tonalidad* listaTonalidades;
  if (modoActual == "mayor") {
    listaTonalidades = tonalidadesMayores;
  } else {
    listaTonalidades = tonalidadesMenores;
  }
  
  for (int i = 0; i < 12; i++) {
    if (i == indiceActual) {
      Serial.print("> ");
    } else {
      Serial.print("  ");
    }
    
    Serial.print(listaTonalidades[i].nombre);
    
    // Mostrar alteraciones de forma resumida
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
  Serial.println("SELECT: Ver detalles | BACK: Menú principal");
}

void mostrarTonalidadDetalle() {
  Tonalidad t;
  if (modoActual == "mayor") {
    t = tonalidadesMayores[indiceActual];
  } else {
    t = tonalidadesMenores[indiceActual];
  }
  
  Serial.println("\n========================================");
  Serial.println("        DETALLES DE TONALIDAD");
  Serial.println("========================================");
  Serial.println("Tonalidad: " + t.nombre + " " + (t.tipo == "mayor" ? "Mayor" : "Menor"));
  
  // Mostrar información del círculo de quintas
  Serial.print("Posición en círculo de quintas: ");
  if (t.numSostenidos > 0) {
    Serial.println(String(t.numSostenidos) + " sostenidos");
  } else if (t.numBemoles > 0) {
    Serial.println(String(t.numBemoles) + " bemoles");
  } else {
    Serial.println("Sin alteraciones");
  }
  
  // Mostrar alteraciones
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
  
  // Mostrar escala completa
  Serial.println("\n--- ESCALA COMPLETA ---");
  Serial.print("Escala de " + t.nombre + " " + (t.tipo == "mayor" ? "Mayor" : "Menor") + ": ");
  
  // Calcular y mostrar las notas de la escala
  String escala[7];
  calcularEscala(t, escala);
  
  for (int i = 0; i < 7; i++) {
    Serial.print(escala[i]);
    if (i < 6) Serial.print(" - ");
  }
  Serial.println();
  
  // Mostrar relativo (mayor si es menor, menor si es mayor)
  String relativo = calcularRelativo(t);
  if (t.tipo == "mayor") {
    Serial.println("Relativo menor: " + relativo + " menor");
  } else {
    Serial.println("Relativo mayor: " + relativo + " Mayor");
  }
  
  Serial.println("\n========================================");
  Serial.println("Presione BACK para volver a la lista");
}

void calcularEscala(Tonalidad t, String escala[]) {
  // Encontrar la posición de la tónica en las notas naturales
  String notaBase = t.nombre;
  
  // Para tonalidades con sostenidos/bemoles en el nombre, tomar solo la letra
  if (notaBase.length() > 1 && (notaBase.charAt(1) == '#' || notaBase.charAt(1) == 'b')) {
    if (notaBase.length() > 2 && (notaBase.charAt(2) == '#' || notaBase.charAt(2) == 'b')) {
      notaBase = notaBase.substring(0, 2);
    } else {
      notaBase = notaBase.substring(0, 1);
    }
  } else if (notaBase == "F#/Gb" || notaBase == "D#/Eb" || notaBase == "A#/Bb") {
    // Para enarmonías, usar la primera opción
    notaBase = notaBase.substring(0, 2);
  }
  
  // Encontrar índice inicial
  int indiceInicio = -1;
  for (int i = 0; i < 7; i++) {
    if (notasNaturales[i] == notaBase.substring(0, 1)) {
      indiceInicio = i;
      break;
    }
  }
  
  if (indiceInicio == -1) {
    // Si no se encuentra, usar C por defecto
    indiceInicio = 0;
  }
  
  // Para simplificar, usaremos las notas naturales con alteraciones aplicadas
  for (int i = 0; i < 7; i++) {
    int notaIndex = (indiceInicio + i) % 7;
    escala[i] = notasNaturales[notaIndex];
    
    // Aplicar alteraciones si corresponden
    for (int j = 0; j < t.numSostenidos; j++) {
      if (t.sostenidos[j] != "") {
        String notaAlterada = t.sostenidos[j];
        String notaNatural = notaAlterada.substring(0, 1);
        if (notaNatural == escala[i]) {
          escala[i] = notaAlterada;
        }
      }
    }
    
    // Aplicar bemoles si corresponden
    for (int j = 0; j < t.numBemoles; j++) {
      if (t.bemoles[j] != "") {
        String notaAlterada = t.bemoles[j];
        String notaNatural = notaAlterada.substring(0, 1);
        if (notaNatural == escala[i]) {
          escala[i] = notaAlterada;
        }
      }
    }
  }
  
  // Ajustar específicamente para escalas menores naturales (patrón T-S-T-T-S-T-T)
  // Esto es una simplificación para mostrar la escala correcta
}

String calcularRelativo(Tonalidad t) {
  // Grados en notación americana
  String grados[7] = {"C", "D", "E", "F", "G", "A", "B"};
  
  // Posición de la tónica
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
    return t.tipo == "mayor" ? "A" : "C"; // Por defecto
  }
  
  if (t.tipo == "mayor") {
    // Relativo menor está a 1.5 tonos por debajo (6to grado)
    int indiceRelativo = (indiceTonica + 5) % 7;
    return grados[indiceRelativo];
  } else {
    // Relativo mayor está a 1.5 tonos por arriba (3er grado)
    int indiceRelativo = (indiceTonica + 2) % 7;
    return grados[indiceRelativo];
  }
}