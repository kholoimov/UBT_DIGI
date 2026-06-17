#include "EventData.hh"

EventData& EventData::Instance() {
  static EventData instance;
  return instance;
}

void EventData::Reset() {
  fPrimaryParticle.clear();
  fPrimaryKineticEnergy = 0.0;
  fPrimaryMomentum = 0.0;
  fEnergyDeposit = 0.0;
  fScintillationPhotons = 0;
  fPrimaryMuonTrackLength = 0.0;
}

void EventData::SetPrimaryParticle(const std::string& name) {
  fPrimaryParticle = name;
}

void EventData::SetPrimaryKineticEnergy(double energy) {
  fPrimaryKineticEnergy = energy;
}

void EventData::SetPrimaryMomentum(double momentum) {
  fPrimaryMomentum = momentum;
}

void EventData::AddEnergyDeposit(double edep) { fEnergyDeposit += edep; }

void EventData::AddScintillationPhotons(int count) {
  fScintillationPhotons += count;
}

void EventData::AddPrimaryMuonTrackLength(double length) {
  fPrimaryMuonTrackLength += length;
}

const std::string& EventData::GetPrimaryParticle() const {
  return fPrimaryParticle;
}

double EventData::GetPrimaryKineticEnergy() const { return fPrimaryKineticEnergy; }

double EventData::GetPrimaryMomentum() const { return fPrimaryMomentum; }

double EventData::GetEnergyDeposit() const { return fEnergyDeposit; }

int EventData::GetScintillationPhotons() const { return fScintillationPhotons; }

double EventData::GetPrimaryMuonTrackLength() const {
  return fPrimaryMuonTrackLength;
}
