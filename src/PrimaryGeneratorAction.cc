#include "PrimaryGeneratorAction.hh"

#include "Randomize.hh"
#include "G4ParticleDefinition.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4SystemOfUnits.hh"

namespace {
constexpr double kMuonMinEnergy = 1.0 * GeV;
constexpr double kMuonMaxEnergy = 20.0 * GeV;
constexpr double kMuonBeamSpotHalfSize = 15.0 * mm;
}

PrimaryGeneratorAction::PrimaryGeneratorAction() {
  fParticleGun = new G4ParticleGun(1);

  auto* particle = G4ParticleTable::GetParticleTable()->FindParticle("mu-");
  fParticleGun->SetParticleDefinition(particle);
  fParticleGun->SetParticleEnergy(1.0 * GeV);
  fParticleGun->SetParticlePosition({0.0, 0.0, -30.0 * mm});
  fParticleGun->SetParticleMomentumDirection({0.0, 0.0, 1.0});
}

PrimaryGeneratorAction::~PrimaryGeneratorAction() { delete fParticleGun; }

void PrimaryGeneratorAction::GeneratePrimaries(G4Event* event) {
  const auto* particle = fParticleGun->GetParticleDefinition();
  if (particle != nullptr && particle->GetParticleName() == "mu-") {
    const double randomEnergy =
        kMuonMinEnergy + G4UniformRand() * (kMuonMaxEnergy - kMuonMinEnergy);
    const double randomX =
        (2.0 * G4UniformRand() - 1.0) * kMuonBeamSpotHalfSize;
    const double randomY =
        (2.0 * G4UniformRand() - 1.0) * kMuonBeamSpotHalfSize;
    fParticleGun->SetParticleEnergy(randomEnergy);
    fParticleGun->SetParticlePosition({randomX, randomY, -30.0 * mm});
  }

  fParticleGun->GeneratePrimaryVertex(event);
}
