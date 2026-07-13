#include "EventData.hh"

EventData& EventData::Instance() {
  static thread_local EventData instance;
  return instance;
}

void EventData::Reset() {
  fPrimaryParticle.clear();
  fPrimaryKineticEnergy = 0.0;
  fPrimaryMomentum = 0.0;
  fEnergyDeposit = 0.0;
  fScintillationPhotons = 0;
  fPrimaryMuonTrackLength = 0.0;
  fPmtIncidentPhotons = 0;
  fPmtPhotoelectrons = 0;
  fFirstPmtHitTime = -1.0;
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

void EventData::AddPmtIncidentPhoton(double time) {
  ++fPmtIncidentPhotons;
  if (fFirstPmtHitTime < 0.0 || time < fFirstPmtHitTime) {
    fFirstPmtHitTime = time;
  }
}

void EventData::AddPmtPhotoelectron() { ++fPmtPhotoelectrons; }

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

int EventData::GetPmtIncidentPhotons() const { return fPmtIncidentPhotons; }

int EventData::GetPmtPhotoelectrons() const { return fPmtPhotoelectrons; }

double EventData::GetFirstPmtHitTime() const { return fFirstPmtHitTime; }

bool EventData::HasPmtHit() const { return fFirstPmtHitTime >= 0.0; }
