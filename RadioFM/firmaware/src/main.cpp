#include <Arduino.h>
#include <TEA5767.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <Wire.h>
#include <EEPROM.h>

#define SCREEN_WIDTH 128 
#define SCREEN_HEIGHT 64 
#define OLED_RESET     -1 
#define SCREEN_ADDRESS 0x3C 
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Encoder pins
#define ENCODER_SW 2
#define ENCODER_A  3
#define ENCODER_B  4

// Global status flags
#define ST_AUTO    0      // Auto mode (toggled by the push button)
#define ST_GO_UP   2      // Encoder being turned clockwise
#define ST_GO_DOWN 3      // Encoder being turned counterclockwise
#define ST_SEARCH  4      // Radio module is perfoming an automatic search

const int LED = 0;



unsigned char buf[5];
int TEA5767_STEREO;
int signalLevel;
int searchDirection;
int i = 0;

long seconds = 0;
float memo;


TEA5767 Radio;
float frequency = 88.5;
byte status = 0;

void IsrEncoder() {
  
  delay(50);    // Debouncing (for crappy encoders)
  
  if(digitalRead(ENCODER_B) == HIGH){
   
    bitWrite(status, ST_GO_UP, 1);
  
  } else { bitWrite(status, ST_GO_DOWN, 1); }

}

void Teleradio () {
  
  display.clearDisplay();
  display.setCursor(37,0);
  display.setTextSize(1);
  display.print("Radio FM");
  display.setCursor(0,12);
  display.setTextSize(3);
  
  if(frequency < 100) {

    display.print(" ");
    display.print(frequency);
    display.setTextSize(1);
    display.print("MHz");
    display.setCursor(10,40);
    display.setTextSize(1);
    display.print(" ");
    display.println(bitRead(status, ST_AUTO) ? 'A' : 'M');
    display.print(bitRead(status, ST_AUTO) ? "AUTO" : "MANUAL");
    display.setCursor(75,40);
    display.setTextSize(1);
    display.print("Sig: ");
    display.print(String (signalLevel));
    display.print("%");
    display.setCursor(30,50);
    display.setTextSize(1);
    display.print("Memo: ");
    display.print(memo);

    display.display();

  }
        
}

void setup() {
  
  pinMode(ENCODER_SW, INPUT); 
  pinMode(ENCODER_A, INPUT);  
  pinMode(ENCODER_B, INPUT);  
  
  digitalWrite(ENCODER_SW, HIGH);
  digitalWrite(ENCODER_A, HIGH);
  digitalWrite(ENCODER_B, HIGH);  

  attachInterrupt(1, isrEncoder, RISING);

  // Initialize the radio module
  Wire.begin();
  Serial.begin(9600);

  memo = 93.3;
  EEPROM.put(0, memo);
  EEPROM.get(0, memo);
  frequency = memo;
      
  Radio.init();
  Radio.set_frequency(frequency);
  telaradio ();

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.setTextSize(2);
  display.println("Lazzatti");
  display.setCursor(0,16);
  display.println("Rossi");
  display.setCursor(0,32);
  display.setTextSize(1);
  display.println("E.E.S.T. N5");
  display.setCursor(0,56);
  display.println("5to 5ta         2023");
  display.display();
  delay(2000);

}

void loop() {

  seconds++;

  if(seconds == 200) {
    
    if(EEPROM.get(0, memo)!=frequency) {
      
      memo = frequency;
      EEPROM.put(0, memo);
      telaradio ();            
    
    }
  
  } 

  if bitRead(status, ST_AUTO) {digitalWrite(LED, LOW);}
  else {digitalWrite(LED, HIGH);}

  if (digitalRead(ENCODER_SW) == LOW) {
    
    if(bitRead(status, ST_AUTO)) {
       
      bitWrite(status, ST_AUTO, 0);
      
    } else {
      
      bitWrite(status, ST_AUTO, 1);
      delay(100);
      telaradio ();

    }

  }

  if (Radio.read_status(buf) == 1) {
    
    frequency = floor(Radio.frequency_available(buf) / 100000 + .5) / 10;
    TEA5767_STEREO = Radio.stereo(buf);
    signalLevel = (Radio.signal_level(buf) * 100) / 15;
    telaradio ();
    
  }
  
  if(bitRead(status, ST_SEARCH)) {
    
    if(Radio.process_search(buf, searchDirection) == 1) {
    
      bitWrite(status, ST_SEARCH, 0);
      telaradio ();
      seconds = 0;
    
    }
  
  }
  


  // Encoder being turned clockwise (+)
  if(bitRead(status, ST_GO_UP)) {
    
    if(bitRead(status, ST_AUTO) && !bitRead(status, ST_SEARCH)) {
    
      bitWrite(status, ST_SEARCH, 1);
      searchDirection = TEA5767_SEARCH_DIR_UP;
      Radio.search_up(buf);
      delay(50);
    
    } else {
    
      if(frequency < 108.1) {
        
        frequency += 0.1;
        Radio.set_frequency(frequency);
        seconds = 0;
        
      } else  {
          
        frequency = 88;
        Radio.set_frequency(frequency);
        seconds = 0;
      
      }

                   
    }
    
    bitWrite(status, ST_GO_UP, 0);
    telaradio ();  
  
  }

  // Encoder being turned counterclockwise (-)
  if(bitRead(status, ST_GO_DOWN)) {
   
    if(bitRead(status, ST_AUTO) && !bitRead(status, ST_SEARCH)) {
    
      bitWrite(status, ST_SEARCH, 1);
      searchDirection = TEA5767_SEARCH_DIR_DOWN;
      Radio.search_down(buf);
      delay(50);
    
    } else {

      if(frequency > 88) {
        
        frequency -= 0.1;
        Radio.set_frequency(frequency);
        seconds = 0;
      
      } else  {
           
        frequency = 108;
        Radio.set_frequency(frequency);
        seconds = 0;
      
      } 

    }
    
    bitWrite(status, ST_GO_DOWN, 0);
    telaradio ();  
  
  }

}