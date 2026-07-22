#ifndef OUTPUTCONFIGURATION_HH
#define OUTPUTCONFIGURATION_HH

class OutputConfiguration {
 public:
  OutputConfiguration();
  ~OutputConfiguration() = default;

  bool GetEnableScintillatorPhotonStudies() const {
    return fEnableScintillatorPhotonStudies;
  }

 private:
  bool fEnableScintillatorPhotonStudies = false;
};

#endif
