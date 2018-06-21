//Demo code for calculate the pressure relative to local atmospheric pressure.
// This demo code is used to test 0.5MPa range pressure sensor but not others.
//Store: http://www.aliexpress.com/store/1199788
//      http://dx.com

// Calculation formula for 0.5MPa range sensor:
// VOUT = VCC*(1.6*P + 0.1)
// VOUT is the voltage from the yellow wire of the sensor
// VCC is the supply voltage for red wire of the sensor
// P is the pressure you want to detect
// So P = (VOUT-0.1*VCC)/(1.6*VCC) and VOUT = analogRead(SENSOR) *VCC / 1024.0
// So P =  ( analogRead(SENSOR) / 1024 -0.1 ) / 1.6   and the unit is MPa

/*macro definition of sensor*/
#define SENSOR A0//the YELLOW pin of the Sensor is connected with A0 of Arduino/OPEN-SMART UNO R3

void setup()
{
    Serial.begin(9600);
}
void loop()
{
  int raw = analogRead(SENSOR);
  
  Serial.println("Pressure is");
  float pressure_MPa = (raw / 1024 - 0.1) / 1.6;  // voltage to pressure
  Serial.print(pressure_MPa);
  Serial.println(" MPa");
  
  float pressure_kPa = pressure_MPa * 1000;
  Serial.print(pressure_kPa);
  Serial.println(" kPa");
  
  float pressure_psi = pressure_kPa * 0.14503773773020923;    // kPa to psi
  Serial.print(pressure_psi);
  Serial.println(" psi");
  delay(500);
}

