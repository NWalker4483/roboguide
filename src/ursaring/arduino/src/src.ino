//////////////////////
/* 
  _      _      _
>(.)__ <(.)__ =(.)__
 (___/  (___/  (___/ */
/////////////////////////////
#define RightEncoderPinA     2 // Digital Interrupt Pin
#define RightEncoderPinB     4 // Digital Pin
#define LeftEncoderPinA      3 // Digital Interrupt Pin
#define LeftEncoderPinB      5// Digital Pin
/////////////////////////////
#define UsingMixedMode       false
#define RightTreadControlPin A0 // Also acts as X when in mixed mode and throttle when
#define LeftTreadControlPin  0
/////////////////////////////
#define CRAWL_SPEED          20  // %
#define MAX_SPEED            30 // %
/////////////////////////////
// Physical Properties of the robot
#define WHEEL_BASE           10 //inches // I forgot Y I did this in inches
#define ENCODER_RESOLUTION   2.5 // right_pulses per revolution
#define WHEEL_DIAMETER       12 // cm
#define PI                   3.1415926535897932384626433832795
/////////////////////////////
#define LoopTime             100
/////////////////////////////a
#include <ros.h>
#include <ros/time.h>

#include <std_msgs/Int16.h>
#include <geometry_msgs/Twist.h>
#include <sensor_msgs/Joy.h>

#include <TimerOne.h>
#include <mine.cpp>

//Globals
unsigned long lastMilli = 0; // time at the end of the last loop 
byte mode = 'R'; // ROS Mode
byte ledstatus = LOW;

int right_rpm= 0;
volatile unsigned long right_pulses = 0; // rev counter
unsigned long old_right_pulses = 0;

int left_rpm= 0;
volatile unsigned long left_pulses = 0; // rev counter
unsigned long old_left_pulses = 0;

ros::NodeHandle nh;
ros::Time current_time;

std_msgs::Int16 left_ticks_msg;
std_msgs::Int16 right_ticks_msg;

ros::Publisher pub_left_ticks("/left_ticks",&left_ticks_msg);
ros::Publisher pub_right_ticks("/right_ticks",&right_ticks_msg);

void DriverCallback(const geometry_msgs::Twist&);
ros::Subscriber<geometry_msgs::Twist> drive("cmd_vel", &DriverCallback); 

void getMotorData(unsigned long time) {
  double delta_time = double(time)/1000; // must be in seconds for these formulas 
  
  right_rpm = (60*(double(right_pulses-old_right_pulses)/ENCODER_RESOLUTION))/delta_time; // 60 is to convert from seconds to minutes
  right_ticks_msg.data += right_pulses - old_right_pulses;
  old_right_pulses = right_pulses;
  
  left_rpm = (60*(double(left_pulses-old_left_pulses)/ENCODER_RESOLUTION))/delta_time; // 60 is to convert from seconds to minutes
  left_ticks_msg.data += left_pulses - old_left_pulses;
  old_left_pulses = left_pulses;
}
void RightEncoderEvent() { // Counts right_pulses on the  Encoder
  if (digitalRead(RightEncoderPinA) == HIGH) {
    if (digitalRead(RightEncoderPinB) == LOW) {
      right_pulses++;
    } else {
      right_pulses--;
    }
  } else {
    if (digitalRead(RightEncoderPinB) == LOW) {
      right_pulses--;
    } else {
      right_pulses++;
    }
  }
}
void LeftEncoderEvent() { // Counts right_pulses on the  Encoder
 if (digitalRead(LeftEncoderPinA) == HIGH) {
    if (digitalRead(LeftEncoderPinB) == LOW) {
      left_pulses++;
    } else {
      left_pulses--;
    }
  } else {
    if (digitalRead(LeftEncoderPinB) == LOW) {
      left_pulses--;
    } else {
      left_pulses++;
    }
  }
}
void DriverCallback(const geometry_msgs::Twist& cmd_msg) {
  // Lin -.5:.5  Ang -1.5:1.5
  if (UsingMixedMode){
    analogWrite(RightTreadControlPin,mapf(cmd_msg.linear.x,-.5,.5,0,1023)); 
    analogWrite(LeftTreadControlPin,mapf(cmd_msg.angular.z,-.5,.5,0,1023)); 
  } else {
    Timer1.pwm(RightTreadControlPin, (dutyCycle / 100) * 1023);
    Timer1.pwm(LeftTreadControlPin, (dutyCycle / 100) * 1023);
  }
}

void PublishTICKS(unsigned long time) {
  pub_left_ticks.publish(&left_ticks_msg);
  pub_right_ticks.publish(&right_ticks_msg);
  
  left_ticks_msg.data = 0;
  right_ticks_msg.data = 0;
}

void setup() {
  nh.initNode();
  nh.getHardware()->setBaud(57600); // Ros Node uses 57600 by default
  nh.subscribe(drive);
  nh.advertise(pub_left_ticks);
  nh.advertise(pub_right_ticks);
  left_ticks_msg.data = 0;
  right_ticks_msg.data = 0;
  
  pinMode(RightTreadControlPin, OUTPUT); 
  pinMode(RightEncoderPinA, INPUT); 
  pinMode(RightEncoderPinB, INPUT); 
  // digitalPinToInterrupt(RightEncoderPinA) == 0
  attachInterrupt(0, RightEncoderEvent, CHANGE); // Trigger right_rpmcounter whenever hall sensor pulses
  
  pinMode(LeftTreadControlPin, OUTPUT);
  pinMode(LeftEncoderPinA, INPUT); 
  pinMode(LeftEncoderPinB, INPUT); 
  // digitalPinToInterrupt(LeftEncoderPinA) == 1
  attachInterrupt(1,  LeftEncoderEvent, CHANGE); // Trigger right_rpmcounter whenever hall sensor pulses

  Timer1.initialize(10000);  // 10000 us = 100 Hz
}

void loop() {
  nh.spinOnce();
  unsigned long time = millis(); // time - lastMilli == time passed
  //OUPUTS
  if(time - lastMilli >= LoopTime)   { // Enter Timed Loop 
    getMotorData(time - lastMilli);
    PublishTICKS(time - lastMilli);// Publish and Restart Loop 
    lastMilli = time;
  }
}
//https://answers.ros.org/question/73627/how-to-increase-rosserial-buffer-size/
//http://andrewjkramer.net/motor-encoders-arduino/
