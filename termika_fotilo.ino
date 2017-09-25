/*
 * Termika Fotilo
 * Make: IV/2017
 * 
 * Arduino Nano mit LCD 128x160 (ST7735) und MLX90621 als Waermebildkamera
 * 
 * ACHTUNG!
 * In den Libraries 
 * \arduino\hardware\arduino\avr\libraries\Wire\src\Wire.h und
 * \arduino\hardware\arduino\avr\libraries\Wire\src\utility\twi.h
 * muessen unbedingt die #defines fuer
 * BUFFER_LENGTH und
 * TWI_BUFFER_LENGTH
 * auf den Wert 64 geandert werden
 * 
 * Zwei Warning-Ausgaben beim compilieren sind normal, weil die SD-Karte auf dem Display nicht genutzt wird
 * 
 */
 
#include <ESP8266WiFi.h>
#include "Adafruit_ST7735.h"
#include "termika_fotilo.h"
#include "mlx90621.h"

Adafruit_ST7735 TFTscreen = Adafruit_ST7735(CS_, DC_, RST_);    // TFT Konstruktor
MLX90621 MLXtemp;                       // Objekt fuer Tempsensor erzeugen

void setup() {
//  Serial.begin(9600);
  StartScreen();
  while (!MLXtemp.init())        // MLX90620 init failed
    delay (100);

  Serial.println("start loop");
}

void loop()   // endlos
{
  OutAmbientTemp();
  OutTempField();
  delay(100);         // etwas weniger Stress
}

/**
  @brief  Zeichnet den Grundaufbau auf das Display
  @param  none
  @return none
*/
void StartScreen(void)
{
  uint8_t i;
  float R, G, B;
  float gradstep;
  char puffer[10];

  TFTscreen.initR(INITR_BLACKTAB);
  TFTscreen.setRotation(1);
  TFTscreen.background (0, 0, 0);     // falsche Implementierung? Angeblich RGB ist aber BGR bei allen Farbangaben

  TFTscreen.stroke (0, 0xFF, 0xFF);
  TFTscreen.fill (50, 100, 20);
  TFTscreen.rect (0, 0, 159, 20);          // Titelkasten

  TFTscreen.stroke (0,0,255);
  TFTscreen.setTextSize (2);
  TFTscreen.text ("Make:",100,3);     // 5x7 Font x2

  TFTscreen.stroke (0xFF , 0xFF, 0xFF);
  TFTscreen.setTextSize (1);
  TFTscreen.text ("TERMIKA FOTILO",3,7);

  TFTscreen.stroke (0x50, 0x50, 0xFF);
  TFTscreen.setTextSize (1);
  TFTscreen.text ("Init...", 0, 112);
  TFTscreen.text ("Please wait", 0, 120);


  TFTscreen.stroke (127, 127, 127);
  TFTscreen.fill (100, 0, 0);
  TFTscreen.rect (0, 27, 16+2, 4+2);                    // Original Daten
  TFTscreen.rect (0, 40, 16*ZOOM+2, 4*ZOOM+2);          // Daten xZOOM
  TFTscreen.rect (0, 75, 15*ZOOM+2, 3*ZOOM+2);      // Daten xZOOM interpoliert

  for (i=0; i < 103; i++)                 // 103 Farb-Schritte werden angezeigt
  {
    HSVtoRGB (R, G, B, i * (HUEMAX/103.0), 1, .5);        // 320/103  
    TFTscreen.stroke (B * 255, G * 255, R * 255);
    TFTscreen.line (150, i+25, 159, i+25);
  }

  TFTscreen.stroke (0xFF , 0xFF, 0xFF);
  TFTscreen.setTextSize (1);
  
  itoa (MINTEMP, puffer, 10);
  TFTscreen.text (puffer, 130, 120);
  
  itoa (MAXTEMP, puffer, 10);
  TFTscreen.text (puffer, 130, 25);        // => 150+20 = 170 Grad-Schritte umfasst der Farbbalken => bei 102 Farb-Schritte = 170/103 = 1,65 °C/Farb-Schritt

  // Zero degree line
  gradstep = (MAXTEMP + abs(MINTEMP)) / 103.0;      // Absoultwert von Min
  i = 25 + (uint8_t)(MAXTEMP / gradstep);
  TFTscreen.line (145, i, 150, i);
}

/**
  @brief  Schreibt die Umgebungstemperatur des Chips aufs Display
  @param  none
  @return none
*/
void OutAmbientTemp(void)
{
  float t = MLXtemp.get_ptat();
  char puffer[10];

 // Serial.println (t);

  TFTscreen.stroke (0, 0, 0);
  TFTscreen.fill (0, 0, 0);
  TFTscreen.rect (0, 112, 70, 128-112);          // Alte Zahl loeschen

  TFTscreen.stroke (0, 0xFF, 0xFF);
  TFTscreen.setTextSize (2);

  dtostrf (t, 6, 1, puffer);
  TFTscreen.text (puffer, 0, 112);     // 5x7 Font x2
  TFTscreen.setTextSize (1);
  TFTscreen.text ("\xF7", 74, 112);     // "°C" ASCII 0xF7 fuer Gradzeichen
  TFTscreen.text ("C", 80, 112);    
}

/**
  @brief  Konvertiert einen HDV-Farbwert in RGB um (http://wisotop.de/rgb-nach-hsv.php)
  @param  RGB-Werte als Call by reference (0..1)
          H = 0..360
          S = 0..1
          V = 0..1
  @return none
*/
void HSVtoRGB(float &r, float &g, float &b, float h, float s, float v) 
{
   int i;
   float f, p, q, t;
   if( s == 0 ) { // achromatisch (Grau)
      r = g = b = v;
      return;
   }
   h /= 60;           // sector 0 to 5
   i = floor( h );
   f = h - i;         // factorial part of h
   p = v * ( 1 - s );
   q = v * ( 1 - s * f );
   t = v * ( 1 - s * ( 1 - f ) );
   switch( i ) {
      case 0: r = v; g = t; b = p; break;
      case 1: r = q; g = v; b = p; break;
      case 2: r = p; g = v; b = t; break;
      case 3: r = p; g = q; b = v; break;
      case 4: r = t; g = p; b = v; break;
      default:  // case 5:
         r = v; g = p; b = q; break;
   }
}

/**
  @brief  Grafische Ausgabe
  @param  none
  @return none
*/
void OutTempField(void)
{
  int8_t x, y, xmod, xorg1, xorg2, ymod, yorg1, yorg2; //, xz, yz;
  char puffer[10];
  float temps[16][4];
  float i, i1, i2;
  float hue;
  float R, G, B;
  float interpoltemp;
  
  MLXtemp.read_all_irfield (temps);

  // Reihe umkehren, da Sensor nach vorne und Display nach hinten => Temperatur-Feld so wie, auf Display zu zeigen
  for (y = 0; y < 4; y++)
  {
    for (x = 0; x < 8; x++)       
    {
      i = temps[15 - x][y];                       // Wert von rechts retten
      temps[15 - x][y] = temps[x][y];
      temps[x][y] = i;
    }
  }

  // Ausgabe des Temperaturfeldes auf Display
  for (y = 0; y < 4; y++)
  {
    for (x = 0; x < 16; x++)       
    {
      hue = HUEMAX - (temps[x][y] + abs(MINTEMP)) * (HUEMAX / (float)(MAXTEMP + abs(MINTEMP)));
      HSVtoRGB (R, G, B, hue, 1, .5);        
      TFTscreen.stroke (B * 255, G * 255, R * 255);
      TFTscreen.fill (B * 255, G * 255, R * 255);

      TFTscreen.point (x + 1, y + 28);                                // 1:1 Datenfeld
      TFTscreen.rect (x * ZOOM + 1, y * ZOOM + 41, ZOOM, ZOOM);       // Daten xZOOM

//      dtostrf (temps[x][y], 6, 1, puffer);    // Ausgabe Zahlenwerte
//      Serial.print (puffer);
    }
//    Serial.println();
  }
//    Serial.println("***");

  // Ausgabe interpolierte Werte (Problemloesung durch viel Rechnezeit, da wenig Speicher)
  for(x = 0; x < (ZOOM * 15); x++)
  {
    for(y = 0; y < (ZOOM * 3); y++)
    {
      xmod = x % ZOOM;
      xorg1 = x - xmod;
      xorg2 = xorg1 + ZOOM;
      ymod = y % ZOOM;
      yorg1 = y - ymod;
      yorg2 = yorg1 + ZOOM;
      i1 = LinInterpol (temps[x / ZOOM][y / ZOOM], temps[x / ZOOM + 1][y / ZOOM], xorg1 , xorg2, x);                  // horizontale Interpolation Ausgangsdaten Zeile n
      i2 = LinInterpol (temps[x / ZOOM][y / ZOOM + 1], temps[x / ZOOM + 1][y / ZOOM + 1], xorg1 , xorg2, x);          // horizontale Interpolation Ausgangsdaten Zeile n+1
      interpoltemp = LinInterpol (i1, i2, yorg1, yorg2, y);                                                           // vertikale Interpolation

      hue = HUEMAX - (interpoltemp + abs(MINTEMP)) * (HUEMAX / (float)(MAXTEMP + abs(MINTEMP)));
      HSVtoRGB (R, G, B, hue, 1, .5);        
      TFTscreen.stroke (B * 255, G * 255, R * 255);
      TFTscreen.fill (B * 255, G * 255, R * 255);
      TFTscreen.point (x + 1, y + 75 + 1);                    // interpol. Pixel
    }
  }
}

/**
  @brief  Fuehrt lineare Interpolation durch
  @param  x1 <= x <= x2
          t1, t2 Temperatur
  @return interpolierte Temperatur fuer x
*/
float LinInterpol (float t1, float t2, uint8_t x1, uint8_t x2, uint8_t x)
{
  return t1 * (x2 - x) / ZOOM + t2 * (x - x1) / ZOOM;
}

/**
  @brief  Ermittler kleinste Temperatur
  @param  Temp-Feld
  @return Minimum
*/
float FindMinTemp (float temperatures[])
{
  uint8_t i;
  float temp = 300.0;     // maximal possible Temp

  for (i = 0; i < 64; i++)
  {
    if (temperatures[i] < temp)
      temp = temperatures[i];
  }

  return temp;
}

/**
  @brief  Ermittler hoechste Temperatur
  @param  Temp-Feld
  @return Maximoum
*/
float FindMaxTemp (float temperatures[])
{
  uint8_t i;
  float temp = -20.0;     // min. possible Temp

  for (i = 0; i < 64; i++)
  {
    if (temperatures[i] > temp)
      temp = temperatures[i];
  }

  return temp;
}

