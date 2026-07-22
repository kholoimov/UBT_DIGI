#include <TCanvas.h>
#include <TFile.h>
#include <TProfile2D.h>
#include <TStyle.h>
#include <TSystem.h>
#include <TTree.h>

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

  int eventId = -1;
  int adcCounts = 0;
  double edepMeV = 0.0;
  double hitXmm = 0.0;
  double hitYmm = 0.0;
  events->SetBranchAddress("event_id", &eventId);
  events->SetBranchAddress("edep_mev", &edepMeV);
  events->SetBranchAddress("primary_hit_x_mm", &hitXmm);
  events->SetBranchAddress("primary_hit_y_mm", &hitYmm);
  events->SetBranchAddress("adc_counts", &adcCounts);

  TProfile2D adcPerEdep(
      "adc_per_edep_vs_position",
      "Energy-normalized ADC response;Primary hit x [mm];Primary hit y [mm];"
      "Mean ADC counts / E_{dep} [MeV]",
      40, -20.0, 20.0, 40, -20.0, 20.0);

  Long64_t selectedEvents = 0;
  for (Long64_t entry = 0; entry < events->GetEntries(); ++entry) {
    events->GetEntry(entry);
    if (eventId < 0 || edepMeV <= 0.0 || adcCounts < 0) {
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

  std::cout << "Filled ADC/E_dep position map with " << selectedEvents
            << " physical events; output written to " << outputBase
            << ".{root,png,pdf}" << std::endl;
}
