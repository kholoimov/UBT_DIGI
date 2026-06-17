#include "PhysicsList.hh"

#include "G4OpticalPhysics.hh"

PhysicsList::PhysicsList() {
  auto* opticalPhysics = new G4OpticalPhysics();
  opticalPhysics->SetScintillationTrackSecondariesFirst(true);
  RegisterPhysics(opticalPhysics);
}
