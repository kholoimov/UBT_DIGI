#include "PrimaryGeneratorAction.hh"

#include "EventData.hh"

#include "Randomize.hh"
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
    fParticleGun->SetParticleEnergy(randomEnergy);
  }

  fParticleGun->GeneratePrimaryVertex(event);
}
