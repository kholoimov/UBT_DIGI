#include "ActionInitialization.hh"
#include "DetectorConstruction.hh"
#include "G4RunManagerFactory.hh"
#include "G4UIExecutive.hh"
#include "G4UImanager.hh"
#include "G4VisExecutive.hh"
#include "OutputConfiguration.hh"
#include "PhysicsList.hh"

int main(int argc, char** argv) {
  auto* runManager = G4RunManagerFactory::CreateRunManager();

  runManager->SetUserInitialization(new DetectorConstruction());
  runManager->SetUserInitialization(new PhysicsList());
  auto* outputConfiguration = new OutputConfiguration();
  runManager->SetUserInitialization(
      new ActionInitialization(outputConfiguration));

  auto* visManager = new G4VisExecutive();
  visManager->Initialize();

  auto* uiManager = G4UImanager::GetUIpointer();

  if (argc == 1) {
    auto* ui = new G4UIExecutive(argc, argv);
    uiManager->ApplyCommand("/control/execute macros/vis.mac");
    ui->SessionStart();
    delete ui;
  } else {
    G4String command = "/control/execute ";
    G4String macroName = argv[1];
    uiManager->ApplyCommand(command + macroName);
  }

  delete visManager;
  delete runManager;
  delete outputConfiguration;
  return 0;
}
