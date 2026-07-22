#include "OutputConfiguration.hh"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <string>

OutputConfiguration::OutputConfiguration() {
  const char* setting = std::getenv("UBT_ENABLE_SCINTILLATOR_PHOTON_STUDIES");
  if (setting == nullptr) {
    return;
  }
  std::string value(setting);
  std::transform(value.begin(), value.end(), value.begin(),
                 [](unsigned char character) {
                   return static_cast<char>(std::tolower(character));
                 });
  fEnableScintillatorPhotonStudies =
      value == "1" || value == "true" || value == "yes" || value == "on";
}
