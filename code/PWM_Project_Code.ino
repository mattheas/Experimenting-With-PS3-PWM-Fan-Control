/*
As mentioned in the write up code snippets were taken and altered from:

[3] B. Ripley, Three Ways To Read A PWM Signal With Arduino, Jun. 2014. Accessed on: Nov. 7, 2020. [Online]. Available: https://www.benripley.com/diy/arduino/three-ways-to-read-a-pwm-signal-with-arduino/

[4] Unknown Author (Oct 30, 2016) Arduino Ethernet Shield-LED ON/OFF from webpage (Version 1.0) [Source code]. https://alselectro.wordpress.com/2016/10/30/arduino-ethernet-shield-led-onoff-from-webpage/

[5] Federico Dossena (Uknown date) HOW TO PROPERLY CONTROL PWM FANS WITH ARDUINO (Version 1.0) [Source code]. https://fdossena.com/?p=ArduinoFanControl/i.md

*/


#include <SPI.h>
#include <Ethernet.h>


byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED }; //physical mac address
byte ip[] = { 192, 168, 1, 250 }; // IP address in LAN – need to change according to your Network address
byte gateway[] = { 192, 168, 0, 1 }; // internet access via router
byte subnet[] = { 255, 255, 255, 0 }; //subnet mask
EthernetServer server(80); //server port

String readString;
byte PWM_PIN = 6; // pin for reading in ps3 pwm signal
float pwm_ps3_input; // pulse width of ps3 input signal, eventually turned into percentage between 0.0 to 1.0
float ps3_pwm_period=40; // period of original ps3 pwm signal
boolean is_auto_last_pressed = true; // start fan control in auto
boolean first_iteration = true; // used for if-statement that needs to be run once
float pwm_current; // used when we are manual increase/decrease mode to remember previous values



//configure Timer 1 (pins 9,10) to output 25kHz PWM
void setupTimer1(){
    //Set PWM frequency to about 25khz on pins 9,10 (timer 1 mode 10, no prescale, count to 320)
    TCCR1A = (1 << COM1A1) | (1 << COM1B1) | (1 << WGM11);
    TCCR1B = (1 << CS10) | (1 << WGM13);
    ICR1 = 320;
    OCR1A = 0;
    OCR1B = 0;
}

//configure Timer 2 (pin 3) to output 25kHz PWM. Pin 11 will be unavailable for output in this mode
void setupTimer2(){
    //Set PWM frequency to about 25khz on pin 3 (timer 2 mode 5, prescale 8, count to 79)
    TIMSK2 = 0;
    TIFR2 = 0;
    TCCR2A = (1 << COM2B1) | (1 << WGM21) | (1 << WGM20);
    TCCR2B = (1 << WGM22) | (1 << CS21);
    OCR2A = 79;
    OCR2B = 0;
}

//equivalent of analogWrite on pin 9
void setPWM1A(float f){
    f=f<0?0:f>1?1:f;
    OCR1A = (uint16_t)(320*f);
}

//equivalent of analogWrite on pin 3
void setPWM2(float f){
    f=f<0?0:f>1?1:f;
    OCR2B = (uint8_t)(79*f);
}



void setup(){
    // code for reading in ps3 pwm signal
    pinMode(PWM_PIN, INPUT);
    Serial.begin(115200);


    // code for generating 25khz signal
    pinMode(9,OUTPUT); //1A
    setupTimer1();
    //enable outputs for Timer 2
    pinMode(3,OUTPUT); //2
    setupTimer2();
    

    // code for starting Ethernet
    Ethernet.begin(mac, ip, gateway, subnet);
    server.begin();
    
}// end of void setup()



void loop(){

    // code for reading in ps3 pwm signal
    pwm_ps3_input = pulseIn(PWM_PIN, HIGH);
    pwm_ps3_input = pwm_ps3_input/ps3_pwm_period; // turn pulse width into float between 0 and 1.0
    //Serial.println(pwm_ps3_input);
  
    if(first_iteration){
      pwm_current = pwm_ps3_input;
      first_iteration = false;
    }

    // will keep the external fan updating if nothing on webpage is clicked
    if(is_auto_last_pressed){  
      Serial.println("updating");
      //set duty on pin 9 to current input, i.e match speed of external fan to internal ps3 fan
      setPWM2(pwm_ps3_input); 

      // set fan to auto control, this only changes if increase or decrease is clicked
      is_auto_last_pressed = true;

      // updates our pwm_current to give us a referene if we switch into manual increase/ decrease mode
      pwm_current = pwm_ps3_input;

    }

  Serial.println("void loop");
    // Create a client connection
    EthernetClient client = server.available();
    if (client) {
      Serial.println("inside if");
        while (client.connected()) {
          Serial.println("inside while");
            if (client.available()) {
              Serial.println("inside second if");
                char c = client.read();

                //read char by char HTTP request
                if (readString.length() < 100) {

                    //store characters to string
                    readString += c;
                }

                //if HTTP request has ended– 0x0D is Carriage Return \n ASCII
                if (c == 0x0D) {
                    client.println("HTTP/1.1 200 OK"); //send new page
                    client.println("Content-Type: text/html");
                    client.println();

                    client.println("<HTML>");
                    client.println("<HEAD>");
                    client.println("<TITLE> Control Page</TITLE>");
                    client.println("</HEAD>");
                    client.println("<BODY>");
                    client.println("<hr>");
                    client.println("<hr>");
                    client.println("<br>");
                    client.println("<H1 style=\"color:green;\">INCREASE/DECREASE OR AUTO FAN SPEED FROM WEBPAGE</H1>");
                    client.println("<hr>");
                    client.println("<br>");

                    client.println("<H2><a href=\"/?AUTO\"\">Auto Fan Speed</a><br></H2>");
                    client.println("<H2><a href=\"/?INCREASE\"\">Increase Fan Speed</a><br></H2>");
                    client.println("<H2><a href=\"/?DECREASE\"\">Decrease Fan Speed</a><br></H2>");

                    client.println("</BODY>");
                    client.println("</HTML>");

                    delay(50);
                    //stopping client
                    client.stop();


                    // control arduino output PWM pin
                    
                    if(readString.indexOf("?INCREASE") > -1){
                      Serial.println("increasr");
                        // sets fan to manual control
                        is_auto_last_pressed = false;

                        if(pwm_current <= 0.9){ 
                          // increase duty cycle of fan by 10%
                          pwm_current = pwm_current + 0.1;
                          
                        }

                        //set duty on pin 9
                        setPWM2(pwm_current); 
                        
                    }else if(readString.indexOf("?DECREASE") > -1){ // checks for DECREASE
                      Serial.println("decrease");
                        // sets fan to manual control
                        is_auto_last_pressed = false;
                        
                        if(pwm_current >= 0.3){
                          // decrease duty cycle of fan by 10%
                          pwm_current = pwm_current - 0.1;
                          
                        }

                        //set duty on pin 9
                        setPWM2(pwm_current);
                       
                    }else if(is_auto_last_pressed ||  readString.indexOf("?AUTO") > -1){  
                      Serial.println("here");
                        //set duty on pin 9 to current input, i.e match speed of external fan to internal ps3 fan
                        setPWM2(pwm_ps3_input); 

                        // set fan to auto control, this only changes if increase or decrease is clicked
                        is_auto_last_pressed = true;

                        // updates our pwm_current to give us a referene if we switch into manual increase/ decrease mode
                        pwm_current = pwm_ps3_input;

                    }

                    
                    //clearing string for next read
                    readString="";

                }
            }
        }
    }
}
