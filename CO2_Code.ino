/***************************************************************************
Code for the carbon dioxide monitoring device.
**************************************************************************/
#include <BlynkSimpleEsp8266.h>
#include <ESP8266WiFi.h>

/* Parameters for average calculation */
const int numReadings = 10;
int readings[numReadings]; // the readings from the analog input
int readIndex = 0; // the index of the current reading
float total = 0; // the running total
float average = 0; // the average

/* The load resistance on the board */
#define RLOAD 10.0

/* Parameters for calculating ppm of CO2 from sensor resistance */
#define PARA 116.6020682
#define PARB 2.769034857

/* Atmospheric CO2 level for calibration purposes */
#define ATMOCO2 397.13

/* Analog PIN */
uint8_t _pin = 0;
int pinValue;

/* Parameters for Calibration */
float RCurrent;
float RMax;

/* Parameters for indicator LED */
int PINblue = 12;
int PINgreen = 13;
int PINred = 15;
float LEDgreen;
float LEDred;
float LEDblue;

/* Parameter for notification status */
int notified;

/* BLYNK Connection Settings */
char auth[] = "VSS1YwptcHnSAnWbVpMwZibSZMsLaiLs";
char ssid[] = "HUAWEI Mate 20 X (5G)";
char pass[] = "1357997531";

/* Get the resistance of the sensor, ie. the measurement value. Return the sensor resistance in kOhm */
float getResistance() {
int val = average;
return ((1023./(float)val) - 1.)*RLOAD;
}

/* Get the ppm of CO2 sensed (assuming only CO2 in the air. Return The ppm of CO2 in the air */
float getPPM() {
return PARA * pow((getResistance()/RMax), -PARB);
}

/* Get the resistance RZero of the sensor for calibration purposes. Return the sensor resistance RZero in kOhm */
float getRZero() {
return getResistance() * pow((ATMOCO2/PARA), (1./PARB));
}

/* Restart calibration if button is pushed on Blynk */
BLYNK_WRITE(V2)
{
int pinValue = param.asInt();
if (pinValue == 1) RMax = 0;
Serial.print("V1 Slider value is: ");
Serial.println(pinValue);
}

void setup() {
/* Prepare array for average calculation */
  for (int thisReading = 0; thisReading < numReadings; thisReading++) { 
    readings[thisReading] = 0; 
  } 
  Blynk.begin(auth, ssid, pass); 
} 

void loop() { /* Ensure that Blynk reconnects in case of lost connection */ 
  if(!Blynk.connected()){ 
    Blynk.connect();
    } 
  Blynk.run(); /* Read sensor data and create average */ 
  total = total - readings[readIndex]; 
  readings[readIndex] = analogRead(_pin); 
  total = total + readings[readIndex]; 
  readIndex = readIndex + 1; 
  if (readIndex >= numReadings) {
  readIndex = 0;
  }
average = total / numReadings;

/* Sensor very volatile, smooth calibration, consider RCurrent with only 0,1% */
float RCurrent = getRZero();
if ((((999 * RMax) + RCurrent)/1000) > RMax) {
RMax = (((999 * RMax) + RCurrent)/1000);
}

/* Calculate PPM */
float ppm = getPPM();

/* Send data to Blynk */
Blynk.virtualWrite(V1, ppm);

/* Indicator LED */
LEDblue = 0;
LEDred = (1024*((ppm-500)/500))/3;
LEDgreen = 1024-((1024*((ppm-500)/500))/3);
if (LEDred > 1024) LEDred = 1024;
if (LEDred < 0) LEDred = 0; if (LEDgreen > 1024)LEDgreen = 1024;
if (LEDgreen < 0) LEDgreen = 0; 
/* Blynk notification if PPM exceeds 2000 */ 
  if (ppm > 2000) {
     if (notified == 0) Blynk.notify("Please open window");
     notified = 1;
  }
  else {
    notified = 0;
  }
  
/* Calibration LED indicator. Switch already from blue to green when within 95% range. */
if (ppm < (0.95*ATMOCO2)) { LEDgreen = 0; LEDred = 0; LEDblue = 1024; } /* After calibration, one measurement per second is enough */ if (ppm > ATMOCO2) delay(1000);

/* Send values to LEDs */
analogWrite(PINgreen,LEDgreen);
analogWrite(PINred,LEDred);
analogWrite(PINblue,LEDblue);
}
