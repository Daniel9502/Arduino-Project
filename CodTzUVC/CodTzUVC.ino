#include <Wire.h>
#include <Adafruit_AHTX0.h>
#include "ScioSense_ENS160.h"
#include "DFRobot_MICS.h"
#include <SoftwareSerial.h>

// Pinul LED-ului de pe placa
#define LED_PIN 13

// Prototipuri de funcții
void citesteSenzoriGaz();
void sirenaS();
void alarmaGaz();
void oprireReleeSiAlarma();
bool miscareDetectata();
void pornireRelee();
void monitorizareMiscarile();
void monitorizareSerial();

// AHT20 Sensor
Adafruit_AHTX0 aht;
int tempC;
int humidity;

// ENS160 Sensor
ScioSense_ENS160 ens160(ENS160_I2CADDR_1);

// Senzorul ZE03-O3
SoftwareSerial ze03Serial(0, 1); 

// DFRobot MICS Sensor
#define CALIBRATION_TIME 1
#define ADC_PIN A2
#define POWER_PIN 5
DFRobot_MICS_ADC mics(ADC_PIN, POWER_PIN);

// Senzor MQ-02
#define MQ02_PIN A1
#define GAS_THRESHOLD_MQ02 500

// Senzor MQ-05 pentru GPL
#define MQ05_PIN A3
#define GPL_THRESHOLD_MQ05 500

// Senzor MQ-06 pentru GPL
#define MQ06_PIN A0
#define GAS_THRESHOLD_MQ06 500

// Pinii pentru relee, buzzer și PIR
const int led = 13;
const int releu1 = 2;
const int releu2 = 4;
const int releu3 = 7;
const int buzzer = 10;
const int pir1 = 3;
const int pir2 = 6;
const int pir3 = 9;
const int pir4 = 11;

// Definire semnale pentru relee
#define semnalLreleu LOW
#define semnalHreleu HIGH

// Array pentru pinii PIR și valori
const int pirPins[] = {pir1, pir2, pir3, pir4};
int pirValues[4];

// Variabile pentru monitorizarea prezenței
unsigned long timpInceputDetectare = 0;
bool persoanaInZona = false;
bool sistemOprit = false;

bool releuState = semnalHreleu;
unsigned long timpUltimaSchimbare = 0;
unsigned long intervalRelee = 100;

void setup() {
  pinMode(led, OUTPUT);
  pinMode(releu1, OUTPUT);
  pinMode(releu2, OUTPUT);
  pinMode(releu3, OUTPUT);
  pinMode(buzzer, OUTPUT);
  pinMode(MQ05_PIN, INPUT);

  for (int i = 0; i < 4; i++) {
    pinMode(pirPins[i], INPUT);
  }

  digitalWrite(releu1, semnalHreleu);
  digitalWrite(releu2, semnalHreleu);
  digitalWrite(releu3, semnalHreleu);
  digitalWrite(buzzer, semnalLreleu);

  Serial.begin(9600);
  ze03Serial.begin(9600);

  bool allSensorsPresent = true; // Flag pentru a verifica toți senzorii

  // Verificarea senzorului ENS160
  if (!ens160.begin() || !ens160.available()) {
    Serial.println("Eroare senzor ENS160!");
    allSensorsPresent = false;
  } else {
    Serial.println("ENS160 conectat.");
    ens160.setMode(ENS160_OPMODE_STD);
  }

  // Verificarea senzorului AHT20
  if (!aht.begin()) {
    Serial.println("Eroare senzor AHT!");
    allSensorsPresent = false;
  } else {
    Serial.println("AHT20 conectat.");
  }

  // Verificarea senzorului MICS
  if (!mics.begin()) {
    Serial.println("Eroare senzor MICS!");
    allSensorsPresent = false;
  } else {
    Serial.println("MICS conectat.");
  }

  // Verificarea senzorilor de gaz analogici MQ-02, MQ-05, MQ-06
  if (analogRead(MQ02_PIN) == 0) {
    Serial.println("Eroare la senzorul MQ-02!");
    allSensorsPresent = false;
  } else {
    Serial.println("MQ-02 conectat.");
  }

  if (analogRead(MQ05_PIN) == 0) {
    Serial.println("Eroare la senzorul MQ-05!");
    allSensorsPresent = false;
  } else {
    Serial.println("MQ-05 conectat.");
  }

  if (analogRead(MQ06_PIN) == 0) {
    Serial.println("Eroare la senzorul MQ-06!");
    allSensorsPresent = false;
  } else {
    Serial.println("MQ-06 conectat.");
  }


  
  // Verificarea finală
  if (allSensorsPresent) {
    Serial.println("Toți senzorii sunt conectați corespunzător.");
  } else {
    Serial.println("Unele erori au fost detectate. Verificați senzorii și conexiunile.");
  }

  // Ton de inițializare a sistemului
  tone(buzzer, 1000);
  delay(500);
  noTone(buzzer);
}
void loop() {
   // Verificare date primite prin Bluetooth
  if (Serial.available()) {
    String comanda = Serial.readStringUntil('\n');
    Serial.print("Comandă primită: ");
    Serial.println(comanda);

    // Dacă primim o anumită comandă, oprim releele
    if (comanda == "stop") {
      oprireReleeSiAlarma();
    }
  }
  
  if (sistemOprit) {
    monitorizareSerial();
    return;  // Nu facem nimic dacă sistemul este oprit
  }

  monitorizareMiscarile();  // Monitorizăm timpul cât o persoană se află în zonă

  if (persoanaInZona) {
    oprireReleeSiAlarma();
    sirenaS();
    return;
  }

  // Dacă nu este persoană în zonă, pornim releele
  if (!persoanaInZona) {
    pornireRelee();  // Pornim releele în lipsa mișcării
  }
  
  citesteSenzoriGaz();
}

// Funcția pentru monitorizarea timpului de prezență
void monitorizareMiscarile() {
  bool miscareDetectata = false;

  // Verificăm dacă senzorii PIR detectează mișcare
  for (int i = 0; i < 4; i++) {
    pirValues[i] = digitalRead(pirPins[i]);
    if (pirValues[i] == semnalHreleu) {
      miscareDetectata = true;
    }
  }

  if (miscareDetectata) {
    if (!persoanaInZona) {
      persoanaInZona = true;
      timpInceputDetectare = millis();  // Începem să contorizăm timpul
    } else if (millis() - timpInceputDetectare >= 20000) {
      // Dacă persoana este în zonă mai mult de 20 secunde, oprim sistemul
      Serial.println("Persoană detectată timp de 20 secunde! Oprirea sistemului.");
      Serial.println("Sistemul trebuie repornit de către operator!");
      sistemOprit = true;
      oprireReleeSiAlarma();  // Oprim sistemul complet
    }
  } else {
    persoanaInZona = false;  // Resetăm flag-ul dacă nu mai este mișcare
  }
}

// Funcția pentru citirea senzorilor de gaz GPL
void citesteSenzoriGaz() {
  // Citirea valorilor de la AHT20 (temperatură și umiditate)
  sensors_event_t humidityEvent, tempEvent;
  aht.getEvent(&humidityEvent, &tempEvent);
  float tempC = tempEvent.temperature;
  float humidity = humidityEvent.relative_humidity;

  // Afișarea valorilor de temperatură și umiditate
  Serial.print("Temperatura: "); Serial.print(tempC); Serial.println(" °C");
  Serial.print("Umiditate: "); Serial.print(humidity); Serial.println(" %rH");

  // Citirea și afișarea datelor de la ENS160
  if (ens160.available()) {
    ens160.set_envdata(tempC, humidity);
    ens160.measure(true);
    ens160.measureRaw(true);

    int aqi = ens160.getAQI();
    int tvoc = ens160.getTVOC();
    int eco2 = ens160.geteCO2();

    Serial.print("Indicele de calitate a aerului: "); Serial.print(aqi);
    Serial.print("  Compuși organici volatili: "); Serial.print(tvoc); Serial.println(" ppb");
    Serial.print("eCO2: "); Serial.print(eco2); Serial.print(" ppm");

    if (eco2 > 1000 || aqi > 100 || tvoc > 800) {
      Serial.println("Contaminare aer detectată! Oprim releele și pornim alarma.");
      oprireReleeSiAlarma();  // Se opresc releele și pornește alarma în caz de contaminare aer
      return;
    }
  }

  // Citire pentru MQ-02 (Propan, Hidrogen, C3H8)
  int mq02Value = analogRead(MQ02_PIN);
  Serial.print("MQ-02 gaz detectat: "); Serial.print(mq02Value); Serial.println(" ppm");

  if (mq02Value > GAS_THRESHOLD_MQ02) {
    Serial.println("Gaz detectat de MQ-02! Oprim releele și pornim alarma.");
    oprireReleeSiAlarma();
    return;
  }

  // Citirea și afișarea datelor de la DFRobot MICS
  float co = mics.getGasData(CO);
  float ch4 = mics.getGasData(CH4);
  float etanol = mics.getGasData(C2H5OH);
  float isobutan = mics.getGasData(C4H10);
  float h2 = mics.getGasData(H2);
  float h2s = mics.getGasData(H2S);
  float nh3 = mics.getGasData(NH3);
  float no = mics.getGasData(NO);
  float no2 = mics.getGasData(NO2);

 // Serial.print("Monoxid de carbon: "); Serial.print(co, 1); Serial.println(" ppm");
 if (co >= 0) {
    Serial.print("Monoxid de carbon: "); Serial.print(co, 1); Serial.println(" ppm");
  } else {
    Serial.println("Monoxid de carbon: 0 ppm ");
  }

  Serial.print("Metan: "); Serial.print(ch4, 1); Serial.println(" ppm");
  Serial.print("Etanol: "); Serial.print(etanol, 1); Serial.println(" ppm");
 // Serial.print("Izobutan: "); Serial.print(isobutan, 1); Serial.println(" ppm");
 if (isobutan >= 0) {
    Serial.print("Izobutan: "); Serial.print(isobutan, 1); Serial.println(" ppm");
  } else {
    Serial.println("Izobutan: 0 ppm ");
  }

  Serial.print("Hidrogen: "); Serial.print(h2, 1); Serial.println(" ppm");
  
  //Serial.print("Hidrogen sulfurat: "); Serial.print(h2s, 1); Serial.println(" ppm");
  if (h2s >= 0)
   {
    Serial.print("Hidrogen sulfurat: "); Serial.print(h2s, 1); Serial.println(" ppm");
  } else {
    Serial.println("Hidrogen sulfurat: 0 ppm ");
  }
 
 // Serial.print("Amoniac: "); Serial.print(nh3, 1); Serial.println(" ppm");
  if (nh3 >= 0) {
    Serial.print("Amoniac: "); Serial.print(nh3, 1); Serial.println(" ppm");
  } else {
    Serial.println("Amoniac: 0 ppm ");
  }

  //Serial.print("Oxid de azot: "); Serial.print(no, 1); Serial.println(" ppm");
  if (no >= 0) {
    Serial.print("Oxid de azot: "); Serial.print(no, 1); Serial.println(" ppm");
  } else {
    Serial.println("Oxid de azot: 0 ppm ");
  }

  Serial.print("Dioxid de azot: "); Serial.print(no2, 1); Serial.println(" ppm");

  if (Serial.available()) {
    String dateOzon = Serial.readStringUntil('\n');
    Serial.print("Concentrația de ozon: ");
    Serial.println(dateOzon);
    
    // Dacă valoarea depășește pragul
    float concentratieOzon = dateOzon.toFloat();
    if (concentratieOzon > 0.1) {
      Serial.println("Nivel ridicat de ozon detectat! Oprire relee.");
      oprireReleeSiAlarma();
    }
  }
  
  // Verificăm dacă valorile depășesc pragurile pentru MQ-05 (GPL)
  int mq05Value = analogRead(MQ05_PIN);
  Serial.print("GPL: "); Serial.print(mq05Value); Serial.println(" ppm");

  if (mq05Value > GPL_THRESHOLD_MQ05) {
    Serial.println("GPL detectat la MQ-05! Oprim releele și pornim alarma.");
    alarmaGaz();
  }
  // Citire pentru MQ-06 (Isobutan, Propan)
  int mq06Value = analogRead(MQ06_PIN);
  Serial.print("MQ-06 gaz detectat: "); Serial.print(mq06Value); Serial.println(" ppm");

  if (mq06Value > GAS_THRESHOLD_MQ06) {
    Serial.println("Gaz detectat de MQ-06! Oprim releele și pornim alarma.");
    oprireReleeSiAlarma();
    return;
  }
}

void sirenaS() {
    tone(buzzer, 1000); // Sunet fix la 1000Hz pentru mișcare
  delay(200);         // Pauză
  noTone(buzzer);
  delay(200);
}

void alarmaGaz() {
  oprireReleeSiAlarma();

  while (true) {
    tone(buzzer, 2500);
    delay(200);
    noTone(buzzer);
    delay(200);

    int mq05Value = analogRead(MQ05_PIN);
    if (mq05Value < GPL_THRESHOLD_MQ05) {
      Serial.println("Nivelul de gaz a revenit la normal.");
      break;
    }
  }

  Serial.println("Relee oprite din cauza detectării gazului.");
}

void oprireReleeSiAlarma() {
  digitalWrite(releu1, semnalHreleu);
  digitalWrite(releu2, semnalHreleu);
  digitalWrite(releu3, semnalHreleu);
  Serial.println("Relee oprite din cauza detectării mișcării sau gazului.");
}

bool miscareDetectata() {
  for (int i = 0; i < 4; i++) {
    if (pirValues[i] == semnalHreleu) {
      return true;
    }
  }
  return false;
}

// Funcția pentru pornirea releelor fără întârziere
void pornireRelee() {
  digitalWrite(releu1, semnalLreleu);
  delay(500);
  digitalWrite(releu2, semnalLreleu);
  delay(500);
  digitalWrite(releu3, semnalLreleu);
}

// Funcția pentru monitorizarea serialului pentru codul 115
void monitorizareSerial() {
  if (Serial.available() > 0) {
    int cod = Serial.parseInt();
    if (cod == 115) {
      Serial.println("Sistemul este repornit de operator.");
      sistemOprit = false;
      pornireRelee();
    }
  }
}
