#include <WiFi.h>
#include <ThingSpeak.h>

#include <OneWire.h>
#include <DallasTemperature.h>

const int analogInPin = 34;  // Pin analógico para el sensor de pH
int buf[10], temp;
float pH;

// Pin del sensor de temperatura
const int oneWireBus = 5;  // Pin D32 en el ESP32
OneWire oneWire(oneWireBus);
DallasTemperature sensors(&oneWire);

struct TemperaturaData {
    float tempC= 0.0; 
};
TemperaturaData temperatureData;

//Sensor de profundidad 
#define TRIG_PIN 12 //cafee
#define ECHO_PIN 13//amarillo

float duration, distance_cm;
float full = 23;
float empty = 53;


//bUZSER
const int buzzerPin = 15; // Pin conectado al buzzer

//const char* ssid = "Totalplay-E4A7";  // Reemplaza con tu SSID
//const char* password = "E4A7FEE2y5jh8Ak2";

const char* ssid = "Redmi Note 10";  // Reemplaza con tu SSID
const char* password = "perla123";
  
WiFiClient client;

unsigned long myChannelNumber = 2483355;  // Tu Channel ID en ThingSpeak
const char* myWriteAPIKey = "Q4PY42P3UGVS60GA";  // Tu Write API Key

void setup(void) {
  Serial.begin(115200);  
  analogReadResolution(12); 
  sensors.begin();  // Inicia el sensor de temperatura
  // Inicia Distance Sensor
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(buzzerPin, OUTPUT);  // Configura el pin del buzzer como salid
  connectWiFi();  // Conecta a la red WiFi
  ThingSpeak.begin(client);  // Inicializa ThingSpeak
}


//sensor nivel
float get_distance() {
  float distance_cm = 0;
  while (distance_cm == 0) {
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(20);
    duration = pulseIn(ECHO_PIN, HIGH, 24000);
    // Convertir la duración del pulso en centímetros
    distance_cm = (duration * 0.0343) / 2;
    delay(100);
  }
  return distance_cm;
}

float get_level() {
  float distance_cm = get_distance();
  if (distance_cm >= empty) distance_cm = empty;
  if (distance_cm <= full) distance_cm = full;
  return map(distance_cm, empty, full, 0, 100);
}


void loop(void) {
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();  // Reconectar si se pierde la conexión WiFi
  } else {
    Medir_PH();  // Obtiene el valor del PH
    Medir_Temperatura();  // Obtiene la temperatura
    Serial.print("Nivel de pH: ");
    Serial.println(pH, 2);  // Imprime el valor de pH medido
      // Water Level Control
    float level = get_level();
    distance_cm = get_distance(); // Update the global variable
    Serial.print("Level: ");
    Serial.print(level);
    Serial.println(" %");
    Serial.print("Distance: ");
    Serial.print(distance_cm);
    Serial.println(" cm");
    sendDataToThingSpeak();  // Envía el dato a ThingSpeak
  }
  delay(5000);  // Espera 20 segundos antes de la próxima medición y envío
}

void Medir_PH(void) {
  for (int i = 0; i < 10; i++) {
    buf[i] = analogRead(analogInPin);
    delay(100);
  }
  // Ordenar las lecturas para obtener un valor más estable
  for (int i = 0; i < 9; i++) {
    for (int j = i + 1; j < 10; j++) {
      if (buf[i] > buf[j]) {
        temp = buf[i];
        buf[i] = buf[j];
        buf[j] = temp;
      }
    }
  }
  unsigned long int conversor = 0;
  for (int i = 2; i < 8; i++) {
    conversor += buf[i];
  }
  float pHVol = (float)conversor * 3.3 / 4095 / 6;  // Calcula el voltaje promedio
  
  // Ajusta estos valores basados en tus mediciones de calibración
  float slope = -5.70;  // Pendiente (debe ser calibrada)
  float intercept = 19.25;  // Intersección (debe ser calibrada)
  
  pH = slope * pHVol + intercept;
    // Activa el buzzer si el pH es menor o igual a 3
  if (pH <= 3.0) {
    digitalWrite(buzzerPin, HIGH);  // Enciende el buzzer
  } else {
    digitalWrite(buzzerPin, LOW);  // Apaga el buzzer
  }

}

void Medir_Temperatura() {
  sensors.requestTemperatures();  // Solicita la temperatura al sensor
  float tempC = sensors.getTempCByIndex(0);  // Lee la temperatura en grados Celsius
  if (tempC != DEVICE_DISCONNECTED_C) {
    Serial.print("Temperatura: ");
    Serial.print(tempC);
    Serial.println("°C");

    // Guarda la temperatura en la estructura
    temperatureData.tempC = tempC;
  } else {
    Serial.println("Error al leer la temperatura!");
  }
}
void connectWiFi() {
  Serial.print("Conectando a WiFi");
  WiFi.begin(ssid, password);
  int retries = 0;
  while (WiFi.status() != WL_CONNECTED && retries < 10) {
    delay(500);
    Serial.print(".");
    retries++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi Conectado.");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nError al conectar a WiFi. Intentando de nuevo...");
  }
}

void sendDataToThingSpeak() {
  ThingSpeak.setField(1, temperatureData.tempC);  // Envía la temperatura como segundo campo
  ThingSpeak.setField(2, pH);   
  ThingSpeak.setField(3, distance_cm);
  ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
  Serial.println("Datos enviados a ThingSpeak.");
}
