#ifndef STEPPINGACTION_HH
#define STEPPINGACTION_HH

#include "G4UserSteppingAction.hh"

class G4Step;

class SteppingAction : public G4UserSteppingAction {
 public:
  SteppingAction() = default;
  ~SteppingAction() override = default;

  void UserSteppingAction(const G4Step* step) override;
};

#endif
