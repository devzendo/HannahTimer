
// STATE MANAGEMENT ------------------------------------------------------------------------------------------------------------
class State {
  public:
  State() {
  }
  
  virtual void enter() {}; // Override this to provide entry behaviour for this state
  virtual void exit() {};  // Override this to provide exit behaviour for this state

  
  // Override these to process individual events
  virtual void onStopWatchRelease() {};
  virtual void onStopWatchPress() {};
  virtual void onTimerRelease() {};
  virtual void onTimerPress() {};
  virtual void onGoRelease() {};
  virtual void onGoPress() {};
  virtual void onSetRelease() {};
  virtual void onSetPress() {};
  virtual void onClockwise() {};
  virtual void onAntiClockwise() {};
  virtual void onTick() {};
};

// You must do this in your setup() to get everything started..
extern void initialise();

// You must set the initial state in setup(), then in your State subclasses' onXXX overridden methods, you change
// to the next state on receipt of the XXX trigger.
extern void setNextState(State &newState);

// You must call this in your loop(), so that your State subclasses receive the calls to onXXX when the user presses
// buttons etc.
extern void processNextEvent();



// HARDWARE CONTROL ------------------------------------------------------------------------------------------------------------

// Flash one or more LEDs, or stop flashing.
const int NoLEDs = 0;
const int StopWatchLED = 1;
const int TimerLED = 2;
const int GoLED = 4;
const int SetLED = 8;
extern void flashLEDs(const int leds); // e.g. flashLEDs(StopWatchLED | TimerLED) or flashLEDs(NoLEDs);

// Reset the stopwatch/timer to 00:00:00
extern void resetTime();

// Increment the stopwatch/timer by one second and display it
extern void tickTimeUp();

// Decrement the stopwatch/timer by one second, display it, and return true iff it is 00:00:00
extern bool tickTimeDown();

// Flash hours, minutes or seconds, or stop flashing.
const int NoFlashing = 0;
const int SecondsFlashing = 1;
const int MinutesFlashing = 2;
const int HoursFlashing = 4;
extern void flashTimeComponent(const int component);

// Start or stop getting tick events once per second (so you can tickTimeUp/Down).
extern void startTicking();
extern void stopTicking();

// Beeeeep!
extern void beep();
