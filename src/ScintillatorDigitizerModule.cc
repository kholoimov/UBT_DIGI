#include "ScintillatorDigitizerModule.hh"

#include "EventData.hh"
#include "ScintillatorDigi.hh"

#include "G4EventManager.hh"
#include "G4SystemOfUnits.hh"

#include <algorithm>
#include <cmath>

namespace {
constexpr double kPhotonTransportEfficiency = 0.22;
constexpr double kPhotonDetectionEfficiency = 0.35;
constexpr double kGainAdcPerPhotoelectron = 8.0;
constexpr double kTriggerThresholdMeV = 0.20;
}  // namespace

ScintillatorDigitizerModule::ScintillatorDigitizerModule(
    const G4String& moduleName)
    : G4VDigitizerModule(moduleName) {
  collectionName.push_back("ScintillatorDigiCollection");
}

void ScintillatorDigitizerModule::Digitize() {
  const auto& eventData = EventData::Instance();
  auto digi = std::make_unique<ScintillatorDigi>();

  const double edep = eventData.GetEnergyDeposit();
  const int scintPhotons = eventData.GetScintillationPhotons();
  const double detectedPE = scintPhotons * kPhotonTransportEfficiency *
                            kPhotonDetectionEfficiency;
  const int adcCounts = static_cast<int>(
      std::lround(std::max(0.0, detectedPE * kGainAdcPerPhotoelectron)));
  const bool triggered =
      (edep >= kTriggerThresholdMeV * CLHEP::MeV) && (scintPhotons > 0);

  auto* currentEvent = G4EventManager::GetEventManager()->GetConstCurrentEvent();
  digi->SetEventID(currentEvent != nullptr ? currentEvent->GetEventID() : -1);
  digi->SetPrimaryParticle(eventData.GetPrimaryParticle());
  digi->SetPrimaryKineticEnergy(eventData.GetPrimaryKineticEnergy());
  digi->SetPrimaryMomentum(eventData.GetPrimaryMomentum());
  digi->SetPrimaryMuonTrackLength(eventData.GetPrimaryMuonTrackLength());
  digi->SetEnergyDeposit(edep);
  digi->SetScintillationPhotons(scintPhotons);
  digi->SetDetectedPhotoelectrons(detectedPE);
  digi->SetAdcCounts(adcCounts);
  digi->SetTriggered(triggered);

  fLastDigi = std::move(digi);
}

const ScintillatorDigi* ScintillatorDigitizerModule::GetLastDigi() const {
  return fLastDigi.get();
}
