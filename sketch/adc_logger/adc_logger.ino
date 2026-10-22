// LM35 reading logger for the Proteus simulation
// Prints the raw ADC value and the converted temperature once per second.
// LM35 output: 10 mV per degree C, Arduino ADC: 10 bits over 0-5 V.

const int LM35_PIN = A1;   // same pin as the smart home project

void setup() {
  Serial.begin(9600);
  Serial.println("adc_reading,temp_c");   // CSV header
}

void loop() {
  int adc = analogRead(LM35_PIN);
  float temp_c = (adc * 5.0 * 100.0) / 1024.0;

  Serial.print(adc);
  Serial.print(",");
  Serial.println(temp_c, 2);

  delay(1000);
}
