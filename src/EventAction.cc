#include "EventAction.hh"

#include "EventData.hh"
#include "RunAction.hh"
#include "ScintillatorDigi.hh"
#include "ScintillatorDigitizerModule.hh"

#include "G4DigiManager.hh"
#include "G4Event.hh"
#include "G4SystemOfUnits.hh"
#include "G4ios.hh"

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
    G4cout << "[Event " << event->GetEventID() << "] Edep="
           << digi->GetEnergyDeposit() / CLHEP::MeV << " MeV, Nscint="
           << digi->GetScintillationPhotons() << ", PE="
           << digi->GetDetectedPhotoelectrons() << ", ADC="
           << digi->GetAdcCounts() << ", Trigger=" << digi->GetTriggered()
           << G4endl;
  }
}
