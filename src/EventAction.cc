#include "EventAction.hh"

#include "EventData.hh"
#include "RunAction.hh"
#include "ScintillatorDigi.hh"
#include "ScintillatorDigitizerModule.hh"

#include "G4DigiManager.hh"
#include "G4Event.hh"
#include "G4SystemOfUnits.hh"
#include "G4ios.hh"

namespace {
constexpr double kPicocoulomb = 1.0e-12 * CLHEP::coulomb;
}

EventAction::EventAction(RunAction* runAction) : fRunAction(runAction) {
  auto* digiManager = G4DigiManager::GetDMpointer();
  fDigitizer = new ScintillatorDigitizerModule("ScintillatorDigitizer");
  digiManager->AddNewModule(fDigitizer);
}

void EventAction::BeginOfEventAction(const G4Event*) { EventData::Instance().Reset(); }

void EventAction::EndOfEventAction(const G4Event* event) {
  auto* digiManager = G4DigiManager::GetDMpointer();
  digiManager->Digitize("ScintillatorDigitizer");

  const auto* digi = fDigitizer->GetLastDigi();
  if (digi != nullptr) {
    fRunAction->RecordDigi(*digi);
    G4cout << "[Event " << event->GetEventID() << "] Primary="
           << digi->GetPrimaryParticle() << ", Ekin="
           << digi->GetPrimaryKineticEnergy() / CLHEP::MeV << " MeV, p="
           << digi->GetPrimaryMomentum() / CLHEP::MeV
           << " MeV/c, MuRange=" << digi->GetPrimaryMuonTrackLength() / CLHEP::mm
           << " mm, Edep="
           << digi->GetEnergyDeposit() / CLHEP::MeV << " MeV, Nscint="
           << digi->GetScintillationPhotons() << ", PMTphot="
           << digi->GetPmtIncidentPhotons() << ", PE="
           << digi->GetDetectedPhotoelectrons() << ", ADC="
           << digi->GetAdcCounts() << ", Q="
           << digi->GetPmtCharge() / kPicocoulomb
           << " pC, t0="
           << (digi->GetFirstPmtHitTime() >= 0.0
                   ? digi->GetFirstPmtHitTime() / CLHEP::ns
                   : -1.0)
           << " ns, t80="
           << (digi->GetThreshold80TimeFromPrimary() >= 0.0
                   ? digi->GetThreshold80TimeFromPrimary() / CLHEP::ns
                   : -1.0)
           << " ns, Trigger=" << digi->GetTriggered()
           << G4endl;
  }
}
