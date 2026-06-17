#include "EventData.hh"

EventData& EventData::Instance() {
  static EventData instance;
  return instance;
}

void EventData::Reset() {
  fEnergyDeposit = 0.0;
  fScintillationPhotons = 0;
}

void EventData::AddEnergyDeposit(double edep) { fEnergyDeposit += edep; }

void EventData::AddScintillationPhotons(int count) {
  fScintillationPhotons += count;
}

double EventData::GetEnergyDeposit() const { return fEnergyDeposit; }

int EventData::GetScintillationPhotons() const { return fScintillationPhotons; }
