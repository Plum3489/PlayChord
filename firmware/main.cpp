#include <Adafruit_TinyUSB.h>
#include <MIDI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Da Oled Display
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const int SDA_PIN = D4;
const int SCL_PIN = D5;

// USB MIDI 
Adafruit_USBD_MIDI usb_midi;
MIDI_CREATE_INSTANCE(Adafruit_USBD_MIDI, usb_midi, MIDI);

// The Pins
const int ROW_PINS[3] = {D0,D1,D2};
const int COL_PINS[4] = {D6,D7,D8,D9};
const int ENCODER_A = D3;
const int ENCODER_B = D10;

// Mapping the button matrix
const int SWITCH_MAP[3][4]={
  {1,2,3,4}, // Row 0 => SW1-4
  {5,6,7,8}, // Row 1 => SW5-8
  {9,10,11,12} // ROW 2 => SW9-12
};

// debouncing input
bool buttonState[13] = {false};
bool lastButtonState[13] = {false};
unsigned long lastDebounceTime[13] = {0};
const unsigned long debounceDelay = 50;

// Rotary encoder
volatile int encoderPos = 0;
int lastEncoderPos = 0;
volatile unsigned long lastEncoderTime = 0;

// Music theory stuff
const char* KEY_NAMES[] = {"C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab", "A", "Bb", "B"};
int currentKey = 0; // 0 = C, 1 = C# ergo for each halfstep +1
bool isMajor = true; // true => Major | false => minor
int octaveShift = 0; // -2 to +2 range
int chordType = 0; // 0=Triad, 1=Seventh, 2=Ninth

const int BASE_OCTAVE = 60; // C4 

// Defining the chords
const int MAJOR_TRIADS[7][3] = {
  {0,4,7}, // I: Majpor (C E G)
  {2,5,9}, // ii Minor (D-F-A)
  {4,7,11}, // iii: Minor (E G B)
  {5,9,12}, // IV: Major (F A C)
  {7,11,14}, // V: Major (G B D)
  {9,12,16}, // vi: Moll (A C E)
  {11,14,17} // vii°: aug (B D F)
};

const int MINOR_TRIADS[7][3] = {
  {0,3,7}, // i: Minor (C Eb G)
  {2,5,8}, // ii°: aug (D F Ab)
  {3,7,10}, // III: Major (Eb G Bb)
  {5,8,12}, // iv: Minor (F Ab C)
  {7,10,14}, // v: Minor (G Bb D)
  {8,12,15}, // VI: Major (Ab C Eb)
  {10,14,17} // VII: Major (Bb D F)
};

// Seventh
const int SEVENTH_INTERVALS[7] = {11,12,14,16,17,19,21};

// Ninth
const int NINTH_INTERVALS[7] = {14,14,16,17,19,21,21};

// Track active notes
bool activeNotes[128] = {false};

void updateDisplay();
void scanMatrix();
void checkEncoder();
void handleButtonPress(int sw);
void handleButtonRelease(int sw);
void playChord(int chordIndex);
void stopChord(int chordIndex);
void stopAllNotes();

// Interrupt when rotary encoder
void encoderISR(){
  unsigned long currentTime = millis();

  // Debounce
  if (currentTime - lastEncoderTime > 5){
    if(digitalRead((ENCODER_B) == HIGH)){
      encoderPos++;
    }else{
      encoderPos--;
    }
    lastEncoderTime = currentTime;
  }
}

void setup(){
  Serial.begin(115200);

  // Wait for serial (for debugging lol)
  // while (!Serial) delay(10);

  Serial.println("XIAO RP2040 Chord Midi Keyboard: PlayChord :P");

  // init USB midi
  usb_midi.setStringDescriptor("PlayChord");

 
  Wire.setSDA(SDA_PIN);
  Wire.setSCL(SCL_PIN);
  Wire.begin();
  
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("OLED init failed!"));
  } else {
    Serial.println("OLED initialized");
  }

  // Splash
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println("PlayChord v1.0");
  display.println("XIAO RP2040");
  display.println("Initializing... :3");
  display.display();
  delay(1500);

  // Setup matrix
  for (int i = 0; i<3; i++){
    pinMode(ROW_PINS[i], OUTPUT),
    digitalWrite(ROW_PINS[i], HIGH);
  }
  for(int i = 0; i<4; i++){
    pinMode(COL_PINS[i], INPUT_PULLUP);
  }

  // Rotary encoder setup
  pinMode(ENCODER_A, INPUT_PULLUP);
  pinMode(ENCODER_B, INPUT_PULLUP);

  // Interrupt 
  attachInterrupt(digitalPinToInterrupt(ENCODER_A), encoderISR, FALLING);

  // start midi
  MIDI.begin(MIDI_CHANNEL_OMNI);

  Serial.println("Setup complete! :P");
  Serial.println("Waiting for USB Midi connection ...");

  updateDisplay();

  
}

void loop(){
  // Scan Matrix
  scanMatrix();

  // Checking da encoder
  checkEncoder();

  static unsigned long lastDisplayUpdate = 0;
  if(millis() - lastDisplayUpdate > 100){
    updateDisplay();
    lastDisplayUpdate = millis();
  }

  delay(5);
}

void scanMatrix(){
  for(int row = 0; row < 3 ; row ++){
    // Activate Row, put on LOW
    digitalWrite(ROW_PINS[row], LOW);
    delayMicroseconds(10);

    // Read all Columns in the row
    for (int col = 0; col<4; col++){
      int switchNum = SWITCH_MAP[row][col];
      bool reading = !digitalRead(COL_PINS[col]);
      
      // Debounce
      if(reading != lastButtonState[switchNum]){
        lastDebounceTime[switchNum] = millis();

      }

      if((millis() - lastDebounceTime[switchNum])> debounceDelay){
        if(reading != buttonState[switchNum]){
          buttonState[switchNum] = reading;

          if(reading){
            handleButtonPress(switchNum);
          }else{
            handleButtonRelease(switchNum);
          }
        
        }
        
      }
      lastButtonState[switchNum] = reading;
    }

    // Deactivate row
    digitalWrite(ROW_PINS[row], HIGH);


  }
}

void handleButtonPress(int sw){
  Serial.print("Button ");
  Serial.print(sw);
  Serial.println(" pressed");

  if(sw>=1 && sw <= 7){
    playChord(sw-1);
  }
  else if(sw==8){
    //SW8: Ocatve Up
    if(octaveShift <2){
      octaveShift++;
      stopAllNotes();
      Serial.print("Pctave shift: ");
      Serial.println(octaveShift);
    }
  }
  else if(sw == 9){
    // SW9 (Encoder button): switch chordtype
    chordType = (chordType +1)%3;
    stopAllNotes();
    Serial.print("Chord type: ");
    Serial.println(chordType == 0 ? "Triad" : (chordType == 1 ? "7th" : "9th"));

  }
   else if (sw == 10) {
    // SW10: octave down
    if (octaveShift > -2) {
      octaveShift--;
      stopAllNotes();
      Serial.print("Octave shift: ");
      Serial.println(octaveShift);
    }
  } 
  else if (sw == 11) {
    // SW11: Major/Minor 
    isMajor = !isMajor;
    stopAllNotes();
    Serial.println(isMajor ? "Major" : "Minor");
  } 
  else if (sw == 12) {
    // SW12: Panic Button all off
    stopAllNotes();
    Serial.println("PANIC - All notes off");
  }
}

void handleButtonRelease(int sw){
  if(sw >= 1 && sw <= 7){
    // Stop the chord
    stopChord(sw -1);
  }
}

void playChord(int chordIndex){
  // choose the right chord from the table
  const int (*triads)[3] = isMajor ? MAJOR_TRIADS : MINOR_TRIADS;

  Serial.print("Playing chord: ");
  Serial.print(chordIndex +1);
  Serial.print(" in ");
  Serial.print(KEY_NAMES[currentKey]);
  Serial.println(isMajor ? " Major" : " Minor");

  // Play the notes for the triad
  for (int i = 0; i < 3; i++) {
    int note = BASE_OCTAVE + currentKey + triads[chordIndex][i] + (octaveShift * 12);
    
    if (note >= 0 && note < 128) {
      MIDI.sendNoteOn(note, 100, 1);
      activeNotes[note] = true;
      Serial.print("  Note ON: ");
      Serial.println(note);
    }
  }
  
  // Add the 7th
  if (chordType >= 1) {
    int note = BASE_OCTAVE + currentKey + SEVENTH_INTERVALS[chordIndex] + (octaveShift * 12);
    
    if (note >= 0 && note < 128) {
      MIDI.sendNoteOn(note, 100, 1);
      activeNotes[note] = true;
      Serial.print("  7th ON: ");
      Serial.println(note);
    }
  }
  
  // Add the 9th
  if (chordType == 2) {
    int note = BASE_OCTAVE + currentKey + NINTH_INTERVALS[chordIndex] + (octaveShift * 12);
    
    if (note >= 0 && note < 128) {
      MIDI.sendNoteOn(note, 90, 1);
      activeNotes[note] = true;
      Serial.print("  9th ON: ");
      Serial.println(note);
    }
  }
}

void stopChord(int chordIndex){
  // Stop all active notes
  for(int i = 0; i < 128; i++){
    if(activeNotes[i]){
      MIDI.sendNoteOff(i,0,1);
      activeNotes[i] = false;
    }
  }
}

void stopAllNotes(){
  // Panic aahhh
  for(int i = 0; i<128; i++){
    if(activeNotes[i]){
      MIDI.sendNoteOff(i,0,1);
      activeNotes[i] = false;
    }
  }
}

void checkEncoder(){
  if(encoderPos != lastEncoderPos){
    int diff = encoderPos - lastEncoderPos;
    currentKey += diff;

    // wrap around between i (0) and B(11)
    while(currentKey < 0) currentKey += 12;
    while(currentKey >= 12) currentKey -= 12;

    lastEncoderPos = encoderPos;
    stopAllNotes();

    Serial.print("Key changed to: ");
    Serial.println(KEY_NAMES[currentKey]);
  }
}

void updateDisplay(){
  display.clearDisplay();

  // ln 1 Key, large
  display.setCursor(0,0);
  display.print(KEY_NAMES[currentKey]);
  display.print(" ");
  display.print(isMajor ? "Maj" : "Min");

  // ln 2 Octave and Type
  display.setTextSize(1);
  display.setCursor(0,18);
  display.print("Oct: ");
  if(octaveShift>=0){display.print("+");}
  display.print(octaveShift);

  display.setCursor(60,18);
  display.print("Type: ");
  if (chordType == 0) display.print("3");
  else if (chordType == 1) display.print("7");
  else display.print("9");
  
  // ln 3 Status
  display.setCursor(0, 26);
  display.print("USB MIDI Ready");
  
  display.display();

}
