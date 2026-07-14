#ifndef PRIMARYGENERATORACTION_HH
#define PRIMARYGENERATORACTION_HH

#include "G4VUserPrimaryGeneratorAction.hh"

class G4Event;
class G4GenericMessenger;
class G4ParticleGun;

class PrimaryGeneratorAction : public G4VUserPrimaryGeneratorAction {
 public:
  PrimaryGeneratorAction();
  ~PrimaryGeneratorAction() override;

  void GeneratePrimaries(G4Event* event) override;

 private:
  void ConfigureMessenger();

  G4ParticleGun* fParticleGun = nullptr;
  G4GenericMessenger* fMessenger = nullptr;
  bool fRandomizeMuonPosition = true;
  double fMuonBeamSpotHalfSize = 0.0;
};

#endif
