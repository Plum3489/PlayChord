# PlayChord - XIAO RP2040 MIDI Chord Controller
PlayChord is a USB MIDI chord controller built around the Seeed XIAO RP2040 microcontroller. It allows musicians and producers to play full chords with a single button press :]

### Key Features
- **7 Chord Buttons** - Play diatonic chords (I, ii, iii, IV, V, vi, vii°) in any key
- **Rotary Encoder** - Switch between 12 musical keys (C through B)
- **Chord Type Selection** - Toggle between Triads (3-note), 7th chords, and 9th chords
- **Octave Control** - Shift the entire chord range up/down by +-2 octaves
- **Major/Minor Mode** - Switch between Major and Natural Minor scales
- **OLED Display** - Display of current key, mode, and settings
- **USB MIDI** - Direct USB-C connection to DAW/music software

<img width="408" height="702" alt="image" src="https://github.com/user-attachments/assets/cfd08f54-1a2b-4d5e-8c52-4448078fb2fe" />

## Hardware Specification

### Bill of Materials (BOM)
Qty	Item	Notes	Price
1	PCB	PCB 1 (unsoldered)	
1	Seeed XIAO RP2040		
12	Through-hole 1N4148 Diodes		
11	MX-Style switches		
1	EC11 Rotary encoder		
1	0.91 inch OLED display	Pin order must be GND-VCC-SCL-SDA. Ensure your PCB matches.	
11	White blank DSA keycaps		
3	M3×16 mm screws		
3	M3×5×4 mm heat-set inserts		



### PCB Specifications
PCB Dimensions: 80mm × 91.835mm
Layer Count: 2 

<img width="495" height="548" alt="pcb" src="https://github.com/user-attachments/assets/7c3643be-084d-4e2d-b792-67fb7c965b41" />

<img width="610" height="637" alt="schem" src="https://github.com/user-attachments/assets/c63c4083-63f9-43b0-ae04-3fd1517cb17c" />


### Case Specifications
Outer Dimensions: 100mm × 111.84mm × 16.9mm

<img width="855" height="578" alt="case" src="https://github.com/user-attachments/assets/9c9d6566-c1c8-4040-b43b-691713e7c0a8" />


## Software Architecture

### Stack

- **Framework:** Arduino (PlatformIO compatible)
- **USB Communication:** Adafruit TinyUSB Library (MIDI over USB)
- **Display Driver:** Adafruit SSD1306 (I2C)
- **MIDI Library:** fortyseveneffects/MIDI Library v5.0.2

### How it works ^^

#### 1. **Matrix Scanning** (`scanMatrix()`)

Purpose: Read button states from 3×4 switch matrix
Method:  Row-by-row activation with column polling
Timing:  Scanned every 5ms in main loop

Process:
1. Set Row pin to LOW (activate)
2. Read all 4 Column pins (INPUT_PULLUP)
3. Debounce each switch (50ms delay)
4. Trigger press/release events
5. Set Row pin to HIGH (deactivate)
6. Move to next row

Rotary Encoder
Purpose: Change musical key via rotary encoder
Method:  Hardware interrupt on encoder Pin A
Timing:  Interrupt-driven (no polling)

Process:
1. Encoder Pin A triggers FALLING interrupt
2. Read Pin B to determine direction
3. Increment/Decrement encoderPos
4. Debounce: Ignore interrupts < 5ms apart
5. In main loop: Convert position delta to key change
6. Wrap around between C (0) and B (11)

Yeah!

##Chord Gen!!
Purpose: Convert button press -> MIDI note-on events
Input:   Chord index (0-6, representing I through vii°)

Process:
1. Select chord table (MAJOR_TRIADS or MINOR_TRIADS)
2. Calculate 3 base notes:
   - note = BASE_OCTAVE + currentKey + interval + (octaveShift × 12)
   - BASE_OCTAVE = 60 (C4 = middle C in MIDI)
   - currentKey = 0-11 (semitone offset)
   - interval = chord tone (0, 4, 7 for C Major triad)
   - octaveShift = -2 to +2 (user-controlled)
3. Send MIDI Note-On (velocity=100) for each note
4. Track active notes in activeNotes[128] array
5. If chordType ≥ 1: Add 7th tone
6. If chordType = 2: Add 9th tone

##Button functionality 
Button	    Mode	    Action
------------------------------
SW1-7	    Any	        Play diatonic chord I-vii°
SW8	    Release	     Octave shift +1 (max +2)
SW9	    Press	     Toggle chord type: Triad → 7th → 9th
SW10	    Release	     Octave shift -1 (min -2)
SW11	    Release      Toggle Major ↔ Minor mode
SW12	    Press	     PANIC: All notes off




# Flashing with PlatformIO
1. Open project folder in VS Code
2. Click PlatformIO Home icon (alien head)
3. Click "Open Project"
4. Select your project folder
5. Verify board is set to: seeed_xiao_rp2040
6. Connect XIAO RP2040 via USB-C
7. Click: PlatformIO -> Upload
8. Wait for "Upload successful"

