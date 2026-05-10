#include "arduinoFFT.h"


#define CHANNEL 4
const uint16_t samples = 1024; //This value MUST ALWAYS be a power of 2
const double samplingFrequency = 8000; //Hz, must be less than 10000 due to ADC
unsigned int sampling_period_us;
unsigned long microseconds;

double vReal[samples];
double vImag[samples];

// Pines de salida para leds de afinacion
int LED_BEMOL=15;
int LED_SOS=33;
int LED_OK=32;


ArduinoFFT<double> FFT = ArduinoFFT<double>(vReal, vImag, samples, samplingFrequency);

/* ================= NOTAS ================= */
const char* nombresNotas[] = {
  "C", "C#", "D", "D#", "E", "F",
  "F#", "G", "G#", "A", "A#", "B"
};
const double A4_FREQ = 440.0;
const int A4_MIDI = 69;
const double MARGEN_AFINADO = 1.0; // Hz
// Convierte frecuencia a nota MIDI
int frecuenciaANota(double freq) {
  return round(12.0 * log2(freq / A4_FREQ) + A4_MIDI);
}

// Convierte nota MIDI a frecuencia ideal
double notaAFrecuencia(int nota) {
  return A4_FREQ * pow(2.0, (nota - A4_MIDI) / 12.0);
}


#define SCL_INDEX 0x00
#define SCL_TIME 0x01
#define SCL_FREQUENCY 0x02
#define SCL_PLOT 0x03
void PrintVector(double *vData, uint16_t bufferSize, uint8_t scaleType);
void setup()
{
  sampling_period_us = round(1000000*(1.0/samplingFrequency));
  Serial.begin(115200);
  while(!Serial);
  Serial.println("Ready");
  pinMode(LED_SOS, OUTPUT);
  pinMode(LED_OK, OUTPUT);
  pinMode(LED_BEMOL, OUTPUT);
}

void loop()
{

  microseconds = micros();
  for(int i=0; i<samples; i++)
  {
      vReal[i] = analogRead(CHANNEL);
      vImag[i] = 0;
      while(micros() - microseconds < sampling_period_us){
    
      }
      microseconds += sampling_period_us;
  }

  FFT.windowing(FFTWindow::Hamming, FFTDirection::Forward); /* Weigh data */
  FFT.compute(FFTDirection::Forward); /* Compute FFT */
  FFT.complexToMagnitude(); /* Compute magnitudes */
  double x = FFT.majorPeak();
  Serial.println(x, 6); //Print out what frequency is the most dominant.
  //while(1); /* Run Once */

  if (x < 60 || x > 2000) {
    Serial.print("Frecuencia fuera de rango: ");
    Serial.print(x, 2);
    Serial.println(" Hz\n");
    digitalWrite(LED_BEMOL,LOW);
    digitalWrite(LED_SOS,LOW);
    digitalWrite(LED_OK,LOW);
    delay(300);
    return;
  }
  int notaMidi = frecuenciaANota(x);
  double freqIdeal = notaAFrecuencia(notaMidi);
  double error = x - freqIdeal;

  int indiceNota = notaMidi % 12;
  int octava = (notaMidi / 12) - 1;
  /* ======== SALIDA SERIAL ======== */
  Serial.println("--------------------------------------");
  Serial.print("Frecuencia detectada: ");
  Serial.print(x, 2);
  Serial.println(" Hz");

  Serial.print("Nota detectada: ");
  Serial.print(nombresNotas[indiceNota]);
  Serial.print(octava);

  Serial.print("  (");
  Serial.print(freqIdeal, 2);
  Serial.println(" Hz)");

  Serial.print("Desviacion: ");
  Serial.print(error, 2);
  Serial.println(" Hz");

  if (abs(error) <= MARGEN_AFINADO) {
    Serial.println("✔ AFINADO");
    digitalWrite(LED_BEMOL,LOW);
    digitalWrite(LED_SOS,LOW);
    digitalWrite(LED_OK,HIGH);
  }
  else if (error > 0) {
    Serial.println("⬇ BAJA EL TONO");
    digitalWrite(LED_BEMOL,LOW);
    digitalWrite(LED_SOS,HIGH);
    digitalWrite(LED_OK,LOW);
  }
  else {
    Serial.println("⬆ SUBE EL TONO");
    digitalWrite(LED_BEMOL,HIGH);
    digitalWrite(LED_OK,LOW);
    digitalWrite(LED_SOS,LOW);
  }

  Serial.println("--------------------------------------\n");


delay(100);

}

