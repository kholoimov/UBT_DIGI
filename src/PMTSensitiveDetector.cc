#include "PMTSensitiveDetector.hh"

#include "EventData.hh"

#include "G4OpticalPhoton.hh"
#include "G4Step.hh"
#include "G4StepPoint.hh"
#include "G4StepStatus.hh"
#include "G4Track.hh"
#include "Randomize.hh"

namespace {
constexpr double kQuantumEfficiency = 0.27;
}

PMTSensitiveDetector::PMTSensitiveDetector(const G4String& name)
    : G4VSensitiveDetector(name) {}

G4bool PMTSensitiveDetector::ProcessHits(G4Step* step, G4TouchableHistory*) {
  auto* track = step->GetTrack();
  if (track == nullptr ||
      track->GetDefinition() != G4OpticalPhoton::OpticalPhotonDefinition()) {
    return false;
  }

  const auto* preStepPoint = step->GetPreStepPoint();
  if (preStepPoint == nullptr ||
      preStepPoint->GetStepStatus() != fGeomBoundary) {
    return false;
  }

  EventData::Instance().AddPmtIncidentPhoton(track->GetGlobalTime());
  if (G4UniformRand() < kQuantumEfficiency) {
    EventData::Instance().AddPmtPhotoelectron();
  }

  track->SetTrackStatus(fStopAndKill);
  return true;
}
