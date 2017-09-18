

// Displayanschluesse:
#define CS_  15 // HW CS on GPIO15 / D8
#define DC_  16 // Data or Command pin on GPIO16 / D0
#define RST_ 0  // not connected, do not trigger reset on begin()

// Farb-Spektrum
int8_t MINTEMP = -5;
uint8_t MAXTEMP = 30;
uint16_t HUEMAX = 320;    // Schrittweite für Farbstrahl. Eigentlich 360 aber wir wollen die lila Farben etwas vermeiden, weil ähnlich wie rot

uint8_t ZOOM = 7;         // Vergroesserungfaktor

void StartScreen(void);
void OutAmbientTemp(void);
void HSVtoRGB(float &, float &, float &, float, float, float);
void OutTempField(void);
float LinInterpol (float, float, uint8_t, uint8_t, uint8_t);
float FindMinTemp (float *);
float FindMaxTemp (float *);

