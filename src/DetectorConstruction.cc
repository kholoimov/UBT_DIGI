#include "DetectorConstruction.hh"

#include "ScintillatorSensitiveDetector.hh"

#include "G4Box.hh"
#include "G4LogicalVolume.hh"
#include "G4Material.hh"
#include "G4MaterialPropertiesTable.hh"
#include "G4NistManager.hh"
#include "G4PVPlacement.hh"
#include "G4SDManager.hh"
#include "G4SystemOfUnits.hh"

G4VPhysicalVolume* DetectorConstruction::Construct() {
  auto* nist = G4NistManager::Instance();

  auto* worldMaterial = nist->FindOrBuildMaterial("G4_AIR");

  auto* carbon = nist->FindOrBuildElement("C");
  auto* hydrogen = nist->FindOrBuildElement("H");

  auto* scintillator = new G4Material("PlasticScintillator", 1.032 * g / cm3, 2);
  scintillator->AddElement(carbon, 9);
  scintillator->AddElement(hydrogen, 10);

  auto* mpt = new G4MaterialPropertiesTable();

  const G4int nEntries = 2;
  G4double photonEnergy[nEntries] = {2.0 * eV, 3.5 * eV};
  G4double refractiveIndex[nEntries] = {1.58, 1.58};
  G4double absorptionLength[nEntries] = {210.0 * cm, 210.0 * cm};
  G4double emissionSpectrum[nEntries] = {1.0, 1.0};

  mpt->AddProperty("RINDEX", photonEnergy, refractiveIndex, nEntries);
  mpt->AddProperty("ABSLENGTH", photonEnergy, absorptionLength, nEntries);
  mpt->AddProperty("SCINTILLATIONCOMPONENT1", photonEnergy, emissionSpectrum,
                   nEntries);
  mpt->AddConstProperty("SCINTILLATIONYIELD", 10000.0 / MeV);
  mpt->AddConstProperty("RESOLUTIONSCALE", 1.0);
  mpt->AddConstProperty("SCINTILLATIONTIMECONSTANT1", 2.1 * ns);
  mpt->AddConstProperty("SCINTILLATIONYIELD1", 1.0);

  scintillator->SetMaterialPropertiesTable(mpt);

  auto* worldSolid = new G4Box("World", 0.5 * m, 0.5 * m, 0.5 * m);
  auto* worldLogical =
      new G4LogicalVolume(worldSolid, worldMaterial, "WorldLogical");
  auto* worldPhysical =
      new G4PVPlacement(nullptr, {}, worldLogical, "World", nullptr, false, 0);

  auto* scintillatorSolid =
      new G4Box("Scintillator", 20.0 * mm, 20.0 * mm, 7.5 * mm);
  fScintillatorLogical = new G4LogicalVolume(scintillatorSolid, scintillator,
                                             "ScintillatorLogical");

  new G4PVPlacement(nullptr, {}, fScintillatorLogical, "Scintillator",
                    worldLogical, false, 0);

  return worldPhysical;
}

void DetectorConstruction::ConstructSDandField() {
  auto* scintillatorSD =
      new ScintillatorSensitiveDetector("ScintillatorSensitiveDetector");
  G4SDManager::GetSDMpointer()->AddNewDetector(scintillatorSD);
  SetSensitiveDetector(fScintillatorLogical, scintillatorSD);
}
