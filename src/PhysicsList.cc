#include "PhysicsList.hh"

#include "G4OpticalPhysics.hh"
#include "G4OpticalParameters.hh"

PhysicsList::PhysicsList() {
  auto* opticalPhysics = new G4OpticalPhysics();
  G4OpticalParameters::Instance()->SetScintFiniteRiseTime(true);
  RegisterPhysics(opticalPhysics);
}
