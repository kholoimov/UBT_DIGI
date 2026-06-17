#include "ActionInitialization.hh"

#include "EventAction.hh"
#include "PrimaryGeneratorAction.hh"
#include "RunAction.hh"
#include "SteppingAction.hh"

void ActionInitialization::Build() const {
  auto* runAction = new RunAction();
  SetUserAction(runAction);
  SetUserAction(new PrimaryGeneratorAction());
  SetUserAction(new EventAction(runAction));
  SetUserAction(new SteppingAction());
}
