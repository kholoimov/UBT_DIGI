#ifndef RUNACTION_HH
#define RUNACTION_HH

#include "G4UserRunAction.hh"

class G4Run;
class ScintillatorDigi;

class RunAction : public G4UserRunAction {
 public:
  RunAction(bool enableScintillatorPhotonStudies, double scintillatorSizeMm);
  ~RunAction() override;

  void BeginOfRunAction(const G4Run* run) override;
  void EndOfRunAction(const G4Run* run) override;

  void RecordDigi(const ScintillatorDigi& digi);
  bool GetEnableScintillatorPhotonStudies() const {
    return fEnableScintillatorPhotonStudies;
  }

 private:
  bool fEnableScintillatorPhotonStudies;
  double fScintillatorSizeMm;
};

#endif
