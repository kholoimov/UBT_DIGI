#ifndef SCINTILLATORSENSITIVEDETECTOR_HH
#define SCINTILLATORSENSITIVEDETECTOR_HH

#include "G4VSensitiveDetector.hh"

class G4Step;
class G4TouchableHistory;

class ScintillatorSensitiveDetector : public G4VSensitiveDetector {
 public:
  explicit ScintillatorSensitiveDetector(const G4String& name);
  ~ScintillatorSensitiveDetector() override = default;

  G4bool ProcessHits(G4Step* step, G4TouchableHistory* history) override;
};

#endif
