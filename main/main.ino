/*-----Import Libraries-----*/
#include <Wire.h>               // Wire library. I2C communication. 
#include <Math.h>
#include <LiquidCrystal_I2C.h>  // LCD I2C library. 2x16 LCD display.
#include <EEPROM.h>             // EEPROM library. Data storage.
#include <PID_v1.h>             // PID library. Temperature control.
#include <OneWire.h>            // 1-wire library. Temperature sensor control.
#include <DallasTemperature.h>  // Temperature sensor library. Provides functions to obtain temp in degrees C. 
/*-----Define Variables-----*/

// Define encoder pins
#define outputA 6
#define outputB 7

// Data wire for temp sensor
#define ONE_WIRE_BUS 2

// Setup a oneWire instance to communicate with any OneWire devices  
// (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);
 
// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

// Define LCD
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);  // Set the LCD I2C address

//#### Define variables ####

// Define EEPROM variables
int eeprom_temp_ad = 0;   // eeprom last temperature address
int eeprom_min_ad = 0;    // eeprom last minute address 
int eeprom_sec_ad = 0;    // eeprom last second address
int init_temp = 0;        // last temperature variable
int init_min = 0;         // last chosen time (minutes)
int init_sec = 0;         // last chosen time (seconds)

// Definre PID variables
double Setpoint, Input, Output;
int WindowSize = 5000;    // 5 seconds window
unsigned long windowStartTime;

// Define MOSFET gate pin for heating cartridges 
int mosfet_pin = 9;

// Define the aggressive and conservative Tuning Parameters
// ***** NEEDS TO BE ADJUSTED AND TESTED *****
double aggKp = 4, aggKi = 0.2, aggKd = 1;
double consKp = 1, consKi = 0.05, consKd = 0.25;

// Create a PID controller
PID tempPID(&Input, &Output, &Setpoint,consKp,consKi,consKd, DIRECT);



int flag = 0;                 // Used to navigate menu
//float final_time = 0;       
int selected_value = 0;       // Used to store selected encoder value
int prev_selected_value = 0;
//int result;
int led_pin = 10;
int hours;                   // exposure time (hours) 
int minutes;                 // exposure time (minutes)
int seconds;                 // exposure time (seconds)

int speaker_pin = 8;      
int counter = 0;
int aState;
int aLastState;
int x = 0;
int initial = 0;
int temperature = 0;
bool heating = false;
bool exposure = false;
unsigned long currentMillis;
unsigned long previousMillis = 0;
long interval = 1000;
unsigned long time_now;
unsigned long time_prev = 0;


void setup()
{

  // LED temperature testing
  pinMode(14, OUTPUT);
  
  // temperature setup
  sensors.begin(); 
  
  // Encoder setup
  pinMode (outputA, INPUT); // encoder
  pinMode (outputB, INPUT); // encoder 
  aLastState = digitalRead(outputA); // read value of the output A
  Serial.begin(9600);  // Used to type in characters
  
  // PID setup
  windowStartTime = millis();
  tempPID.SetOutputLimits(0, WindowSize);
  tempPID.SetMode(AUTOMATIC);

  
  // LED setup
  pinMode(led_pin, OUTPUT);
  digitalWrite(led_pin, LOW);

  // Speaker setup
  pinMode(speaker_pin, OUTPUT);

  // LCD setup
  lcd.begin(16, 2);  // initialize the lcd for 16 chars 2 lines
  lcd.backlight();
  lcd.setCursor(0, 0); //Start at character 0 on line 0
  lcd.print("Hi");
  delay(400);
  lcd.setCursor(0, 1);
  lcd.print("Select time!");
  delay(2000);
  lcd.clear();

  // TFT setup
  
  
}


void loop() {
  // Keep track on the elapsed time
  Serial.println(heating);
  time_now = millis();
  if (heating == false) {
    digitalWrite(13,LOW);
  }
  //Serial.println("TIME TIME TIME");
  /* flag 0: Main menu. Choose between heating 
   *  block settings or light exposure time
   */
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  if (flag == 0)  {
    if (heating == true)  {
      pid_temperature();
    }
    if (initial == 0) {
        //Initial display (Heating block)
        lcd.noBlink();
        lcd.clear();
        lcd.setCursor(4, 0);
        lcd.print("Heating");
        lcd.setCursor(5, 1);
        lcd.print("Block");
        initial = 1;
    }
    aState = digitalRead(outputA);    // Read output A
    if ((aState != aLastState) && (flag == 0)) {     // compare last and current state
      if (digitalRead(outputB) != aState && flag == 0) {     // if different    
        counter ++;
      }
      else  {
        counter --;
      }
      selected_value = counter;

      /* Divisor chosen based on the main menu 
       *  option number.
       */
      if (prev_selected_value != selected_value)  {
        tone(speaker_pin, 1000, 50);
        //noTone(speaker_pin);
      }
      x = counter % 2;
      Serial.println(x);
      
     /*check if the counter value has changed. If so,
      *  change the selection on the main menu.
      *  
      */
      
      if (x == 0)  {
        //Heating block display
        lcd.clear();
        lcd.setCursor(4, 0);
        lcd.print("Heating");
        lcd.setCursor(5, 1);
        lcd.print("Block");      
      }
      else  {
        //Light exposure display
        lcd.clear();
        lcd.setCursor(5, 0);
        lcd.print("Light");
        lcd.setCursor(4, 1);
        lcd.print("Exposure");     
      }
      aLastState = aState;
      prev_selected_value = selected_value;
    }

    /*
     * Enter Heating block menu
     */
    if ((x == 0) && ( analogRead(A2) < 500 ) && (time_now - time_prev) > 300) {
      tone(speaker_pin, 1000, 50);
      //delay(400);
      Serial.println("HEAT"); 
      flag = 1;
      initial = 0;
      aState = 0;
      aLastState = 0;
      Serial.println("FLAG");
      Serial.println(flag); 
      time_prev = time_now;
    }
    /* Enter Light exposure menu
     *  
     */
    if ((x != 0) && ( analogRead(A2) < 500) && (time_now - time_prev) > 300)  {
      tone(speaker_pin, 1000, 50);
      Serial.println("EXP"); 
      //delay(400);
      flag = 2;
      initial = 0;
      aState = 0;
      aLastState = 0;
      time_prev = time_now;
    }
  }
  /*  
   *  flag 1: Sub menu 1. Choose to set temperature or 
   *  switch the heating ON or OFF
   */
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  else if (flag == 1)  {
    if (heating == true)  {
      pid_temperature();
    }
    if (initial == 0) {
        //Initial display (Heating block)
        lcd.noBlink();
        lcd.clear();
        lcd.setCursor(6, 0);
        lcd.print("Set");
        lcd.setCursor(3, 1);
        lcd.print("Temperature"); 
        initial = 1;
    }
    aState = digitalRead(outputA);    // Read output A
    if ((aState != aLastState) && (flag ==1)) {     // compare last and current state
      if (digitalRead(outputB) != aState && flag == 1) {     // if different    
        counter ++;  
      }
      else  {
        counter --;
      }
      selected_value = counter;
 
      /* Divisor chosen based on the sub menu 
       *  option number.
       */
      if (prev_selected_value != selected_value)  {
        tone(speaker_pin, 1000, 50);
        //noTone(speaker_pin);
      }
      x = counter % 2;
      Serial.println(x);

      
     /*check if the counter value has changed. If so,
      *  change the selection on the sub menu.
      *  
      */
      if (x == 0)  {
        //Heating block display
        lcd.clear();
        lcd.setCursor(6, 0);
        lcd.print("Set");
        lcd.setCursor(3, 1);
        lcd.print("Temperature");      
      }
      else  {
        //Light exposure display
        lcd.clear();
        lcd.setCursor(4, 0);
        lcd.print("Heating");
        lcd.setCursor(4, 1);
        lcd.print("ON/OFF");     
      }
      aLastState = aState;
      prev_selected_value = selected_value;
    }
    // Enter temperature selection
    if ((x == 0) && ( analogRead(A2) < 500 ) && (time_now - time_prev) > 300) {
      tone(speaker_pin, 1000, 50);
      //delay(400);
      Serial.println("SHIT");
      flag = 3;
      Serial.println("flag");
      Serial.println(flag);
      initial = 0;
      counter = 0;
      x = 0;
      time_prev = time_now;
      
    // Enter Heating ON/OFF menu
    }
    else if ((x != 0) && (analogRead(A2) < 500 ) && (time_now - time_prev) > 300) {
      tone(speaker_pin, 1000, 50);
      //delay(400);
      flag = 4;
      Serial.println("flag");
      Serial.println(flag);
      initial = 0;
      counter = 0;
      x = 0;
      time_prev = time_now;
    // Go back to main menu
    }
    else if (analogRead(A1) > 1000 && (time_now - time_prev) > 300) {
      tone(speaker_pin, 1000, 50);
      //delay(400);
      flag = 0;
      Serial.println("flag");
      Serial.println(flag);
      initial = 0;
      counter = 0;
      x = 0;
      time_prev = time_now;
    }
  }
  /* flag 3: Temperature selection. Select a desired
 *  temperature of the heating block in degrees
 *  celsius
 */
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  else if (flag == 3)  {
    if (heating == true)  {
      pid_temperature();
    }    
    aState = digitalRead(outputA); 
    if (initial == 0) {
      init_temp = EEPROM.read(eeprom_temp_ad);
      if (init_temp == 255) {
        init_temp = 0;
      }
      Serial.println(flag);
    //Initial display (Heating block)
      lcd.noBlink();
      lcd.clear();
      lcd.setCursor(5, 1);
      lcd.print(init_temp);
      lcd.setCursor(7, 1);
      lcd.print(char(223));
      lcd.setCursor(8, 1);
      lcd.print("C");
      initial = 1;   
    }
    // Read output A
    if ((aState != aLastState) && (flag == 3)) {     // compare last and current state
      if (digitalRead(outputB) != aState && flag == 3) {     // if different    
        counter ++;
      }
      else  {
        if (counter > 0) {
          counter --;  
        }  
      }
     
      
      // Set temperature limits (eg. 0 - 99)
      if (counter > 98) {
        counter = 99;
      }
      if (counter < 0)  {
        counter = 0;
      }
      selected_value = counter;
      Serial.println(counter);

      /* Divisor chosen based on the main menu 
       *  option number.
       */
      // Tone
      if (prev_selected_value != selected_value)  {
        tone(speaker_pin, 1000, 50);
        //noTone(speaker_pin);
      }
      // Display value
      lcd.setCursor(5, 1);
      lcd.print(selected_value);
      
      // Fix value position on the display when going form 2 to 1 digit values
      if (prev_selected_value == 10 && selected_value == 9 ) {
        lcd.clear();
        lcd.setCursor(5, 1);
        lcd.print(selected_value);
        lcd.setCursor(7, 1);
        lcd.print(char(223));
        lcd.setCursor(8, 1);
        lcd.print("C");
         
      }
      
      aLastState = aState;
      prev_selected_value = selected_value;
    }

    /*
     * Enter Heating ON/OFF menu
     */
    if ( analogRead(A2) < 500  && (time_now - time_prev) > 300) {
      tone(speaker_pin, 1000, 50);
      //delay(400);
      flag = 4;
      initial = 0;
      x = 0;
      temperature = selected_value;
      lcd.print(temperature);
      EEPROM.update(eeprom_temp_ad, temperature);  
      time_prev = time_now;
    }
    // Go back to Heating Block sub menu
    else if (analogRead(A1) > 1000 && (time_now - time_prev) > 300) {
      tone(speaker_pin, 1000, 50);
      //delay(400);
      flag = 1;
      initial = 0;
      x = 0;
      time_prev = time_now;
    }
  }
  
   /* flag 4: Heating ON/OFF.
    * Set heating ON or OFF 
    */
 ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  else if (flag == 4)  {
    if (heating == true)  {
      pid_temperature();
    }    
    aState = digitalRead(outputA); 
    if (initial == 0) {
    //Initial display (Heating block)
      lcd.noBlink();
      lcd.clear();
      lcd.setCursor(5, 0);
      lcd.print("Heating");
      initial = 1; 
    }
    // Read output A
    if ((aState != aLastState) && (flag == 4) ) {     // compare last and current state
      if ((digitalRead(outputB) != aState) && (flag == 4)) {     // if different    
        counter ++;
      }
      else  {
        counter --;    
      }
      selected_value = counter;
      x = counter % 2;
      Serial.println(x);
      
      // tone
      if (prev_selected_value != selected_value)  {
        tone(speaker_pin, 1000, 50);
        //noTone(speaker_pin);
      }
      
      if (x == 0)  {
        //Heating block display
        lcd.clear();
        lcd.setCursor(5, 0);
        lcd.print("Heating");
        lcd.setCursor(7, 1);
        lcd.print("ON");
      }
      else  {
        //Light exposure display
        lcd.clear();
        lcd.setCursor(5, 0);
        lcd.print("Heating");
        lcd.setCursor(7, 1);
        lcd.print("OFF");     
      }
      aLastState = aState;
      prev_selected_value = selected_value;
    }
    // Heating ON
    if ((x == 0) && ( analogRead(A2) < 500 ) && (time_now - time_prev) > 300) {
      tone(speaker_pin, 1000, 50);
      //delay(400);
      heating = true;
      counter = 0;
      x = 0;
      flag = 1;
      initial = 0;
      Serial.println("flag");
      Serial.println(flag);
      Serial.println(heating); 
      time_prev = time_now;
    }
      
    // Heating OFF
    else if ((x != 0) && ( analogRead(A2) < 500 ) && (time_now - time_prev) > 300) {
      tone(speaker_pin, 1000, 50);
      //delay(400);
      heating = false;
      counter = 0;
      x = 0;
      flag = 1;
      initial = 0;
      Serial.println("flag");
      Serial.println(flag);
      Serial.println(heating);
      time_prev = time_now;
    }
    // Go back to Heating Block sum menu
    else if (analogRead(A1) > 1000 && (time_now - time_prev) > 300) {
      tone(speaker_pin, 1000, 50);
      //delay(400);
      flag = 1;
      initial = 0;
      x = 0;
      Serial.println("flag");
      Serial.println(flag);   
      time_prev = time_now;
    }
  }
 ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 /* flag 2: Set light exposure time
  *  MINUTES
  */
  else if (flag == 2)  {
    if (heating == true)  {
      pid_temperature();
    }    
    aState = digitalRead(outputA);
    init_min = EEPROM.read(eeprom_min_ad);
    if (init_min == 255)  {
      init_min = 0 ;
    }
    if (initial == 0) {
    //Initial display (Heating block)
      lcd.noBlink();
      lcd.clear();
      lcd.setCursor(5, 0);
      lcd.print("Minutes");
      initial = 1; 
    }
    // Read output A
    if (aState != aLastState) {     // compare last and current state
      if (digitalRead(outputB) != aState) {     // if different    
        counter ++;
      }
      else  {
        if (counter > 0) {
          counter --;  
        }  
      }

      if (counter > 59) {
        counter = 60;
      }
      if (counter < 0)  {
        counter = 0;
      }
    
      
      selected_value = counter;   
      Serial.println(counter);   
      // tone
      if (prev_selected_value != selected_value)  {
        tone(speaker_pin, 1000, 50);
        //noTone(speaker_pin);
      }


      lcd.setCursor(7, 1);
      lcd.print(selected_value);
      if (prev_selected_value == 10 && selected_value == 9 ) {
        lcd.clear();
        lcd.setCursor(5, 0);
        lcd.print("Minutes");
        lcd.setCursor(7, 1);
        lcd.print(selected_value); 
      }
      
      aLastState = aState;
      prev_selected_value = selected_value;
    }
    // Select current value
    if ( analogRead(A2) < 500 && (time_now - time_prev) > 300 ) {
      tone(speaker_pin, 1000, 50);
      //delay(400);
      minutes = selected_value;
      counter = 0;
      x = 0;
      flag = 5;
      initial = 0;
      Serial.println("flag");
      Serial.println(flag);  
      time_prev = time_now;
    }
    // Go back to main menu
    else if (analogRead(A1) > 1000 && (time_now - time_prev) > 300) {
      tone(speaker_pin, 1000, 50);
      //delay(400);
      flag = 0;
      initial = 0;
      x = 0;
      Serial.println("flag");
      Serial.println(flag);  
      time_prev = time_now;
    }
  }
 ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 /* flag 5: Set light exposure time
  *  SECONDS
  */
  else if (flag == 5)  {
    if (heating == true)  {
      pid_temperature();
    }
    aState = digitalRead(outputA); 
    init_sec = EEPROM.read(eeprom_sec_ad);
    if (init_sec == 255)  {
      init_sec = 0;
    }
    if (initial == 0) {
    //Initial display (Heating block)
      lcd.noBlink();
      lcd.clear();
      lcd.setCursor(5, 0);
      lcd.print("Seconds");
      lcd.setCursor(7, 1);
      lcd.print(init_sec); 
      initial = 1; 
    }
    // Read output A
    if ((aState != aLastState) && (flag == 5) ) {     // compare last and current state
      if ((digitalRead(outputB) != aState) && (flag == 5)) {     // if different    
        counter ++;
      }
      else  {
        if (counter > 0) {
          counter --;  
        }  
      }

      //Set limits

      if (counter > 59) {
        counter = 60;
      }
      if (counter < 0)  {
        counter = 0;
      }
      selected_value = counter;

      
      // tone
      if (prev_selected_value != selected_value)  {
        tone(speaker_pin, 1000, 50);
        //noTone(speaker_pin);
      }


      lcd.setCursor(7, 1);
      lcd.print(selected_value);
      if (prev_selected_value == 10 && selected_value == 9 ) {
        lcd.clear();
        lcd.setCursor(5, 0);
        lcd.print("Seconds");
        lcd.setCursor(7, 1);
        lcd.print(selected_value); 
      }
      
      aLastState = aState;
      prev_selected_value = selected_value;
    }
    // Select current value
    if ( analogRead(A2) < 500 && (time_now - time_prev) > 300 ) {
      tone(speaker_pin, 1000, 50);
      //delay(400);
      seconds = selected_value;
      counter = 0;
      x = 0;
      flag = 6;
      initial = 0;
      Serial.println("flag");
      Serial.println(flag);  
      time_prev = time_now;
    }
    // Go back to MINUTE selection
    else if (analogRead(A1) > 1000 && (time_now - time_prev) > 300) {
      tone(speaker_pin, 1000, 50);
      //delay(400);
      flag = 2;
      initial = 0;
      x = 0;
      Serial.println("flag");
      Serial.println(flag); 
      time_prev = time_now;
    }
  }
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 /* flag 6: Confirm exposure time
 */
  else if (flag == 6)  {
    if (heating == true)  {
      pid_temperature();
    }
    aState = digitalRead(outputA); 
    if (initial == 0) {
    //Initial display (Heating block)
      lcd.clear();
      lcd.setCursor(5, 0);
      lcd.print("Start?");
      lcd.blink();
      initial = 1; 
    }
    // Read output A
    if ((aState != aLastState) && (flag == 6) ) {     // compare last and current state
      if ((digitalRead(outputB) != aState) && (flag == 6)) {     // if different    
        counter ++;
      }
      else  {
        if (counter > 0) {
          counter --;  
        }  
      }
      selected_value = counter;
      x = counter % 2;
      Serial.println(x);
      // tone
      if (prev_selected_value != selected_value)  {
        tone(speaker_pin, 1000, 50);
        //noTone(speaker_pin);
      }

      if (x == 0)  {
        //Heating block display
        lcd.clear();
        lcd.setCursor(5, 0);
        lcd.print("Start?");        
        lcd.setCursor(6, 1);
        lcd.print("NO");    
      }
      else  {
        //Light exposure display
        lcd.clear();
        lcd.setCursor(5, 0);
        lcd.print("Start?");  
        lcd.setCursor(6, 1);
        lcd.print("YES");   
      }
      aLastState = aState;
      prev_selected_value = selected_value;
    }
    // Exposure NO. Go back exposure time selection menu
    if ((x == 0) && ( analogRead(A2) < 500 ) && (time_now - time_prev) > 300) {
      tone(speaker_pin, 1000, 50);
      //delay(400);
      exposure = false;
      hours = 0;
      minutes = 0;
      seconds = 0;
      counter = 0;
      x = 0;
      flag = 2;
      initial = 0;
      Serial.println("flag");
      Serial.println(flag);  
      time_prev = time_now;
    }
      
    // Exposure YES. Start coutndown and turn on LEDs
    if ((x != 0) && ( analogRead(A2) < 500 ) && (time_now - time_prev) > 300) {
      tone(speaker_pin, 1000, 50);
      //delay(400);
      exposure = true;
      flag = 7;
      counter = 0;
      x = 0;
      initial = 0;
      Serial.println("flag");
      Serial.println(flag);
      time_prev = time_now;
    }  
  }
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////  
  /* flag 7: Start exposure and countdown.
   *  User can pause or camcel the process.
   */
  else if ( flag == 7)  {
    if (heating == true)  {
      pid_temperature();
    }
    //EEPROM.update(eeprom_min_ad, minutes);
    //EEPROM.update(eeprom_sec_ad, seconds); 
    while ((hours > 0 || minutes > 0 || seconds >= 0) && flag == 7) {
      currentMillis = millis();
      time_now = millis();
      if (currentMillis - previousMillis > interval)  {
        digitalWrite(led_pin,HIGH);
        lcd.clear();
        lcd.noBlink();
        lcd.setCursor(0, 0);
        lcd.print("exp in progress");
        lcd.setCursor(4, 2);
        previousMillis = currentMillis;
        time_prev = time_now;
        (hours < 10) ? lcd.print("0") : NULL;
        lcd.print(hours);
        lcd.print(":");
        (minutes < 10) ? lcd.print("0") : NULL;
        lcd.print(minutes);
        lcd.print(":");
        (seconds < 10) ? lcd.print("0") : NULL;
        lcd.print(seconds);
        lcd.display();
        stepDown();
      }     
      // Cancel exposure. Don't pause the proces. Confirmation required. 
      if ( analogRead(A1) > 1000 && (currentMillis - previousMillis) > 300) {
        tone(speaker_pin, 1000, 50);
        //delay(400);
        flag = 8;
        initial = 0;
        x = 0;
        currentMillis = 0;
        previousMillis = 0;
        Serial.println("flag");
        Serial.println(flag); 
        previousMillis = currentMillis;
        time_prev = time_now;
        
      }
      // Pause exposure. Ask to continue.
      if ( analogRead(A2) < 500 && (currentMillis - previousMillis) > 300) {
        tone(speaker_pin, 1000, 50);
        //delay(400);
        flag = 9;
        initial = 0;
        x = 0;
        currentMillis = 0;
        previousMillis = 0;
        Serial.println("flag");
        Serial.println(flag); 
        previousMillis = currentMillis;
        time_prev = time_now;
      }
    }
  }
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/* flag 8: Warning if you really wnat to cancel.
 *  Print is not going to be paused.
 */
  else if (flag == 8) {
    if (heating == true)  {
      pid_temperature();
    }
    aState = digitalRead(outputA); 
    if (initial == 0) {
    //Initial display (Heating block)
      lcd.clear();
      lcd.setCursor(2, 0);
      lcd.print("Cancel Exp?");
      lcd.blink();
      initial = 1; 
    }
    // Read output A
    if ((aState != aLastState) && (flag == 8) ) {     // compare last and current state
      if ((digitalRead(outputB) != aState) && (flag == 8)) {     // if different    
        counter ++;
      }
      else  {
        if (counter > 0) {
          counter --;  
        }  
      }
      selected_value = counter;
      x = counter % 2;
      Serial.println(x);
      // tone
      if (prev_selected_value != selected_value)  {
        tone(speaker_pin, 1000, 50);
        //noTone(speaker_pin);
      }

      if (x == 0)  {
        // Cancel light exposure
        lcd.clear();
        lcd.setCursor(2, 0);
        lcd.print("Cancel Exp?");        
        lcd.setCursor(5, 1);
        lcd.print("YES");    
      }
      else  {
        //Continue light exposure
        lcd.clear();
        lcd.setCursor(2, 0);
        lcd.print("Cancel Exp?");  
        lcd.setCursor(5, 1);
        lcd.print("NO");   
      }
      aLastState = aState;
      prev_selected_value = selected_value;
    }
    // Selected YES.Cancel exposure. Go back to main menu.
    if ((x == 0) && ( analogRead(A2) < 500 ) && (time_now - time_prev) > 300) {
      tone(speaker_pin, 1000, 50);
      //delay(400);
      exposure = false;
      counter = 0;
      x = 0;
      flag = 0;
      initial = 0;
      trigger();
      Serial.println("flag");
      Serial.println(flag);  
      time_prev = time_now;
    }
      
    // Selected No. Continue with the light exposure.
    else if ((x != 0) && ( analogRead(A2) < 500 ) && (time_now - time_prev) > 300) {
      tone(speaker_pin, 1000, 50);
      //delay(400);
      exposure = true;
      counter = 0;
      x = 0;
      initial = 0;
      flag = 7;
      Serial.println("flag");
      Serial.println(flag);
      lcd.noBlink();
      time_prev = time_now;
    } 
  }
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/* flag 9: Pause light exposure.
 *  Press the encoder button to continue.
 */
  else if (flag == 9) {
    if (heating == true)  {
      pid_temperature();
    }
    if (initial == 0)  {
      lcd.clear();
      lcd.setCursor(2, 0);
      lcd.print("Exp paused");
      lcd.setCursor(2, 1);
      lcd.print("Press to cont");
      lcd.blink();
      initial = 1;
      Serial.println(time_now);
      Serial.println(time_prev);
    }
    if (analogRead(A2) < 500 && (time_now - time_prev) > 500)  {
      tone(speaker_pin, 1000, 50);
      //delay(400);
      x = 0;
      initial = 0;
      flag = 7;
      lcd.noBlink();  
      time_prev = time_now;
      Serial.println("ENCODER BUTTON");
    }
    if (analogRead(A1) > 1000 && (time_now - time_prev) > 500)  {
      tone(speaker_pin, 1000, 50);
      //delay(400);
      x = 0;
      initial = 0;
      flag = 7; 
      lcd.noBlink();
      time_prev = time_now;
      Serial.println("RED BUTTON");
    }
  }
}

/*
  int read_pot_value (int flag) {
  int sensorValue = analogRead(A0);
  float voltage = sensorValue * (15.0 / 1019.0);
  int result = (int)voltage;
  //Serial.println(voltage);
  lcd.setCursor(flag, 0);
  lcd.print(result);
  return result;
  }

  int read_pot_value (int flag,int counter,int new_aState,int new_aLastState) {
  Serial.println("wrong");
  if (digitalRead(outputB) != new_aState) {
    counter ++;
  }
  else  {
    counter --;
  }
  lcd.setCursor(flag, 0);
  lcd.print(counter);
  aState = new_aState;
  return counter;
  }
*/

void stepDown() {
  if (seconds > 0) {
    seconds -= 1;
  }
  else {
    if (minutes > 0) {
      seconds = 59;
      minutes -= 1;
    }
    else {
      if (hours > 0) {
        seconds = 59;
        minutes = 59;
        hours -= 1;
      }
      else {
        trigger();
      }
    }
  }
}

void trigger() {
  digitalWrite(led_pin, LOW);
  lcd.noBlink();
  lcd.clear(); // clears the screen and buffer
  lcd.setCursor(5, 1); // set timer position on lcd for end.
  lcd.print("END");
  tone(speaker_pin, 1000, 1000);
  delay(1000);
  flag = 0;
  exposure = false;
}
void pid_temperature()  {
  if (heating == true)  {
    digitalWrite(13,HIGH);
    Serial.println("PID");
  }
  sensors.requestTemperatures();  // Send the command to get temperature readings 
  Input = sensors.getTempCByIndex(0);
  tempPID.Compute();
  unsigned long now = millis();
  if(now - windowStartTime>WindowSize)
  { //time to shift the Relay Window
    windowStartTime += WindowSize;
  }
  if(Output > now - windowStartTime) digitalWrite(mosfet_pin,HIGH);
  else digitalWrite(mosfet_pin,LOW);
}


