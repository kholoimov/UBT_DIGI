#include "ScintillatorSensitiveDetector.hh"

#include "EventData.hh"

#include "G4Step.hh"

ScintillatorSensitiveDetector::ScintillatorSensitiveDetector(const G4String& name)
    : G4VSensitiveDetector(name) {}

G4bool ScintillatorSensitiveDetector::ProcessHits(
    G4Step* step, G4TouchableHistory*) {
  const double edep = step->GetTotalEnergyDeposit();
  if (edep > 0.0) {
    EventData::Instance().AddEnergyDeposit(edep);
  }
  return true;
}
