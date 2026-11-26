// Solar-Powered Smart Microgrid & Load Balancer (Wokwi Firmware)
const int PIN_SOLAR = A0;
const int PIN_BATT_V = A1;
const int PIN_DEMAND_A = A2;
const int PIN_TEMP = A3;

const int PIN_FAN_PWM = 5;
const int PIN_CRITICAL_LOAD = 6;    // Green LED: Clinic / Essential Infrastructure
const int PIN_NONCRITICAL_LOAD = 7; // Yellow LED: Sheddable Residential Load
const int PIN_ALARM_LED = 8;        // Red LED: Thermal / Brownout Alarm

unsigned long lastTick = 0;

void setup() {
  Serial.begin(115200);
  pinMode(PIN_FAN_PWM, OUTPUT);
  pinMode(PIN_CRITICAL_LOAD, OUTPUT);
  pinMode(PIN_NONCRITICAL_LOAD, OUTPUT);
  pinMode(PIN_ALARM_LED, OUTPUT);

  Serial.println("time_ms,irradiance_wm2,batt_v,demand_a,temp_c,fan_pwm,crit_load,noncrit_load,grid_state");
}

void loop() {
  if (millis() - lastTick >= 500) {
    lastTick = millis();

    // Read & Convert Wokwi Sensors
    float irradiance = map(analogRead(PIN_SOLAR), 0, 1023, 1000, 0); // Inverted LDR module in Wokwi
    float battV = 10.5 + (analogRead(PIN_BATT_V) / 1023.0) * 3.9;
    float demandA = (analogRead(PIN_DEMAND_A) / 1023.0) * 12.0;
    
    // Wokwi NTC Thermistor Steinhart-Hart conversion to Celsius
    int analogTemp = analogRead(PIN_TEMP);
    float tempC = 1.0 / (log(1.0 / (1023.0 / analogTemp - 1.0)) / 3950.0 + 1.0 / 298.15) - 273.15;

    // Mathematical Power Balance (200W Solar Array + Battery Reserve)
    float solarPowerW = irradiance * 0.20; 
    float demandPowerW = battV * demandA;
    float netPowerW = solarPowerW - demandPowerW;

    int fanPWM = 0;
    int critLoad = 1;
    int nonCritLoad = 1;
    int gridState = 0; // 0: Balanced, 1: Load Shedding, 2: Thermal Cooling, 3: Critical Brownout

    if (tempC > 45.0) {
      fanPWM = constrain(map((int)tempC, 45, 70, 128, 255), 0, 255);
    }

    if (tempC >= 60.0 || battV < 11.0) {
      gridState = 3;
      critLoad = 1;
      nonCritLoad = 0;
      digitalWrite(PIN_ALARM_LED, HIGH);
    } else if (netPowerW < -30.0 && battV < 12.0) {
      gridState = 1;
      critLoad = 1;
      nonCritLoad = 0;
      digitalWrite(PIN_ALARM_LED, LOW);
    } else if (tempC >= 45.0) {
      gridState = 2;
      critLoad = 1;
      nonCritLoad = 1;
      digitalWrite(PIN_ALARM_LED, HIGH);
    } else {
      gridState = 0;
      critLoad = 1;
      nonCritLoad = 1;
      digitalWrite(PIN_ALARM_LED, LOW);
    }

    analogWrite(PIN_FAN_PWM, fanPWM);
    digitalWrite(PIN_CRITICAL_LOAD, critLoad);
    digitalWrite(PIN_NONCRITICAL_LOAD, nonCritLoad);

    // Output CSV Telemetry
    Serial.print(lastTick); Serial.print(",");
    Serial.print(irradiance, 1); Serial.print(",");
    Serial.print(battV, 2); Serial.print(",");
    Serial.print(demandA, 2); Serial.print(",");
    Serial.print(tempC, 1); Serial.print(",");
    Serial.print(fanPWM); Serial.print(",");
    Serial.print(critLoad); Serial.print(",");
    Serial.print(nonCritLoad); Serial.print(",");
    Serial.println(gridState);
  }
}
