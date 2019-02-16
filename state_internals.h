class NothingState: public State {
  // ommmmm.......
};

enum Event {
  NONE,
  STOPWATCH_UP, STOPWATCH_DOWN,
  TIMER_UP, TIMER_DOWN,
  GO_UP, GO_DOWN,
  SET_UP, SET_DOWN,
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
      
    case STOPWATCH_UP:
#ifdef DEBUG
      Serial.println("STOPWATCH_UP");
#endif
      currentState.onStopWatchUp();
      break;
    case STOPWATCH_DOWN:
#ifdef DEBUG
      Serial.println("STOPWATCH_DOWN");
#endif
      currentState.onStopWatchDown();
      break;

    case TIMER_UP:
#ifdef DEBUG
      Serial.println("TIMER_UP");
#endif
      currentState.onTimerUp();
      break;
    case TIMER_DOWN:
#ifdef DEBUG
      Serial.println("TIMER_DOWN");
#endif
      currentState.onTimerDown();
      break;

    case GO_UP:
#ifdef DEBUG
      Serial.println("GO_UP");
#endif
      currentState.onGoUp();
      break;
    case GO_DOWN:
#ifdef DEBUG
      Serial.println("GO_DOWN");
#endif
      currentState.onGoDown();
      break;

    case SET_UP:
#ifdef DEBUG
      Serial.println("SET_UP");
#endif
      currentState.onSetUp();
      break;
    case SET_DOWN:
#ifdef DEBUG
      Serial.println("SET_DOWN");
#endif
      currentState.onSetDown();
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


