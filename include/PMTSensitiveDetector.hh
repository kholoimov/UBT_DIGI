#ifndef PMTSENSITIVEDETECTOR_HH
#define PMTSENSITIVEDETECTOR_HH

#include "G4VSensitiveDetector.hh"

class G4Step;
class G4TouchableHistory;

class PMTSensitiveDetector : public G4VSensitiveDetector {
 public:
  explicit PMTSensitiveDetector(const G4String& name);
  ~PMTSensitiveDetector() override = default;

  G4bool ProcessHits(G4Step* step, G4TouchableHistory* history) override;
};

#endif
