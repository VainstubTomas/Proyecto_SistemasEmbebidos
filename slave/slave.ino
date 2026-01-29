#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>
#include <WiFi.h>
#include <HTTPClient.h>

// --- CONFIGURACIÓN DE PANTALLA ---
// Si no enciende, prueba cambiar 0x27 por 0x3F
LiquidCrystal_I2C lcd(0x27, 16, 2); 

// --- PINES ---
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
// Asegúrate de que esta IP sea fija en tu PC
const char* API_URL = "http://192.168.0.104:8080/api/parking"; // Para estado en tiempo real
const char* API_EVENT_URL = "http://192.168.0.104:8080/api/parking/event"; // Para cobros

// --- CONTROL DE TIEMPOS POR PLAZA ---
bool p1OcupadoPrev = false;
bool p2OcupadoPrev = false;
unsigned long p1InicioMs = 0;
unsigned long p2InicioMs = 0;

void conectarWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;
  
  // Solo intentamos conectar si no estamos conectados
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  Serial.print("Conectando a WiFi");
  unsigned long inicio = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - inicio) < 10000) { // Timeout reducido a 10s
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  if(WiFi.status() == WL_CONNECTED){
    Serial.println("WiFi Conectado!");
    Serial.print("IP: "); Serial.println(WiFi.localIP());
  } else {
    Serial.println("Fallo al conectar WiFi");
  }
}

// Envía estado actual (Ocupación)
void enviarDatos(long d1, long d2, int ocupados, int libres) {
  if (WiFi.status() != WL_CONNECTED) conectarWiFi();
  if (WiFi.status() != WL_CONNECTED) return; // Si falla, salimos para no colgar el loop

  HTTPClient http;
  http.begin(API_URL);
  http.addHeader("Content-Type", "application/json");

  // JSON Optimizado
  String payload = "{";
  payload += "\"lugaresLibres\":" + String(libres) + ",";
  payload += "\"ocupados\":" + String(ocupados) + ",";
  payload += "\"p1\":" + String((d1 > 0 && d1 < 10) ? 1 : 0) + ",";
  payload += "\"p2\":" + String((d2 > 0 && d2 < 10) ? 1 : 0);
  payload += "}";

  int httpResponseCode = http.POST(payload);
  
  if (httpResponseCode > 0) {
    Serial.printf("Datos enviados. Código: %d\n", httpResponseCode);
  } else {
    Serial.printf("Error enviando datos: %s\n", http.errorToString(httpResponseCode).c_str());
  }
  http.end();
}

// Envía evento de FINALIZACIÓN (Cobro)
void enviarEvento(const char* plaza, unsigned long duracionMs) {
  if (WiFi.status() != WL_CONNECTED) conectarWiFi();
  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  http.begin(API_EVENT_URL);
  http.addHeader("Content-Type", "application/json");

  // Convertimos a segundos para facilitar el cálculo monetario en el servidor
  unsigned long duracionSegundos = duracionMs / 1000;

  String payload = "{";
  payload += "\"plaza\":\"" + String(plaza) + "\",";
  payload += "\"duracion_segundos\":" + String(duracionSegundos) + ","; 
  payload += "\"evento\":\"salida\""; // Etiqueta útil para el backend
  payload += "}";

  Serial.println("Enviando evento de cobro: " + payload);

  int httpResponseCode = http.POST(payload);

  if (httpResponseCode == 200) {
    String response = http.getString();
    Serial.println("Cobro registrado exitosamente.");
    Serial.println("Respuesta del servidor: " + response);
    // Aquí podrías parsear 'response' si el servidor devuelve el costo calculado
    // Ejemplo: {"costo": 1500}
  } else {
    Serial.printf("Error en cobro. Código: %d\n", httpResponseCode);
  }
  
  http.end();
}

void setup() {
  Serial.begin(115200);

  // Inicialización de la LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("   PROYECTO   ");
  lcd.setCursor(0, 1);
  lcd.print("   PARKING    ");

  conectarWiFi();

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
  // --- LÓGICA DE SENSORES ---
  long d1 = medirDistancia(TRIG_1, ECHO_1);
  delay(50); // Pequeña pausa entre sonares para evitar interferencia
  long d2 = medirDistancia(TRIG_2, ECHO_2);

  // Umbral de 10cm para considerar ocupado
  bool p1Ocupado = (d1 > 0 && d1 < 10);
  bool p2Ocupado = (d2 > 0 && d2 < 10);

  // --- DETECCIÓN DE EVENTOS (Flancos) ---
  
  // PLAZA 1
  if (!p1OcupadoPrev && p1Ocupado) {
    p1InicioMs = millis(); // Auto entra
    Serial.println("Auto entro en P1");
  }
  if (p1OcupadoPrev && !p1Ocupado) {
    // Auto sale -> Calcular y cobrar
    unsigned long duracion = millis() - p1InicioMs;
    Serial.print("Auto salio de P1. Duracion (ms): "); Serial.println(duracion);
    // Filtro de ruido: Si duró menos de 5 segundos, quizás fue alguien pasando caminando
    if (duracion > 5000) { 
        enviarEvento("P1", duracion);
    }
  }

  // PLAZA 2
  if (!p2OcupadoPrev && p2Ocupado) {
    p2InicioMs = millis();
    Serial.println("Auto entro en P2");
  }
  if (p2OcupadoPrev && !p2Ocupado) {
    unsigned long duracion = millis() - p2InicioMs;
    Serial.print("Auto salio de P2. Duracion (ms): "); Serial.println(duracion);
    if (duracion > 5000) {
        enviarEvento("P2", duracion);
    }
  }

  p1OcupadoPrev = p1Ocupado;
  p2OcupadoPrev = p2Ocupado;

  // --- ACTUALIZACIÓN DE ESTADO ---
  int ocupados = (p1Ocupado ? 1 : 0) + (p2Ocupado ? 1 : 0);
  int nuevosLibres = 2 - ocupados;

  if (nuevosLibres != lugaresLibres) {
    lugaresLibres = nuevosLibres;
    actualizarLCD();
    enviarDatos(d1, d2, ocupados, lugaresLibres);
  }

  // --- CONTROL DE BARRERA ---
  if (digitalRead(PIN_BOTON) == LOW && lugaresLibres > 0) {
    abrirBarrera("  BIENVENIDO   ");
  }
  if (digitalRead(PIN_IR_SALIDA) == HIGH) { // Asumiendo que HIGH detecta salida
    abrirBarrera("  BUEN VIAJE   ");
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
  lcd.print("LIBRES: "); lcd.print(lugaresLibres);
  lcd.setCursor(0, 1);
  
  // Medimos de nuevo rápido para actualizar UI, o usamos variables globales mejor
  // Para simplicidad, invocamos medirDistancia pero idealmente usaríamos el estado 'p1Ocupado'
  lcd.print("P1:"); lcd.print(medirDistancia(TRIG_1, ECHO_1) < 10 ? "[X]" : "[ ]");
  lcd.print(" P2:"); lcd.print(medirDistancia(TRIG_2, ECHO_2) < 10 ? "[X]" : "[ ]");
}
