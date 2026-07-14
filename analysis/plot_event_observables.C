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
  // gStyle->SetOptStat(0);
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

// Normalized shifted-Gamma density
double gammaComponent(double x, double area,
                      double x0, double k, double theta)
{
    const double z = x - x0;

    if (z <= 0.0 || area < 0.0 || k <= 0.0 || theta <= 0.0)
        return 0.0;

    const double logPdf =
          (k - 1.0) * TMath::Log(z)
        - z / theta
        - TMath::LnGamma(k)
        - k * TMath::Log(theta);

    return area * TMath::Exp(logPdf);
}

// par[0] = area of narrow peak
// par[1] = common threshold x0
// par[2] = shape k1
// par[3] = scale theta1
// par[4] = area of broad tail
// par[5] = shape k2
// par[6] = scale theta2
double doubleShiftedGamma(double *x, double *par)
{
    const double peak =
        gammaComponent(x[0], par[0], par[1], par[2], par[3]);

    const double tail =
        gammaComponent(x[0], par[4], par[1], par[5], par[6]);

    return peak + tail;
}

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
  auto* pmtPhotonBirths = dynamic_cast<TTree*>(file->Get("pmt_photon_births"));

  int eventId = -1;
  char primaryParticle[128] = {};
  double primaryEnergyMeV = 0.0;
  double primaryHitXmm = 0.0;
  double primaryHitYmm = 0.0;
  int scintillationPhotons = 0;
  int pmtIncidentPhotons = 0;
  double photoelectrons = 0.0;
  int thresholdScanPe = -1;
  double pmtChargePC = 0;
  double thresholdScanSigmaNs = -1.0;
  std::array<double, kMaxArrivalIndices> arrivalTimesNs = {};

  events->SetBranchAddress("event_id", &eventId);
  events->SetBranchAddress("primary_particle", primaryParticle);
  events->SetBranchAddress("primary_energy_mev", &primaryEnergyMeV);
  events->SetBranchAddress("primary_hit_x_mm", &primaryHitXmm);
  events->SetBranchAddress("primary_hit_y_mm", &primaryHitYmm);
  events->SetBranchAddress("scintillation_photons", &scintillationPhotons);
  events->SetBranchAddress("pmt_incident_photons", &pmtIncidentPhotons);
  events->SetBranchAddress("pmt_charge_pc", &pmtChargePC);
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
  auto photonBirthMap = std::make_unique<TH2D>(
      "pmt_photon_birth_map",
      "Birth Positions of Photons Reaching Sensor;birth x [mm];birth y [mm]",
      41, -20.5, 20.5, 41, -20.5, 20.5);
  auto muonHitMap = std::make_unique<TH2D>(
      "muon_hit_map",
      "Muon Hit Positions in Scintillator;hit x [mm];hit y [mm]",
      41, -20.5, 20.5, 41, -20.5, 20.5);

  auto pmt_charge_pc_histogram = std::make_unique<TH1D>(
    "pmt_charge_pc_histogram",
    "pmt_charge_pc_histogram",
    100, 100, 600
  );

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
      muonHitMap->Fill(primaryHitXmm, primaryHitYmm);

      pmt_charge_pc_histogram->Fill(pmtChargePC);

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
    TCanvas canvas("produced_photoelectrons", "produced_photoelectrons", 1100, 800);

    int firstBin = -1;
    int lastBin  = -1;

    for (int bin = 1; bin <= pmt_charge_pc_histogram->GetNbinsX(); ++bin) {
        if (pmt_charge_pc_histogram->GetBinContent(bin) > 0.0) {
            firstBin = bin;
            break;
        }
    }

    for (int bin = pmt_charge_pc_histogram->GetNbinsX(); bin >= 1; --bin) {
        if (pmt_charge_pc_histogram->GetBinContent(bin) > 0.0) {
            lastBin = bin;
            break;
        }
    }


    double fitMin = pmt_charge_pc_histogram->GetXaxis()->GetBinLowEdge(firstBin);
    double fitMax = pmt_charge_pc_histogram->GetXaxis()->GetBinUpEdge(lastBin);
    const double binWidth = pmt_charge_pc_histogram->GetXaxis()->GetBinWidth(1);

    /*
      Multiplying by binWidth makes the function value correspond
      approximately to the expected number of counts in a bin.
    */
    TF1 *fitFunction = new TF1(
        "doubleGamma",
        [binWidth](double *x, double *p) {
            return binWidth * doubleShiftedGamma(x, p);
        },
        fitMin,
        fitMax,
        7
    );

    fitFunction->SetParNames(
        "Peak area",
        "Threshold x_{0}",
        "Peak k",
        "Peak #theta",
        "Tail area",
        "Tail k",
        "Tail #theta"
    );

    const double totalCounts =
        pmt_charge_pc_histogram->Integral(pmt_charge_pc_histogram->FindBin(fitMin),
                       pmt_charge_pc_histogram->FindBin(fitMax));

    /*
      Starting values chosen from the displayed histogram.

      Component 1:
        narrow feature with mode near 190

      Component 2:
        broader feature producing the high-x tail

      Gamma mode = x0 + (k - 1)*theta
    */
    fitFunction->SetParameters(
        0.65 * totalCounts, // peak area
        135.0,              // common x0
        5.0,                // peak k
        14.0,               // peak theta: mode ~191
        0.35 * totalCounts, // tail area
        2.0,                // tail k
        75.0                 // tail theta
    );

    fitFunction->SetParLimits(0, 0.0, 2.0 * totalCounts);
    fitFunction->SetParLimits(1, 125.0, 150.0);

    fitFunction->SetParLimits(2, 1.01, 30.0);
    fitFunction->SetParLimits(3, 1.0, 50.0);

    fitFunction->SetParLimits(4, 0.0, 2.0 * totalCounts);
    fitFunction->SetParLimits(5, 1.01, 10.0);
    fitFunction->SetParLimits(6, 10.0, 300.0);

    /*
      R = use specified fit range
      L = binned Poisson likelihood
      S = return full fit result
      0 = do not draw until explicitly requested
    */
    TFitResultPtr result = pmt_charge_pc_histogram->Fit(
        fitFunction,
        "RLS0"
    );

    
    pmt_charge_pc_histogram->Draw("HIST");
    pmt_charge_pc_histogram->SetStats(1);
    fitFunction->Draw("SAME");

    const double mostProbableValue =
        fitFunction->GetMaximumX(fitMin, fitMax);

    const double fittedMaximum =
        fitFunction->Eval(mostProbableValue);

        const double lineTop =
        TMath::Max(pmt_charge_pc_histogram->GetMaximum(), fittedMaximum) * 1.05;

    TLine *modeLine = new TLine(
        mostProbableValue,
        0.0,
        mostProbableValue,
        lineTop
    );

    modeLine->SetLineColor(kGreen + 2);
    modeLine->SetLineWidth(3);
    modeLine->SetLineStyle(2);
    modeLine->Draw("SAME");

    /*
     * Add a label next to the line.
     */
    TLatex *modeLabel = new TLatex(
        mostProbableValue + 2.0 * binWidth,
        0.90 * lineTop,
        Form("MPV = %.2f", mostProbableValue)
    );

    modeLabel->SetTextColor(kGreen + 2);
    modeLabel->SetTextSize(0.035);
    modeLabel->Draw("SAME");




    SaveCanvas(canvas, outputDir, "photoelectrons");
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

  if (pmtPhotonBirths != nullptr) {
    double birthXmm = 0.0;
    double birthYmm = 0.0;
    pmtPhotonBirths->SetBranchAddress("birth_x_mm", &birthXmm);
    pmtPhotonBirths->SetBranchAddress("birth_y_mm", &birthYmm);

    const Long64_t birthEntries = pmtPhotonBirths->GetEntries();
    for (Long64_t i = 0; i < birthEntries; ++i) {
      pmtPhotonBirths->GetEntry(i);
      photonBirthMap->Fill(birthXmm, birthYmm);
    }

    TCanvas canvas("c_pmt_photon_birth_map",
                   "Birth Positions of Photons Reaching Sensor", 900, 800);
    photonBirthMap->Draw("COLZ");
    SaveCanvas(canvas, outputDir, "pmt_photon_birth_map");
  } else {
    std::cerr << "No 'pmt_photon_births' tree found; birth-position map was not "
                 "produced."
              << std::endl;
  }

  {
    TCanvas canvas("c_muon_hit_map", "Muon Hit Positions in Scintillator", 900,
                   800);
    muonHitMap->Draw("COLZ");
    SaveCanvas(canvas, outputDir, "muon_hit_map");
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
