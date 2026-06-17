#include "RunAction.hh"

#include "ScintillatorDigi.hh"

#include "G4Run.hh"
#include "G4ios.hh"
#include "G4SystemOfUnits.hh"

RunAction::RunAction() = default;

RunAction::~RunAction() {
  if (fOutput.is_open()) {
    fOutput.close();
  }
}

void RunAction::BeginOfRunAction(const G4Run*) {
  fOutput.open("scintillator_digi.csv", std::ios::out | std::ios::trunc);
  fOutput << "event_id,edep_mev,scintillation_photons,photoelectrons,adc_counts,triggered\n";
}

void RunAction::EndOfRunAction(const G4Run*) {
  if (fOutput.is_open()) {
    fOutput.close();
  }
  G4cout << "Digitized event data written to scintillator_digi.csv" << G4endl;
}

void RunAction::RecordDigi(const ScintillatorDigi& digi) {
  if (!fOutput.is_open()) {
    return;
  }

  fOutput << digi.GetEventID() << ','
          << digi.GetEnergyDeposit() / CLHEP::MeV << ','
          << digi.GetScintillationPhotons() << ','
          << digi.GetDetectedPhotoelectrons() << ',' << digi.GetAdcCounts()
          << ',' << digi.GetTriggered() << '\n';
}
