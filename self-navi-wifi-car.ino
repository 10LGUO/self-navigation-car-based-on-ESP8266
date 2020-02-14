// Define Trig and Echo pin:
#include <ESP8266WiFi.h>
#ifndef STASSID
#define STASSID "wifi id to be connected"
#define STAPSK  "wifi password"
#endif

//pins configuration
#define trigPin 16
#define echoMiddle 4 // middle echo
#define echoRight 0 // right echo
#define echoLeft 15 // left echo
#define STBY 2
#define AIN2 12
#define AIN1 13 // left and right motor share the same pair of input pinds
#define PWMRight 5
#define PWMLeft 14

#define STOP      0
#define FORWARD   4
#define BACKWARD  2
#define TURNLEFT  3
#define TURNRIGHT -3



const char* ssid     = STASSID;
const char* password = STAPSK;

const char* host = "djxmmx.net";
const uint16_t port = 17;


// Define variables:
int middle, right, left;
int rssi = 0;
int rssi_sum = 0;
int i = 0;
int pre;
int cur;
int next_turn;
int approaching = 0;

void setup() {
  // put your setup code here, to run once:
  pinMode(echoMiddle, INPUT);
  pinMode(echoRight, INPUT);
  pinMode(echoLeft, INPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(STBY,OUTPUT);
  
  pinMode(PWMRight,OUTPUT);
  pinMode(AIN1,OUTPUT);
  pinMode(AIN2,OUTPUT);
  
  pinMode(PWMLeft,OUTPUT);

  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password); // connect to beacon

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  pre = WiFi.RSSI();
  next_turn = TURNLEFT; // next turn initialized to be left.
  Serial.begin(9600);
  
}


void loop() {
  // put your main code here, to run repeatedly:

  middle = getDistance(echoMiddle); // get the distance to the obstacle from the middle ultrasonic sensor

  if (middle < 10){ // if obstacle suddenly get in the way, the distance may be smaller than 10cm
    digitalWrite(STBY,LOW); 
    motorRun(BACKWARD,400); // stop and go backward
  }
  
  rssi = WiFi.RSSI(); //get the wifi signal strength
  rssi_sum += rssi; 
  i += 1; // add up rssi values for 'i' times

  if(rssi >= -58){ // set stopping condition, the beacon is approximately around the car if rssi < 58
    digitalWrite(STBY, LOW);
    while((WiFi.RSSI() + WiFi.RSSI() + WiFi.RSSI())/3 > -70){ // stop and wait. if beacon leaves, the car will go back to moving
      digitalWrite(STBY, LOW);
    }
  }
  
  
  else if (middle < 20){  
    digitalWrite(STBY, LOW);
    right = getDistance(echoRight);
    left = getDistance(echoLeft);
    middle = getDistance(echoMiddle); // if middle distance is smaller than 20, there is an obstace expected in the way
                                      // then get the distance measured by left and right sensor to detect the edge of the obstacle
    
    if (left < 10 || right < 10 || middle < 10){ // if any distance measured is smaller than 10 ,going backward
        digitalWrite(STBY, LOW);
        delay(300);
        motorRun(BACKWARD,400);
      } 
      
    if (right < left && right < 15){ // if left edge is farther and there is no space for the right turn, turn left
      while(right < 15){ // keep turning until the car leaves the obstacle
        i = 0;
        rssi_sum = 0;
        motorRun(TURNLEFT, 300);
        right = (right + getDistance(echoRight))/2;
      }
      next_turn = TURNLEFT;
    }
    else if (right > left && left < 15){ // same logic as left turn
      //turn right
      while(left < 15){
        i = 0;
        rssi_sum = 0;
        motorRun(TURNRIGHT, 300);
        left = (left + getDistance(echoLeft))/2;
      }
      next_turn = TURNRIGHT;
    }
    else{                              // if there is enough space to right and left edge, turn according to the navigation plan
      motorRun(next_turn, 450);
    }
  }
  else{
    motorRun(FORWARD, 500);  // obstacle not dected, move forward
  }

  
  if (i == 2){              // measure the average rssi every 2 times, if the current average rssi is samller than the previous one - 2, take some turn
    digitalWrite(STBY,LOW);
    cur = rssi_sum/2;
    if (cur < pre - 2){     // the vehicle is not making much progress, take turn
      motorRun(next_turn, 450);
      if (approaching > 2){ // if the same turn have been made for three times and still not progress, switch to another direction
        next_turn = -next_turn;
        approaching = 0;
      }
    }
    approaching += 1;
    pre = cur;
    i = 0;
    rssi_sum = 0;
  }  
}

int getDistance(int echoPin) // measure the distance by ultrasonic sensor
{
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  int distance = pulseIn(echoPin, HIGH)/58; // calculate distance according to the duration between triger and echo
  pulseIn(echoPin, HIGH);
  return distance;
}

void motorRun(int cmd, int value){ // motor control function. The car can move forward, backward, left and right and stop.
  value = value + 30;              // some adjustment can be made according to the floor friction, battery voltage level, etc.
  switch(cmd){
    case FORWARD:
      analogWrite(PWMRight, value);
      analogWrite(PWMLeft, value);
      digitalWrite(AIN1, LOW);
      digitalWrite(AIN2, HIGH);
      digitalWrite(STBY, HIGH);
      break;
    case BACKWARD:
      analogWrite(PWMRight, value - 50);
      analogWrite(PWMLeft, value - 50);
      digitalWrite(AIN1, HIGH);
      digitalWrite(AIN2, LOW);
      digitalWrite(STBY, HIGH);
      delay(500);
      digitalWrite(STBY, LOW);
      break;
    case TURNLEFT:
      analogWrite(PWMRight, value + 100);
      analogWrite(PWMLeft, 100);
      digitalWrite(AIN1, LOW);
      digitalWrite(AIN2, HIGH);
      digitalWrite(STBY, HIGH);
      delay(700);
      digitalWrite(STBY, LOW);
      break;
    case TURNRIGHT:
      analogWrite(PWMRight, 100);
      analogWrite(PWMLeft, value + 50);
      digitalWrite(AIN1, LOW);
      digitalWrite(AIN2, HIGH);
      digitalWrite(STBY, HIGH);
      delay(700);
      digitalWrite(STBY, LOW);
      break;
    default:
      analogWrite(PWMRight, 200);
      analogWrite(PWMLeft, 200);
      digitalWrite(STBY, LOW);
  }
}
