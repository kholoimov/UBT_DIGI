#include "RunAction.hh"

#include <array>

#include "EventData.hh"
#include "G4AnalysisManager.hh"
#include "G4Run.hh"
#include "G4SystemOfUnits.hh"
#include "G4Threading.hh"
#include "G4ios.hh"
#include "ScintillatorDigi.hh"

namespace {
constexpr double kPicocoulomb = 1.0e-12 * CLHEP::coulomb;
constexpr int kTimingHistogramBins = 200;
constexpr double kTimingHistogramMinNs = 0.0;
constexpr double kTimingHistogramMaxNs = 50.0;
constexpr int kArrivalTimeColumnOffset = 16;
constexpr int kTileSizeColumn = kArrivalTimeColumnOffset + 20;
constexpr std::array<int, 20> kStoredArrivalPe = {
    1,  2,  3,  4,  5,   10,  20,  30,  40,  50,
    60, 70, 80, 90, 100, 120, 140, 160, 180, 200};
}  // namespace

RunAction::RunAction(bool enableScintillatorPhotonStudies,
                     double scintillatorSizeMm)
    : fEnableScintillatorPhotonStudies(enableScintillatorPhotonStudies),
      fScintillatorSizeMm(scintillatorSizeMm) {
  auto* analysisManager = G4AnalysisManager::Instance();
  analysisManager->SetDefaultFileType("root");
  analysisManager->SetFileName("scintillator_digi");
  analysisManager->SetVerboseLevel(1);
  analysisManager->SetNtupleMerging(true);
  analysisManager->CreateH1(
      "scintillation_production_time_ns",
      "Scintillation photon production time;time [ns];counts",
      kTimingHistogramBins, kTimingHistogramMinNs, kTimingHistogramMaxNs);
  analysisManager->CreateH1(
      "photoelectron_arrival_time_ns",
      "Detected photoelectron arrival time;time [ns];counts",
      kTimingHistogramBins, kTimingHistogramMinNs, kTimingHistogramMaxNs);
  analysisManager->CreateNtuple("events",
                                "General event and digitization studies");
  analysisManager->CreateNtupleIColumn("event_id");
  analysisManager->CreateNtupleSColumn("primary_particle");
  analysisManager->CreateNtupleDColumn("primary_energy_mev");
  analysisManager->CreateNtupleDColumn("primary_momentum_mev_c");
  analysisManager->CreateNtupleDColumn("muon_range_mm");
  analysisManager->CreateNtupleDColumn("primary_hit_x_mm");
  analysisManager->CreateNtupleDColumn("primary_hit_y_mm");
  analysisManager->CreateNtupleDColumn("edep_mev");
  analysisManager->CreateNtupleIColumn("scintillation_photons");
  analysisManager->CreateNtupleIColumn("pmt_incident_photons");
  analysisManager->CreateNtupleDColumn("photoelectrons");
  analysisManager->CreateNtupleDColumn("pmt_first_hit_ns");
  analysisManager->CreateNtupleDColumn("pmt_charge_pc");
  analysisManager->CreateNtupleIColumn("adc_counts");
  analysisManager->CreateNtupleIColumn("triggered");
  analysisManager->CreateNtupleDColumn(
      "photoelectron_threshold_5_from_muon_ns");
  for (int thresholdPe : kStoredArrivalPe) {
    analysisManager->CreateNtupleDColumn("photoelectron_arrival_" +
                                         std::to_string(thresholdPe) +
                                         "_from_muon_ns");
  }
  analysisManager->CreateNtupleDColumn("tile_size_mm");
  analysisManager->FinishNtuple();
  analysisManager->CreateNtuple(
      "pmt_photon_births",
      "Birth positions of photons that reached the sensor");
  analysisManager->CreateNtupleIColumn("event_id");
  analysisManager->CreateNtupleDColumn("birth_x_mm");
  analysisManager->CreateNtupleDColumn("birth_y_mm");
  analysisManager->CreateNtupleDColumn("birth_z_mm");
  analysisManager->CreateNtupleDColumn("birth_time_ns");
  analysisManager->CreateNtupleDColumn("arrival_time_ns");
  analysisManager->FinishNtuple();
  analysisManager->CreateNtuple("scintillation_photon_birth_times",
                                "Birth times of all scintillation photons");
  analysisManager->CreateNtupleIColumn("event_id");
  analysisManager->CreateNtupleDColumn("birth_time_ns");
  analysisManager->FinishNtuple();
}

RunAction::~RunAction() { delete G4AnalysisManager::Instance(); }

void RunAction::BeginOfRunAction(const G4Run*) {
  G4cout << "Opening ROOT output on "
         << (G4Threading::IsMasterThread() ? "master" : "worker") << " thread."
         << G4endl;
  if (G4Threading::IsMasterThread()) {
    G4cout << "Scintillator transverse size: " << fScintillatorSizeMm << " x "
           << fScintillatorSizeMm << " mm2; SiPM active area: 6 x 6 mm2"
           << G4endl;
    G4cout << "Per-photon studies: "
           << (GetEnableScintillatorPhotonStudies() ? "enabled" : "disabled")
           << G4endl;
  }
  G4AnalysisManager::Instance()->OpenFile();
}

void RunAction::EndOfRunAction(const G4Run*) {
  auto* analysisManager = G4AnalysisManager::Instance();
  analysisManager->Write();
  analysisManager->CloseFile();
  if (G4Threading::IsMasterThread()) {
    G4cout << "Digitized event data written to scintillator_digi.root"
           << G4endl;
  }
}

void RunAction::RecordDigi(const ScintillatorDigi& digi) {
  auto* analysisManager = G4AnalysisManager::Instance();
  analysisManager->FillNtupleIColumn(0, digi.GetEventID());
  analysisManager->FillNtupleSColumn(1, digi.GetPrimaryParticle());
  analysisManager->FillNtupleDColumn(
      2, digi.GetPrimaryKineticEnergy() / CLHEP::MeV);
  analysisManager->FillNtupleDColumn(3, digi.GetPrimaryMomentum() / CLHEP::MeV);
  analysisManager->FillNtupleDColumn(
      4, digi.GetPrimaryMuonTrackLength() / CLHEP::mm);
  analysisManager->FillNtupleDColumn(5, digi.GetPrimaryHitX() / CLHEP::mm);
  analysisManager->FillNtupleDColumn(6, digi.GetPrimaryHitY() / CLHEP::mm);
  analysisManager->FillNtupleDColumn(7, digi.GetEnergyDeposit() / CLHEP::MeV);
  analysisManager->FillNtupleIColumn(8, digi.GetScintillationPhotons());
  analysisManager->FillNtupleIColumn(9, digi.GetPmtIncidentPhotons());
  analysisManager->FillNtupleDColumn(10, digi.GetDetectedPhotoelectrons());
  analysisManager->FillNtupleDColumn(11,
                                     digi.GetFirstPmtHitTime() >= 0.0
                                         ? digi.GetFirstPmtHitTime() / CLHEP::ns
                                         : -1.0);
  analysisManager->FillNtupleDColumn(12, digi.GetPmtCharge() / kPicocoulomb);
  analysisManager->FillNtupleIColumn(13, digi.GetAdcCounts());
  analysisManager->FillNtupleIColumn(14, digi.GetTriggered() ? 1 : 0);
  analysisManager->FillNtupleDColumn(
      15, digi.GetThreshold80TimeFromPrimary() >= 0.0
              ? digi.GetThreshold80TimeFromPrimary() / CLHEP::ns
              : -1.0);
  for (std::size_t i = 0; i < kStoredArrivalPe.size(); ++i) {
    analysisManager->FillNtupleDColumn(
        kArrivalTimeColumnOffset + i,
        digi.GetPhotoelectronArrivalTime(kStoredArrivalPe[i] - 1) >= 0.0
            ? digi.GetPhotoelectronArrivalTime(kStoredArrivalPe[i] - 1) /
                  CLHEP::ns
            : -1.0);
  }
  analysisManager->FillNtupleDColumn(kTileSizeColumn, fScintillatorSizeMm);
  analysisManager->AddNtupleRow();

  if (!GetEnableScintillatorPhotonStudies()) {
    return;
  }

  for (const auto& photon :
       EventData::Instance().GetPmtIncidentPhotonRecords()) {
    analysisManager->FillNtupleIColumn(1, 0, digi.GetEventID());
    analysisManager->FillNtupleDColumn(1, 1, photon[0] / CLHEP::mm);
    analysisManager->FillNtupleDColumn(1, 2, photon[1] / CLHEP::mm);
    analysisManager->FillNtupleDColumn(1, 3, photon[2] / CLHEP::mm);
    analysisManager->FillNtupleDColumn(1, 4, photon[3] / CLHEP::ns);
    analysisManager->FillNtupleDColumn(1, 5, photon[4] / CLHEP::ns);
    analysisManager->AddNtupleRow(1);
  }

  for (const double time :
       EventData::Instance().GetScintillationPhotonTimes()) {
    analysisManager->FillNtupleIColumn(2, 0, digi.GetEventID());
    analysisManager->FillNtupleDColumn(2, 1, time / CLHEP::ns);
    analysisManager->AddNtupleRow(2);
    analysisManager->FillH1(0, time / CLHEP::ns);
  }

  for (const double time : EventData::Instance().GetPmtPhotoelectronTimes()) {
    analysisManager->FillH1(1, time / CLHEP::ns);
  }
}
