#ifndef EVENTACTION_HH
#define EVENTACTION_HH

#include "G4UserEventAction.hh"

class G4Event;
class RunAction;
class ScintillatorDigitizerModule;

class EventAction : public G4UserEventAction {
 public:
  explicit EventAction(RunAction* runAction);
  ~EventAction() override = default;

  void BeginOfEventAction(const G4Event* event) override;
  void EndOfEventAction(const G4Event* event) override;

 private:
  RunAction* fRunAction = nullptr;
  ScintillatorDigitizerModule* fDigitizer = nullptr;
};

#endif
