#include "ScintillatorDigi.hh"

#include "G4SystemOfUnits.hh"
#include "G4ios.hh"

void ScintillatorDigi::SetEventID(int value) { fEventID = value; }

void ScintillatorDigi::SetEnergyDeposit(double value) { fEnergyDeposit = value; }

void ScintillatorDigi::SetScintillationPhotons(int value) {
  fScintillationPhotons = value;
}

void ScintillatorDigi::SetDetectedPhotoelectrons(double value) {
  fDetectedPhotoelectrons = value;
}

void ScintillatorDigi::SetAdcCounts(int value) { fAdcCounts = value; }

void ScintillatorDigi::SetTriggered(bool value) { fTriggered = value; }

int ScintillatorDigi::GetEventID() const { return fEventID; }

double ScintillatorDigi::GetEnergyDeposit() const { return fEnergyDeposit; }

int ScintillatorDigi::GetScintillationPhotons() const {
  return fScintillationPhotons;
}

double ScintillatorDigi::GetDetectedPhotoelectrons() const {
  return fDetectedPhotoelectrons;
}

int ScintillatorDigi::GetAdcCounts() const { return fAdcCounts; }

bool ScintillatorDigi::GetTriggered() const { return fTriggered; }

void ScintillatorDigi::Draw() {}

void ScintillatorDigi::Print() {
  G4cout << "ScintillatorDigi: event=" << fEventID
         << ", edep(MeV)=" << fEnergyDeposit / CLHEP::MeV
         << ", scintPhotons=" << fScintillationPhotons
         << ", photoelectrons=" << fDetectedPhotoelectrons
         << ", adc=" << fAdcCounts << ", triggered=" << fTriggered << G4endl;
}
