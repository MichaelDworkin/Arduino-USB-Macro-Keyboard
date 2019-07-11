// Resistiven Touchscreen Panel als USB Macro Keypad 
// shortcuts programmierbare Keyboard
// Frei Programmierbare USB Makro Tastatur
// Arduino Leonardo Micro + Touchscreen Panel

// Befehlformat: "§", Befehl, Wert, Leerzeichen (Beispiel "Wort1§t1000 Wort2" Befehl warte 1 sekunde)
// §p - Press         "§p0x80 "  Strg drücken
// §r - Release       "§r0x80 "  Strg loslassen
// §a - Release All   "§a "      Alle Tasten Loslassen
// §w - Write         "§w0xB0 "  Enter klicken
// §t - Timer         "§t3000  " 3 Sekunden abwarten
// Scancodes Zuordnung bitte hier entnehmen
// https://github.com/arduino-libraries/Keyboard/blob/master/src/Keyboard.h

#include <stdint.h>
#include <EEPROM.h>
#include "TouchScreen.h"
#include "KeyboardDE.h"




// ------------------- Touchscreen Panel Anschluss Zuordnung ----------------
#define X1 A1
#define X2 A3
#define Y1 A0
#define Y2 A2

/*
  #define Y1 A1
  #define Y2 A3
  #define X1 A0
  #define X2 A2
*/

#define PIEZO_PIN 3  // Pin mit Pezowandler Piepser

int kalibriere = 0;
boolean losgelassen = 1;
int KalibrWert[4];

// ----------------- Hier bitte die Azahl der Spalten und Reien antragen -----------------

#define Reihen 3
#define Spalten 4

// ----------------- Befehle und Texte zugeordnet zum jeweiliger Zelle -------------------
// ----------------- Zweidimensionales Array entspricht der Tabelle ----------------------

const String data[Reihen][Spalten] =
{
  { "§p0x87 §t100 §r0x87 ", "3", "§w0xB1 §t500 mdworkin§w0xB0 " },
  { "Sehr geherte Damen und Herren,\n", "Michael Dworkin\nReichstr. 18\n42281 Wuppertal", "Mit freundlichen Grüßen\nMichael Dworkin", "Bitte Bestellen" },
  { "§p0x87 r§r0x87 §t100 shutdown -s -t 0 -f", "8", "9", "§w0xb0 Neue Zeile"},
};

/*
  const String data[Reihen][Spalten] =
  {
  { "1", "2", "3", "A" },
  { "4", "5", "6", "B" },
  { "7", "8", "9", "C"},
  };
*/


// For better pressure precision, we need to know the resistance
// between X+ and X- Use any multimeter to read it
// For the one we're using, its 300 ohms across the X plate

TouchScreen ts = TouchScreen(X1, Y1, X2, Y2, 300);


// ---------------- HEX String in Integer umwandeln  -----------------------

unsigned int hexToDec(String hexString)
{

  unsigned int decValue = 0;
  int nextInt;
  hexString.toUpperCase();
  for (int i = 0; i < hexString.length(); i++)
  {
    nextInt = int(hexString.charAt(i));
    if (nextInt >= 48 && nextInt <= 57) nextInt = map(nextInt, 48, 57, 0, 9);
    if (nextInt >= 65 && nextInt <= 70) nextInt = map(nextInt, 65, 70, 10, 15);
    if (nextInt >= 97 && nextInt <= 102) nextInt = map(nextInt, 97, 102, 10, 15);
    nextInt = constrain(nextInt, 0, 15);
    decValue = (decValue * 16) + nextInt;
  }
  return decValue;
}

// ---------------- Setup ------------------------------------------

void setup(void)
{
  Serial.begin(115200);
  pinMode(PIEZO_PIN, OUTPUT);

  Keyboard.begin();
  EEPROM.get( 0 , KalibrWert );
  if (KalibrWert[0] >= KalibrWert[1]  || KalibrWert[2] >= KalibrWert[3])
  {
    //    Serial.println("Falsche Kalibrierwerte lade Default");
    KalibrWert[0] = 0;
    KalibrWert[1] = 1000;
    KalibrWert[2] = 0;
    KalibrWert[3] = 1000;
  }
}

// ---------- Ausgabe des Textes über HID-Keybord und Ausführung der Befehle -----

void KeyOutput(const String str)
{
  int pos = str.indexOf('§');
  while (pos >= 0)
  {
    if (pos > 0) Keyboard.print(str.substring(0, pos - 1));
    str.remove(0, pos + 1);
    char kode = str.charAt(0);
    pos = str.indexOf(' ');
    String Daten = str.substring(1, pos);
    byte z;
    switch (kode)             // Befehle
    {
      case 'w':
        z = hexToDec(Daten);
        Keyboard.write(z);
        break;
      case 't':
        delay(Daten.toInt());
        break;
      case 'p':
        z = hexToDec(Daten);
        Keyboard.press(z);
        break;
      case 'r':
        z = hexToDec(Daten);
        Keyboard.release(z);
        break;
      case 'a':
        Keyboard.releaseAll();
        break;
    }
    str.remove(0, pos + 1);
    pos = str.indexOf('§');
  }
  if (str.length() > 0)Keyboard.print(str);
}

void loop(void) {
  TSPoint p = ts.getPoint(); // a point object holds x y and z coordinates

  // we have some minimum pressure we consider 'valid'
  if (p.z < ts.pressureThreshhold) losgelassen = 1;
  if (p.z > ts.pressureThreshhold && losgelassen)   // pressure of 0 means no pressing!
  {
    int x = map( p.x, KalibrWert[0], KalibrWert[1], 0, Spalten);
    int y = map( p.y, KalibrWert[2], KalibrWert[3], 0, Reihen);
    if (y < Reihen && x < Spalten)
    {
      // Serial.println(data[y][x]);
      digitalWrite(PIEZO_PIN, !digitalRead(PIEZO_PIN));
      if (!kalibriere) KeyOutput(data[y][x]);

    }
    else Serial.print("Reihen/Spalten position ausserhalb den Datenarray!");
    losgelassen = 0;
    delay(300);

    switch (kalibriere)             // Auswahlsteuerung
    {
      case 1:
        kalibriere = 2;
        KalibrWert[0] = p.x; // x min
        KalibrWert[2] = p.y; // y min
        delay(1000);
        Serial.println("Unten Rechts");
        break;
      case 2:
        kalibriere = 0;
        Serial.println("");
        KalibrWert[1] = p.x; // x max
        KalibrWert[3] = p.y; // y max
        delay(1000);
        Serial.println("Kalibrierung Abgeschlossen");
        EEPROM.put( 0, KalibrWert);
        Serial.println("X Min \tX Max \tY Min \tY Max");
        Serial.print(String(KalibrWert[0]) + "\t");
        Serial.print(String(KalibrWert[1]) + "\t");
        Serial.print(String(KalibrWert[2]) + "\t");
        Serial.println(String(KalibrWert[3]) + "\t");
        delay(1000);
        break;
      case 3:
        Serial.println("Messwerte:");
        Serial.print("X = "); Serial.print(p.x);
        Serial.print("\tY = "); Serial.print(p.y);
        Serial.println("  ");
        Serial.println("Kalibrierte Tabellenwerte:");
        Serial.print("X = ");
        Serial.print(x);
        Serial.print("\tY = ");
        Serial.println(y);
        Serial.println("  ");
        break;
    }

  }


  // 2 Befehle über Serielle Terminal
  // Zeichen "k"- Kalibrieren
  // Zeichen "r"- Absolute Positonswerte und Rasterwerte anzeigen

  if (Serial.available())     //Wenn Eingabe erfolgt
  {
    char  Wahl = Serial.read();     //Hole eingegebenes Zeichen
    Serial.flush();
    Serial.println(Wahl);     //Zeige eingegebenes Zeichen
    switch (Wahl)             //Auswahlsteuerung
    {
      case 'k':
        kalibriere = 1;
        Serial.println("Kalibriere");
        Serial.println("Druecke:");
        Serial.println("Oben Links");
        break;
      case 'r': // Rohdaten
        if (!kalibriere)
        {
          kalibriere = 3;
          Serial.println("Positonswerte und Rasterwerte");
          Serial.println("zurueck mir \"r\"");
        }
        else
        {
          kalibriere = 0;
          Serial.println("USB-Keyboard wieder An");
        }
        break;
    }
  }
}
