#include "SteppingAction.hh"

#include "EventData.hh"

#include "G4OpticalPhoton.hh"
#include "G4Step.hh"
#include "G4SystemOfUnits.hh"
#include "G4Track.hh"
#include "G4VProcess.hh"

namespace {
// Safety-net cutoff for optical photons trapped by repeated total internal
// reflection. The scintillation timing model's slow decay component is
// ~11.4 ns (TimingModelParameters::kSlowDecayTimeNs), so essentially all
// photons are emitted, transported, and either absorbed or detected well
// before this cutoff; it is set an order of magnitude beyond the recorded
// timing histograms (0-50 ns, see RunAction.cc) so it only removes
// pathologically trapped photons and does not truncate the physical
// arrival-time tail.
constexpr double kOpticalPhotonMaxGlobalTimeNs = 200.0;
}  // namespace

void SteppingAction::UserSteppingAction(const G4Step* step) {
  auto* track = step->GetTrack();
  if (track != nullptr &&
      track->GetDefinition() == G4OpticalPhoton::OpticalPhotonDefinition() &&
      step->GetPostStepPoint()->GetGlobalTime() >
          kOpticalPhotonMaxGlobalTimeNs * ns) {
    track->SetTrackStatus(fStopAndKill);
    return;
  }

  const auto* prePoint = step->GetPreStepPoint();
  if (prePoint == nullptr) {
    return;
  }

  const auto* volume = prePoint->GetTouchableHandle()->GetVolume();
  if (volume == nullptr || volume->GetName() != "Scintillator") {
    return;
  }

  if (track != nullptr && track->GetTrackID() == 1) {
    EventData::Instance().UpdatePrimaryHitTime(track->GetGlobalTime());
    const auto& position = prePoint->GetPosition();
    EventData::Instance().UpdatePrimaryHitPosition(position.x(), position.y(),
                                                   position.z());
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
