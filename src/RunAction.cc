#include "RunAction.hh"

#include "EventData.hh"
#include "ScintillatorDigi.hh"

#include "G4AnalysisManager.hh"
#include "G4AutoLock.hh"
#include "G4Run.hh"
#include "G4SystemOfUnits.hh"
#include "G4Threading.hh"
#include "G4ios.hh"

#include <algorithm>
#include <array>

namespace {
constexpr double kPicocoulomb = 1.0e-12 * CLHEP::coulomb;
constexpr int kTimingHistogramBins = 200;
constexpr double kTimingHistogramMinNs = 0.0;
constexpr double kTimingHistogramMaxNs = 50.0;

G4Mutex gTimingHistogramMutex = G4MUTEX_INITIALIZER;
std::array<int, kTimingHistogramBins> gScintillationTimingCounts = {};
std::array<int, kTimingHistogramBins> gPhotoelectronTimingCounts = {};

void ResetHistogramCounts() {
  gScintillationTimingCounts.fill(0);
  gPhotoelectronTimingCounts.fill(0);
}

void FillHistogramCounts(const std::vector<double>& times,
                         std::array<int, kTimingHistogramBins>& counts) {
  constexpr double binWidthNs =
      (kTimingHistogramMaxNs - kTimingHistogramMinNs) / kTimingHistogramBins;

  for (const double time : times) {
    const double timeNs = time / CLHEP::ns;
    if (timeNs < kTimingHistogramMinNs || timeNs >= kTimingHistogramMaxNs) {
      continue;
    }

    const int bin =
        static_cast<int>((timeNs - kTimingHistogramMinNs) / binWidthNs);
    if (bin >= 0 && bin < kTimingHistogramBins) {
      ++counts[bin];
    }
  }
}

double InterpolateHalfMaximum(double x1, double y1, double x2, double y2,
                              double halfMaximum) {
  if (y1 == y2) {
    return 0.5 * (x1 + x2);
  }

  return x1 + (halfMaximum - y1) * (x2 - x1) / (y2 - y1);
}

double ComputeFwhmNs(const std::array<int, kTimingHistogramBins>& counts) {
  const int peak = *std::max_element(counts.begin(), counts.end());
  if (peak <= 0) {
    return -1.0;
  }

  const double halfMaximum = 0.5 * peak;
  const auto leftIt =
      std::find_if(counts.begin(), counts.end(),
                   [halfMaximum](int count) { return count >= halfMaximum; });
  const auto rightIt =
      std::find_if(counts.rbegin(), counts.rend(),
                   [halfMaximum](int count) { return count >= halfMaximum; });
  if (leftIt == counts.end() || rightIt == counts.rend()) {
    return -1.0;
  }

  constexpr double binWidthNs =
      (kTimingHistogramMaxNs - kTimingHistogramMinNs) / kTimingHistogramBins;
  const int leftIndex = static_cast<int>(std::distance(counts.begin(), leftIt));
  const int rightIndex =
      static_cast<int>(counts.size() - 1 -
                       std::distance(counts.rbegin(), rightIt));

  double leftCrossingNs = kTimingHistogramMinNs + leftIndex * binWidthNs;
  if (leftIndex > 0) {
    leftCrossingNs = InterpolateHalfMaximum(
        kTimingHistogramMinNs + (leftIndex - 0.5) * binWidthNs,
        counts[leftIndex - 1],
        kTimingHistogramMinNs + (leftIndex + 0.5) * binWidthNs,
        counts[leftIndex], halfMaximum);
  }

  double rightCrossingNs =
      kTimingHistogramMinNs + (rightIndex + 1) * binWidthNs;
  if (rightIndex + 1 < static_cast<int>(counts.size())) {
    rightCrossingNs = InterpolateHalfMaximum(
        kTimingHistogramMinNs + (rightIndex + 0.5) * binWidthNs,
        counts[rightIndex],
        kTimingHistogramMinNs + (rightIndex + 1.5) * binWidthNs,
        counts[rightIndex + 1], halfMaximum);
  }

  return std::max(0.0, rightCrossingNs - leftCrossingNs);
}
}  // namespace

RunAction::RunAction() {
  auto* analysisManager = G4AnalysisManager::Instance();
  analysisManager->SetDefaultFileType("root");
  analysisManager->SetFileName("scintillator_digi");
  analysisManager->SetVerboseLevel(1);
  analysisManager->SetNtupleMerging(true);
  analysisManager->CreateH1("scintillation_production_time_ns",
                            "Scintillation photon production time;time [ns];counts",
                            kTimingHistogramBins, kTimingHistogramMinNs,
                            kTimingHistogramMaxNs);
  analysisManager->CreateH1("photoelectron_arrival_time_ns",
                            "Detected photoelectron arrival time;time [ns];counts",
                            kTimingHistogramBins, kTimingHistogramMinNs,
                            kTimingHistogramMaxNs);
  analysisManager->CreateNtuple("events", "Digitized scintillator data");
  analysisManager->CreateNtupleIColumn("event_id");
  analysisManager->CreateNtupleSColumn("primary_particle");
  analysisManager->CreateNtupleDColumn("primary_energy_mev");
  analysisManager->CreateNtupleDColumn("primary_momentum_mev_c");
  analysisManager->CreateNtupleDColumn("muon_range_mm");
  analysisManager->CreateNtupleDColumn("edep_mev");
  analysisManager->CreateNtupleIColumn("scintillation_photons");
  analysisManager->CreateNtupleIColumn("pmt_incident_photons");
  analysisManager->CreateNtupleDColumn("photoelectrons");
  analysisManager->CreateNtupleDColumn("pmt_first_hit_ns");
  analysisManager->CreateNtupleDColumn("pmt_charge_pc");
  analysisManager->CreateNtupleIColumn("adc_counts");
  analysisManager->CreateNtupleIColumn("triggered");
  analysisManager->FinishNtuple();
  analysisManager->CreateNtuple("run_summary", "Run-level timing summary");
  analysisManager->CreateNtupleDColumn("scintillation_production_fwhm_ns");
  analysisManager->CreateNtupleDColumn("photoelectron_arrival_fwhm_ns");
  analysisManager->FinishNtuple();
}

RunAction::~RunAction() {
  delete G4AnalysisManager::Instance();
}

void RunAction::BeginOfRunAction(const G4Run*) {
  if (G4Threading::IsMasterThread()) {
    G4AutoLock lock(&gTimingHistogramMutex);
    ResetHistogramCounts();
  }

  G4cout << "Opening ROOT output on "
         << (G4Threading::IsMasterThread() ? "master" : "worker")
         << " thread." << G4endl;
  G4AnalysisManager::Instance()->OpenFile();
}

void RunAction::EndOfRunAction(const G4Run*) {
  auto* analysisManager = G4AnalysisManager::Instance();
  double scintillationFwhmNs = -1.0;
  double photoelectronFwhmNs = -1.0;

  if (G4Threading::IsMasterThread()) {
    G4AutoLock lock(&gTimingHistogramMutex);
    scintillationFwhmNs = ComputeFwhmNs(gScintillationTimingCounts);
    photoelectronFwhmNs = ComputeFwhmNs(gPhotoelectronTimingCounts);
    analysisManager->FillNtupleDColumn(1, 0, scintillationFwhmNs);
    analysisManager->FillNtupleDColumn(1, 1, photoelectronFwhmNs);
    analysisManager->AddNtupleRow(1);
  }

  analysisManager->Write();
  analysisManager->CloseFile();
  if (G4Threading::IsMasterThread()) {
    G4cout << "Digitized event data written to scintillator_digi.root"
           << " | FWHM(scint)=" << scintillationFwhmNs
           << " ns, FWHM(pe)=" << photoelectronFwhmNs << " ns" << G4endl;
  }
}

void RunAction::RecordDigi(const ScintillatorDigi& digi) {
  auto* analysisManager = G4AnalysisManager::Instance();
  analysisManager->FillNtupleIColumn(0, digi.GetEventID());
  analysisManager->FillNtupleSColumn(1, digi.GetPrimaryParticle());
  analysisManager->FillNtupleDColumn(2,
                                     digi.GetPrimaryKineticEnergy() / CLHEP::MeV);
  analysisManager->FillNtupleDColumn(3, digi.GetPrimaryMomentum() / CLHEP::MeV);
  analysisManager->FillNtupleDColumn(4,
                                     digi.GetPrimaryMuonTrackLength() / CLHEP::mm);
  analysisManager->FillNtupleDColumn(5, digi.GetEnergyDeposit() / CLHEP::MeV);
  analysisManager->FillNtupleIColumn(6, digi.GetScintillationPhotons());
  analysisManager->FillNtupleIColumn(7, digi.GetPmtIncidentPhotons());
  analysisManager->FillNtupleDColumn(8, digi.GetDetectedPhotoelectrons());
  analysisManager->FillNtupleDColumn(9, digi.GetFirstPmtHitTime() >= 0.0
                                            ? digi.GetFirstPmtHitTime() / CLHEP::ns
                                            : -1.0);
  analysisManager->FillNtupleDColumn(10, digi.GetPmtCharge() / kPicocoulomb);
  analysisManager->FillNtupleIColumn(11, digi.GetAdcCounts());
  analysisManager->FillNtupleIColumn(12, digi.GetTriggered() ? 1 : 0);
  analysisManager->AddNtupleRow();

  for (const double time : EventData::Instance().GetScintillationPhotonTimes()) {
    analysisManager->FillH1(0, time / CLHEP::ns);
  }

  for (const double time : EventData::Instance().GetPmtPhotoelectronTimes()) {
    analysisManager->FillH1(1, time / CLHEP::ns);
  }

  G4AutoLock lock(&gTimingHistogramMutex);
  FillHistogramCounts(EventData::Instance().GetScintillationPhotonTimes(),
                      gScintillationTimingCounts);
  FillHistogramCounts(EventData::Instance().GetPmtPhotoelectronTimes(),
                      gPhotoelectronTimingCounts);
}
