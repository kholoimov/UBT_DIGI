#ifndef EVENTDATA_HH
#define EVENTDATA_HH

#include <string>

class EventData {
 public:
  static EventData& Instance();

  void Reset();
  void SetPrimaryParticle(const std::string& name);
  void SetPrimaryKineticEnergy(double energy);
  void SetPrimaryMomentum(double momentum);
  void AddEnergyDeposit(double edep);
  void AddScintillationPhotons(int count);
  void AddPrimaryMuonTrackLength(double length);

  const std::string& GetPrimaryParticle() const;
  double GetPrimaryKineticEnergy() const;
  double GetPrimaryMomentum() const;
  double GetEnergyDeposit() const;
  int GetScintillationPhotons() const;
  double GetPrimaryMuonTrackLength() const;

 private:
  EventData() = default;

  std::string fPrimaryParticle;
  double fPrimaryKineticEnergy = 0.0;
  double fPrimaryMomentum = 0.0;
  double fEnergyDeposit = 0.0;
  int fScintillationPhotons = 0;
  double fPrimaryMuonTrackLength = 0.0;
};

#endif
