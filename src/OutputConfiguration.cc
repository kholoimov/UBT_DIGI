#include "OutputConfiguration.hh"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <stdexcept>
#include <string>

OutputConfiguration::OutputConfiguration() {
  const char* setting = std::getenv("UBT_ENABLE_SCINTILLATOR_PHOTON_STUDIES");
  if (setting != nullptr) {
    std::string value(setting);
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char character) {
                     return static_cast<char>(std::tolower(character));
                   });
    fEnableScintillatorPhotonStudies =
        value == "1" || value == "true" || value == "yes" || value == "on";
  }

  const char* tileSizeSetting = std::getenv("UBT_SCINTILLATOR_SIZE_MM");
  if (tileSizeSetting == nullptr) {
    return;
  }
  fScintillatorSizeMm = std::stod(tileSizeSetting);
  if (fScintillatorSizeMm != 20.0 && fScintillatorSizeMm != 40.0) {
    throw std::invalid_argument(
        "UBT_SCINTILLATOR_SIZE_MM must be either 20 or 40");
  }
}
