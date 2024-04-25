#include <SoftwareSerial.h>


#define DEBUG true

// Pin Definitions
int PWR_KEY = 9;
int RST_KEY = 6;
int LOW_PWR_KEY = 5;
const int digitalPin = 2;
const int ledPin = 10;

SoftwareSerial A9GSerial(3, 4); // RX, TX - Define the RX and TX pins for SoftwareSerial 87

// Variables
float test1;
float test2;
String st1 = "";
String st2 = "";
String lat;
String lng;
String latr;
String lngr;
String test="30.1234";
int b = 33;
unsigned long timeCount;

bool ModuleState = false;
volatile bool wireConnected = true;
volatile int t=1;
volatile bool interrupt_trigger=false;
volatile bool flag_loop=false;
volatile bool initial_except = false;
volatile bool session_out= false;
volatile bool wire_connection = false;

void setup() {
    pinMode(digitalPin, INPUT_PULLUP);
    pinMode(ledPin, OUTPUT);
    digitalWrite(ledPin, HIGH);

    wire_connection =  digitalRead(digitalPin);

    pinMode(PWR_KEY, OUTPUT);
    pinMode(RST_KEY, OUTPUT);
    pinMode(LOW_PWR_KEY, OUTPUT);
    digitalWrite(RST_KEY, LOW);
    digitalWrite(LOW_PWR_KEY, HIGH);
    digitalWrite(PWR_KEY, HIGH);

    Serial.begin(115200);
    A9GSerial.begin(115200); // Initialize SoftwareSerial

    digitalWrite(PWR_KEY, LOW);
    delay(3000);
    digitalWrite(PWR_KEY, HIGH);
    delay(5000);
    ModuleState = moduleStateCheck();
    if (ModuleState == false) { // if it's off, turn it on
        digitalWrite(PWR_KEY, LOW);
        delay(3000);
        digitalWrite(PWR_KEY, HIGH);
        delay(5000);
        Serial.println("Now turning the A9/A9G on.");
    }

    sendData("AT+GPS=1", 1000, DEBUG); // Turn on GPS
    sendData("AT+CGATT=1", 1000, DEBUG);
    sendData("AT+CGATT=1", 1000, DEBUG);

    sendData("AT+CGDCONT=1,\"IP\",\"airtelive\"",1000, DEBUG);
    

    sendData("AT+CGACT=1,1",1000, DEBUG);
    sendData("AT+GPS=1", 1000, DEBUG);
    Serial.println("Arduino Uno - A9G GPS Coordinates:");
    led_locked();
    digitalWrite(ledPin, HIGH);

    Serial.println("lock status");
    Serial.println(wire_connection);
    attachInterrupt(digitalPinToInterrupt(digitalPin), wireStatusChanged, CHANGE);


}

void loop() {
  
  // Check module state
  ModuleState = moduleStateCheck();
    if (ModuleState == false) {     // if it's off, turn it on
        digitalWrite(PWR_KEY, LOW);
        delay(3000);
        digitalWrite(PWR_KEY, HIGH);
        delay(5000);
        Serial.println("Now turning the A9/A9G on.");
        initialization();
    }

    if(!wire_connection){
      Serial.println("TRiggered");
      interrupt_trigger = true;
      flag_loop = true;
      initial_except = true;
      wire_connection = true;
   }

  if(flag_loop){
    delay(1000);
    flag_loop=false;
    
    led_locked();
    Serial.println("Interrupt activated");
    attachInterrupt(digitalPinToInterrupt(digitalPin), wireStatusChanged, FALLING);
    }


    if (millis() - timeCount > 5000) {
    String coordinates = sendData("AT+LOCATION=2", 1000, DEBUG);
    int index = coordinates.indexOf(',');

Serial.println("THis is index"+index);
String latStr = coordinates.substring(13, index);
String lngStr = coordinates.substring(index +1 );
Serial.println("test 1"+latStr);
Serial.println("test 2"+lngStr);

// Convert the string to a float
double lat = latStr.toDouble();
double lng = lngStr.toDouble();
Serial.println(lat);
Serial.println(lng);

// Construct the URL for sending GPS data
// Format: "https://example.com/{b}/{latitude}/{longitude}"
String newURL2 = "https://example.com/"+ String(b) +"/"+String(lat)+"/"+String(lng) ;
Serial.println("//////////////////////////"+newURL2);
String httpGETCommand2 = "AT+HTTPGET=\"" + newURL2 +"\"";
sendData(httpGETCommand2, 8000, DEBUG);

Serial.println(""+httpGETCommand2);
      
String request1 = sendData(httpGETCommand2, 8000, DEBUG);

   ModuleState = moduleStateCheck();
    if (ModuleState == false) { // if it's off, turn it on
        digitalWrite(PWR_KEY, LOW);
        delay(3000);
        digitalWrite(PWR_KEY, HIGH);
        delay(5000);
        Serial.println("Now turning the A9/A9G on.");
        initialization();
    }

       
  while(session_out){
      ModuleState = moduleStateCheck();
    if (ModuleState == false) { // if it's off, turn it on
        digitalWrite(PWR_KEY, LOW);
        delay(3000);
        digitalWrite(PWR_KEY, HIGH);
        delay(5000);
        Serial.println("Now turning the A9/A9G on.");
        initialization();
    }


      if (t==0){
        Serial.println("Wire Disconnected **");
        sendSMS("Mobile number","Device Unlocked. #ID - D001");
        delay(1000);
        b=11;
        t=1;

    
  }
  session_finished();
  }
Serial.println("t: " + String(t));         
}
}
 
void printVariableType(char* variableName, int value) {
  Serial.print("Type of ");
  Serial.print(variableName);
  Serial.print(": int");
  Serial.println();
}


bool moduleStateCheck() {
  // Check the state of the A9/A9G module
  int i = 0;
  bool state = false;
  for (i = 0; i < 10; i++) {
    String msg = sendData("AT", 1000, DEBUG);
    if (msg.indexOf("OK") >= 0) {
        Serial.println("A9/A9G Module had turned on.");
        state = true;
        return state;
        }
        delay(500);
    }
    return state;
}

String sendData(String command, const int timeout, boolean debug) {
    String response = "";
    A9GSerial.println(command);
    long int time = millis();
    while ((time + timeout) > millis()) {
        while (A9GSerial.available()) {
            char c = A9GSerial.read();
            response += c;
        }
    }
    if (debug) {
        Serial.print(response);
    }
    return response;
}

String reverseString(String str) {
    int length = str.length();
    for(int i = 0; i < length / 2; i++) {
        char temp = str[i];
        str[i] = str[length - i - 1];
        str[length - i - 1] = temp;
    }
    return str;
}
void sendSMS(String phoneNumber, String message) {
  // Send an SMS using the A9/A9G module
  // Set SMS mode to text
  sendData("AT+CMGF=1", 1000, DEBUG);

  // Specify the recipient's phone number
  String command = "AT+CMGS=\"" + phoneNumber + "\"";
  sendData(command, 1000, DEBUG);

  // Send the message
  sendData(message + char(26), 1000, DEBUG);
}

void led_locked(){
  
for (int i = 0; i < 2; i++) {
  digitalWrite(ledPin, HIGH);
  // Wait for 1 second
  delay(100);
  // Turn the LED off
  digitalWrite(ledPin, LOW);
  // Wait for 1 second
  delay(100);
     digitalWrite(ledPin, HIGH);
  // Wait for 1 second
  delay(100);
  // Turn the LED off
  digitalWrite(ledPin, LOW);
  // Wait for 1 second
  delay(100);
  }

}

void session_finished(){
  digitalWrite(ledPin, HIGH);
  delay(500);
  digitalWrite(ledPin, LOW);
  delay(500);
}

void initialization(){
   sendData("AT+GPS=1", 1000, DEBUG); // Turn on GPS
    sendData("AT+CGATT=1", 1000, DEBUG);
    sendData("AT+CGATT=1", 1000, DEBUG);

    sendData("AT+CGDCONT=1,\"IP\",\"APN\"",1000, DEBUG); // Add APN
    sendData("AT+CGACT=1,1",1000, DEBUG);
    sendData("AT+GPS=1", 1000, DEBUG); // Get GPS address
    timeCount = millis();
    Serial.println("Arduino Uno - A9G GPS Coordinates:");
}

void wireStatusChanged() {
  // Handle wire status change interrupt
  Serial.println("Interrupt triggered");
  if(!interrupt_trigger ){
    detachInterrupt(digitalPinToInterrupt(digitalPin));

    digitalWrite(ledPin, LOW);
    interrupt_trigger = true;
    Serial.println("wire connected");
    flag_loop=true;
    initial_except = true;
    return;
     
  }

    
  if (digitalRead(digitalPin) == HIGH && initial_except) {
      digitalWrite(ledPin, HIGH);
      Serial.println("INT 1");
      t = 0;
      detachInterrupt(digitalPinToInterrupt(digitalPin));
      session_out = true;
    }
}