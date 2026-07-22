#ifndef ACTIONINITIALIZATION_HH
#define ACTIONINITIALIZATION_HH

#include "G4VUserActionInitialization.hh"

class OutputConfiguration;

class ActionInitialization : public G4VUserActionInitialization {
 public:
  explicit ActionInitialization(const OutputConfiguration* outputConfiguration)
      : fOutputConfiguration(outputConfiguration) {}
  ~ActionInitialization() override = default;

  void Build() const override;
  void BuildForMaster() const override;

 private:
  const OutputConfiguration* fOutputConfiguration;
};

#endif
