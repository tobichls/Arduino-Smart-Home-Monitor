#include <Wire.h>
#include <Adafruit_RGBLCDShield.h>
#include <utility/Adafruit_MCP23017.h>
Adafruit_RGBLCDShield lcd = Adafruit_RGBLCDShield();

#define DEBUG
#ifdef DEBUG
#define debug_print(x) Serial.println(x)
#else
#define debug_print(x) do {} while(0)
#endif

//Define states for the FSM
#define Waiting 0
#define Error 1 
#define Purple 2
#define Display 3
#define OFFdevices 4
#define ONdevices 5

int state = Waiting; // Decare initial state in FSM

// FREERAM
#ifdef __arm__
// should use uinstd.h to define sbrk but Due causes a conflict
extern "C" char* sbrk(int incr);
#else  // __ARM__
extern char *__brkval;
#endif  // __arm__

int freeMemory() {
  char top;
#ifdef __arm__
  return &top - reinterpret_cast<char*>(sbrk(0));
#elif defined(CORE_TEENSY) || (ARDUINO > 103 && ARDUINO != 151)
  return &top - __brkval;
#else  // __arm__
  return __brkval ? &top - __brkval : &top - __malloc_heap_start;
#endif  // __arm__
}

//create Device class
class Device {
  private:
    char ID[4];
    char location[16];
    char type;
    String state;
    int WXYZ;

  public:
    Device(){}
    Device(char ID[4], char location[16], char type, String state, int WXYZ){
      for ( int i = 0; i < 4 ; i++ ){
        this->ID[i] = ID[i];
      }
      for ( int i = 0; i < 16 ; i++ ){
        this->location[i] = location[i];
      }
      this->type = type;
      this->state = state;
      this->WXYZ = WXYZ;
    }
    char* getID(){
      return ID;
    }
    char* getlocation(){
      return location;
    }
    char gettype(){
      return type;
    }
    String getstate(){
      return state;
    }
    int getWXYZ(){
      return WXYZ;
    }
    void setState(String state) {
      this->state = state;
    }
    void setLocation(char* location){
      for ( int i = 0; i < 16 ; i++ ){
        this->location[i] = location[i];
      }
    }
    void setType(char type){
      this->type = type;
    }
    void setPower(int WXYZ){
      this->WXYZ = WXYZ;
    }
    String deviceToString(){
      return "Device ID: "+String(ID) + " Location: "+String(location)+" Type: "+String(type)+" State: "+state+" WXYZ: "+String(WXYZ);
    }

};

void setup() {

  //display
  lcd.begin(16,2);
  lcd.setBacklight(5);
  lcd.setCursor(0, 0);

  lcd.print("");

  Serial.begin(9600);

  //Synchronisation phase
  while (Serial.available()==0){
    Serial.print("Q");
    delay(1000);
  }

  Serial.println("BASIC");
  lcd.setBacklight(7);
  
  //clear input buffer
  while (Serial.available()) {
  Serial.read();
  }

}


bool validSize = true;
bool validSize2 = true;
bool validSize3 = true;
bool validSize4 = true;
bool validType = false; 
bool validONState = true;
bool validOFFState = true;


// create Device array
Device devices[10]; 

// declare variables for LCD display
int currentDevice = 0;
int newDevice = currentDevice;
unsigned long lastUpdateTime = 0;
const unsigned long updateInterval = 500; // update every 500ms
int leftButtonState = HIGH;
int rightButtonState = HIGH;
bool showOnDevicesOnly = false;
bool showOffDevicesOnly = false;

// UDCHARS
const byte upArrow[8] = {
  0b00100,
  0b01110,
  0b10101,
  0b00100,
  0b00100,
  0b00100,
  0b00100,
  0b00000
};
const byte downArrow[8] = {
  0b00000,
  0b00100,
  0b00100,
  0b00100,
  0b00100,
  0b10101,
  0b01110,
  0b00100
};

void loop() {
  // Main phase
  switch (state) {
    case Waiting: 
    {
      static int lastButtonState = LOW;

      // read button input and set new currentDevice value
      int button_state = lcd.readButtons();
      int button = 0;

      if (button_state != lastButtonState){
        lastUpdateTime = millis();
      }

      if ( (millis() - lastUpdateTime) >= updateInterval ){
        if ( button_state & BUTTON_UP ) {
          newDevice = (currentDevice + 1) % 10;
        } else if ( button_state & BUTTON_DOWN ) {
          newDevice = ( currentDevice - 1 + 10 ) % 10;
        } else if ( button_state & BUTTON_SELECT ) {
          debug_print("before");
          state = Purple;
          break;
          debug_print("after");
        } else if ( (button_state & BUTTON_LEFT) && leftButtonState == HIGH ){
          leftButtonState = LOW;
          showOnDevicesOnly = false;
          showOffDevicesOnly = !showOffDevicesOnly; // toggle variable
          if (showOffDevicesOnly){
            state = OFFdevices;
            break;
          } else {
            state = Display;
            break;
          }
        } else if ( (button_state & BUTTON_LEFT) && leftButtonState == LOW ){
          leftButtonState = HIGH;
          state = Display;
          break;
        } else if ( (button_state & BUTTON_RIGHT) && rightButtonState == HIGH ){
          rightButtonState = LOW;
          showOffDevicesOnly = false;
          showOnDevicesOnly = !showOnDevicesOnly; // toggle variable
          if (showOnDevicesOnly){
            state = ONdevices;
            break;
          } else {
            state = Display;
            break;
          }
        } else if ( (button_state & BUTTON_RIGHT) && rightButtonState == LOW ){
          rightButtonState = HIGH;
          state = Display;
          break;
        } else {
          button = 0;
        }
      }

      lastButtonState = button_state;

      // update the currentDevice value when necessary
      if (newDevice != currentDevice){
        currentDevice = newDevice;
      }
      
      if(Serial.available() > 0){
        static const int BUFFER_SIZE = 30; // declare buffer size
        static char buffer[BUFFER_SIZE] = {'\0'}; // create character buffer array
        static int steps = 0; // keeps track of how many times you have looped through the states

        memset( buffer, '\0', BUFFER_SIZE );


        int byteLength = Serial.readBytesUntil('\0', buffer , BUFFER_SIZE);
        char LocationResult[byteLength-8];

        char IDresult[4]= {toupper(buffer[2]),toupper(buffer[3]),toupper(buffer[4]),'\0'};

        switch (toupper(buffer[0])){
          case 'A':
          {
            // verify size of character array is correct
            for ( int i = 23 ; i < 30 ; i++ ){
              if (buffer[i] != '\0' ) {
                validSize = false;
              } else {
                validSize = true;
              }
            }
            // verify input-type entered is correct
            if (toupper(buffer[6]) == 'S' || toupper(buffer[6]) == 'O' || toupper(buffer[6]) == 'T' || toupper(buffer[6]) == 'L' || toupper(buffer[6]) == 'C'){
              validType = true;
            } else {
              validType = false;
            }

            for ( int i = 0 ; i < byteLength-8 ; i++){ //subtract all characters (including the final newline character) except from the Location character
              LocationResult[i] = buffer[i+8];
            }
            LocationResult[byteLength-9] = '\0'; // add null character to the end

            bool matchFound = false; // declare matchFound variable to track if ID has been found in array
            // verify that the whole input is in the correct format
            if ( buffer[1] == '-' && buffer[5] == '-' && buffer[7] == '-' && validSize == true && validType == true && steps <10 ){ 
              for ( int i=0 ; i< 10 ; i++ ){
                if ( strcmp(devices[i].getID(), IDresult) == 0 ){
                  devices[i].setType(buffer[6]);
                  devices[i].setLocation(LocationResult);
                  matchFound = true;
                  break;
                }
              }
              if (!matchFound){
                // create a new Device object depending on the type entered
                switch (buffer[6]) {
                  case 'S':
                    devices[steps] = Device( IDresult , LocationResult , 'S', "OFF", 0 );
                    steps += 1;
                    break;
                  case 'O':
                    devices[steps] = Device( IDresult , LocationResult , 'O', "OFF", 0 );
                    steps += 1;
                    break;
                  case 'T':
                    devices[steps] = Device( IDresult , LocationResult , 'T', "OFF", 9 );
                    steps += 1;
                    break;
                  case 'L':
                    devices[steps] = Device( IDresult , LocationResult , 'L', "OFF", 0 );
                    steps += 1;
                    break;
                  case 'C':
                    devices[steps] = Device( IDresult , LocationResult , 'C', "OFF", 0 );
                    steps += 1;
                    break;
                }
              }
            } else {
              state = Error;
            }
            matchFound = false; // reset matchFound back to false
          }
            break;
          case 'S':
          {
            // verify size of character array is correct
            for ( int i = 10 ; i < 30 ; i++ ){
              if (buffer[i] != '\0' ) {
                validSize2 = false;
              } else {
                validSize2 = true;
              }
            }

            char stateInput[4] = { toupper(buffer[6]),toupper(buffer[7]),toupper(buffer[8]), '\0' };

            // verify whether state entered is "ON" or "OFF"
            if ( strncmp(stateInput, "ON", 2) == 0 ) {
              validONState = true;
              validOFFState = false;
            } else if ( strncmp(stateInput, "OFF", 3) == 0 ){
              validOFFState = true;
              validONState = false;
            } else {
              validONState = false;
              validOFFState = false;
            }
            
            // perform operation if all conditions are met
            if ( buffer[1] == '-' && buffer[5] == '-' && validSize2 == true && ( (validONState == true && validOFFState == false ) || (validONState == false && validOFFState == true) )){
              for (int i = 0; i < 10; i++) {
                if (validONState && strcmp(devices[i].getID(), IDresult) == 0 ){
                  devices[i].setState( String(stateInput) );
                  break;
                } else if (validOFFState && strcmp(devices[i].getID(), IDresult) == 0) {
                  devices[i].setState( String(stateInput) );
                  break;
                }
              }
            } else {
              state = Error;
            }
          }
            break;
          case 'P':
          {
            // verify size of character array is correct
            for ( int i = 9 ; i < 30 ; i++ ){
              if (buffer[i] != 0 ) {
                validSize4 = false;
              } else {
                validSize4 = true;
              }
            }
            // perform operation if all conditions are met
            if ( buffer[1] == '-' && buffer[5] == '-' && validSize4 == true ){
              // set power input array elements
              char pwrResult[4] = {buffer[6],buffer[7],buffer[8],'\0'};
              int powerResult = atoi(pwrResult);

              for ( int i=0 ; i<10 ; i++ ){
                if ( strcmp(devices[i].getID(), IDresult) == 0 ){
                  // set device power output based on type of device
                  switch (devices[i].gettype()) {
                    case 'S':
                      if ( powerResult >= 0 && powerResult <= 100 ){
                        devices[i].setPower(powerResult);
                      }
                      break;
                    case 'L':
                      if ( powerResult >= 0 && powerResult <= 100 ){
                        devices[i].setPower(powerResult);
                      }
                      break;
                    case 'T':
                      if ( powerResult >= 9 && powerResult <= 32 ){
                        devices[i].setPower(powerResult);
                      }
                      break;
                  }
                }
              }
            } else {
              state = Error;
            } 
          }   
            break;
          case 'R':
          {
            // verify size of character array is correct
            for ( int i = 5 ; i < 30 ; i++ ){
              if (buffer[i] != '\0' ) {
                validSize3 = false;
              } else{
                validSize3 = true;
              }
            }
            // "delete" element if all conditions are met
            if ( buffer[1] == '-' && validSize3 == true ){
              for ( int i=0 ; i<steps ; i++ ){ 
                if ( strcmp(devices[i].getID(), IDresult) == 0 ){ // find object with inputted ID
                  for (int j = i; j<steps ; j++){
                    devices[j] = devices[j+1]; //shift all subsequent objects in the array one position to the left
                    }
                  devices[steps] = Device(); // set the last significant object in the array to the default constructor values
                  steps -= 1;
                  }
                }
            } else {
              state = Error;
            }
          }
        }
      }
      if ( (millis() - lastUpdateTime) >= updateInterval ){
        lastUpdateTime = millis();
        state = Display; }
    }
      break;
    case Error: 
    {
      Serial.println("ERROR: followed by the none conforming line string");
      state = Waiting;
    }
    break;
    case Purple: 
    {
      debug_print("in purple");
      // set backlight to purple display student ID
      lcd.clear();
      lcd.setBacklight(5);
      lcd.print("F227248");

      static int i;

      byte *ptr = (byte*)malloc(i);
      if (ptr == NULL)
      {
        Serial.println("malloc failed");
      }
      else
      {
        Serial.println("malloc succeeded");
        
        Serial.println(freeMemory());
        free(ptr);
      }
      i++;
      delay(500);
      state = Waiting;
      

    }
    break;
    case Display:
    {
      // UDCHARS
      lcd.createChar(0, upArrow);
      lcd.createChar(1, downArrow);
      lcd.clear();

      if ( (showOnDevicesOnly == false) && (showOffDevicesOnly == false) ){
        lcd.setCursor(0, 0);
        lcd.write(byte(0)); // print upArrow
        lcd.setCursor(1, 0);
        lcd.print(devices[currentDevice].getID());
        lcd.setCursor(5,0);
        lcd.print(devices[currentDevice].getlocation());
        // second line
        lcd.setCursor(0, 1);
        lcd.write(byte(1)); // print downArrow
        lcd.setCursor(1,1);
        lcd.print(devices[currentDevice].gettype());
        lcd.setCursor(3,1);
        lcd.print(devices[currentDevice].getstate());
        // set lcd backlight to green if device state is on and yellow if it is off
        if ( strncmp(devices[currentDevice].getstate().c_str(), "ON", 2) == 0 ){
          lcd.setBacklight(2);
        } else if ( strncmp(devices[currentDevice].getstate().c_str(), "OFF", 3) == 0 ){
          lcd.setBacklight(3);
        }
        lcd.setCursor(7,1);
        lcd.print(devices[currentDevice].getWXYZ());
        if ( (millis() - lastUpdateTime) >= updateInterval ){
          lastUpdateTime = millis();
          state = Waiting; }
      }
      for ( int i=0 ; i<10 ; i++ ){
        if ( showOnDevicesOnly && strncmp(devices[i].getstate().c_str(), "ON", 2) == 0 ){
          if ( (millis() - lastUpdateTime) >= updateInterval ){
            lastUpdateTime = millis();
            state = ONdevices; }
        } else if ( showOffDevicesOnly && strncmp(devices[i].getstate().c_str(), "OFF", 3) == 0 ){
          if ( (millis() - lastUpdateTime) >= updateInterval ){
            lastUpdateTime = millis();
            state = OFFdevices; }
        }
      }
    }
    break;
    case OFFdevices: // HCI
    {
      boolean foundOffDevices = false;
      for (int i = 0; i< 10; i++){
        if ( strncmp(devices[i].getstate().c_str(), "OFF", 3) == 0 ){
          lcd.clear();
          // UDCHARS
          lcd.createChar(0, upArrow);
          lcd.createChar(1, downArrow);
          // first line
          lcd.setCursor(0, 0);
          lcd.write(byte(0)); // print upArrow
          lcd.setCursor(1, 0);
          lcd.print(devices[i].getID());
          lcd.setCursor(5,0);
          lcd.print(devices[i].getlocation());
          // second line
          lcd.setCursor(0, 1);
          lcd.write(byte(1)); // print downArrow
          lcd.setCursor(1,1);
          lcd.print(devices[i].gettype());
          lcd.setCursor(3,1);
          lcd.print(devices[i].getstate());
          lcd.setCursor(7,1);
          lcd.print(devices[i].getWXYZ());
          foundOffDevices = true;
        }
      }
      if (!foundOffDevices){
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("NOTHING'S OFF");
      }
      if ( (millis() - lastUpdateTime) >= updateInterval ){
        lastUpdateTime = millis();
        state = Waiting; }
    }
    break;
    case ONdevices: // HCI
    {
      boolean foundOnDevices = false;
      for (int i = 0; i< 10; i++){
        if ( strncmp(devices[i].getstate().c_str(), "ON", 2) == 0 ){
          lcd.clear();
          // UDCHARS
          lcd.createChar(0, upArrow);
          lcd.createChar(1, downArrow);
          // first line
          lcd.setCursor(0, 0);
          lcd.write(byte(0)); // print upArrow
          lcd.setCursor(1, 0);
          lcd.print(devices[i].getID());
          lcd.setCursor(5,0);
          lcd.print(devices[i].getlocation());
          // second line
          lcd.setCursor(0, 1);
          lcd.write(byte(1)); // print downArrow
          lcd.setCursor(1,1);
          lcd.print(devices[i].gettype());
          lcd.setCursor(3,1);
          lcd.print(devices[i].getstate());
          lcd.setCursor(7,1);
          lcd.print(devices[i].getWXYZ());
          foundOnDevices = true;
        }
      }
      if (!foundOnDevices){
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("NOTHING'S ON");
      }
      if ( (millis() - lastUpdateTime) >= updateInterval ){
        lastUpdateTime = millis();
        state = Waiting; }
    }
    break;
    }
}
