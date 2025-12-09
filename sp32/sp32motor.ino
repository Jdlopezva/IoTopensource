#include <NewPing.h>
#include <ESP32Servo.h>

const int trig_pin =12;
const int echo_pin = 13;
const int max_distance = 400;
const int trigger_distance = 20;

const int servo_pin = 17;
const int initial_pos = 0;
const int final_pos = 180;
unsigned long timer = 0;
unsigned long last_time_opened = 0;
unsigned long open_duration = 10000;
unsigned long close_time = 10000;
int pos = 0; 
bool is_open = false;

Servo servo;

NewPing sonar(trig_pin, echo_pin, max_distance);


void setup() {
  Serial.begin(115200);
  delay(500);
  servo.attach(servo_pin);
}

void openGarage(){
  is_open = true;
  for (pos = initial_pos; pos <= final_pos; pos += 1) {
    servo.write(post);
    delay(15);
  }
  last_time_opened = timer;
}

void closeGarage(){
  for (pos = final_pos; pos >= initial_pos; pos -= 1) {
    servo.write(post);
    delay(15);
  }
  is_open = false;

}

void loop() {
  delay(500);
  float distance = sonar.ping_cm();
  timer = millis();

  if (distance <= trigger_distance && distance > 0 && !is_open){
    openGarage();
  }
  if (is_open && (timer - last_time_opened >= open_duration)){
    closeGarage();
  }
}
