class NothingState: public State {
  // ommmmm.......
};

enum Event {
  NONE,
  STOPWATCH_RELEASE, STOPWATCH_PRESS,
  TIMER_RELEASE, TIMER_PRESS,
  GO_RELEASE, GO_PRESS,
  SET_RELEASE, SET_PRESS,
  CLOCKWISE, ANTICLOCKWISE,
  TICK
};


State currentState = NothingState();
bool flashState = false;
int ledsToFlash = NoLEDs;
int timeComponentsToFlash = NoFlashing;
int hours = 0;
int minutes = 0;
int seconds = 0;
bool tickEnabled = false;
long tickEnableTime = 0L;

void initialise() {
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

void startTicking() {
  tickEnabled = true;
}

void stopTicking() {
  tickEnabled = false;
}

void beep() {
  // TODO 
}
