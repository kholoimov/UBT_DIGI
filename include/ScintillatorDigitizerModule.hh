#ifndef SCINTILLATORDIGITIZERMODULE_HH
#define SCINTILLATORDIGITIZERMODULE_HH

#include <memory>

#include "G4VDigitizerModule.hh"

class ScintillatorDigi;

class ScintillatorDigitizerModule : public G4VDigitizerModule {
 public:
  explicit ScintillatorDigitizerModule(const G4String& moduleName);
  ~ScintillatorDigitizerModule() override = default;

  void Digitize() override;
 const ScintillatorDigi* GetLastDigi() const;

 private:
  std::unique_ptr<ScintillatorDigi> fLastDigi;
};

#endif
