#ifndef SCINTILLATORDIGI_HH
#define SCINTILLATORDIGI_HH

#include <string>

#include "G4VDigi.hh"

class ScintillatorDigi : public G4VDigi {
 public:
  ScintillatorDigi() = default;
  ~ScintillatorDigi() override = default;

  void SetEventID(int value);
  void SetPrimaryParticle(const std::string& value);
  void SetPrimaryKineticEnergy(double value);
  void SetPrimaryMomentum(double value);
  void SetPrimaryMuonTrackLength(double value);
  void SetEnergyDeposit(double value);
  void SetScintillationPhotons(int value);
  void SetPmtIncidentPhotons(int value);
  void SetDetectedPhotoelectrons(double value);
  void SetFirstPmtHitTime(double value);
  void SetThreshold80TimeFromPrimary(double value);
  void SetPmtCharge(double value);
  void SetAdcCounts(int value);
  void SetTriggered(bool value);

  int GetEventID() const;
  const std::string& GetPrimaryParticle() const;
  double GetPrimaryKineticEnergy() const;
  double GetPrimaryMomentum() const;
  double GetPrimaryMuonTrackLength() const;
  double GetEnergyDeposit() const;
  int GetScintillationPhotons() const;
  int GetPmtIncidentPhotons() const;
  double GetDetectedPhotoelectrons() const;
  double GetFirstPmtHitTime() const;
  double GetThreshold80TimeFromPrimary() const;
  double GetPmtCharge() const;
  int GetAdcCounts() const;
  bool GetTriggered() const;

  void Draw() override;
  void Print() override;

 private:
  int fEventID = -1;
  std::string fPrimaryParticle;
  double fPrimaryKineticEnergy = 0.0;
  double fPrimaryMomentum = 0.0;
  double fPrimaryMuonTrackLength = 0.0;
  double fEnergyDeposit = 0.0;
  int fScintillationPhotons = 0;
  int fPmtIncidentPhotons = 0;
  double fDetectedPhotoelectrons = 0.0;
  double fFirstPmtHitTime = -1.0;
  double fThreshold80TimeFromPrimary = -1.0;
  double fPmtCharge = 0.0;
  int fAdcCounts = 0;
  bool fTriggered = false;
};

#endif
