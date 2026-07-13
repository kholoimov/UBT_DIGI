#include "ScintillatorDigitizerModule.hh"

#include "EventData.hh"
#include "ScintillatorDigi.hh"

#include "G4EventManager.hh"
#include "G4PrimaryParticle.hh"
#include "G4PrimaryVertex.hh"
#include "G4SystemOfUnits.hh"

#include <algorithm>
#include <cmath>

namespace {
constexpr double kPmtGain = 2.8e6;
constexpr double kElectronChargeC = 1.602176634e-19;
constexpr double kPicocoulomb = 1.0e-12 * CLHEP::coulomb;
constexpr double kAdcCountsPerPc = 1.0;
constexpr double kTriggerThresholdMeV = 0.20;
constexpr double kTriggerThresholdPc = 5.0;
constexpr int kPhotoelectronThresholdCount = 5;

double ComputeThresholdTimeFromPrimary(const std::vector<double>& times,
                                       double primaryHitTime) {
  if (primaryHitTime < 0.0 ||
      times.size() < static_cast<std::size_t>(kPhotoelectronThresholdCount)) {
    return -1.0;
  }

  auto sortedTimes = times;
  std::sort(sortedTimes.begin(), sortedTimes.end());
  return sortedTimes[kPhotoelectronThresholdCount - 1] - primaryHitTime;
}
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
  const int pmtPhotons = eventData.GetPmtIncidentPhotons();
  const double detectedPE = static_cast<double>(eventData.GetPmtPhotoelectrons());
  const double threshold5TimeFromPrimary = ComputeThresholdTimeFromPrimary(
      eventData.GetPmtPhotoelectronTimes(), eventData.GetPrimaryHitTime());
  const double pmtCharge =
      detectedPE * kPmtGain * kElectronChargeC * CLHEP::coulomb;
  const int adcCounts = static_cast<int>(
      std::lround(std::max(
          0.0, (pmtCharge / kPicocoulomb) * kAdcCountsPerPc)));
  const bool triggered =
      (edep >= kTriggerThresholdMeV * CLHEP::MeV) &&
      ((pmtCharge / kPicocoulomb) >= kTriggerThresholdPc);

  auto* currentEvent = G4EventManager::GetEventManager()->GetConstCurrentEvent();
  digi->SetEventID(currentEvent != nullptr ? currentEvent->GetEventID() : -1);
  digi->SetPrimaryMuonTrackLength(eventData.GetPrimaryMuonTrackLength());
  digi->SetEnergyDeposit(edep);
  digi->SetScintillationPhotons(scintPhotons);
  digi->SetPmtIncidentPhotons(pmtPhotons);
  digi->SetDetectedPhotoelectrons(detectedPE);
  digi->SetFirstPmtHitTime(eventData.GetFirstPmtHitTime());
  digi->SetThreshold80TimeFromPrimary(threshold5TimeFromPrimary);
  digi->SetPmtCharge(pmtCharge);
  digi->SetAdcCounts(adcCounts);
  digi->SetTriggered(triggered);

  if (currentEvent != nullptr) {
    const auto* primaryVertex = currentEvent->GetPrimaryVertex();
    if (primaryVertex != nullptr) {
      const auto* primary = primaryVertex->GetPrimary();
      if (primary != nullptr) {
        digi->SetPrimaryParticle(primary->GetParticleDefinition()->GetParticleName());
        digi->SetPrimaryKineticEnergy(primary->GetKineticEnergy());

        const double px = primary->GetPx();
        const double py = primary->GetPy();
        const double pz = primary->GetPz();
        digi->SetPrimaryMomentum(std::sqrt(px * px + py * py + pz * pz));
      }
    }
  }

  fLastDigi = std::move(digi);
}

const ScintillatorDigi* ScintillatorDigitizerModule::GetLastDigi() const {
  return fLastDigi.get();
}
