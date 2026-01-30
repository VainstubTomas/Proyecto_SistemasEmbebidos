#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>

WiFiClientSecure espClient;

PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;

// config mqtt
// dir mqtt broker ip adreess
const char* mqtt_server = "j72b9212.ala.us-east-1.emqxsl.com";
const char* mqtt_user = "SistemasEmbebidos";        // Tu Username
const char* mqtt_password = "SistemasEmbebidos"; // Tu Password

// certificado broker
const char* EMQX_CA_CERT = R"EOF(
-----BEGIN CERTIFICATE-----
MIIDjjCCAnagAwIBAgIQAzrx5qcRqaC7KGSxHQn65TANBgkqhkiG9w0BAQsFADBh
MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3
d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBH
MjAeFw0xMzA4MDExMjAwMDBaFw0zODAxMTUxMjAwMDBaMGExCzAJBgNVBAYTAlVT
MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j
b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IEcyMIIBIjANBgkqhkiG
9w0BAQEFAAOCAQ8AMIIBCgKCAQEAuzfNNNx7a8myaJCtSnX/RrohCgiN9RlUyfuI
2/Ou8jqJkTx65qsGGmvPrC3oXgkkRLpimn7Wo6h+4FR1IAWsULecYxpsMNzaHxmx
1x7e/dfgy5SDN67sH0NO3Xss0r0upS/kqbitOtSZpLYl6ZtrAGCSYP9PIUkY92eQ
q2EGnI/yuum06ZIya7XzV+hdG82MHauVBJVJ8zUtluNJbd134/tJS7SsVQepj5Wz
tCO7TG1F8PapspUwtP1MVYwnSlcUfIKdzXOS0xZKBgyMUNGPHgm+F6HmIcr9g+UQ
vIOlCsRnKPZzFBQ9RnbDhxSJITRNrw9FDKZJobq7nMWxM4MphQIDAQABo0IwQDAP
BgNVHRMBAf8EBTADAQH/MA4GA1UdDwEB/wQEAwIBhjAdBgNVHQ4EFgQUTiJUIBiV
5uNu5g/6+rkS7QYXjzkwDQYJKoZIhvcNAQELBQADggEBAGBnKJRvDkhj6zHd6mcY
1Yl9PMWLSn/pvtsrF9+wX3N3KjITOYFnQoQj8kVnNeyIv/iPsGEMNKSuIEyExtv4
NeF22d+mQrvHRAiGfzZ0JFrabA0UWTW98kndth/Jsw1HKj2ZL7tcu7XUIOGZX1NG
Fdtom/DzMNU+MeKNhJ7jitralj41E6Vf8PlwUHBHQRFXGU7Aj64GxJUTFy8bJZ91
8rGOmaFvE7FBcf6IKshPECBV1/MUReXgRPTqh5Uykw7+U0b6LJ3/iyK5S9kJRaTe
pLiaWN0bfVKfjllDiIGknibVb63dDcY3fe0Dkhvld1927jyNxF1WW6LZZm6zNTfl
MrY=
-----END CERTIFICATE-----
)EOF";

// --- INTENTA CON 0x27, SI NO FUNCIONA CAMBIA A 0x3F ---
LiquidCrystal_I2C lcd(0x27, 16, 2);
const int PIN_BOTON = 4;
const int PIN_IR_SALIDA = 13;
const int PIN_SERVO = 27;

const int TRIG_1 = 32; const int ECHO_1 = 35;
const int TRIG_2 = 25; const int ECHO_2 = 26;

Servo barrera;
int lugaresLibres = 2;

// Variables WiFi
const char* ssid = "TP-Link_6D76";
const char* password = "55500722";

// Variables de control de tiempo
unsigned long tiempoInicialP1 = 0;
unsigned long tiempoInicialP2 = 0;
bool estadoAnteriorP1 = false;
bool estadoAnteriorP2 = false;

void setup() {
  Serial.begin(115200);
  setup_wifi();
  espClient.setCACert(EMQX_CA_CERT);
  client.setServer(mqtt_server, 8883);

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

void setup_wifi(){
  delay(10);
  Serial.println();
  Serial.print("conectado a");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while(WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi conectado");
    Serial.println("IP ADRESS:");
    Serial.println(WiFi.localIP());
}

// realiza la reconexiòn MQTT en caso de fallo
void reconnect (){
  while (!client.connected()){
    Serial.print("intentando conexiòn MQTT");

    if (client.connect ("ESP32 client", mqtt_user, mqtt_password)){
      Serial.println("conectado");
      } else {
        Serial.print ("fallo, rc=");
        Serial.print (client.state());
        Serial.println ("intente en 5s");
        delay (5000);
        }
    }
  }

void publicarDatosMQTT() {
  long now = millis();
  if (now - lastMsg > 1000) {
    lastMsg = now;

    long d1 = medirDistancia(TRIG_1, ECHO_1);
    delay(60);
    long d2 = medirDistancia(TRIG_2, ECHO_2);

    bool ocupadoP1 = (d1 > 0 && d1 < 10);
    bool ocupadoP2 = (d2 > 0 && d2 < 10);

    // ===== PARKING 1 =====
    if (ocupadoP1 && !estadoAnteriorP1) {
      tiempoInicialP1 = now;
      Serial.println(">>> VEHICULO ENTRÓ A P1");

    } else if (!ocupadoP1 && estadoAnteriorP1) {
      unsigned long tiempoFinal = now;
      unsigned long diferenciaTiempo = tiempoFinal - tiempoInicialP1;

      Serial.println(">>> VEHICULO SALIÓ DE P1");
      Serial.print("Tiempo: ");
      Serial.print(diferenciaTiempo / 1000);
      Serial.println(" seg");

      char payload[250];
      sprintf(payload, "{\"espacio\":\"P1\",\"tiempoInicial\":%lu,\"tiempoFinal\":%lu,\"diferenciaTiempo\":%lu}", 
              tiempoInicialP1, tiempoFinal, diferenciaTiempo);
      client.publish("embebidos/parking/evento", payload);

      tiempoInicialP1 = 0;
    }

    // ===== PARKING 2 =====
    if (ocupadoP2 && !estadoAnteriorP2) {
      tiempoInicialP2 = now;
      Serial.println(">>> VEHICULO ENTRÓ A P2");

    } else if (!ocupadoP2 && estadoAnteriorP2) {
      unsigned long tiempoFinal = now;
      unsigned long diferenciaTiempo = tiempoFinal - tiempoInicialP2;

      Serial.println(">>> VEHICULO SALIÓ DE P2");
      Serial.print("Tiempo: ");
      Serial.print(diferenciaTiempo / 1000);
      Serial.println(" seg");

      // PUBLICAR TODO EN UN SOLO MENSAJE
      char payload[250];
      sprintf(payload, "{\"espacio\":\"P2\",\"tiempoInicial\":%lu,\"tiempoFinal\":%lu,\"diferenciaTiempo\":%lu}", 
              tiempoInicialP2, tiempoFinal, diferenciaTiempo);
      client.publish("embebidos/parking/evento", payload);

      tiempoInicialP2 = 0;
    }

    estadoAnteriorP1 = ocupadoP1;
    estadoAnteriorP2 = ocupadoP2;
  }
}

void loop() {
  //integracion MQTT
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  publicarDatosMQTT();

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

  int ocupados = ( (d1 > 0 && d1 < 10) ? 1 : 0 ) + ( (d2 > 0 && d2 < 10) ? 1 : 0 );
  int nuevosLibres = 2 - ocupados;

  if (nuevosLibres != lugaresLibres) {
    lugaresLibres = nuevosLibres;
    actualizarLCD();
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