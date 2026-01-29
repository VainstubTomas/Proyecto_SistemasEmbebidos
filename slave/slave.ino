#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>
#include <WiFi.h>
#include <HTTPClient.h>

// --- INTENTA CON 0x27, SI NO FUNCIONA CAMBIA A 0x3F ---
LiquidCrystal_I2C lcd(0x27, 16, 2); 

const int PIN_BOTON = 4;
const int PIN_IR_SALIDA = 13; 
const int PIN_SERVO = 27;

const int TRIG_1 = 32; const int ECHO_1 = 35;
const int TRIG_2 = 25; const int ECHO_2 = 26;

Servo barrera;
int lugaresLibres = 2;

// --- WIFI ---
const char* WIFI_SSID = "TP-Link_6D76";
const char* WIFI_PASSWORD = "55500722";

// --- SERVIDOR API (Node/Express) ---
// Usa la IP local del PC donde corre el servidor
const char* API_URL = "http://192.168.1.100:8080/api/parking";
const char* API_EVENT_URL = "http://192.168.1.100:8080/api/parking/event";

// --- CONTROL DE TIEMPOS POR PLAZA ---
bool p1OcupadoPrev = false;
bool p2OcupadoPrev = false;
unsigned long p1InicioMs = 0;
unsigned long p2InicioMs = 0;

void conectarWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long inicio = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - inicio) < 15000) {
    delay(500);
  }
}

void enviarDatos(long d1, long d2, int ocupados, int libres) {
  if (WiFi.status() != WL_CONNECTED) {
    conectarWiFi();
    if (WiFi.status() != WL_CONNECTED) return;
  }

  HTTPClient http;
  http.begin(API_URL);
  http.addHeader("Content-Type", "application/json");

  String payload = "{";
  payload += "\"lugaresLibres\":" + String(libres) + ",";
  payload += "\"ocupados\":" + String(ocupados) + ",";
  payload += "\"p1\":" + String((d1 > 0 && d1 < 10) ? 1 : 0) + ",";
  payload += "\"p2\":" + String((d2 > 0 && d2 < 10) ? 1 : 0);
  payload += "}";

  http.POST(payload);
  http.end();
}

void enviarEvento(const char* plaza, unsigned long duracionMs) {
  if (WiFi.status() != WL_CONNECTED) {
    conectarWiFi();
    if (WiFi.status() != WL_CONNECTED) return;
  }

  HTTPClient http;
  http.begin(API_EVENT_URL);
  http.addHeader("Content-Type", "application/json");

  String payload = "{";
  payload += "\"plaza\":\"" + String(plaza) + "\",";
  payload += "\"duracionMs\":" + String(duracionMs);
  payload += "}";

  http.POST(payload);
  http.end();
}

void setup() {
  Serial.begin(115200);

  conectarWiFi();

  // Inicialización de la LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("   PROYECTO   ");
  lcd.setCursor(0, 1);
  lcd.print("   PARKING    ");

  pinMode(PIN_BOTON, INPUT_PULLUP);
  pinMode(PIN_IR_SALIDA, INPUT_PULLUP);
  pinMode(TRIG_1, OUTPUT); pinMode(ECHO_1, INPUT);
  pinMode(TRIG_2, OUTPUT); pinMode(ECHO_2, INPUT);

  ESP32PWM::allocateTimer(0);
  barrera.setPeriodHertz(50);
  barrera.attach(PIN_SERVO, 500, 2400);
  barrera.write(0);

  delay(2000);
  actualizarLCD();
}

void loop() {
  // Lógica de Entrada
  if (digitalRead(PIN_BOTON) == LOW && lugaresLibres > 0) {
    abrirBarrera("  BIENVENIDO   ");
  }

  // Lógica de Salida (Sensor IR Pin 13)
  if (digitalRead(PIN_IR_SALIDA) == HIGH) { 
    abrirBarrera("  BUEN VIAJE   ");
  }

  // Medición de ocupación
  long d1 = medirDistancia(TRIG_1, ECHO_1);
  delay(60); 
  long d2 = medirDistancia(TRIG_2, ECHO_2);

  bool p1Ocupado = (d1 > 0 && d1 < 10);
  bool p2Ocupado = (d2 > 0 && d2 < 10);

  if (!p1OcupadoPrev && p1Ocupado) {
    p1InicioMs = millis();
  }
  if (p1OcupadoPrev && !p1Ocupado) {
    unsigned long duracion = millis() - p1InicioMs;
    enviarEvento("P1", duracion);
  }

  if (!p2OcupadoPrev && p2Ocupado) {
    p2InicioMs = millis();
  }
  if (p2OcupadoPrev && !p2Ocupado) {
    unsigned long duracion = millis() - p2InicioMs;
    enviarEvento("P2", duracion);
  }

  p1OcupadoPrev = p1Ocupado;
  p2OcupadoPrev = p2Ocupado;

  int ocupados = ( p1Ocupado ? 1 : 0 ) + ( p2Ocupado ? 1 : 0 );
  int nuevosLibres = 2 - ocupados;

  if (nuevosLibres != lugaresLibres) {
    lugaresLibres = nuevosLibres;
    actualizarLCD();
    enviarDatos(d1, d2, ocupados, lugaresLibres);
  }
  delay(200);
}

long medirDistancia(int trig, int echo) {
  digitalWrite(trig, LOW); delayMicroseconds(2);
  digitalWrite(trig, HIGH); delayMicroseconds(10);
  digitalWrite(trig, LOW);
  return pulseIn(echo, HIGH, 30000) * 0.034 / 2;
}

void abrirBarrera(String msg) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(msg);
  lcd.setCursor(0, 1);
  lcd.print("ABRIENDO...     ");
  barrera.write(90); 
  delay(3000);
  barrera.write(0);
  actualizarLCD();
}

void actualizarLCD() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("LIBRES: ");
  lcd.print(lugaresLibres);
  lcd.setCursor(0, 1);
  lcd.print("P1:");
  lcd.print(medirDistancia(TRIG_1, ECHO_1) < 10 ? "[X]" : "[ ]");
  lcd.print(" P2:");
  lcd.print(medirDistancia(TRIG_2, ECHO_2) < 10 ? "[X]" : "[ ]");
}