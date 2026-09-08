#include "PMTSensitiveDetector.hh"

#include "EventData.hh"

#include "G4OpticalPhoton.hh"
#include "G4PhysicalConstants.hh"
#include "G4Step.hh"
#include "G4StepPoint.hh"
#include "G4StepStatus.hh"
#include "G4SystemOfUnits.hh"
#include "G4Track.hh"
#include "Randomize.hh"

namespace {
constexpr double kLightCollectionEfficiency = 0.30;

// Hamamatsu S13360-6050PE (6x6 mm^2 active area, 50 um pixel pitch) -- the
// device matching the modeled photosensor footprint and digitizer gain
// scale (kPmtGain in ScintillatorDigitizerModule.cc). Photon detection
// efficiency as a function of wavelength, so that a photon's actual energy
// (not a single averaged constant) sets its detection probability.
//
// Source: Hamamatsu S13360 series datasheet (KAPD1052E):
// - the "Photon detection efficiency vs. wavelength (typical example),
//   Pixel pitch: 50 um" plot (p.4) sets the curve shape (S13360-**50CS/PE,
//   Ta = 25 C);
// - the peak value is anchored to the datasheet's tabulated
//   PDE(lambda = lambda_p = 450 nm) = 40% for the 50 um pitch row at the
//   recommended operating overvoltage (p.3).
// Away from the anchor point the table is a manual reading of the plotted
// curve, not a digitized dataset, and is only reliable to about
// +/-1-2 PDE points. It is tabulated well beyond the ~400-451 nm range
// populated by this simulation's scintillator emission spectrum so that
// Cerenkov photons (also produced within the same RINDEX energy domain)
// are covered without extrapolation.
constexpr int kPdeCurveEntries = 15;
constexpr double kPdeCurveWavelengthNm[kPdeCurveEntries] = {
    300.0, 320.0, 340.0, 360.0, 380.0, 400.0, 420.0, 450.0,
    480.0, 500.0, 550.0, 600.0, 650.0, 700.0, 900.0};
constexpr double kPdeCurveValue[kPdeCurveEntries] = {
    0.03, 0.10, 0.19, 0.27, 0.32, 0.35, 0.38, 0.40,
    0.39, 0.37, 0.30, 0.23, 0.17, 0.13, 0.03};

double WavelengthNm(double photonEnergy) {
  return (CLHEP::h_Planck * CLHEP::c_light / photonEnergy) / nm;
}

// Linear interpolation of the PDE curve above; clamps to the end values
// outside the tabulated wavelength range.
double PhotonDetectionEfficiency(double photonEnergy) {
  const double wavelengthNm = WavelengthNm(photonEnergy);
  if (wavelengthNm <= kPdeCurveWavelengthNm[0]) {
    return kPdeCurveValue[0];
  }
  if (wavelengthNm >= kPdeCurveWavelengthNm[kPdeCurveEntries - 1]) {
    return kPdeCurveValue[kPdeCurveEntries - 1];
  }
  for (int i = 0; i + 1 < kPdeCurveEntries; ++i) {
    if (wavelengthNm <= kPdeCurveWavelengthNm[i + 1]) {
      const double x0 = kPdeCurveWavelengthNm[i];
      const double x1 = kPdeCurveWavelengthNm[i + 1];
      const double y0 = kPdeCurveValue[i];
      const double y1 = kPdeCurveValue[i + 1];
      return y0 + (y1 - y0) * (wavelengthNm - x0) / (x1 - x0);
    }
  }
  return kPdeCurveValue[kPdeCurveEntries - 1];
}
}  // namespace

PMTSensitiveDetector::PMTSensitiveDetector(const G4String& name)
    : G4VSensitiveDetector(name) {}

G4bool PMTSensitiveDetector::ProcessHits(G4Step* step, G4TouchableHistory*) {
  auto* track = step->GetTrack();
  if (track == nullptr ||
      track->GetDefinition() != G4OpticalPhoton::OpticalPhotonDefinition()) {
    return false;
  }

  const auto* preStepPoint = step->GetPreStepPoint();
  if (preStepPoint == nullptr ||
      preStepPoint->GetStepStatus() != fGeomBoundary) {
    return false;
  }

  if (G4UniformRand() >= kLightCollectionEfficiency) {
    track->SetTrackStatus(fStopAndKill);
    return true;
  }

  const auto& vertexPosition = track->GetVertexPosition();
  const double birthTime = track->GetGlobalTime() - track->GetLocalTime();
  EventData::Instance().AddPmtIncidentPhoton(
      track->GetGlobalTime(), birthTime, vertexPosition.x(),
      vertexPosition.y(), vertexPosition.z());
  if (G4UniformRand() < PhotonDetectionEfficiency(track->GetTotalEnergy())) {
    EventData::Instance().AddPmtPhotoelectron(track->GetGlobalTime());
  }

  track->SetTrackStatus(fStopAndKill);
  return true;
}
