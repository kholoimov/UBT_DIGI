#include "DetectorConstruction.hh"

#include "PMTSensitiveDetector.hh"
#include "ScintillatorSensitiveDetector.hh"
#include "TimingModelParameters.hh"

#include "G4Box.hh"
#include "G4LogicalBorderSurface.hh"
#include "G4LogicalVolume.hh"
#include "G4LogicalSkinSurface.hh"
#include "G4Material.hh"
#include "G4MaterialPropertiesTable.hh"
#include "G4NistManager.hh"
#include "G4OpticalSurface.hh"
#include "G4PVPlacement.hh"
#include "G4SDManager.hh"
#include "G4SystemOfUnits.hh"

namespace {
constexpr G4double kScintillatorHalfX = 20.0 * mm;
constexpr G4double kScintillatorHalfY = 20.0 * mm;
constexpr G4double kScintillatorHalfZ = 5.0 * mm;
constexpr G4double kSipmHalfX = 3.0 * mm;
constexpr G4double kSipmHalfY = 3.0 * mm;
constexpr G4double kWrapReflectivity = 0.96;
constexpr G4double kWrapSigmaAlpha = 0.35;
}  // namespace

G4VPhysicalVolume* DetectorConstruction::Construct() {
  auto* nist = G4NistManager::Instance();

  auto* worldMaterial = nist->FindOrBuildMaterial("G4_AIR");
  auto* glassMaterial = nist->FindOrBuildMaterial("G4_Pyrex_Glass");

  auto* carbon = nist->FindOrBuildElement("C");
  auto* hydrogen = nist->FindOrBuildElement("H");
  auto* silicon = nist->FindOrBuildElement("Si");

  auto* scintillator = new G4Material("PTerphenylScintillator", 1.23 * g / cm3, 2);
  scintillator->AddElement(carbon, 18);
  scintillator->AddElement(hydrogen, 14);

  auto* opticalGrease = new G4Material("OpticalGrease", 1.05 * g / cm3, 2);
  opticalGrease->AddElement(carbon, 2);
  opticalGrease->AddElement(hydrogen, 6);

  auto* photocathode = new G4Material("PhotocathodeMaterial", 2.33 * g / cm3, 1);
  photocathode->AddElement(silicon, 1);

  auto* mpt = new G4MaterialPropertiesTable();

  const G4int nEntries = 4;
  G4double photonEnergy[nEntries] = {2.0 * eV, 2.5 * eV, 3.0 * eV, 3.5 * eV};
  G4double scintRefractiveIndex[nEntries] = {1.58, 1.58, 1.58, 1.58};
  G4double absorptionLength[nEntries] = {210.0 * cm, 210.0 * cm, 210.0 * cm,
                                         210.0 * cm};
  G4double emissionSpectrum[nEntries] = {0.2, 0.8, 1.0, 0.4};
  G4double airRefractiveIndex[nEntries] = {1.00, 1.00, 1.00, 1.00};
  G4double greaseRefractiveIndex[nEntries] = {1.46, 1.46, 1.46, 1.46};
  G4double glassRefractiveIndex[nEntries] = {1.49, 1.49, 1.50, 1.50};
  G4double photocathodeRefractiveIndex[nEntries] = {2.7, 2.7, 2.8, 2.8};
  G4double greaseAbsorption[nEntries] = {500.0 * cm, 500.0 * cm, 500.0 * cm,
                                         500.0 * cm};
  G4double glassAbsorption[nEntries] = {300.0 * cm, 300.0 * cm, 300.0 * cm,
                                        300.0 * cm};
  G4double photocathodeAbsorption[nEntries] = {1.0e-6 * mm, 1.0e-6 * mm,
                                               1.0e-6 * mm, 1.0e-6 * mm};
  G4double wrapReflectivity[nEntries] = {kWrapReflectivity, kWrapReflectivity,
                                         kWrapReflectivity, kWrapReflectivity};

  mpt->AddProperty("RINDEX", photonEnergy, scintRefractiveIndex, nEntries);
  mpt->AddProperty("ABSLENGTH", photonEnergy, absorptionLength, nEntries);
  mpt->AddProperty("SCINTILLATIONCOMPONENT1", photonEnergy, emissionSpectrum,
                   nEntries);
  mpt->AddProperty("SCINTILLATIONCOMPONENT2", photonEnergy, emissionSpectrum,
                   nEntries);
  mpt->AddConstProperty("SCINTILLATIONYIELD", 10000.0 / MeV);
  mpt->AddConstProperty("RESOLUTIONSCALE", 1.0);
  mpt->AddConstProperty("SCINTILLATIONTIMECONSTANT1",
                        TimingModelParameters::kFastDecayTimeNs * ns);
  mpt->AddConstProperty("SCINTILLATIONTIMECONSTANT2",
                        TimingModelParameters::kSlowDecayTimeNs * ns);
  mpt->AddConstProperty("SCINTILLATIONRISETIME1",
                        TimingModelParameters::kRiseTimeNs * ns);
  mpt->AddConstProperty("SCINTILLATIONRISETIME2",
                        TimingModelParameters::kRiseTimeNs * ns);
  mpt->AddConstProperty("SCINTILLATIONYIELD1",
                        TimingModelParameters::kFastComponentYield);
  mpt->AddConstProperty("SCINTILLATIONYIELD2",
                        TimingModelParameters::kSlowComponentYield);

  scintillator->SetMaterialPropertiesTable(mpt);

  auto* airMpt = new G4MaterialPropertiesTable();
  airMpt->AddProperty("RINDEX", photonEnergy, airRefractiveIndex, nEntries);
  worldMaterial->SetMaterialPropertiesTable(airMpt);

  auto* greaseMpt = new G4MaterialPropertiesTable();
  greaseMpt->AddProperty("RINDEX", photonEnergy, greaseRefractiveIndex, nEntries);
  greaseMpt->AddProperty("ABSLENGTH", photonEnergy, greaseAbsorption, nEntries);
  opticalGrease->SetMaterialPropertiesTable(greaseMpt);

  auto* glassMpt = new G4MaterialPropertiesTable();
  glassMpt->AddProperty("RINDEX", photonEnergy, glassRefractiveIndex, nEntries);
  glassMpt->AddProperty("ABSLENGTH", photonEnergy, glassAbsorption, nEntries);
  glassMaterial->SetMaterialPropertiesTable(glassMpt);

  auto* photocathodeMpt = new G4MaterialPropertiesTable();
  photocathodeMpt->AddProperty("RINDEX", photonEnergy,
                               photocathodeRefractiveIndex, nEntries);
  photocathodeMpt->AddProperty("ABSLENGTH", photonEnergy,
                               photocathodeAbsorption, nEntries);
  photocathode->SetMaterialPropertiesTable(photocathodeMpt);

  auto* worldSolid = new G4Box("World", 0.5 * m, 0.5 * m, 0.5 * m);
  auto* worldLogical =
      new G4LogicalVolume(worldSolid, worldMaterial, "WorldLogical");
  auto* worldPhysical =
      new G4PVPlacement(nullptr, {}, worldLogical, "World", nullptr, false, 0);

  auto* scintillatorSolid = new G4Box(
      "Scintillator", kScintillatorHalfX, kScintillatorHalfY, kScintillatorHalfZ);
  fScintillatorLogical = new G4LogicalVolume(scintillatorSolid, scintillator,
                                             "ScintillatorLogical");

  auto* scintillatorPhysical =
      new G4PVPlacement(nullptr, {}, fScintillatorLogical, "Scintillator",
                        worldLogical, false, 0);

  constexpr double greaseThickness = 0.2 * mm;
  constexpr double windowThickness = 1.0 * mm;
  constexpr double photocathodeThickness = 0.1 * mm;

  auto* greaseSolid = new G4Box("OpticalGrease", kSipmHalfX, kSipmHalfY,
                                0.5 * greaseThickness);
  auto* greaseLogical =
      new G4LogicalVolume(greaseSolid, opticalGrease, "OpticalGreaseLogical");
  auto* greasePhysical = new G4PVPlacement(
      nullptr, {0.0, 0.0, kScintillatorHalfZ + 0.5 * greaseThickness}, greaseLogical,
      "OpticalGrease", worldLogical, false, 0);

  auto* windowSolid =
      new G4Box("PmtWindow", kSipmHalfX, kSipmHalfY, 0.5 * windowThickness);
  auto* windowLogical =
      new G4LogicalVolume(windowSolid, glassMaterial, "PmtWindowLogical");
  auto* windowPhysical = new G4PVPlacement(
      nullptr,
      {0.0, 0.0, kScintillatorHalfZ + greaseThickness + 0.5 * windowThickness},
      windowLogical, "PmtWindow", worldLogical, false, 0);

  auto* photocathodeSolid = new G4Box(
      "Photocathode", kSipmHalfX, kSipmHalfY, 0.5 * photocathodeThickness);
  fPhotocathodeLogical = new G4LogicalVolume(photocathodeSolid, photocathode,
                                             "PhotocathodeLogical");
  new G4PVPlacement(
      nullptr,
      {0.0, 0.0,
       kScintillatorHalfZ + greaseThickness + windowThickness +
           0.5 * photocathodeThickness},
      fPhotocathodeLogical, "Photocathode", worldLogical, false, 0);

  auto* opticalSurface = new G4OpticalSurface("OpticalCouplingSurface");
  opticalSurface->SetType(dielectric_dielectric);
  opticalSurface->SetModel(unified);
  opticalSurface->SetFinish(polished);

  auto* wrapSurface = new G4OpticalSurface("DiffuseWrapSurface");
  wrapSurface->SetType(dielectric_metal);
  wrapSurface->SetModel(unified);
  wrapSurface->SetFinish(groundfrontpainted);
  wrapSurface->SetSigmaAlpha(kWrapSigmaAlpha);
  auto* wrapMpt = new G4MaterialPropertiesTable();
  wrapMpt->AddProperty("REFLECTIVITY", photonEnergy, wrapReflectivity, nEntries);
  wrapSurface->SetMaterialPropertiesTable(wrapMpt);

  new G4LogicalBorderSurface("ScintillatorToGrease", scintillatorPhysical,
                             greasePhysical, opticalSurface);
  new G4LogicalBorderSurface("GreaseToWindow", greasePhysical, windowPhysical,
                             opticalSurface);
  new G4LogicalSkinSurface("ScintillatorWrap", fScintillatorLogical, wrapSurface);

  return worldPhysical;
}

void DetectorConstruction::ConstructSDandField() {
  auto* scintillatorSD =
      new ScintillatorSensitiveDetector("ScintillatorSensitiveDetector");
  G4SDManager::GetSDMpointer()->AddNewDetector(scintillatorSD);
  SetSensitiveDetector(fScintillatorLogical, scintillatorSD);

  auto* pmtSD = new PMTSensitiveDetector("PMTSensitiveDetector");
  G4SDManager::GetSDMpointer()->AddNewDetector(pmtSD);
  SetSensitiveDetector(fPhotocathodeLogical, pmtSD);
}
