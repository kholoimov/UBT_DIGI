#include <TCanvas.h>
#include <TFile.h>
#include <TProfile2D.h>
#include <TStyle.h>
#include <TSystem.h>
#include <TTree.h>

#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>

void plot_adc_per_edep_vs_position(
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

  if (events->GetEntries() == 0) {
    std::cerr << "The events tree is empty." << std::endl;
    return;
  }
  events->GetEntry(0);
  const int positionBins = static_cast<int>(tileSizeMm);
  const double tileHalfSizeMm = 0.5 * tileSizeMm;

  TProfile2D adcPerEdep(
      "adc_per_edep_vs_position",
      "Energy-normalized ADC response;Primary hit x [mm];Primary hit y [mm];"
      "Mean ADC counts / E_{dep} [MeV]",
      positionBins, -tileHalfSizeMm, tileHalfSizeMm, positionBins,
      -tileHalfSizeMm, tileHalfSizeMm);

  Long64_t selectedEvents = 0;
  for (Long64_t entry = 0; entry < events->GetEntries(); ++entry) {
    events->GetEntry(entry);
    if (edepMeV <= 0.0 || adcCounts < 0) {
      continue;
    }
    adcPerEdep.Fill(hitXmm, hitYmm, adcCounts / edepMeV);
    ++selectedEvents;
  }

  if (selectedEvents == 0) {
    std::cerr << "No physical events with positive deposited energy were found."
              << std::endl;
    return;
  }

  gSystem->mkdir(outputDirectory, true);
  gStyle->SetOptStat(0);
  TCanvas canvas("c_adc_per_edep_vs_position",
                 "ADC per deposited energy versus position", 1000, 850);
  canvas.SetRightMargin(0.16);
  adcPerEdep.Draw("COLZ");

  const std::string outputBase =
      std::string(outputDirectory) + "/adc_per_edep_vs_position";
  canvas.SaveAs((outputBase + ".png").c_str());
  canvas.SaveAs((outputBase + ".pdf").c_str());

  TFile output((outputBase + ".root").c_str(), "RECREATE");
  adcPerEdep.Write();
  output.Close();

  std::ofstream table(outputBase + ".csv");
  table << "x_center_mm,y_center_mm,entries,mean_adc_per_mev,standard_error\n";
  table << std::setprecision(10);
  for (int xBin = 1; xBin <= adcPerEdep.GetNbinsX(); ++xBin) {
    for (int yBin = 1; yBin <= adcPerEdep.GetNbinsY(); ++yBin) {
      const int bin = adcPerEdep.GetBin(xBin, yBin);
      table << adcPerEdep.GetXaxis()->GetBinCenter(xBin) << ','
            << adcPerEdep.GetYaxis()->GetBinCenter(yBin) << ','
            << adcPerEdep.GetBinEntries(bin) << ','
            << adcPerEdep.GetBinContent(bin) << ','
            << adcPerEdep.GetBinError(bin) << '\n';
    }
  }
  table.close();

  std::cout << "Filled ADC/E_dep position map with " << selectedEvents
            << " physical events; output written to " << outputBase
            << ".{root,png,pdf,csv}" << std::endl;
}
