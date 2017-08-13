/*
 * Library für MLX90621 IT-Sensor
 * 
 * Andere Typen sind aehnlich aber nutzen andere Berechnung
 * 
 * ACHTUNG!
 * In den Libraries 
 * \arduino\hardware\arduino\avr\libraries\Wire\src\Wire.h und
 * \arduino\hardware\arduino\avr\libraries\Wire\src\utility\twi.h
 * muessen unbedingt die #defines fuer
 * BUFFER_LENGTH und
 * TWI_BUFFER_LENGTH
 * auf den Wert 64 geandert werden. Ansonsten ist der Buffer zum empfangen von I2C-Daten zu klein
 */

#include "mlx90621.h"

/**
  @brief  Konstruktor
  @param  none
  @return none
*/
MLX90621::MLX90621(void)
{
  Wire.begin();     // join i2c bus 
}

/**
  @brief  Initialisiert den Sensor. Einmal zu Beginn
  @param  none
  @return 0 = Fehler, 1 = OK
*/
uint8_t MLX90621::init (void)
{
	delay(5);   // erst ab 5ms nach power on reset POR
 
  if (!read_eeprom()) 
    return 0;
  write_trim (eepromMLX[0xF7]);                       // Trimwert im EEPROM-Inhalt Adresse 0xF7
  write_config (eepromMLX[0xF5], eepromMLX[0xF6]);    // Normal mode (no sleep), I2C FM+ mode enabled, ADC low reference enabled 

  configreg = read_config();

  ta = get_ptat();      
  if (ta > 300 || ta < -20)      // out of bounds, wichtig zu pruefen
    return 0;

  vcp = read_compensation();                                  // Compensation pixel 

  acp = (int16_t)( eepromMLX[0xD4] << 8 ) | eepromMLX[0xD3];           // Compensation pixel individual offset coefficient
  bcpee = (int8_t)eepromMLX[0xD5];                                      // Individual Ta dependence (slope) of the compensation pixel offset
  tgc = (int8_t)eepromMLX[0xD8];                                      // Thermal gradient coefficient
  aiscale = eepromMLX[0xD9] >> 4;                             // [7:4] – Scaling coeff for the IR pixels offset
  biscale = eepromMLX[0xD9] & 0x0F;                           // [3:0] – Scaling coeff of the IR pixels offset Ta dependence
  alpha0 = ( eepromMLX[0xE1] << 8 ) | eepromMLX[0xE0];        // Common sensitivity coefficient of IR pixels
  alpha0scale = eepromMLX[0xE2];                              // Scaling coefficient for common sensitivity
  deltaalphascale = eepromMLX[0xE3];                          // Scaling coefficient for individual sensitivity
  alphacp = (( eepromMLX[0xD7] << 8 ) | eepromMLX[0xD6]) / (float) (pow (2, alpha0scale) * pow (2, 3 - ( (configreg >> 4) & 0x03)));       // Sensitivity coefficient of the compensation pixel
  epsilon = ( eepromMLX[0xE5] << 8 ) | eepromMLX[0xE4] ;       // Emissivity
  acommon = (int16_t)(( eepromMLX[0xD1] << 8 ) | eepromMLX[0xD0]);    // IR pixel common offset coefficient
  ksta = (int16_t)(( eepromMLX[0xD7] << 8 ) | eepromMLX[0xD6]);   // KsTa (fixed scale coefficient = 20)
  ks4ee = (int8_t)eepromMLX[0xC4];
  ksscale = eepromMLX[0xC0];
  
  return 1;
}

/**
  @brief  Liest das IR-Feld einmal aus und berechnet die Temperaturen fuer jeden einzelnen IR-Sensor => temperatures[]
  @param  none
  @return none
*/
void MLX90621::read_all_irfield (float temperatures[16][4])
{
  uint8_t x, y, i;
  int16_t vir;
  uint8_t deltaai;               // IR pixel individual offset coefficient
  int8_t bieeprom;
  float bi;               // Individual Ta dependence (slope) of IR pixels offset
  float bcp;
  uint8_t deltaalpha;      // Individual sensitivity coefficient
  float viroffsetcompensated;     //
  float virtgccompensated;
  float vircpoffsetcompensated;
  float vircompensated;
  float ai;
  float alphacomp;
  float alpha;
  float ks4;
  uint64_t tak4;
  float sx;
  float to;
  
  while (test_por ())
  {
    delay (10);
//    Serial.print ("Neustart");
    init ();
  }

   if (!read_ir ())
     return;
    
  for (i = 0; i < 64; i++)
  {
    vir = (int16_t)(irpixels[i * 2 + 1] << 8 | irpixels[i * 2]);      
    deltaai = eepromMLX[i];
    bi = (int8_t)eepromMLX[0x40 + i];   
    deltaalpha = eepromMLX[0x80 + i]; 

    // 7.3.3.1 
    // Offset compensation
    ai = (acommon + deltaai * pow (2, aiscale)) / pow (2, ( (configreg >> 4) & 0x03));      // Bit 5:4 Config Register
    bi = bi / (pow (2, biscale) * pow (2, 3 - ( (configreg >> 4) & 0x03)));
    viroffsetcompensated = vir - (ai + bi * (ta - 25.0));

    // Thermal Gradient Compensation (TGC)
    bcp = bcpee / (pow (2, biscale) * pow (2, 3 - ( (configreg >> 4) & 0x03)));
    vircpoffsetcompensated = vcp - (acp + bcp * (ta - 25.0) );
    virtgccompensated = viroffsetcompensated - ( (tgc / 32.0) *  vircpoffsetcompensated);

    // Emissivity compensation
    vircompensated = virtgccompensated / (epsilon / 32768);
    
    // 7.3.3.2
    alpha = ( ( alpha0 / pow (2, alpha0scale) ) + ( deltaalpha / pow (2, deltaalphascale) ) ) / (float) pow (2, 3 - ( (configreg >> 4) & 0x03)) ;
    alphacomp = (1 + (ksta/pow (2, 20)) * (ta - 25.0)) * (alpha - tgc * alphacp);

    // 7.3.3.1
    ks4 = ks4ee / pow (2, ksscale + 8);
  
    // 7.3.3
    tak4 = pow ( (ta + 273.15), 4);
    sx = ks4 * pow ( ( ( pow (alphacomp, 3) * vircompensated ) + ( pow  ( alphacomp, 4 ) * tak4 ) ), (1 / 4.0) );              // x. Wurzel aus y = y^(1/x)
    to = pow ( (vircompensated / ( alphacomp * (1 - ks4 * 273.15) + sx ) ) + tak4, (1 / 4.0) ) - 273.15;

    temperatures[i%16][(uint8_t)i/16] = to;      // yeah, wir haben die reale Temperatur
  }
}

/**
  @brief  Liest 64 Bytes aus dem EEPROM und speichert diese in eepromMLX
  @param  Startadresse
  @return none
*/
uint8_t MLX90621::read_eeprom_64 (uint8_t start)
{
  uint16_t i;

  Wire.beginTransmission(eeprom_dump_address);      // Adresse des Chips: Whole EEPROM dump
  Wire.write (start);                                // Command: Read the whole EEPROM (start address)
  Wire.endTransmission(0);
  
  if (!Wire.requestFrom(eeprom_dump_address, 64) ) 
    return 0;      // receive 64 Bytes. Wire.h kann eigentlich nur 32
    
  for (i = start; i < (start + 64); i++)
    eepromMLX[i] = Wire.read();

  return 1;
}

/**
  @brief  Liest 256 Bytes aus dem EEPROM und speichert die Daten in eepromMLX
  @param  none
  @return none
*/
uint8_t MLX90621::read_eeprom (void)
{
  uint16_t i;

  if (!read_eeprom_64 (0x00))
    return 0;
  if (!read_eeprom_64 (0x40))
    return 0;
  if (!read_eeprom_64 (0x80))
    return 0;
  if (!read_eeprom_64 (0xC0))
    return 0;

  return 1;
}

/**
  @brief  Speichert den Oszillator-Trim-Wet aus dem EEPROM (HSB immer 0)
  @param  Trimwert
  @return none
*/
void MLX90621::write_trim (uint8_t trimvalue)
{
  Wire.beginTransmission(chip_address);       // Adresse des Chips zum speichern des Trimwertes
  Wire.write (0x04);                          // Command: write trim value
  Wire.write (trimvalue - 0xAA);              // LSByte check
  Wire.write (trimvalue);                     // LSB
  Wire.write (0x00 - 0xAA);                   // HSByte check
  Wire.write (0x00);                          // HSB/MSB
  Wire.endTransmission();
}

/**
  @brief  Speichert die zwei Konfigurations-Bits aus dem EEPROM
  @param  LSB und HSB Konfibytes
  @return none
*/
void MLX90621::write_config (uint8_t lsb, uint8_t hsb)
{
  Wire.beginTransmission(chip_address);       // Adresse des Chips zum speichern der Konfigwerte
  Wire.write (0x03);                          // Command: write config values
  Wire.write (lsb - 0x55);                    // LSByte check
  Wire.write (lsb);                           // LSB
  Wire.write (hsb - 0x55);                    // HSByte check
  Wire.write (hsb);                           // HSB/MSB
  Wire.endTransmission();
}

/**
  @brief  Liest das config-Register ein
  @param  none
  @return Config Register oder -1 wenn Fehler
*/
int32_t MLX90621::read_config (void)
{
  uint16_t configreg;
  
  Wire.beginTransmission(chip_address);       // Adresse des Chips
  Wire.write (0x02);                          // command
  Wire.write (0x92);                          // start address
  Wire.write (0x00);                          // Address step
  Wire.write (0x01);                          // Number of reads
  Wire.endTransmission(0);
 
  if (!Wire.requestFrom(chip_address, 2))      // receive 2 Bytes
    return -1;
  configreg = Wire.read();                  // LSB
  configreg |= (Wire.read() << 8);          // OR HSB

  return configreg;
}

/**
  @brief  Prueft, ob POR/Brown-out Flag gesetzt
  @param  none
  @return 0 = wahr => init notwendig; 1 = OK
*/
uint8_t MLX90621::test_por (void)
{
  return (read_config () & 0x400);    // POR = 11. Bit 
}


/**
  @brief  Liest die Absolute ambient temperature data of the device itself (package temperature)
  @param  none
  @return PTAT Wert (HSB/LSB) oder -1, wenn Fehler
*/
int32_t MLX90621::read_ptat (void)
{
  int32_t ptat = 0;
  
  Wire.beginTransmission(chip_address);       // Adresse des Chips
  Wire.write (0x02);                          // command
  Wire.write (0x40);                          // start address
  Wire.write (0x00);                          // Address step
  Wire.write (0x01);                          // Number of reads
  Wire.endTransmission(0);
 
  if (!Wire.requestFrom(chip_address, 2))      // receive 2 Bytes
    return -1;
  ptat = Wire.read();                 // LSB
  ptat |= Wire.read() << 8;          // OR HSB

  return ptat;
}

/**
  @brief  Liefert die Absolute ambient temperature data of the device itself (package temperature)
  @param  none
  @return Temperatur in °C
*/
float MLX90621::get_ptat (void)
{
  uint8_t exp1, exp2;
  float kt1, kt2, vth, tmp;
  int32_t ptat;
  
  
  do {
    ptat = read_ptat();
    if (ptat == -1)       // wenn Fehler, dann neu init
    {
      delay (1000);
      init();
    }
  } while (ptat == -1);
  
  exp2 = ( (configreg >> 4) & 0x03);      // Bit 5:4 Config Register

  vth = ( eepromMLX[0xDB] << 8 ) | eepromMLX[0xDA];
  if (vth > 32767)
    vth -= 65536;
  vth = vth / pow (2, 3-exp2);

  kt1 = (( eepromMLX[0xDD] << 8 ) | eepromMLX[0xDC]);
  if (kt1 > 32767)
    kt1 -= 65536;

  exp1 = ( eepromMLX[0xD2] >> 4);               // Bit 7:4
  kt1 = kt1 / ( pow (2, exp1) * pow (2, 3-exp2) );
  
  kt2 = (( eepromMLX[0xDF] << 8 ) | eepromMLX[0xDE]);
  if (kt2 > 32767)
    kt2 -= 65536;
  exp1 = (eepromMLX[0xD2] & 0x0F);             // Bit 3:0
  kt2 = kt2 / ( pow (2, exp1+10) * pow (2, 3-exp2) );
  
  return (((kt1 * -1.0) + sqrt( kt1*kt1 - (4 * kt2) * (vth - ptat) )) / (2 * kt2) ) + 25.0;
}

/**
  @brief  Liest das compensation pixel
  @param  none
  @return CP Wert (HSB/LSB) oder -1, wenn Fehler
*/
int16_t MLX90621::read_compensation (void)
{
  int8_t cp = 0;
  
  Wire.beginTransmission(chip_address);       // Adresse des Chips
  Wire.write (0x02);                          // command
  Wire.write (0x41);                          // start address
  Wire.write (0x00);                          // Address step
  Wire.write (0x01);                          // Number of reads
  Wire.endTransmission(0);
 
  if (!Wire.requestFrom(chip_address, 2))      // receive 2 Bytes
    return -1;
  cp = Wire.read();                 // LSB

  return ( (int16_t) Wire.read() << 8 | cp);
}

/**
  @brief  Read whole IR frame and save it
  @param  none
  @return 0 wenn Fehler,1 = OK
*/
uint8_t MLX90621::read_ir (void)
{
  uint8_t i;
  
  Wire.beginTransmission(chip_address);       // Adresse des Chips
  Wire.write (0x02);                          // command: read 1 line
  Wire.write (0x00);                          // start address
  Wire.write (0x04);                          // Address step
  Wire.write (0x10);                          // Number of reads    // nur erste 32 Bytes statt 128
  Wire.endTransmission(0);
 
  if (!Wire.requestFrom(chip_address, 32))      // receive 32 Bytes
    return 0;

  for (i=0; i < 32; i++)
    irpixels[i] = Wire.read(); 

  // Line 2
  Wire.beginTransmission(chip_address);       // Adresse des Chips
  Wire.write (0x02);                          // command: read 1 line
  Wire.write (0x01);                          // start address
  Wire.write (0x04);                          // Address step
  Wire.write (0x10);                          // Number of reads    // nur erste 32 Bytes statt 128
  Wire.endTransmission(0);
 
  if (!Wire.requestFrom(chip_address, 32))      // receive 32 Bytes
    return 0;

  for (i=0; i < 32; i++)
    irpixels[i+32] = Wire.read(); 

  // Line 3
  Wire.beginTransmission(chip_address);       // Adresse des Chips
  Wire.write (0x02);                          // command: read 1 line
  Wire.write (0x02);                          // start address
  Wire.write (0x04);                          // Address step
  Wire.write (0x10);                          // Number of reads    // nur erste 32 Bytes statt 128
  Wire.endTransmission(0);
 
  if (!Wire.requestFrom(chip_address, 32))      // receive 32 Bytes
    return 0;

  for (i=0; i < 32; i++)
    irpixels[i+64] = Wire.read(); 

  // Line 4
  Wire.beginTransmission(chip_address);       // Adresse des Chips
  Wire.write (0x02);                          // command: read 1 line
  Wire.write (0x03);                          // start address
  Wire.write (0x04);                          // Address step
  Wire.write (0x10);                          // Number of reads    // nur erste 32 Bytes statt 128
  Wire.endTransmission(0);
 
  if (!Wire.requestFrom(chip_address, 32))      // receive 32 Bytes
    return 0;

  for (i=0; i < 32; i++)
    irpixels[i+96] = Wire.read(); 


  
  
  return 1;
}




















