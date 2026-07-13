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
#include <cmath>

namespace {
constexpr double kPicocoulomb = 1.0e-12 * CLHEP::coulomb;
constexpr int kTimingHistogramBins = 200;
constexpr double kTimingHistogramMinNs = 0.0;
constexpr double kTimingHistogramMaxNs = 50.0;
constexpr std::array<int, 13> kThresholdScanPe = {1, 2, 3, 4, 5, 6, 7,
                                                  8, 9, 10, 20, 30, 40};

G4Mutex gTimingHistogramMutex = G4MUTEX_INITIALIZER;
std::array<int, kTimingHistogramBins> gScintillationTimingCounts = {};
double gThreshold80TimeSumNs = 0.0;
int gThreshold80TimeCount = 0;
std::array<double, kThresholdScanPe.size()> gThresholdScanTimeSumNs = {};
std::array<double, kThresholdScanPe.size()> gThresholdScanTimeSqSumNs = {};
std::array<int, kThresholdScanPe.size()> gThresholdScanCounts = {};

void ResetHistogramCounts() {
  gScintillationTimingCounts.fill(0);
  gThreshold80TimeSumNs = 0.0;
  gThreshold80TimeCount = 0;
  gThresholdScanTimeSumNs.fill(0.0);
  gThresholdScanTimeSqSumNs.fill(0.0);
  gThresholdScanCounts.fill(0);
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

double ComputeThresholdTimeNs(const std::vector<double>& times,
                              double primaryHitTime, int thresholdPe) {
  if (primaryHitTime < 0.0 ||
      times.size() < static_cast<std::size_t>(thresholdPe)) {
    return -1.0;
  }

  auto sortedTimes = times;
  std::sort(sortedTimes.begin(), sortedTimes.end());
  return (sortedTimes[thresholdPe - 1] - primaryHitTime) / CLHEP::ns;
}

double ComputeSigmaNs(double sumNs, double sumSqNs, int count) {
  if (count <= 1) {
    return -1.0;
  }

  const double meanNs = sumNs / count;
  const double varianceNs =
      std::max(0.0, (sumSqNs / count) - meanNs * meanNs);
  return std::sqrt(varianceNs);
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
  analysisManager->CreateNtupleDColumn("scintillation_production_fwhm_ns");
  analysisManager->CreateNtupleDColumn("photoelectron_threshold_80_from_muon_ns");
  analysisManager->CreateNtupleIColumn("threshold_scan_pe");
  analysisManager->CreateNtupleDColumn("threshold_scan_sigma_ns");
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
  double meanThreshold80TimeNs = -1.0;

  if (G4Threading::IsMasterThread()) {
    G4AutoLock lock(&gTimingHistogramMutex);
    scintillationFwhmNs = ComputeFwhmNs(gScintillationTimingCounts);
    if (gThreshold80TimeCount > 0) {
      meanThreshold80TimeNs = gThreshold80TimeSumNs / gThreshold80TimeCount;
    }
    analysisManager->FillNtupleIColumn(0, 0, -1);
    analysisManager->FillNtupleSColumn(0, 1, "RUN_SUMMARY");
    analysisManager->FillNtupleDColumn(0, 2, -1.0);
    analysisManager->FillNtupleDColumn(0, 3, -1.0);
    analysisManager->FillNtupleDColumn(0, 4, -1.0);
    analysisManager->FillNtupleDColumn(0, 5, -1.0);
    analysisManager->FillNtupleIColumn(0, 6, -1);
    analysisManager->FillNtupleIColumn(0, 7, -1);
    analysisManager->FillNtupleDColumn(0, 8, -1.0);
    analysisManager->FillNtupleDColumn(0, 9, -1.0);
    analysisManager->FillNtupleDColumn(0, 10, -1.0);
    analysisManager->FillNtupleIColumn(0, 11, -1);
    analysisManager->FillNtupleIColumn(0, 12, -1);
    analysisManager->FillNtupleDColumn(0, 13, scintillationFwhmNs);
    analysisManager->FillNtupleDColumn(0, 14, meanThreshold80TimeNs);
    analysisManager->FillNtupleIColumn(0, 15, -1);
    analysisManager->FillNtupleDColumn(0, 16, -1.0);
    analysisManager->AddNtupleRow(0);

    for (std::size_t i = 0; i < kThresholdScanPe.size(); ++i) {
      analysisManager->FillNtupleIColumn(0, 0, -2);
      analysisManager->FillNtupleSColumn(0, 1, "THRESHOLD_SCAN");
      analysisManager->FillNtupleDColumn(0, 2, -1.0);
      analysisManager->FillNtupleDColumn(0, 3, -1.0);
      analysisManager->FillNtupleDColumn(0, 4, -1.0);
      analysisManager->FillNtupleDColumn(0, 5, -1.0);
      analysisManager->FillNtupleIColumn(0, 6, -1);
      analysisManager->FillNtupleIColumn(0, 7, -1);
      analysisManager->FillNtupleDColumn(0, 8, -1.0);
      analysisManager->FillNtupleDColumn(0, 9, -1.0);
      analysisManager->FillNtupleDColumn(0, 10, -1.0);
      analysisManager->FillNtupleIColumn(0, 11, -1);
      analysisManager->FillNtupleIColumn(0, 12, -1);
      analysisManager->FillNtupleDColumn(0, 13, -1.0);
      analysisManager->FillNtupleDColumn(0, 14, -1.0);
      analysisManager->FillNtupleIColumn(0, 15, kThresholdScanPe[i]);
      analysisManager->FillNtupleDColumn(
          0, 16, ComputeSigmaNs(gThresholdScanTimeSumNs[i],
                                gThresholdScanTimeSqSumNs[i],
                                gThresholdScanCounts[i]));
      analysisManager->AddNtupleRow(0);
    }
  }

  analysisManager->Write();
  analysisManager->CloseFile();
  if (G4Threading::IsMasterThread()) {
    G4cout << "Run summary: FWHM(scintillation production) = "
           << scintillationFwhmNs
           << " ns, mean t80 from muon hit = "
           << meanThreshold80TimeNs << " ns" << G4endl;
    G4cout << "Threshold scan sigma(ns):";
    for (std::size_t i = 0; i < kThresholdScanPe.size(); ++i) {
      G4cout << " " << kThresholdScanPe[i] << "pe="
             << ComputeSigmaNs(gThresholdScanTimeSumNs[i],
                               gThresholdScanTimeSqSumNs[i],
                               gThresholdScanCounts[i]);
    }
    G4cout << G4endl;
    G4cout << "Digitized event data written to scintillator_digi.root"
           << G4endl;
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
  analysisManager->FillNtupleDColumn(13, -1.0);
  analysisManager->FillNtupleDColumn(
      14, digi.GetThreshold80TimeFromPrimary() >= 0.0
              ? digi.GetThreshold80TimeFromPrimary() / CLHEP::ns
              : -1.0);
  analysisManager->FillNtupleIColumn(15, -1);
  analysisManager->FillNtupleDColumn(16, -1.0);
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
  if (digi.GetThreshold80TimeFromPrimary() >= 0.0) {
    gThreshold80TimeSumNs += digi.GetThreshold80TimeFromPrimary() / CLHEP::ns;
    ++gThreshold80TimeCount;
  }
  for (std::size_t i = 0; i < kThresholdScanPe.size(); ++i) {
    const double thresholdTimeNs = ComputeThresholdTimeNs(
        EventData::Instance().GetPmtPhotoelectronTimes(),
        EventData::Instance().GetPrimaryHitTime(), kThresholdScanPe[i]);
    if (thresholdTimeNs >= 0.0) {
      gThresholdScanTimeSumNs[i] += thresholdTimeNs;
      gThresholdScanTimeSqSumNs[i] += thresholdTimeNs * thresholdTimeNs;
      ++gThresholdScanCounts[i];
    }
  }
}
