/** pressureReg.ino
 *  Proof-of-concept test program for OSE cutting torch pressure control
 *  Dynamically adjusts pressure between commanded setpoints
 *  http://opensourceecology.org/wiki/CNC_Torch_Table_Control_Overview#Oxyfuel_.28oxyacetylene.2C_oxypropane.2C_etc.29_cutting_torch
 *
 *  Arduino UNO
 *  Freescale MPXHZ6400AC6T1 pressure sensor on analog in A0
 *  Automation Direct AVS-523C1-24D 5-port 3-pos closed center air solenoid valve 
 *    on digital pins 2 & 3
 *  ULN2003 darlington buffer between digital pins and 24V solenoid coils
 *
 *  Control via Arduino serial port 9600 8N1 through 5 settable parameters
 *    0 - target pressure - e.g.  0p600s (parameter 0 setting 600)
 *         setting is in ADC counts, 600 ~ 20psig
 *    1 - slew rate - e.g. 1p2s
 *         setting is ADC counts per update cycle (see parameter 4, cycle_time)
 *    2 - hysteresis - e.g. 2p20s
 *         setting is ADC counts deadband, i.e. error required to initiate a correction
 *    3 - report rate - e.g. 3p25s
 *         setting is how often (in update cycles) to report <command> <measurement> over serial
 *    4 - cycle time - e.g. 4p20s
 *         setting is milliseconds per update cycle
 *
 * ver 0.1 21Jul2013 C.Harrison
 * Public domain
 */
 
const int sensorPin = A0;    // select the input pin for the pressure sensor
const int ledPin = 13;
const int upPin = 2;
const int dnPin = 3;

int sensorValue;  // variable to store the value coming from the sensor
uint32_t when;
int reportCounter=0;

// pressure control parameters
enum {
  target,
  slewrate,
  hysteresis,
  report_rate,
  cycle_time,
  num_params
};
int parameters[num_params];
int command;

int adjusting_param=0;
int input_number = 0;

void setup() {
  // declare the ledPin as an OUTPUT:
  pinMode(ledPin, OUTPUT);
  digitalWrite(upPin, LOW);
  digitalWrite(dnPin, LOW);
  pinMode(upPin, OUTPUT);
  pinMode(dnPin, OUTPUT);
  when = millis();
  
  parameters[target] = 300;
  parameters[slewrate] = 2;
  parameters[hysteresis] = 15;
  parameters[report_rate] = 20;
  parameters[cycle_time] = 20;
  sensorValue = analogRead(sensorPin);
  command = sensorValue;
  
  Serial.begin(9600);
}

void loop() {
  // handle any user command
  if (Serial.available()) {
    char c = Serial.read();
    Serial.write(c); //echo
    if (c >= '0' && c <= '9') {
      input_number = 10*input_number + (c-'0');
    }
    else if (c=='p') {
      if (input_number < num_params) {
        adjusting_param = input_number;
      }
      input_number = 0;
    }
    else if (c == 's') {
      parameters[adjusting_param] = input_number;
      input_number = 0;
    }
    else {
      input_number = 0;
    }
  } 
  // read the value from the sensor:
  sensorValue = analogRead(sensorPin);
  if (sensorValue > command+parameters[hysteresis]) {
    if (digitalRead(upPin)==LOW) {
      digitalWrite(dnPin, HIGH);
    }
    digitalWrite(upPin, LOW);
  } else if (sensorValue < command-parameters[hysteresis]) {
    if (digitalRead(dnPin)==LOW) {
      digitalWrite(upPin, HIGH);
    }
    digitalWrite(dnPin, LOW);
  } else {
    digitalWrite(upPin, LOW);
    digitalWrite(dnPin, LOW);
  }
  if (parameters[target] > command) {
    command = command + parameters[slewrate];
    if (command > parameters[target]) {
      command = parameters[target];
    }
  }
  if (parameters[target] < command) {
    command = command - parameters[slewrate];
    if (command < parameters[target]) {
      command = parameters[target];
    }
  }
  if (++reportCounter >= parameters[report_rate]) {
    Serial.print(command, DEC);
    Serial.write(' ');
    Serial.println(sensorValue, DEC);
    reportCounter = 0;
  }
  while (millis() < when) { ; }
  when += parameters[cycle_time];
}
