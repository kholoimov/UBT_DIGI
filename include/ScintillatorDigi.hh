#ifndef SCINTILLATORDIGI_HH
#define SCINTILLATORDIGI_HH

#include "G4VDigi.hh"

class ScintillatorDigi : public G4VDigi {
 public:
  ScintillatorDigi() = default;
  ~ScintillatorDigi() override = default;

  void SetEventID(int value);
  void SetEnergyDeposit(double value);
  void SetScintillationPhotons(int value);
  void SetDetectedPhotoelectrons(double value);
  void SetAdcCounts(int value);
  void SetTriggered(bool value);

  int GetEventID() const;
  double GetEnergyDeposit() const;
  int GetScintillationPhotons() const;
  double GetDetectedPhotoelectrons() const;
  int GetAdcCounts() const;
  bool GetTriggered() const;

  void Draw() override;
  void Print() override;

 private:
  int fEventID = -1;
  double fEnergyDeposit = 0.0;
  int fScintillationPhotons = 0;
  double fDetectedPhotoelectrons = 0.0;
  int fAdcCounts = 0;
  bool fTriggered = false;
};

#endif
