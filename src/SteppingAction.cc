#include "SteppingAction.hh"

#include "EventData.hh"

#include "G4OpticalPhoton.hh"
#include "G4Step.hh"
#include "G4Track.hh"
#include "G4VProcess.hh"

void SteppingAction::UserSteppingAction(const G4Step* step) {
  const auto* prePoint = step->GetPreStepPoint();
  if (prePoint == nullptr) {
    return;
  }

  const auto* volume = prePoint->GetTouchableHandle()->GetVolume();
  if (volume == nullptr || volume->GetName() != "Scintillator") {
    return;
  }

  const auto* track = step->GetTrack();
  if (track != nullptr && track->GetTrackID() == 1) {
    EventData::Instance().UpdatePrimaryHitTime(track->GetGlobalTime());
    if (track->GetDefinition()->GetParticleName() == "mu-") {
      EventData::Instance().AddPrimaryMuonTrackLength(step->GetStepLength());
    }
  }

  const auto* secondaries = step->GetSecondaryInCurrentStep();
  if (secondaries == nullptr || secondaries->empty()) {
    return;
  }

  int scintillationPhotons = 0;
  for (const auto* secondary : *secondaries) {
    if (secondary->GetDefinition() != G4OpticalPhoton::Definition()) {
      continue;
    }

    const auto* creator = secondary->GetCreatorProcess();
    if (creator != nullptr && creator->GetProcessName() == "Scintillation") {
      ++scintillationPhotons;
      EventData::Instance().AddScintillationPhotonTime(
          secondary->GetGlobalTime());
    }
  }

  if (scintillationPhotons > 0) {
    EventData::Instance().AddScintillationPhotons(scintillationPhotons);
  }
}
