#ifndef EVENTDATA_HH
#define EVENTDATA_HH

#include <array>
#include <string>
#include <vector>

class EventData {
 public:
  static EventData& Instance();

  void Reset();
  void SetEnableScintillatorPhotonStudies(bool enabled);
  void SetPrimaryParticle(const std::string& name);
  void SetPrimaryKineticEnergy(double energy);
  void SetPrimaryMomentum(double momentum);
  void UpdatePrimaryHitTime(double time);
  void UpdatePrimaryHitPosition(double x, double y, double z);
  void AddEnergyDeposit(double edep);
  void AddScintillationPhotons(int count);
  void AddScintillationPhotonTime(double time);
  void AddPrimaryMuonTrackLength(double length);
  void AddPmtIncidentPhoton(double arrivalTime, double birthTime, double birthX,
                            double birthY, double birthZ);
  void AddPmtPhotoelectron(double time);

  const std::string& GetPrimaryParticle() const;
  double GetPrimaryKineticEnergy() const;
  double GetPrimaryMomentum() const;
  double GetPrimaryHitTime() const;
  double GetPrimaryHitX() const;
  double GetPrimaryHitY() const;
  double GetPrimaryHitZ() const;
  double GetEnergyDeposit() const;
  int GetScintillationPhotons() const;
  const std::vector<double>& GetScintillationPhotonTimes() const;
  double GetPrimaryMuonTrackLength() const;
  int GetPmtIncidentPhotons() const;
  int GetPmtPhotoelectrons() const;
  double GetFirstPmtHitTime() const;
  const std::vector<std::array<double, 5>>& GetPmtIncidentPhotonRecords() const;
  const std::vector<double>& GetPmtPhotoelectronTimes() const;
  bool HasPmtHit() const;

 private:
  EventData() = default;

  std::string fPrimaryParticle;
  double fPrimaryKineticEnergy = 0.0;
  double fPrimaryMomentum = 0.0;
  double fPrimaryHitTime = -1.0;
  double fPrimaryHitX = 0.0;
  double fPrimaryHitY = 0.0;
  double fPrimaryHitZ = 0.0;
  bool fHasPrimaryHitPosition = false;
  double fEnergyDeposit = 0.0;
  int fScintillationPhotons = 0;
  std::vector<double> fScintillationPhotonTimes;
  double fPrimaryMuonTrackLength = 0.0;
  int fPmtIncidentPhotons = 0;
  int fPmtPhotoelectrons = 0;
  double fFirstPmtHitTime = -1.0;
  std::vector<std::array<double, 5>> fPmtIncidentPhotonRecords;
  std::vector<double> fPmtPhotoelectronTimes;
  bool fEnableScintillatorPhotonStudies = false;
};

#endif
