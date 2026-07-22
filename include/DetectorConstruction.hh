#ifndef DETECTORCONSTRUCTION_HH
#define DETECTORCONSTRUCTION_HH

#include "G4VUserDetectorConstruction.hh"

class G4LogicalVolume;
class G4VPhysicalVolume;

class DetectorConstruction : public G4VUserDetectorConstruction {
 public:
  explicit DetectorConstruction(double scintillatorSizeMm)
      : fScintillatorHalfSize(0.5 * scintillatorSizeMm) {}
  ~DetectorConstruction() override = default;

  G4VPhysicalVolume* Construct() override;
  void ConstructSDandField() override;

 private:
  G4LogicalVolume* fScintillatorLogical = nullptr;
  G4LogicalVolume* fPhotocathodeLogical = nullptr;
  double fScintillatorHalfSize;
};

#endif
