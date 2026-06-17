#include "RunAction.hh"

#include "ScintillatorDigi.hh"

#include "G4AnalysisManager.hh"
#include "G4Run.hh"
#include "G4SystemOfUnits.hh"
#include "G4ios.hh"

RunAction::RunAction() {
  auto* analysisManager = G4AnalysisManager::Instance();
  analysisManager->SetDefaultFileType("root");
  analysisManager->SetFileName("scintillator_digi");
  analysisManager->SetVerboseLevel(1);
  analysisManager->CreateNtuple("events", "Digitized scintillator data");
  analysisManager->CreateNtupleIColumn("event_id");
  analysisManager->CreateNtupleSColumn("primary_particle");
  analysisManager->CreateNtupleDColumn("primary_energy_mev");
  analysisManager->CreateNtupleDColumn("primary_momentum_mev_c");
  analysisManager->CreateNtupleDColumn("muon_range_mm");
  analysisManager->CreateNtupleDColumn("edep_mev");
  analysisManager->CreateNtupleIColumn("scintillation_photons");
  analysisManager->CreateNtupleDColumn("photoelectrons");
  analysisManager->CreateNtupleIColumn("adc_counts");
  analysisManager->CreateNtupleIColumn("triggered");
  analysisManager->FinishNtuple();
}

RunAction::~RunAction() {
  delete G4AnalysisManager::Instance();
}

void RunAction::BeginOfRunAction(const G4Run*) {
  G4AnalysisManager::Instance()->OpenFile();
}

void RunAction::EndOfRunAction(const G4Run*) {
  auto* analysisManager = G4AnalysisManager::Instance();
  analysisManager->Write();
  analysisManager->CloseFile();
  G4cout << "Digitized event data written to scintillator_digi.root" << G4endl;
}

void RunAction::RecordDigi(const ScintillatorDigi& digi) {
  auto* analysisManager = G4AnalysisManager::Instance();
  analysisManager->FillNtupleIColumn(0, digi.GetEventID());
  analysisManager->FillNtupleSColumn(1, digi.GetPrimaryParticle());
  analysisManager->FillNtupleDColumn(2,
                                     digi.GetPrimaryKineticEnergy() / CLHEP::MeV);
  analysisManager->FillNtupleDColumn(3, digi.GetPrimaryMomentum() / CLHEP::MeV);
  analysisManager->FillNtupleDColumn(4,
                                     digi.GetPrimaryMuonTrackLength() / CLHEP::mm);
  analysisManager->FillNtupleDColumn(5, digi.GetEnergyDeposit() / CLHEP::MeV);
  analysisManager->FillNtupleIColumn(6, digi.GetScintillationPhotons());
  analysisManager->FillNtupleDColumn(7, digi.GetDetectedPhotoelectrons());
  analysisManager->FillNtupleIColumn(8, digi.GetAdcCounts());
  analysisManager->FillNtupleIColumn(9, digi.GetTriggered() ? 1 : 0);
  analysisManager->AddNtupleRow();
}
