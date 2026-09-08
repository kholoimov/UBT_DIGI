#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "TCanvas.h"
#include "TFile.h"
#include "TGraphErrors.h"
#include "TLegend.h"
#include "TMultiGraph.h"
#include "TROOT.h"
#include "TString.h"
#include "TStyle.h"
#include "TSystem.h"
#include "TTree.h"

namespace {

struct ManifestRow {
  std::string particle;
  std::string label;
  double energyMeV = 0.0;
  std::string rootFile;
};

struct ScanPoint {
  double energyMeV = 0.0;
  double meanAdc = 0.0;
  double adcStdErr = 0.0;
  double triggeredFraction = 0.0;
  double meanEdepMeV = 0.0;
  int nEvents = 0;
};

std::vector<ManifestRow> ReadManifest(const char* manifestPath) {
  std::vector<ManifestRow> rows;
  std::ifstream file(manifestPath);
  if (!file) {
    std::cerr << "Could not open manifest: " << manifestPath << std::endl;
    return rows;
  }
  std::string line;
  bool firstLine = true;
  while (std::getline(file, line)) {
    if (firstLine) {
      firstLine = false;
      continue;  // header
    }
    if (line.empty()) {
      continue;
    }
    std::stringstream ss(line);
    std::string particle, label, energyStr, rootFile;
    std::getline(ss, particle, ',');
    std::getline(ss, label, ',');
    std::getline(ss, energyStr, ',');
    std::getline(ss, rootFile, ',');
    ManifestRow row;
    row.particle = particle;
    row.label = label;
    row.energyMeV = std::stod(energyStr);
    row.rootFile = rootFile;
    rows.push_back(row);
  }
  return rows;
}

bool AnalyzeRootFile(const std::string& path, ScanPoint& point) {
  std::unique_ptr<TFile> file(TFile::Open(path.c_str(), "READ"));
  if (!file || file->IsZombie()) {
    std::cerr << "Failed to open ROOT file: " << path << std::endl;
    return false;
  }
  auto* events = dynamic_cast<TTree*>(file->Get("events"));
  if (events == nullptr) {
    std::cerr << "No 'events' tree in " << path << std::endl;
    return false;
  }

  int adcCounts = 0;
  int triggered = 0;
  double edepMeV = 0.0;
  events->SetBranchAddress("adc_counts", &adcCounts);
  events->SetBranchAddress("triggered", &triggered);
  events->SetBranchAddress("edep_mev", &edepMeV);

  const Long64_t entries = events->GetEntries();
  if (entries <= 0) {
    return false;
  }

  double sumAdc = 0.0;
  double sumAdc2 = 0.0;
  double sumTriggered = 0.0;
  double sumEdep = 0.0;
  for (Long64_t i = 0; i < entries; ++i) {
    events->GetEntry(i);
    sumAdc += adcCounts;
    sumAdc2 += static_cast<double>(adcCounts) * adcCounts;
    sumTriggered += triggered;
    sumEdep += edepMeV;
  }

  const double n = static_cast<double>(entries);
  const double meanAdc = sumAdc / n;
  const double variance =
      entries > 1 ? std::max(0.0, (sumAdc2 - n * meanAdc * meanAdc) / (n - 1))
                  : 0.0;

  point.nEvents = static_cast<int>(entries);
  point.meanAdc = meanAdc;
  point.adcStdErr = std::sqrt(variance / n);
  point.triggeredFraction = sumTriggered / n;
  point.meanEdepMeV = sumEdep / n;
  return true;
}

}  // namespace

void plot_adc_vs_particle_energy(const char* manifestPath,
                                 const char* outputDir) {
  gROOT->SetBatch(kTRUE);
  gStyle->SetOptStat(0);
  gSystem->mkdir(outputDir, kTRUE);

  const std::vector<ManifestRow> rows = ReadManifest(manifestPath);
  if (rows.empty()) {
    std::cerr << "No rows found in manifest: " << manifestPath << std::endl;
    return;
  }

  // Preserve first-seen particle order for stable legend/colour assignment.
  std::vector<std::string> labelOrder;
  std::map<std::string, std::vector<ScanPoint>> pointsByLabel;

  const TString summaryCsvPath =
      TString::Format("%s/adc_vs_particle_energy_summary.csv", outputDir);
  std::ofstream csv(summaryCsvPath.Data());
  csv << "particle,label,energy_mev,n_events,mean_adc,adc_stderr,"
         "triggered_fraction,mean_edep_mev\n";

  for (const auto& row : rows) {
    ScanPoint point;
    point.energyMeV = row.energyMeV;
    if (!AnalyzeRootFile(row.rootFile, point)) {
      continue;
    }
    if (pointsByLabel.find(row.label) == pointsByLabel.end()) {
      labelOrder.push_back(row.label);
    }
    pointsByLabel[row.label].push_back(point);

    csv << row.particle << "," << row.label << "," << point.energyMeV << ","
        << point.nEvents << "," << point.meanAdc << "," << point.adcStdErr
        << "," << point.triggeredFraction << "," << point.meanEdepMeV
        << "\n";
  }
  csv.close();
  std::cout << "Wrote energy-scan summary table: " << summaryCsvPath
            << std::endl;

  TCanvas canvas("c_adc_vs_particle_energy",
                 "Mean ADC counts vs primary particle energy", 1100, 800);
  canvas.SetLogx();

  TMultiGraph multiGraph;
  multiGraph.SetTitle(
      "Mean ADC Counts vs Primary Particle Energy;Primary kinetic energy "
      "[MeV];Mean ADC counts");

  TLegend legend(0.16, 0.68, 0.42, 0.88);

  const std::array<int, 3> colors = {kBlue + 2, kRed + 1, kGreen + 2};
  std::vector<std::unique_ptr<TGraphErrors>> graphs;

  std::size_t colorIndex = 0;
  for (const auto& label : labelOrder) {
    const auto& points = pointsByLabel.at(label);
    auto graph = std::make_unique<TGraphErrors>(static_cast<int>(points.size()));
    for (int i = 0; i < static_cast<int>(points.size()); ++i) {
      graph->SetPoint(i, points[i].energyMeV, points[i].meanAdc);
      graph->SetPointError(i, 0.0, points[i].adcStdErr);
    }
    const int color = colors[colorIndex % colors.size()];
    graph->SetLineColor(color);
    graph->SetMarkerColor(color);
    graph->SetMarkerStyle(20);
    graph->SetMarkerSize(1.1);
    graph->SetLineWidth(2);
    multiGraph.Add(graph.get(), "LP");
    legend.AddEntry(graph.get(), label.c_str(), "lp");
    graphs.push_back(std::move(graph));
    ++colorIndex;
  }

  multiGraph.Draw("A");
  legend.Draw();

  canvas.SaveAs(TString::Format("%s/adc_vs_particle_energy.pdf", outputDir));

  std::cout << "Wrote energy-scan plot: " << outputDir
            << "/adc_vs_particle_energy.pdf" << std::endl;
}
