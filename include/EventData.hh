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
  void AddPmtIncidentPhoton(double time);
  void AddPmtPhotoelectron();

  const std::string& GetPrimaryParticle() const;
  double GetPrimaryKineticEnergy() const;
  double GetPrimaryMomentum() const;
  double GetEnergyDeposit() const;
  int GetScintillationPhotons() const;
  double GetPrimaryMuonTrackLength() const;
  int GetPmtIncidentPhotons() const;
  int GetPmtPhotoelectrons() const;
  double GetFirstPmtHitTime() const;
  bool HasPmtHit() const;

 private:
  EventData() = default;

  std::string fPrimaryParticle;
  double fPrimaryKineticEnergy = 0.0;
  double fPrimaryMomentum = 0.0;
  double fEnergyDeposit = 0.0;
  int fScintillationPhotons = 0;
  double fPrimaryMuonTrackLength = 0.0;
  int fPmtIncidentPhotons = 0;
  int fPmtPhotoelectrons = 0;
  double fFirstPmtHitTime = -1.0;
};

#endif
