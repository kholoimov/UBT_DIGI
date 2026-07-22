#include "ActionInitialization.hh"

#include "EventAction.hh"
#include "OutputConfiguration.hh"
#include "PrimaryGeneratorAction.hh"
#include "RunAction.hh"
#include "SteppingAction.hh"

void ActionInitialization::Build() const {
  auto* runAction =
      new RunAction(fOutputConfiguration->GetEnableScintillatorPhotonStudies());
  SetUserAction(runAction);
  SetUserAction(new PrimaryGeneratorAction());
  SetUserAction(new EventAction(runAction));
  SetUserAction(new SteppingAction());
}

void ActionInitialization::BuildForMaster() const {
  SetUserAction(new RunAction(
      fOutputConfiguration->GetEnableScintillatorPhotonStudies()));
}
