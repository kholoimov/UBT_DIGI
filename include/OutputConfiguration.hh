#ifndef OUTPUTCONFIGURATION_HH
#define OUTPUTCONFIGURATION_HH

class OutputConfiguration {
 public:
  OutputConfiguration();
  ~OutputConfiguration() = default;

  bool GetEnableScintillatorPhotonStudies() const {
    return fEnableScintillatorPhotonStudies;
  }
  double GetScintillatorSizeMm() const { return fScintillatorSizeMm; }

 private:
  bool fEnableScintillatorPhotonStudies = false;
  double fScintillatorSizeMm = 40.0;
};

#endif
