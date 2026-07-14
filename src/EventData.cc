#include "EventData.hh"

EventData& EventData::Instance() {
  static thread_local EventData instance;
  return instance;
}

void EventData::Reset() {
  fPrimaryParticle.clear();
  fPrimaryKineticEnergy = 0.0;
  fPrimaryMomentum = 0.0;
  fPrimaryHitTime = -1.0;
  fEnergyDeposit = 0.0;
  fScintillationPhotons = 0;
  fScintillationPhotonTimes.clear();
  fPrimaryMuonTrackLength = 0.0;
  fPmtIncidentPhotons = 0;
  fPmtPhotoelectrons = 0;
  fFirstPmtHitTime = -1.0;
  fPmtIncidentPhotonBirthPositions.clear();
  fPmtPhotoelectronTimes.clear();
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

void EventData::UpdatePrimaryHitTime(double time) {
  if (fPrimaryHitTime < 0.0 || time < fPrimaryHitTime) {
    fPrimaryHitTime = time;
  }
}

void EventData::AddEnergyDeposit(double edep) { fEnergyDeposit += edep; }

void EventData::AddScintillationPhotons(int count) {
  fScintillationPhotons += count;
}

void EventData::AddScintillationPhotonTime(double time) {
  fScintillationPhotonTimes.push_back(time);
}

void EventData::AddPrimaryMuonTrackLength(double length) {
  fPrimaryMuonTrackLength += length;
}

void EventData::AddPmtIncidentPhoton(double time, double birthX, double birthY,
                                     double birthZ) {
  ++fPmtIncidentPhotons;
  fPmtIncidentPhotonBirthPositions.push_back({birthX, birthY, birthZ});
  if (fFirstPmtHitTime < 0.0 || time < fFirstPmtHitTime) {
    fFirstPmtHitTime = time;
  }
}

void EventData::AddPmtPhotoelectron(double time) {
  ++fPmtPhotoelectrons;
  fPmtPhotoelectronTimes.push_back(time);
}

const std::string& EventData::GetPrimaryParticle() const {
  return fPrimaryParticle;
}

double EventData::GetPrimaryKineticEnergy() const { return fPrimaryKineticEnergy; }

double EventData::GetPrimaryMomentum() const { return fPrimaryMomentum; }

double EventData::GetPrimaryHitTime() const { return fPrimaryHitTime; }

double EventData::GetEnergyDeposit() const { return fEnergyDeposit; }

int EventData::GetScintillationPhotons() const { return fScintillationPhotons; }

const std::vector<double>& EventData::GetScintillationPhotonTimes() const {
  return fScintillationPhotonTimes;
}

double EventData::GetPrimaryMuonTrackLength() const {
  return fPrimaryMuonTrackLength;
}

int EventData::GetPmtIncidentPhotons() const { return fPmtIncidentPhotons; }

int EventData::GetPmtPhotoelectrons() const { return fPmtPhotoelectrons; }

double EventData::GetFirstPmtHitTime() const { return fFirstPmtHitTime; }

const std::vector<std::array<double, 3>>&
EventData::GetPmtIncidentPhotonBirthPositions() const {
  return fPmtIncidentPhotonBirthPositions;
}

const std::vector<double>& EventData::GetPmtPhotoelectronTimes() const {
  return fPmtPhotoelectronTimes;
}

bool EventData::HasPmtHit() const { return fFirstPmtHitTime >= 0.0; }
