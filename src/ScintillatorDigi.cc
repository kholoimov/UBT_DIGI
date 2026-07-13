#include "ScintillatorDigi.hh"

#include "G4SystemOfUnits.hh"
#include "G4ios.hh"

void ScintillatorDigi::SetEventID(int value) { fEventID = value; }

void ScintillatorDigi::SetPrimaryParticle(const std::string& value) {
  fPrimaryParticle = value;
}

void ScintillatorDigi::SetPrimaryKineticEnergy(double value) {
  fPrimaryKineticEnergy = value;
}

void ScintillatorDigi::SetPrimaryMomentum(double value) {
  fPrimaryMomentum = value;
}

void ScintillatorDigi::SetPrimaryMuonTrackLength(double value) {
  fPrimaryMuonTrackLength = value;
}

void ScintillatorDigi::SetEnergyDeposit(double value) { fEnergyDeposit = value; }

void ScintillatorDigi::SetScintillationPhotons(int value) {
  fScintillationPhotons = value;
}

void ScintillatorDigi::SetPmtIncidentPhotons(int value) {
  fPmtIncidentPhotons = value;
}

void ScintillatorDigi::SetDetectedPhotoelectrons(double value) {
  fDetectedPhotoelectrons = value;
}

void ScintillatorDigi::SetFirstPmtHitTime(double value) {
  fFirstPmtHitTime = value;
}

void ScintillatorDigi::SetPmtCharge(double value) { fPmtCharge = value; }

void ScintillatorDigi::SetAdcCounts(int value) { fAdcCounts = value; }

void ScintillatorDigi::SetTriggered(bool value) { fTriggered = value; }

int ScintillatorDigi::GetEventID() const { return fEventID; }

const std::string& ScintillatorDigi::GetPrimaryParticle() const {
  return fPrimaryParticle;
}

double ScintillatorDigi::GetPrimaryKineticEnergy() const {
  return fPrimaryKineticEnergy;
}

double ScintillatorDigi::GetPrimaryMomentum() const { return fPrimaryMomentum; }

double ScintillatorDigi::GetPrimaryMuonTrackLength() const {
  return fPrimaryMuonTrackLength;
}

double ScintillatorDigi::GetEnergyDeposit() const { return fEnergyDeposit; }

int ScintillatorDigi::GetScintillationPhotons() const {
  return fScintillationPhotons;
}

int ScintillatorDigi::GetPmtIncidentPhotons() const {
  return fPmtIncidentPhotons;
}

double ScintillatorDigi::GetDetectedPhotoelectrons() const {
  return fDetectedPhotoelectrons;
}

double ScintillatorDigi::GetFirstPmtHitTime() const { return fFirstPmtHitTime; }

double ScintillatorDigi::GetPmtCharge() const { return fPmtCharge; }

int ScintillatorDigi::GetAdcCounts() const { return fAdcCounts; }

bool ScintillatorDigi::GetTriggered() const { return fTriggered; }

void ScintillatorDigi::Draw() {}

void ScintillatorDigi::Print() {
  G4cout << "ScintillatorDigi: event=" << fEventID
         << ", primary=" << fPrimaryParticle
         << ", Ekin(MeV)=" << fPrimaryKineticEnergy / CLHEP::MeV
         << ", p(MeV/c)=" << fPrimaryMomentum / CLHEP::MeV
         << ", muRange(mm)=" << fPrimaryMuonTrackLength / CLHEP::mm
         << ", edep(MeV)=" << fEnergyDeposit / CLHEP::MeV
         << ", scintPhotons=" << fScintillationPhotons
         << ", pmtPhotons=" << fPmtIncidentPhotons
         << ", photoelectrons=" << fDetectedPhotoelectrons
         << ", firstHit(ns)="
         << (fFirstPmtHitTime >= 0.0 ? fFirstPmtHitTime / CLHEP::ns : -1.0)
         << ", charge(pC)=" << fPmtCharge / CLHEP::picocoulomb
         << ", adc=" << fAdcCounts << ", triggered=" << fTriggered << G4endl;
}
