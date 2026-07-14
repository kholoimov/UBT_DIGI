#include "TCanvas.h"
#include "TFile.h"
#include "TGraph.h"
#include "TH1D.h"
#include "TH2D.h"
#include "TLegend.h"
#include "TProfile.h"
#include "TROOT.h"
#include "TString.h"
#include "TStyle.h"
#include "TSystem.h"
#include "TTree.h"

#include <iostream>
#include <array>
#include <memory>
#include <string>
#include <vector>

namespace {
constexpr int kMaxArrivalIndices = 200;
constexpr std::array<int, 7> kOverlayIndices = {1, 5, 10, 20, 50, 100, 200};

void ConfigureStyle() {
  gROOT->SetBatch(kTRUE);
  gStyle->SetOptStat(0);
  gStyle->SetTitleFontSize(0.04);
  gStyle->SetPadLeftMargin(0.12);
  gStyle->SetPadRightMargin(0.14);
  gStyle->SetPadBottomMargin(0.12);
}

void SaveCanvas(TCanvas& canvas, const TString& outputDir,
                const TString& baseName) {
  // canvas.SaveAs(outputDir + "/" + baseName + ".png");
  canvas.SaveAs(outputDir + "/" + baseName + ".pdf");
}

}  // namespace

void plot_event_observables(
    const char* inputFile = "scintillator_digi.root",
    const char* outputDir = "analysis/plots") {
  ConfigureStyle();
  gSystem->mkdir(outputDir, kTRUE);

  std::unique_ptr<TFile> file(TFile::Open(inputFile, "READ"));
  if (!file || file->IsZombie()) {
    std::cerr << "Failed to open ROOT file: " << inputFile << std::endl;
    return;
  }

  auto* events = dynamic_cast<TTree*>(file->Get("events"));
  if (events == nullptr) {
    std::cerr << "Could not find TTree 'events' in " << inputFile << std::endl;
    return;
  }

  int eventId = -1;
  char primaryParticle[128] = {};
  double primaryEnergyMeV = 0.0;
  int scintillationPhotons = 0;
  int pmtIncidentPhotons = 0;
  double photoelectrons = 0.0;
  int thresholdScanPe = -1;
  double thresholdScanSigmaNs = -1.0;
  std::array<double, kMaxArrivalIndices> arrivalTimesNs = {};

  events->SetBranchAddress("event_id", &eventId);
  events->SetBranchAddress("primary_particle", primaryParticle);
  events->SetBranchAddress("primary_energy_mev", &primaryEnergyMeV);
  events->SetBranchAddress("scintillation_photons", &scintillationPhotons);
  events->SetBranchAddress("pmt_incident_photons", &pmtIncidentPhotons);
  events->SetBranchAddress("photoelectrons", &photoelectrons);
  events->SetBranchAddress("threshold_scan_pe", &thresholdScanPe);
  events->SetBranchAddress("threshold_scan_sigma_ns", &thresholdScanSigmaNs);
  for (int i = 0; i < kMaxArrivalIndices; ++i) {
    const TString branchName =
        TString::Format("photoelectron_arrival_%d_from_muon_ns", i + 1);
    events->SetBranchAddress(branchName, &arrivalTimesNs[i]);
  }

  auto lightVsEnergy = std::make_unique<TH2D>(
      "light_vs_muon_energy",
      "Light Production vs Muon Energy;Muon kinetic energy [MeV];Scintillation photons",
      80, 0.0, 20000.0, 120, 15000.0, 40000.0);
  auto lightVsEnergyProfile = std::make_unique<TProfile>(
      "light_vs_muon_energy_profile", "", 80, 0.0, 20000.0);

  auto arrivedVsPhotons = std::make_unique<TH2D>(
      "arrived_photons_vs_scintillation_photons",
      "Arrived Photons vs Produced Photons;Scintillation photons;Photons arriving at sensor",
      120, 15000.0, 40000.0, 120, 0.0, 4000.0);
  auto arrivedVsPhotonsProfile = std::make_unique<TProfile>(
      "arrived_photons_vs_scintillation_photons_profile", "", 120, 15000.0, 40000.0);

  auto peVsPhotons = std::make_unique<TH2D>(
      "photoelectrons_vs_scintillation_photons",
      "Photoelectrons vs Produced Photons;Scintillation photons;Detected photoelectrons",
      120, 15000.0, 40000.0, 120, 100.0, 400.0);
  auto peVsPhotonsProfile = std::make_unique<TProfile>(
      "photoelectrons_vs_scintillation_photons_profile", "", 120, 15000.0, 40000.0);

  std::vector<double> thresholdValues;
  std::vector<double> sigmaValues;
  std::array<double, kMaxArrivalIndices> arrivalTimeSumsNs = {};
  std::array<double, kMaxArrivalIndices> arrivalTimeSqSumsNs = {};
  std::array<int, kMaxArrivalIndices> arrivalTimeCounts = {};
  std::vector<std::unique_ptr<TH1D>> overlayHistograms;
  overlayHistograms.reserve(kOverlayIndices.size());
  for (int index : kOverlayIndices) {
    overlayHistograms.push_back(std::make_unique<TH1D>(
        TString::Format("arrival_time_pe_%d", index),
        TString::Format("Arrival Time Overlay;Arrival time from muon [ns];Normalized entries"),
        120, 0.0, 6.0));
  }

  const Long64_t entries = events->GetEntries();
  for (Long64_t i = 0; i < entries; ++i) {
    events->GetEntry(i);

    if (eventId >= 0 && std::string(primaryParticle) != "RUN_SUMMARY" &&
        std::string(primaryParticle) != "THRESHOLD_SCAN") {
      lightVsEnergy->Fill(primaryEnergyMeV, scintillationPhotons);
      lightVsEnergyProfile->Fill(primaryEnergyMeV, scintillationPhotons);

      arrivedVsPhotons->Fill(scintillationPhotons, pmtIncidentPhotons);
      arrivedVsPhotonsProfile->Fill(scintillationPhotons, pmtIncidentPhotons);

      peVsPhotons->Fill(scintillationPhotons, photoelectrons);
      peVsPhotonsProfile->Fill(scintillationPhotons, photoelectrons);

      for (int j = 0; j < kMaxArrivalIndices; ++j) {
        if (arrivalTimesNs[j] >= 0.0) {
          arrivalTimeSumsNs[j] += arrivalTimesNs[j];
          arrivalTimeSqSumsNs[j] += arrivalTimesNs[j] * arrivalTimesNs[j];
          ++arrivalTimeCounts[j];
        }
      }

      for (std::size_t j = 0; j < kOverlayIndices.size(); ++j) {
        const int index = kOverlayIndices[j] - 1;
        if (arrivalTimesNs[index] >= 0.0) {
          overlayHistograms[j]->Fill(arrivalTimesNs[index]);
        }
      }
    }

    if (eventId == -2 && std::string(primaryParticle) == "THRESHOLD_SCAN" &&
        thresholdScanPe > 0 && thresholdScanSigmaNs >= 0.0) {
      thresholdValues.push_back(thresholdScanPe);
      sigmaValues.push_back(thresholdScanSigmaNs);
    }
  }

  {
    TCanvas canvas("c_light_vs_energy", "Light vs Muon Energy", 1100, 800);
    lightVsEnergy->SetMarkerColor(kAzure + 1);
    lightVsEnergy->Draw("COLZ");
    lightVsEnergyProfile->SetLineColor(kRed + 1);
    lightVsEnergyProfile->SetLineWidth(3);
    lightVsEnergyProfile->Draw("SAME");

    TLegend legend(0.14, 0.82, 0.42, 0.92);
    legend.AddEntry(lightVsEnergyProfile.get(), "Mean profile", "l");
    legend.Draw();

    SaveCanvas(canvas, outputDir, "light_vs_muon_energy");
  }

  {
    TCanvas canvas("c_arrived_vs_photons", "Arrived Photons vs Produced Photons", 1100,
                   800);
    arrivedVsPhotons->SetMarkerColor(kAzure + 1);
    arrivedVsPhotons->Draw("COLZ");
    arrivedVsPhotonsProfile->SetLineColor(kRed + 1);
    arrivedVsPhotonsProfile->SetLineWidth(3);
    arrivedVsPhotonsProfile->Draw("SAME");

    TLegend legend(0.14, 0.82, 0.42, 0.92);
    legend.AddEntry(arrivedVsPhotonsProfile.get(), "Mean profile", "l");
    legend.Draw();

    SaveCanvas(canvas, outputDir, "arrived_photons_vs_scintillation_photons");
  }

  {
    TCanvas canvas("c_pe_vs_photons", "Photoelectrons vs Produced Photons", 1100,
                   800);
    peVsPhotons->SetMarkerColor(kAzure + 1);
    peVsPhotons->Draw("COLZ");
    // peVsPhotonsProfile->SetLineColor(kRed + 1);
    // peVsPhotonsProfile->SetLineWidth(3);
    // peVsPhotonsProfile->Draw("SAME");

    // TLegend legend(0.14, 0.82, 0.42, 0.92);
    // legend.AddEntry(peVsPhotonsProfile.get(), "Mean profile", "l");
    // legend.Draw();

    SaveCanvas(canvas, outputDir, "photoelectrons_vs_scintillation_photons");
  }

  if (thresholdValues.empty()) {
    for (int i = 0; i < kMaxArrivalIndices; ++i) {
      if (arrivalTimeCounts[i] > 1) {
        const double meanNs = arrivalTimeSumsNs[i] / arrivalTimeCounts[i];
        const double varianceNs =
            std::max(0.0, (arrivalTimeSqSumsNs[i] / arrivalTimeCounts[i]) -
                               meanNs * meanNs);
        thresholdValues.push_back(i + 1);
        sigmaValues.push_back(std::sqrt(varianceNs));
      }
    }
    if (!thresholdValues.empty()) {
      std::cerr << "THRESHOLD_SCAN rows not found; built threshold curve from "
                   "event-level arrival columns instead."
                << std::endl;
    }
  }

  if (!thresholdValues.empty()) {
    TCanvas canvas("c_timing_sigma_vs_threshold",
                   "Timing Sigma vs Photoelectron Threshold", 1000, 700);
    TGraph graph(static_cast<int>(thresholdValues.size()), thresholdValues.data(),
                 sigmaValues.data());
    graph.SetTitle(
        "Timing Sigma vs Photoelectron Threshold;Photoelectron threshold;Timing sigma [ns]");
    graph.SetMarkerStyle(20);
    graph.SetMarkerSize(1.2);
    graph.SetMarkerColor(kBlue + 2);
    graph.SetLineColor(kBlue + 2);
    graph.SetLineWidth(2);
    graph.Draw("ALP");

    SaveCanvas(canvas, outputDir, "timing_sigma_vs_threshold");
  } else {
    std::cerr << "No threshold information could be built from the events tree."
              << std::endl;
  }

  {
    TCanvas canvas("c_arrival_time_overlays",
                   "Arrival-Time Overlays for Selected Photoelectron Indices",
                   1100, 800);
    TLegend legend(0.62, 0.58, 0.88, 0.88);
    const std::array<int, 7> colors = {kBlue + 1, kRed + 1, kGreen + 2,
                                       kMagenta + 1, kOrange + 7,
                                       kCyan + 2, kBlack};

    bool firstDraw = true;
    for (std::size_t i = 0; i < overlayHistograms.size(); ++i) {
      auto& hist = overlayHistograms[i];
      if (hist->Integral() > 0.0) {
        hist->Scale(1.0 / hist->Integral("width"));
      }
      hist->SetLineColor(colors[i]);
      hist->SetLineWidth(3);
      hist->SetTitle(
          "Arrival-Time Overlays for Selected Photoelectron Indices;Arrival time from muon [ns];Normalized entries");
      hist->Draw(firstDraw ? "HIST" : "HIST SAME");
      firstDraw = false;
      legend.AddEntry(hist.get(), TString::Format("PE %d", kOverlayIndices[i]),
                      "l");
    }
    legend.Draw();

    SaveCanvas(canvas, outputDir, "arrival_time_overlays_selected_pe");
  }

  {
    std::vector<double> arrivalIndices;
    std::vector<double> meanArrivalTimesNs;
    arrivalIndices.reserve(kMaxArrivalIndices);
    meanArrivalTimesNs.reserve(kMaxArrivalIndices);

    for (int i = 0; i < kMaxArrivalIndices; ++i) {
      if (arrivalTimeCounts[i] > 0) {
        arrivalIndices.push_back(i + 1);
        meanArrivalTimesNs.push_back(arrivalTimeSumsNs[i] / arrivalTimeCounts[i]);
      }
    }

    if (!arrivalIndices.empty()) {
      TCanvas canvas("c_mean_arrival_time_vs_pe_index",
                     "Mean Arrival Time vs Photoelectron Index", 1000, 700);
      TGraph graph(static_cast<int>(arrivalIndices.size()), arrivalIndices.data(),
                   meanArrivalTimesNs.data());
      graph.SetTitle(
          "Mean Arrival Time vs Photoelectron Index;Photoelectron index;Mean arrival time from muon [ns]");
      graph.SetMarkerStyle(20);
      graph.SetMarkerSize(0.9);
      graph.SetMarkerColor(kBlue + 2);
      graph.SetLineColor(kBlue + 2);
      graph.SetLineWidth(2);
      graph.Draw("ALP");

      SaveCanvas(canvas, outputDir, "mean_arrival_time_vs_pe_index");
    }
  }
}
