#ifndef EVENTDATA_HH
#define EVENTDATA_HH

#include <vector>
#include <string>

class EventData {
 public:
  static EventData& Instance();

  void Reset();
  void SetPrimaryParticle(const std::string& name);
  void SetPrimaryKineticEnergy(double energy);
  void SetPrimaryMomentum(double momentum);
  void UpdatePrimaryHitTime(double time);
  void AddEnergyDeposit(double edep);
  void AddScintillationPhotons(int count);
  void AddScintillationPhotonTime(double time);
  void AddPrimaryMuonTrackLength(double length);
  void AddPmtIncidentPhoton(double time);
  void AddPmtPhotoelectron(double time);

  const std::string& GetPrimaryParticle() const;
  double GetPrimaryKineticEnergy() const;
  double GetPrimaryMomentum() const;
  double GetPrimaryHitTime() const;
  double GetEnergyDeposit() const;
  int GetScintillationPhotons() const;
  const std::vector<double>& GetScintillationPhotonTimes() const;
  double GetPrimaryMuonTrackLength() const;
  int GetPmtIncidentPhotons() const;
  int GetPmtPhotoelectrons() const;
  double GetFirstPmtHitTime() const;
  const std::vector<double>& GetPmtPhotoelectronTimes() const;
  bool HasPmtHit() const;

 private:
  EventData() = default;

  std::string fPrimaryParticle;
  double fPrimaryKineticEnergy = 0.0;
  double fPrimaryMomentum = 0.0;
  double fPrimaryHitTime = -1.0;
  double fEnergyDeposit = 0.0;
  int fScintillationPhotons = 0;
  std::vector<double> fScintillationPhotonTimes;
  double fPrimaryMuonTrackLength = 0.0;
  int fPmtIncidentPhotons = 0;
  int fPmtPhotoelectrons = 0;
  double fFirstPmtHitTime = -1.0;
  std::vector<double> fPmtPhotoelectronTimes;
};

#endif
