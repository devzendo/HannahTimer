// Internals of the state and hardware handler. These are not the droids you are looking for..
// Look in state_api.h, and reconsider your life choices...

// Acknowledgements: Jack Ganssle for his article on firmware debouncing
// http://www.ganssle.com/debouncing-pt2.htm
//
// Rotary encoder reading from an article at
// http://makeatronics.blogspot.co.uk/2013/02/efficiently-reading-quadrature-with.html

// INPUTS ON PINS --------------------------------------------------------------------------------------------------------------
                           //      76543210
const int stopwatchIn = 4; // PIND    x  --
const int stopwatchInBit = 0x10;
const int timerIn = 5;     // PIND   x   --
const int timerInBit = 0x20;
const int goIn = 6;        // PIND  x    --
const int goInBit = 0x40;
const int setIn = 7;       // PIND x     --
const int setInBit = 0x80;


const int encRIn = 8;      // PINB --     x // CLOCKWISE CLK
const int encRInBit = 0x01;
                           //      76543210
const int encLIn = 9;      // PINB --    x  // ANTICLOCKWISE DT
const int encLInBit = 0x02;

// TODO going to have to rewire the encoder:
// should have PINB:
// -     -     13    12    11    10    9     8    
//             LED   MOSI  LED   LED   ENC   ENC
//             SCK         DIN   CS    L     R
// should have PIND:
// 7     6     5     4     3     2     -     -
// SET   GO    TIMER STPWC

// OUTPUTS ON PINS -------------------------------------------------------------------------------------------------------------

const int ledCs = 10; // a.k.a. "LOAD" or "SS"
const int ledDin = 11; // "MISO"
// probably avoid 12 MOSI
const int ledClk = 13; // "SCK"

// Port Manipulation
//    B (digital pin 8 to 13)
//    The two high bits (6 & 7) map to the crystal pins and are not usable 
//    C (analog input pins)
//    D (digital pins 0 to 7) 
//    The two low bits (0 & 1) are for serial comms and shouldn't be changed.

inline uint16_t readPins() {
    return (PIND & 0xF0) | (PINB & 0x03);
}

// input change detection, called in loop() for test harness, or in ISR
volatile uint16_t oldPins;
volatile uint16_t newPins;
volatile uint16_t initialPins;

// EVENT MANAGEMENT ------------------------------------------------------------------------------------------------------------

// We have several events that we can react to....
enum Event {
  NONE,
  STOPWATCH_RELEASE, STOPWATCH_PRESS,
  TIMER_RELEASE, TIMER_PRESS,
  GO_RELEASE, GO_PRESS,
  SET_RELEASE, SET_PRESS,
  CLOCKWISE, ANTICLOCKWISE,
  TICK, FLASH
};

// Events detected by the interrupt handler are enqueued on this FIFO queue, 
// which is read by processNextEvent (in non-interrupt time).
defineFifo(eventFifo, Event, 100)

// Enqueue an event on the FIFO queue.
static char zut[20];
inline void eventOccurred(Event eventCode) {
#ifdef DEBUGEVENT
    sprintf(zut, ">ev:0x%04X", eventCode);
    Serial.println(zut);
#endif    
    if (!eventFifo.put(&eventCode)) {
        Serial.println("FIFO overrun");
    }
}

// STATE MANAGEMENT ------------------------------------------------------------------------------------------------------------

// The state machine must have a starting state - this one does nothing.
class NothingState: public State {
  // ommmmm.......
};

// State machine management
State nothing = NothingState();
State *currentState = &nothing;

// TICK STATE MANAGEMENT -------------------------------------------------------------------------------------------------------

// Flash... ah-ahhhhhh!
volatile bool flashState = false;
volatile long lastSecondTickMillis = 0;
int ledsToFlash = NoLEDs;
int timeComponentsToFlash = NoFlashing;

// Are we ticking (sending TICK events)?
volatile bool tickEnabled = false;

// TIME MANAGEMENT -------------------------------------------------------------------------------------------------------------

int hours = 0;
int minutes = 0;
int seconds = 0;

// 7-SEGMENT LED CONTROL -------------------------------------------------------------------------------------------------------

HCMAX7219 HCMAX7219(ledCs);

// DEBOUNCE CONTROL ------------------------------------------------------------------------------------------------------------

// Debounce logic based on code by Jack Ganssle.
const uint8_t checkMsec = 5;     // Read hardware every so many milliseconds
const uint8_t pressMsec = 10;    // Stable time before registering pressed
const uint8_t releaseMsec = 100; // Stable time before registering released

class Debouncer {
public:
    Debouncer() {
        debouncedKeyPress = true; // If using internal pullups, the initial state is true.
    }
    // called every checkMsec.
    // The key state is +5v=released, 0v=pressed; there are pullup resistors.
    void debounce(bool rawPinState) {
        keyChanged = false;
        keyReleased = debouncedKeyPress;
        if (rawPinState == debouncedKeyPress) {
            // Set the timer which allows a change from current state
            resetTimer();
        } else {
            if (--count == 0) {
                // key has changed - wait for new state to become stable
                debouncedKeyPress = rawPinState;
                keyChanged = true;
                keyReleased = debouncedKeyPress;
                // And reset the timer
                resetTimer();
            }
        }
    }

    // Signals the key has changed from open to closed, or the reverse.
    bool keyChanged;
    // The current debounced state of the key.
    bool keyReleased;

private:
    void resetTimer() {
        if (debouncedKeyPress) {
            count = releaseMsec / checkMsec;
        } else {
            count = pressMsec / checkMsec;
        }
    }

    uint8_t count = releaseMsec / checkMsec;
    // This holds the debounced state of the key.
    bool debouncedKeyPress = false; 
};

Debouncer stopwatchDebounce;
Debouncer timerDebounce;
Debouncer setDebounce;
Debouncer goDebounce;

// INTERRUPT CONTROL -----------------------------------------------------------------------------------------------------------

static int8_t encoderLookupTable[] = {
    0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0
};
static uint8_t encoderValue = 0;

// input change to event conversion, called in loop or ISR
void decodePinsAndEnqueueEvents(uint16_t rawPins) {
    // Do the decoding, and call eventOccurred with each event found...
    // Need to debounce buttons, not so bad on encoder button.
    stopwatchDebounce.debounce(rawPins & stopwatchInBit);
    if (stopwatchDebounce.keyChanged) {
        eventOccurred(stopwatchDebounce.keyReleased? STOPWATCH_RELEASE : STOPWATCH_PRESS);
    }

    timerDebounce.debounce(rawPins & timerInBit);
    if (timerDebounce.keyChanged) {
        eventOccurred(timerDebounce.keyReleased? TIMER_RELEASE : TIMER_PRESS);
    }

    goDebounce.debounce(rawPins & goInBit);
    if (goDebounce.keyChanged) {
        eventOccurred(goDebounce.keyReleased? GO_RELEASE : GO_PRESS);
    }

    setDebounce.debounce(rawPins & setInBit);
    if (setDebounce.keyChanged) {
        eventOccurred(setDebounce.keyReleased? SET_RELEASE : SET_PRESS);
    }

    // rotary encoder....
    encoderValue = encoderValue << 2;
    encoderValue = encoderValue | ((rawPins & (encLInBit | encRInBit)));
    switch (encoderLookupTable[encoderValue & 0b1111]) {
        case 0:
            break;
        case 1:
            eventOccurred(CLOCKWISE);
            break;
        case -1:
            eventOccurred(ANTICLOCKWISE);
            break;
    }
}

// Called by the interrupt handler on each second (after the second timer has been reset)
void secondTick() {
  flashState = ! flashState;

  // Handle flashing of LEDs indicated by ledsToFlash and timeComponentsToFlash outside
  // the interrupt handler.
  if (ledsToFlash != NoLEDs || timeComponentsToFlash != NoFlashing) {
    eventOccurred(FLASH);
  }
  
  // Send a tick event if we are ticking?
  if (tickEnabled) {
    eventOccurred(TICK);
  }
}

void tobin(char *buf, int x) {
  buf[0] = ((x & 0x80) == 0x80) ? '1' : '0';
  buf[1] = ((x & 0x40) == 0x40) ? '1' : '0';
  buf[2] = ((x & 0x20) == 0x20) ? '1' : '0';
  buf[3] = ((x & 0x10) == 0x10) ? '1' : '0';
  buf[4] = ((x & 0x08) == 0x08) ? '1' : '0';
  buf[5] = ((x & 0x04) == 0x04) ? '1' : '0';
  buf[6] = ((x & 0x02) == 0x02) ? '1' : '0';
  buf[7] = ((x & 0x01) == 0x01) ? '1' : '0';
  buf[8] = '\0';
}

void interruptHandler(void) {
  // Process any button/encoder state transitions...
  newPins = readPins();
#ifdef DEBUGPINS
  if (newPins != oldPins) {
    char out[10];
    tobin(out, newPins);
    Serial.println(out);
    HCMAX7219.print7Seg(out, 8);
    HCMAX7219.Refresh();
  }
#endif
  decodePinsAndEnqueueEvents(newPins);
  oldPins = newPins;
  
  // Has a second passed since last interrupt?
  long currentMillis = millis();  
  if (abs(currentMillis - lastSecondTickMillis) > 1000) {
#ifdef DEBUGTICK
    Serial.println("tick");
#endif
    secondTick();
    lastSecondTickMillis = currentMillis;
  }
}

void resetSecondTimerToNow() {
  lastSecondTickMillis = millis();  
}

// Initialise all hardware, interrupt handler.
void initialise() {
  // The buttons...
  pinMode(stopwatchIn, INPUT_PULLUP);
  pinMode(timerIn, INPUT_PULLUP);
  pinMode(goIn, INPUT_PULLUP);
  
  pinMode(setIn, INPUT_PULLUP);
  pinMode(encRIn, INPUT_PULLUP);
  pinMode(encLIn, INPUT_PULLUP);

  initialPins = oldPins = newPins = readPins();

  // 7-Segment LED...
  HCMAX7219.Init();
  HCMAX7219.Clear();
  
  // Interrupt handler
  resetSecondTimerToNow();
  Timer1.initialize(20000); // Every 1/200th of a second (interrupt every 5 milliseconds).
  Timer1.attachInterrupt(interruptHandler);
}

void displayTime() {
static char out[9];

  if (timeComponentsToFlash == NoFlashing || (!flashState)) {
    sprintf(out, "%02d:%02d:%02d", hours, minutes, seconds);

  } else {

    // Precondition: some components needs to flash and flashState is true (flashing components need to be blank)
    out[0] = '\0';
    // 0123456789 <- characters in the out array
    // 0          <- here's the null at end-of-string
    if ((timeComponentsToFlash & HoursFlashing) == HoursFlashing) {
      strcat(out, "  :");
    } else {
      // 0123456789
      // 0
      sprintf(out, "%02d:", hours);
    }
    
    if ((timeComponentsToFlash & MinutesFlashing) == MinutesFlashing) {
      strcat(out, "  :");
    } else {
      // 0123456789
      // dd:0
      sprintf(out + 3, "%02d:", minutes);
    }

    if ((timeComponentsToFlash & SecondsFlashing) == SecondsFlashing) {
      strcat(out, "  ");
    } else {
      // 0123456789
      // dd:dd:0
      sprintf(out + 6, "%02d", seconds);
    }
    // 0123456789
    // dd:dd:dd0
  }
  
  HCMAX7219.print7Seg(out, 8);
  HCMAX7219.Refresh();
}

// Precondition: ledsToFlash or timeComponentsToFlash are indicating something needs to flash
void processFlash() {
  if (!flashState) {
    HCMAX7219.Clear();
    HCMAX7219.Refresh();
    return;
  }
  if (ledsToFlash != NoLEDs) {
    // TODO set LEDs indicated by ledsToFlash flashing
    char out[10];
    out[0]=' ';
    if (ledsToFlash & StopWatchLED == StopWatchLED) {
      out[1] = 39;
    } else if (ledsToFlash & GoLED == GoLED) {
      out[1] = 27;
    } else {
      out[1] = ' ';
    }
    out[2] = out[3] = out[4] = out[5] = ' ';
    if (ledsToFlash & TimerLED == TimerLED) {
      out[6] = 39;
    } else if (ledsToFlash & SetLED == SetLED) {
      out[6] = 27;
    } else {
      out[6] = ' ';
    }
    out[7] = '\0';
    HCMAX7219.print7Seg(out, 8);
    HCMAX7219.Refresh();
  }
  if (timeComponentsToFlash != NoFlashing) {
    displayTime(); // which takes care of the flashing for us
  }
}

void processEvent(const Event e) {
  switch(e) {
    case NONE:
      break;
    
    case FLASH:
#ifdef DEBUG
      Serial.println("FLASH");
#endif
      // Sent only if there is something to flash..
      processFlash();
      break;
      
    case STOPWATCH_RELEASE:
#ifdef DEBUG
      Serial.println("STOPWATCH_RELEASE");
#endif
      currentState->onStopWatchRelease();
      break;
    case STOPWATCH_PRESS:
#ifdef DEBUG
      Serial.println("STOPWATCH_PRESS");
#endif
      currentState->onStopWatchPress();
      break;

    case TIMER_RELEASE:
#ifdef DEBUG
      Serial.println("TIMER_RELEASE");
#endif
      currentState->onTimerRelease();
      break;
    case TIMER_PRESS:
#ifdef DEBUG
      Serial.println("TIMER_PRESS");
#endif
      currentState->onTimerPress();
      break;

    case GO_RELEASE:
#ifdef DEBUG
      Serial.println("GO_RELEASE");
#endif
      currentState->onGoRelease();
      break;
    case GO_PRESS:
#ifdef DEBUG
      Serial.println("GO_PRESS");
#endif
      currentState->onGoPress();
      break;

    case SET_RELEASE:
#ifdef DEBUG
      Serial.println("SET_RELEASE");
#endif
      currentState->onSetRelease();
      break;
    case SET_PRESS:
#ifdef DEBUG
      Serial.println("SET_PRESS");
#endif
      currentState->onSetPress();
      break;

    case CLOCKWISE:
#ifdef DEBUG
      Serial.println("CLOCKWISE");
#endif
      currentState->onClockwise();
      break;
    case ANTICLOCKWISE:
#ifdef DEBUG
      Serial.println("ANTICLOCKWISE");
#endif
      currentState->onAntiClockwise();
      break;
      
    case TICK:
#ifdef DEBUG
      Serial.println("TICK");
#endif
      currentState->onTick();
      break;    
  }
}

void setNextState(State *newState) {
  char buf[100];
  sprintf(buf, "changing state. old state %08x new state %08x", currentState, newState);
  Serial.println(buf);
  currentState->exit();
  currentState = newState;
  currentState->enter();
}

void processNextEvent() {
  // If there any events on the FIFO queue that were pushed by the ISR, process them here in the main non-interrupt loop.
  Event event;
  if (eventFifo.get(&event)) {
    //char buf[80];
    //sprintf(buf, "Got an event 0x%04x in processNextEvent", event);
    //Serial.println(buf);
    processEvent(event);
  }
}

void flashLEDs(const int leds) {
  ledsToFlash = leds;
}

void resetTime() {
  hours = minutes = seconds = 0;
  displayTime();
}

void tickTimeUp() {
  seconds++;
  if (seconds == 60) {
    seconds = 0;
    minutes++;
    if (minutes == 60) {
      minutes = 0;
      hours++;
      if (hours == 24) {
        hours = 0;
      }
    }
  }
  displayTime();
}

bool tickTimeDown() {
  seconds--;
  if (seconds == -1) {
    seconds = 59;
    minutes--;
    if (minutes == -1) {
      minutes = 59;
      hours--;
      if (hours == -1) {
        hours = 23;
      }
    }
  }
  displayTime();
  return (seconds == 0 && minutes == 0 && hours == 0);
}

void flashTimeComponent(const int component) {
  timeComponentsToFlash = component;
  resetSecondTimerToNow();
  
}

void incHours() {
  if (hours < 23) {
    hours++;
    displayTime();
  }
}

void decHours() {
  if (hours > 0) {
    hours--;
    displayTime();
  }
}

int getHours() {
  return hours;
}

void incMinutes() {
  if (minutes < 59) {
    minutes++;
    displayTime();
  }
}

void decMinutes() {
  if (minutes > 0) {
    minutes--;
    displayTime();
  }
}

int getMinutes() {
  return minutes;
}

void incSeconds() {
  if (seconds < 59) {
    seconds++;
    displayTime();
  }
}

void decSeconds() {
  if (seconds > 0) {
    seconds++;
    displayTime();
  }
}

int getSeconds() {
  return seconds;
}

void startTicking() {
  tickEnabled = true;
  resetSecondTimerToNow();
}

void stopTicking() {
  tickEnabled = false;
}

void beep() {
  // TODO 
}
