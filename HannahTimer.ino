//#include <SCoop.h>

#include "state_api.h"
#include "state_internals.h" // do not stare into the abyss, lest it stare back into you


// States 
class ChooseMode : public State {
};

void setup() {
  // Put your setup code here, to run once:
  initialise();
  
  ChooseMode chooseMode = ChooseMode();
  // and instances of other State subclasses...
  
  setNextState(chooseMode);
}

void loop() {
  // Put your main code here, to run repeatedly.
  processNextEvent();
}
