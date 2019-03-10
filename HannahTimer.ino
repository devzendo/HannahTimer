#include <Arduino.h> 
#include <SPI.h>
#include <HCMAX7219.h>
#include <TimerOne.h>
#include "SCoop.h"
#define DEBUG 1
#include "state_api.h"
#include "state_internals.h" // do not stare into the abyss, lest it stare back into you

// Forward instance declarations
State *chooseMode = &nothing;
State *stopWatch = &nothing;
State *timer = &nothing;
State *stopWatching = &nothing;
State *stopWatchPaused = &nothing;
State *timing = &nothing;
State *timingPaused = &nothing;

// States 
int ascii = 1;
char buf[80];

class DigitScroll: public State {
  void onClockwise() {
    ascii++;
    if (ascii == 256) {
      ascii = 1;
    }
    sprintf(buf, "AS %03d %c", ascii, ascii);
    HCMAX7219.print7Seg(buf, 8);
    HCMAX7219.Refresh();
  }
  void onAntiClockwise() {
    ascii--;
    if (ascii == 0) {
      ascii = 255;
    }
    sprintf(buf, "AS %03d %c", ascii, ascii);
    HCMAX7219.print7Seg(buf, 8);
    HCMAX7219.Refresh();
  }
};

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
    // change to a new StopWatching state that 'runs' the watch... 
    // might be easier than doing it all in this state... 
    // why? 
    // consider how you'd handle stopping the timer by pressing 'Go' again..
    setNextState(stopWatching);
  }
};

class StopWatching: public State {
  void enter() {
    startTicking();
  }
  
  void onGoRelease() {
    setNextState(stopWatchPaused);
  }

  void onTick() {
    tickTimeUp();
  }
};

class StopWatchPaused: public State {
  void enter() {
    stopTicking();
  }
  
  void onGoRelease() {
    setNextState(stopWatching);
  }

  void onStopWatchRelease() {
    setNextState(stopWatch);
  }

  void onTimerRelease() {
    setNextState(timer);
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
    switch (flashingTimeComponent) {
      case SecondsFlashing:
        incSeconds();
        break;
      case MinutesFlashing:
        incMinutes();
        break;
      case HoursFlashing:
        incHours();
        break;
      default:
        // nothing
        break;
    }
  }
  
  void onAntiClockwise() {
    switch (flashingTimeComponent) {
      case SecondsFlashing:
        decSeconds();
        break;
      case MinutesFlashing:
        decMinutes();
        break;
      case HoursFlashing:
        decHours();
        break;
      default:
        // nothing
        break;
    }
  }

  void onGoRelease() {
    flashTimeComponent(NoFlashing);
    setNextState(timing);
  }
};

class Timing: public State {
  void enter() {
    startTicking();
  }
  
  void onGoRelease() {
    setNextState(timingPaused);
  }

  void onTick() {
    if (tickTimeDown()) {
      beep();
      setNextState(chooseMode);
    }
  }
};

class TimingPaused: public State {
  void enter() {
    stopTicking();
  }
  
  void onGoRelease() {
    setNextState(timing);
  }

  void onStopWatchRelease() {
    setNextState(stopWatch);
  }

  void onTimerRelease() {
    resetTime();
    setNextState(timer);
  }
};


void setup() {
  Serial.begin(115200);
  Serial.println("Initialising...");
  
  // Put your setup code here, to run once:
  initialise();
  Serial.println("Initialised");
  
  chooseMode = new ChooseMode();
  stopWatch = new StopWatch();
  timer = new Timer();
  stopWatchPaused = new StopWatchPaused();
  stopWatching = new StopWatching();
  timing = new Timing();
  timingPaused = new TimingPaused();
  // and instances of other State subclasses...
/*
  sprintf(buf,"chooseMode is 0x%08x", chooseMode);
  Serial.println(buf);
  sprintf(buf,"stopWatch is 0x%08x", stopWatch);
  Serial.println(buf);
  sprintf(buf,"timer is 0x%08x", timer);
  Serial.println(buf);
  sprintf(buf,"nothing is 0x%08x", nothing);
  Serial.println(buf);
*/
  setNextState(chooseMode);
}



void loop() {
  // Put your main code here, to run repeatedly.
  processNextEvent();
}
