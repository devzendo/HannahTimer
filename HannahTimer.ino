#include <SCoop.h>

#include "state_api.h"
#include "state_internals.h" // do not stare into the abyss, lest it stare back into you

// Forward instance declarations
State &chooseMode = nothing;
State &stopWatch = nothing;
State &timer = nothing;

// States 
class ChooseMode: public State {
  void enter() {
    flashLEDs(StopWatchLED | TimerLED);
  }

  void exit() {
    flashLEDs(NoLEDs);
  }
  
  void onStopWatchRelease() {
    setNextState(stopWatch);
  }
  
  void onTimerRelease() {
    setNextState(timer);
  }
};

class StopWatch: public State {
  void enter() {
    resetTime();
  }
  
  void onGoRelease() {
    startTicking();
    // change to a new StopWatching state that 'runs' the watch... 
    // might be easier than doing it all in this state... 
    // why? 
    // consider how you'd handle stopping the timer by pressing 'Go' again..
  }
};

class Timer: public State {
  int flashingTimeComponent = NoFlashing;
  void enter() {
    resetTime();
    
    flashingTimeComponent = SecondsFlashing;
    flashTimeComponent(flashingTimeComponent);
  }
  
  void onSetRelease() {
    flashTimeComponent(NoFlashing);
    flashingTimeComponent = nextTimeComponent();
    flashTimeComponent(flashingTimeComponent);
  }
  
  int nextTimeComponent() {
    switch (flashingTimeComponent) {
      case SecondsFlashing:
        return MinutesFlashing;
      case MinutesFlashing:
        return HoursFlashing;
      case HoursFlashing:
        return SecondsFlashing;
      default:
        return NoFlashing;
    }
  }
  
  void onClockwise() {
  }
  
  void onAntiClockwise() {
  }
};


void setup() {
  // Put your setup code here, to run once:
  initialise();
  
  chooseMode = ChooseMode();
  stopWatch = StopWatch();
  timer = Timer();
  // and instances of other State subclasses...
  
  setNextState(chooseMode);
}

void loop() {
  // Put your main code here, to run repeatedly.
  processNextEvent();
}
