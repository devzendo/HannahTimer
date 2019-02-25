// Internals of the state and hardware handler. These are not the droids you are looking for..
// Look in state_api.h, and reconsider your life choices...

// Acknowledgements: Jack Ganssle for his article on firmware debouncing
// http://www.ganssle.com/debouncing-pt2.htm
//
// Rotary encoder reading from an article at
// http://makeatronics.blogspot.co.uk/2013/02/efficiently-reading-quadrature-with.html

// EVENT MANAGEMENT ------------------------------------------------------------------------------------------------------------

// We have several events that we can react to....
enum Event {
  NONE,
  STOPWATCH_RELEASE, STOPWATCH_PRESS,
  TIMER_RELEASE, TIMER_PRESS,
  GO_RELEASE, GO_PRESS,
  SET_RELEASE, SET_PRESS,
  CLOCKWISE, ANTICLOCKWISE,
  TICK
};

// Events detected by the interrupt handler are enqueued on this FIFO queue, 
// which is read by processNextEvent (in non-interrupt time).
defineFifo(eventFifo, Event, 100)

// Enqueue an event on the FIFO queue.
static char zut[20];
inline void eventOccurred(Event eventCode) {
    sprintf(zut, ">ev:0x%04X", eventCode);
    Serial.println(zut);
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
State currentState = nothing;

// TICK STATE MANAGEMENT -------------------------------------------------------------------------------------------------------

// Flash... ah-ahhhhhh!
bool flashState = false;
int ledsToFlash = NoLEDs;
int timeComponentsToFlash = NoFlashing;

// Are we ticking (sending TICK events)?
bool tickEnabled = false;
long tickEnableTime = 0L;

// TIME MANAGEMENT -------------------------------------------------------------------------------------------------------------

int hours = 0;
int minutes = 0;
int seconds = 0;

// DEBOUNCE CONTROL ------------------------------------------------------------------------------------------------------------

// Debounce logic based on code by Jack Ganssle.
const uint8_t checkMsec = 4;     // Read hardware every so many milliseconds
const uint8_t pressMsec = 10;    // Stable time before registering pressed
const uint8_t releaseMsec = 100; // Stable time before registering released

class Debouncer {
public:
    // called every checkMsec.
    // The key state is +5v=released, 0v=pressed; there are pullup resistors.
    void debounce(bool rawPinState) {
        bool rawState;
        keyChanged = false;
        keyReleased = debouncedKeyPress;
        if (rawPinState == debouncedKeyPress) {
            // Set the timer which allows a change from current state
            resetTimer();
        } else {
            // key has changed - wait for new state to become stable
            debouncedKeyPress = rawPinState;
            keyChanged = true;
            keyReleased = debouncedKeyPress;
            // And reset the timer
            resetTimer();
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

// ENCODER CONTROL -------------------------------------------------------------------------------------------------------------

static int8_t encoderLookupTable[] = {
    0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0
};
static uint8_t encoderValue = 0;


// Initialise all hardware, interrupt handler.
void initialise() {
  // The buttons...
  
  // TODO set up an interrupt handler on a timer, to read hardware and enqueue events
}

void interruptHandler() {
  // TODO read hardware
  // TODO enqueue events
  
  // TODO has a second passed since last call?
  // secondTick();
  
  // Are we ticking? Has a second passed since last call?
  if (tickEnabled && false /* TODO second passed */) {
    // TODO put a TICK on the event queue
  }
}

// Called by the interrupt handler on each second
void secondTick() {
  flashState = ! flashState;
  // TODO set LEDs indicated by ledsToFlash flashing
  // TODO set time components indicated by timeComponentsToFlash flashing
  
}

void processEvent(const Event e) {
  switch(e) {
    case NONE:
      break;
      
    case STOPWATCH_RELEASE:
#ifdef DEBUG
      Serial.println("STOPWATCH_RELEASE");
#endif
      currentState.onStopWatchRelease();
      break;
    case STOPWATCH_PRESS:
#ifdef DEBUG
      Serial.println("STOPWATCH_PRESS");
#endif
      currentState.onStopWatchPress();
      break;

    case TIMER_RELEASE:
#ifdef DEBUG
      Serial.println("TIMER_RELEASE");
#endif
      currentState.onTimerRelease();
      break;
    case TIMER_PRESS:
#ifdef DEBUG
      Serial.println("TIMER_PRESS");
#endif
      currentState.onTimerPress();
      break;

    case GO_RELEASE:
#ifdef DEBUG
      Serial.println("GO_RELEASE");
#endif
      currentState.onGoRelease();
      break;
    case GO_PRESS:
#ifdef DEBUG
      Serial.println("GO_PRESS");
#endif
      currentState.onGoPress();
      break;

    case SET_RELEASE:
#ifdef DEBUG
      Serial.println("SET_RELEASE");
#endif
      currentState.onSetRelease();
      break;
    case SET_PRESS:
#ifdef DEBUG
      Serial.println("SET_PRESS");
#endif
      currentState.onSetPress();
      break;

    case CLOCKWISE:
#ifdef DEBUG
      Serial.println("CLOCKWISE");
#endif
      currentState.onClockwise();
      break;
    case ANTICLOCKWISE:
#ifdef DEBUG
      Serial.println("ANTICLOCKWISE");
#endif
      currentState.onAntiClockwise();
      break;
      
    case TICK:
#ifdef DEBUG
      Serial.println("TICK");
#endif
      currentState.onTick();
      break;
    
  }
}

void setNextState(State &newState) {
  currentState.exit();
  currentState = newState;
  currentState.enter();
}

void processNextEvent() {
  // TODO get from queue
  // TODO call processEvent(e)
}

void flashLEDs(const int leds) {
  ledsToFlash = leds;
}

void displayTime() {
static char out[9];
  sprintf(out, "%02d:%02d:%02d", hours, minutes, seconds);
  // TODO display on LEDs  
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
}

void stopTicking() {
  tickEnabled = false;
}

void beep() {
  // TODO 
}
