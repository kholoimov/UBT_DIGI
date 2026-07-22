#include <TCanvas.h>
#include <TFile.h>
#include <TH1D.h>
#include <TH2D.h>
#include <TProfile.h>
#include <TProfile2D.h>
#include <TStyle.h>
#include <TSystem.h>
#include <TTree.h>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

namespace {
struct EventRecord {
  double edepMeV;
  double xMm;
  double yMm;
  int adcCounts;
};

void SaveCanvas(TCanvas& canvas, const std::string& outputDirectory,
                const std::string& name) {
  canvas.SaveAs((outputDirectory + "/" + name + ".png").c_str());
  canvas.SaveAs((outputDirectory + "/" + name + ".pdf").c_str());
}
}  // namespace

void study_adc_vs_edep_position(
    const char* inputFile = "scintillator_digi.root",
    const char* outputDirectory = "analysis/adc_studies") {
  TFile input(inputFile, "READ");
  if (input.IsZombie()) {
    std::cerr << "Could not open " << inputFile << std::endl;
    return;
  }

  auto* events = dynamic_cast<TTree*>(input.Get("events"));
  if (events == nullptr) {
    std::cerr << "Could not find the general-studies tree 'events' in "
              << inputFile << std::endl;
    return;
  }

  int adcCounts = 0;
  double edepMeV = 0.0;
  double hitXmm = 0.0;
  double hitYmm = 0.0;
  double tileSizeMm = 40.0;
  events->SetBranchAddress("edep_mev", &edepMeV);
  events->SetBranchAddress("primary_hit_x_mm", &hitXmm);
  events->SetBranchAddress("primary_hit_y_mm", &hitYmm);
  events->SetBranchAddress("adc_counts", &adcCounts);
  if (events->GetBranch("tile_size_mm") != nullptr) {
    events->SetBranchAddress("tile_size_mm", &tileSizeMm);
  }

  std::vector<EventRecord> records;
  records.reserve(events->GetEntries());
  double maximumEdepMeV = 0.0;
  int maximumAdcCounts = 0;
  for (Long64_t entry = 0; entry < events->GetEntries(); ++entry) {
    events->GetEntry(entry);
    if (edepMeV < 0.0 || adcCounts < 0) {
      continue;
    }
    records.push_back({edepMeV, hitXmm, hitYmm, adcCounts});
    maximumEdepMeV = std::max(maximumEdepMeV, edepMeV);
    maximumAdcCounts = std::max(maximumAdcCounts, adcCounts);
  }

  if (records.empty()) {
    std::cerr << "The events tree contains no physical event rows."
              << std::endl;
    return;
  }

  gSystem->mkdir(outputDirectory, true);
  const double edepLimit = std::max(1.0, 1.05 * maximumEdepMeV);
  const double adcLimit = std::max(10.0, 1.05 * maximumAdcCounts);
  const int positionBins = static_cast<int>(tileSizeMm);
  const double tileHalfSizeMm = 0.5 * tileSizeMm;

  TH2D adcVsEdep(
      "adc_vs_edep",
      "ADC response versus deposited energy;Deposited energy [MeV];ADC counts",
      100, 0.0, edepLimit, 100, 0.0, adcLimit);
  TH1D adcCountsDistribution(
      "adc_counts_distribution",
      "ADC-count distribution;ADC counts;Events", 100, 0.0, adcLimit);
  TProfile meanAdcVsEdep("mean_adc_vs_edep",
                         "Mean ADC response versus deposited energy;Deposited "
                         "energy [MeV];Mean ADC counts",
                         50, 0.0, edepLimit);
  TProfile2D meanAdcVsPosition("mean_adc_vs_position",
                               "Mean ADC response across the tile;Primary hit "
                               "x [mm];Primary hit y [mm];Mean ADC counts",
                               positionBins, -tileHalfSizeMm, tileHalfSizeMm,
                               positionBins, -tileHalfSizeMm, tileHalfSizeMm);
  TProfile2D meanAdcPerMeVVsPosition(
      "mean_adc_per_mev_vs_position",
      "Energy-normalized ADC response across the tile;Primary hit x "
      "[mm];Primary hit y [mm];Mean ADC counts / MeV",
      positionBins, -tileHalfSizeMm, tileHalfSizeMm, positionBins,
      -tileHalfSizeMm, tileHalfSizeMm);
  TH2D adcVsDistance("adc_vs_distance_from_center",
                     "ADC response versus distance from tile center;Distance "
                     "from center [mm];ADC counts",
                     80, 0.0, tileHalfSizeMm * std::sqrt(2.0), 100, 0.0,
                     adcLimit);

  for (const auto& record : records) {
    const double radiusMm = std::hypot(record.xMm, record.yMm);
    adcVsEdep.Fill(record.edepMeV, record.adcCounts);
    adcCountsDistribution.Fill(record.adcCounts);
    meanAdcVsEdep.Fill(record.edepMeV, record.adcCounts);
    meanAdcVsPosition.Fill(record.xMm, record.yMm, record.adcCounts);
    if (record.edepMeV > 0.0) {
      meanAdcPerMeVVsPosition.Fill(record.xMm, record.yMm,
                                   record.adcCounts / record.edepMeV);
    }
    adcVsDistance.Fill(radiusMm, record.adcCounts);
  }

  gStyle->SetOptStat(0);
  const std::string outputDir(outputDirectory);
  {
    TCanvas canvas("c_adc_counts_distribution", "ADC-count distribution", 1000,
                   800);
    adcCountsDistribution.SetLineColor(kBlue + 2);
    adcCountsDistribution.SetLineWidth(3);
    adcCountsDistribution.Draw("HIST");
    SaveCanvas(canvas, outputDir, "adc_counts_distribution");
  }
  {
    TCanvas canvas("c_adc_vs_edep", "ADC vs deposited energy", 1000, 800);
    adcVsEdep.Draw("COLZ");
    meanAdcVsEdep.SetLineColor(kRed + 1);
    meanAdcVsEdep.SetLineWidth(3);
    meanAdcVsEdep.Draw("SAME");
    SaveCanvas(canvas, outputDir, "adc_vs_edep");
  }
  {
    TCanvas canvas("c_adc_vs_position", "Mean ADC across tile", 1000, 850);
    meanAdcVsPosition.Draw("COLZ");
    SaveCanvas(canvas, outputDir, "mean_adc_vs_position");
  }
  {
    TCanvas canvas("c_adc_per_mev_vs_position",
                   "Energy-normalized ADC across tile", 1000, 850);
    meanAdcPerMeVVsPosition.Draw("COLZ");
    SaveCanvas(canvas, outputDir, "mean_adc_per_mev_vs_position");
  }
  {
    TCanvas canvas("c_adc_vs_distance", "ADC vs distance from center", 1000,
                   800);
    adcVsDistance.Draw("COLZ");
    SaveCanvas(canvas, outputDir, "adc_vs_distance_from_center");
  }

  TFile output((outputDir + "/adc_vs_edep_position.root").c_str(), "RECREATE");
  adcVsEdep.Write();
  adcCountsDistribution.Write();
  meanAdcVsEdep.Write();
  meanAdcVsPosition.Write();
  meanAdcPerMeVVsPosition.Write();
  adcVsDistance.Write();
  output.Close();

  std::cout << "Analyzed " << records.size() << " physical events; plots and "
            << "histograms written to " << outputDir << std::endl;
}
