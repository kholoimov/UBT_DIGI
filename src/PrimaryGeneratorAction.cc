#include "PrimaryGeneratorAction.hh"

#include "Randomize.hh"
#include "G4GenericMessenger.hh"
#include "G4ParticleDefinition.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4SystemOfUnits.hh"

namespace {
constexpr double kMuonMinEnergy = 1.0 * GeV;
constexpr double kMuonMaxEnergy = 20.0 * GeV;
}

PrimaryGeneratorAction::PrimaryGeneratorAction() {
  fParticleGun = new G4ParticleGun(1);
  fMuonBeamSpotHalfSize = 15.0 * mm;
  ConfigureMessenger();

  auto* particle = G4ParticleTable::GetParticleTable()->FindParticle("mu-");
  fParticleGun->SetParticleDefinition(particle);
  fParticleGun->SetParticleEnergy(1.0 * GeV);
  fParticleGun->SetParticlePosition({0.0, 0.0, -30.0 * mm});
  fParticleGun->SetParticleMomentumDirection({0.0, 0.0, 1.0});
}

PrimaryGeneratorAction::~PrimaryGeneratorAction() {
  delete fMessenger;
  delete fParticleGun;
}

void PrimaryGeneratorAction::ConfigureMessenger() {
  fMessenger = new G4GenericMessenger(this, "/ubt/gun/",
                                      "Primary generator configuration");

  auto& randomizeCmd = fMessenger->DeclareProperty(
      "randomizeMuonPosition", fRandomizeMuonPosition,
      "If true, sample muon x/y position inside the configured beam spot.");
  randomizeCmd.SetParameterName("randomizeMuonPosition", true);
  randomizeCmd.SetDefaultValue("true");

  auto& beamSpotCmd = fMessenger->DeclarePropertyWithUnit(
      "muonBeamSpotHalfSize", "mm", fMuonBeamSpotHalfSize,
      "Half-size of the square muon beam spot used when randomizeMuonPosition is true.");
  beamSpotCmd.SetParameterName("muonBeamSpotHalfSize", true);
  beamSpotCmd.SetDefaultValue("15.0");
}

void PrimaryGeneratorAction::GeneratePrimaries(G4Event* event) {
  const auto* particle = fParticleGun->GetParticleDefinition();
  if (particle != nullptr && particle->GetParticleName() == "mu-") {
    const double randomEnergy =
        kMuonMinEnergy + G4UniformRand() * (kMuonMaxEnergy - kMuonMinEnergy);
    fParticleGun->SetParticleEnergy(randomEnergy);
    if (fRandomizeMuonPosition) {
      const double randomX = (2.0 * G4UniformRand() - 1.0) * fMuonBeamSpotHalfSize;
      const double randomY = (2.0 * G4UniformRand() - 1.0) * fMuonBeamSpotHalfSize;
      fParticleGun->SetParticlePosition({randomX, randomY, -30.0 * mm});
    }
  }

  fParticleGun->GeneratePrimaryVertex(event);
}
