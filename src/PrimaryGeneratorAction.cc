#include "PrimaryGeneratorAction.hh"

#include "Randomize.hh"
#include "G4GenericMessenger.hh"
#include "G4ParticleDefinition.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4SystemOfUnits.hh"

PrimaryGeneratorAction::PrimaryGeneratorAction() {
  fParticleGun = new G4ParticleGun(1);
  fMuonBeamSpotHalfSize = 20.0 * mm;
  // Default randomized-energy range for the general studies (unchanged from
  // the previous hardcoded muon-only constants). The discrete energy-scan
  // macros instead set randomizeEnergy false and a fixed /gun/energy per
  // run, so they do not depend on this default.
  fMinEnergy = 1.0 * GeV;
  fMaxEnergy = 20.0 * GeV;
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

  // NOTE: despite the "Muon" in these two command/parameter names (kept for
  // backward compatibility with existing muon macros), both apply to
  // whichever primary particle is currently selected via the standard
  // /gun/particle command, not only mu-.
  auto& randomizeCmd = fMessenger->DeclareProperty(
      "randomizeMuonPosition", fRandomizeMuonPosition,
      "If true, sample the primary x/y position inside the configured beam "
      "spot, i.e. shoot particles geometrically distributed across the "
      "tile face.");
  randomizeCmd.SetParameterName("randomizeMuonPosition", true);
  randomizeCmd.SetDefaultValue("true");

  auto& beamSpotCmd = fMessenger->DeclarePropertyWithUnit(
      "muonBeamSpotHalfSize", "mm", fMuonBeamSpotHalfSize,
      "Half-size of the square beam spot used when randomizeMuonPosition is true.");
  beamSpotCmd.SetParameterName("muonBeamSpotHalfSize", true);
  beamSpotCmd.SetDefaultValue("15.0");

  auto& randomizeEnergyCmd = fMessenger->DeclareProperty(
      "randomizeEnergy", fRandomizeEnergy,
      "If true, sample a uniform random kinetic energy in [minEnergy, "
      "maxEnergy] for every event, for whichever particle type is set via "
      "/gun/particle. If false, the fixed energy set via the standard "
      "/gun/energy command is used unchanged for every event -- e.g. for a "
      "discrete energy scan.");
  randomizeEnergyCmd.SetParameterName("randomizeEnergy", true);
  randomizeEnergyCmd.SetDefaultValue("true");

  auto& minEnergyCmd = fMessenger->DeclarePropertyWithUnit(
      "minEnergy", "GeV", fMinEnergy,
      "Lower bound of the randomized kinetic-energy range (used only when "
      "randomizeEnergy is true).");
  minEnergyCmd.SetParameterName("minEnergy", true);
  minEnergyCmd.SetDefaultValue("1.0");

  auto& maxEnergyCmd = fMessenger->DeclarePropertyWithUnit(
      "maxEnergy", "GeV", fMaxEnergy,
      "Upper bound of the randomized kinetic-energy range (used only when "
      "randomizeEnergy is true).");
  maxEnergyCmd.SetParameterName("maxEnergy", true);
  maxEnergyCmd.SetDefaultValue("20.0");
}

void PrimaryGeneratorAction::GeneratePrimaries(G4Event* event) {
  if (fRandomizeEnergy) {
    const double randomEnergy =
        fMinEnergy + G4UniformRand() * (fMaxEnergy - fMinEnergy);
    fParticleGun->SetParticleEnergy(randomEnergy);
  }
  if (fRandomizeMuonPosition) {
    const double randomX = (2.0 * G4UniformRand() - 1.0) * fMuonBeamSpotHalfSize;
    const double randomY = (2.0 * G4UniformRand() - 1.0) * fMuonBeamSpotHalfSize;
    fParticleGun->SetParticlePosition({randomX, randomY, -30.0 * mm});
  }

  fParticleGun->GeneratePrimaryVertex(event);
}
