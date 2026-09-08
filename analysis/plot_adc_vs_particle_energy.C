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

// One curve (particle/label variant) to be drawn, individually and
// optionally as part of the combined multi-particle overlay.
struct Series {
  std::string particle;
  std::string label;       // CSV / legend label
  std::string fileStub;    // individual-plot filename stub
  int color = kBlue + 2;
  bool includeInCombined = true;
  std::vector<ScanPoint> points;
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

// Computes mean/stderr ADC etc. from the 'events' tree of one scan-point
// ROOT file. If requireInteraction is true, only events with a non-zero
// energy deposit (i.e. the primary produced at least something in the
// scintillator) are included in the mean.
bool AnalyzeRootFile(const std::string& path, bool requireInteraction,
                     ScanPoint& point) {
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
  Long64_t used = 0;
  for (Long64_t i = 0; i < entries; ++i) {
    events->GetEntry(i);
    if (requireInteraction && !(edepMeV > 0.0)) {
      continue;
    }
    sumAdc += adcCounts;
    sumAdc2 += static_cast<double>(adcCounts) * adcCounts;
    sumTriggered += triggered;
    sumEdep += edepMeV;
    ++used;
  }

  if (used <= 0) {
    return false;
  }

  const double n = static_cast<double>(used);
  const double meanAdc = sumAdc / n;
  const double variance =
      used > 1 ? std::max(0.0, (sumAdc2 - n * meanAdc * meanAdc) / (n - 1))
               : 0.0;

  point.nEvents = static_cast<int>(used);
  point.meanAdc = meanAdc;
  point.adcStdErr = std::sqrt(variance / n);
  point.triggeredFraction = sumTriggered / n;
  point.meanEdepMeV = sumEdep / n;
  return true;
}

void DrawIndividualPlot(const Series& series, const char* outputDir) {
  if (series.points.empty()) {
    return;
  }
  TCanvas canvas(TString::Format("c_adc_vs_energy_%s", series.fileStub.c_str()),
                 series.label.c_str(), 1000, 700);
  canvas.SetLogx();

  TGraphErrors graph(static_cast<int>(series.points.size()));
  for (int i = 0; i < static_cast<int>(series.points.size()); ++i) {
    graph.SetPoint(i, series.points[i].energyMeV, series.points[i].meanAdc);
    graph.SetPointError(i, 0.0, series.points[i].adcStdErr);
  }
  graph.SetTitle(TString::Format(
      "Mean ADC Counts vs Primary Energy: %s;Primary kinetic energy "
      "[MeV];Mean ADC counts",
      series.label.c_str()));
  graph.SetLineColor(series.color);
  graph.SetMarkerColor(series.color);
  graph.SetMarkerStyle(20);
  graph.SetMarkerSize(1.1);
  graph.SetLineWidth(2);
  graph.Draw("ALP");

  canvas.SaveAs(
      TString::Format("%s/adc_vs_energy_%s.pdf", outputDir, series.fileStub.c_str()));
  std::cout << "Wrote individual energy-scan plot: " << outputDir
            << "/adc_vs_energy_" << series.fileStub << ".pdf" << std::endl;
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

  const TString summaryCsvPath =
      TString::Format("%s/adc_vs_particle_energy_summary.csv", outputDir);
  std::ofstream csv(summaryCsvPath.Data());
  csv << "particle,label,energy_mev,n_events,mean_adc,adc_stderr,"
         "triggered_fraction,mean_edep_mev\n";

  auto writeCsvRows = [&csv](const Series& series) {
    for (const auto& point : series.points) {
      csv << series.particle << "," << series.label << "," << point.energyMeV
          << "," << point.nEvents << "," << point.meanAdc << ","
          << point.adcStdErr << "," << point.triggeredFraction << ","
          << point.meanEdepMeV << "\n";
    }
  };

  // One series per particle type for the standard (all-events) mean, plus
  // an extra photon-only series restricted to photons that interacted and
  // deposited some energy ("produced at least something").
  Series muonSeries{"mu-", "Muon", "muon", kBlue + 2, true, {}};
  Series electronSeries{"e-", "Electron", "electron", kRed + 1, true, {}};
  Series photonAllSeries{"gamma", "Photon", "photon_all", kGreen + 2, true, {}};
  Series photonInteractedSeries{
      "gamma", "Photon (interacted)", "photon_interacted", kMagenta + 1,
      false, {}};

  for (const auto& row : rows) {
    ScanPoint pointAll;
    pointAll.energyMeV = row.energyMeV;
    const bool okAll = AnalyzeRootFile(row.rootFile, false, pointAll);

    if (row.particle == "mu-") {
      if (okAll) muonSeries.points.push_back(pointAll);
    } else if (row.particle == "e-") {
      if (okAll) electronSeries.points.push_back(pointAll);
    } else if (row.particle == "gamma") {
      if (okAll) photonAllSeries.points.push_back(pointAll);

      ScanPoint pointInteracted;
      pointInteracted.energyMeV = row.energyMeV;
      if (AnalyzeRootFile(row.rootFile, true, pointInteracted)) {
        photonInteractedSeries.points.push_back(pointInteracted);
      }
    }
  }

  const std::array<const Series*, 4> allSeries = {
      &muonSeries, &electronSeries, &photonAllSeries, &photonInteractedSeries};
  for (const auto* series : allSeries) {
    writeCsvRows(*series);
    DrawIndividualPlot(*series, outputDir);
  }
  csv.close();
  std::cout << "Wrote energy-scan summary table: " << summaryCsvPath
            << std::endl;

  // Combined overlay of the three primary particle types (photon "all",
  // not the interacted-only variant, to match what is nominally incident
  // on the tile).
  TCanvas canvas("c_adc_vs_particle_energy",
                 "Mean ADC counts vs primary particle energy", 1100, 800);
  canvas.SetLogx();

  TMultiGraph multiGraph;
  multiGraph.SetTitle(
      "Mean ADC Counts vs Primary Particle Energy;Primary kinetic energy "
      "[MeV];Mean ADC counts");

  TLegend legend(0.16, 0.68, 0.42, 0.88);
  std::vector<std::unique_ptr<TGraphErrors>> graphs;

  for (const auto* series : allSeries) {
    if (!series->includeInCombined || series->points.empty()) {
      continue;
    }
    auto graph =
        std::make_unique<TGraphErrors>(static_cast<int>(series->points.size()));
    for (int i = 0; i < static_cast<int>(series->points.size()); ++i) {
      graph->SetPoint(i, series->points[i].energyMeV, series->points[i].meanAdc);
      graph->SetPointError(i, 0.0, series->points[i].adcStdErr);
    }
    graph->SetLineColor(series->color);
    graph->SetMarkerColor(series->color);
    graph->SetMarkerStyle(20);
    graph->SetMarkerSize(1.1);
    graph->SetLineWidth(2);
    multiGraph.Add(graph.get(), "LP");
    legend.AddEntry(graph.get(), series->label.c_str(), "lp");
    graphs.push_back(std::move(graph));
  }

  multiGraph.Draw("A");
  legend.Draw();

  canvas.SaveAs(TString::Format("%s/adc_vs_particle_energy.pdf", outputDir));
  std::cout << "Wrote combined energy-scan plot: " << outputDir
            << "/adc_vs_particle_energy.pdf" << std::endl;
}
