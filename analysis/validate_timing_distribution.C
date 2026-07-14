#include "TCanvas.h"
#include "TF1.h"
#include "TFile.h"
#include "TH1D.h"
#include "TLatex.h"
#include "TLegend.h"
#include "TROOT.h"
#include "TString.h"
#include "TStyle.h"
#include "TSystem.h"
#include "TTree.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <memory>

namespace {

double gammaComponent(double x, double area, double x0, double k,
                      double theta) {
  const double z = x - x0;
  if (z <= 0.0 || area < 0.0 || k <= 0.0 || theta <= 0.0) {
    return 0.0;
  }

  const double logPdf = (k - 1.0) * TMath::Log(z) - z / theta -
                        TMath::LnGamma(k) - k * TMath::Log(theta);
  return area * TMath::Exp(logPdf);
}

void ConfigureStyle() {
  gROOT->SetBatch(kTRUE);
  gStyle->SetTitleFontSize(0.04);
  gStyle->SetPadLeftMargin(0.12);
  gStyle->SetPadRightMargin(0.08);
  gStyle->SetPadBottomMargin(0.12);
}

void SaveCanvas(TCanvas& canvas, const TString& outputDir,
                const TString& baseName) {
  canvas.SaveAs(outputDir + "/" + baseName + ".pdf");
}

double InterpolateLevel(const TH1D& hist, int firstBin, int lastBin,
                        double level, bool risingEdge) {
  if (risingEdge) {
    for (int bin = firstBin + 1; bin <= lastBin; ++bin) {
      const double y1 = hist.GetBinContent(bin - 1);
      const double y2 = hist.GetBinContent(bin);
      if (y1 < level && y2 >= level) {
        const double x1 = hist.GetBinCenter(bin - 1);
        const double x2 = hist.GetBinCenter(bin);
        return x1 + (level - y1) * (x2 - x1) / (y2 - y1);
      }
    }
  } else {
    for (int bin = lastBin; bin > firstBin; --bin) {
      const double y1 = hist.GetBinContent(bin);
      const double y2 = hist.GetBinContent(bin - 1);
      if (y1 < level && y2 >= level) {
        const double x1 = hist.GetBinCenter(bin);
        const double x2 = hist.GetBinCenter(bin - 1);
        return x1 + (level - y1) * (x2 - x1) / (y2 - y1);
      }
    }
  }
  return -1.0;
}

double ComputeFwhm(const TH1D& hist) {
  const int peakBin = hist.GetMaximumBin();
  const double halfMax = 0.5 * hist.GetBinContent(peakBin);
  const double left = InterpolateLevel(hist, 1, peakBin, halfMax, true);
  const double right =
      InterpolateLevel(hist, peakBin, hist.GetNbinsX(), halfMax, false);
  if (left < 0.0 || right < 0.0 || right <= left) {
    return -1.0;
  }
  return right - left;
}

double ComputeRiseTime10to90(const TH1D& hist) {
  const int peakBin = hist.GetMaximumBin();
  const double peak = hist.GetBinContent(peakBin);
  const double t10 = InterpolateLevel(hist, 1, peakBin, 0.10 * peak, true);
  const double t90 = InterpolateLevel(hist, 1, peakBin, 0.90 * peak, true);
  if (t10 < 0.0 || t90 < 0.0 || t90 <= t10) {
    return -1.0;
  }
  return t90 - t10;
}

double ComputeFallTime90to10(const TH1D& hist) {
  const int peakBin = hist.GetMaximumBin();
  const double peak = hist.GetBinContent(peakBin);
  const double t90 =
      InterpolateLevel(hist, peakBin, hist.GetNbinsX(), 0.90 * peak, false);
  const double t10 =
      InterpolateLevel(hist, peakBin, hist.GetNbinsX(), 0.10 * peak, false);
  if (t10 < 0.0 || t90 < 0.0 || t10 <= t90) {
    return -1.0;
  }
  return t10 - t90;
}

}  // namespace

void validate_timing_distribution(
    const char* inputFile = "scintillator_digi.root",
    const char* outputDir = "analysis/plots",
    double targetFwhmNs = 2.767, double targetRiseNs = 0.85,
    double targetFallNs = 4.90, double targetMeanNs = 123.668,
    double toleranceFwhmNs = 0.30, double toleranceRiseNs = 0.20,
    double toleranceFallNs = 0.60, double toleranceMeanNs = 2.0) {
  ConfigureStyle();
  gSystem->mkdir(outputDir, kTRUE);

  std::unique_ptr<TFile> file(TFile::Open(inputFile, "READ"));
  if (!file || file->IsZombie()) {
    std::cerr << "Failed to open ROOT file: " << inputFile << std::endl;
    return;
  }

  auto* scintPhotonBirthTimes =
      dynamic_cast<TTree*>(file->Get("scintillation_photon_birth_times"));
  if (scintPhotonBirthTimes == nullptr) {
    std::cerr << "Could not find TTree 'scintillation_photon_birth_times' in "
              << inputFile << std::endl;
    return;
  }

  double birthTimeNs = 0.0;
  scintPhotonBirthTimes->SetBranchAddress("birth_time_ns", &birthTimeNs);

  auto birthHistogram = std::make_unique<TH1D>(
      "scintillation_birth_time_validation",
      "Scintillation Birth-Time Validation;time [ns];counts", 800, 0.0, 80.0);

  const Long64_t entries = scintPhotonBirthTimes->GetEntries();
  for (Long64_t i = 0; i < entries; ++i) {
    scintPhotonBirthTimes->GetEntry(i);
    birthHistogram->Fill(birthTimeNs);
  }

  const double meanNs = birthHistogram->GetMean();
  const double fwhmNs = ComputeFwhm(*birthHistogram);
  const double riseNs = ComputeRiseTime10to90(*birthHistogram);
  const double fallNs = ComputeFallTime90to10(*birthHistogram);

  const int peakBin = birthHistogram->GetMaximumBin();
  const double peakX = birthHistogram->GetBinCenter(peakBin);
  const double binWidth = birthHistogram->GetXaxis()->GetBinWidth(1);
  const double fitMin = std::max(0.0, birthHistogram->GetXaxis()->GetXmin() +
                                          binWidth);
  const double fitMax = 40.0;

  TF1 fitFunction(
      "scint_birth_validation_fit",
      [binWidth](double* x, double* p) {
        return binWidth * gammaComponent(x[0], p[0], p[1], p[2], p[3]);
      },
      fitMin, fitMax, 4);
  fitFunction.SetParNames("Area", "x0", "k", "theta");
  fitFunction.SetParameters(birthHistogram->Integral(), 0.02, 2.5,
                            std::max(0.05, peakX / 3.0));
  fitFunction.SetParLimits(0, 0.0, 10.0 * birthHistogram->Integral());
  fitFunction.SetParLimits(1, 0.0, 2.0);
  fitFunction.SetParLimits(2, 1.01, 30.0);
  fitFunction.SetParLimits(3, 0.02, 20.0);
  fitFunction.SetLineColor(kBlue + 3);
  fitFunction.SetLineWidth(3);
  birthHistogram->Fit(&fitFunction, "R0Q");

  const double x0 = fitFunction.GetParameter(1);
  const double k = fitFunction.GetParameter(2);
  const double theta = fitFunction.GetParameter(3);
  const double fitMeanNs = x0 + k * theta;
  const double fitMpvNs = k > 1.0 ? x0 + (k - 1.0) * theta : x0;
  const double fitSigmaNs = std::sqrt(k) * theta;

  auto withinTolerance = [](double value, double target, double tolerance) {
    return std::abs(value - target) <= tolerance;
  };

  const bool fwhmOk = (fwhmNs >= 0.0) &&
                      withinTolerance(fwhmNs, targetFwhmNs, toleranceFwhmNs);
  const bool riseOk = (riseNs >= 0.0) &&
                      withinTolerance(riseNs, targetRiseNs, toleranceRiseNs);
  const bool fallOk = (fallNs >= 0.0) &&
                      withinTolerance(fallNs, targetFallNs, toleranceFallNs);
  const bool meanOk = withinTolerance(fitMeanNs, targetMeanNs, toleranceMeanNs);

  std::cout << "Timing validation summary for " << inputFile << std::endl;
  std::cout << "  Entries               : " << birthHistogram->GetEntries()
            << std::endl;
  std::cout << "  Histogram mean [ns]   : " << meanNs << std::endl;
  std::cout << "  Fit mean [ns]         : " << fitMeanNs
            << "  target=" << targetMeanNs
            << "  status=" << (meanOk ? "OK" : "CHECK") << std::endl;
  std::cout << "  Fit MPV [ns]          : " << fitMpvNs << std::endl;
  std::cout << "  Fit sigma [ns]        : " << fitSigmaNs << std::endl;
  std::cout << "  FWHM [ns]             : " << fwhmNs
            << "  target=" << targetFwhmNs
            << "  status=" << (fwhmOk ? "OK" : "CHECK") << std::endl;
  std::cout << "  Rise 10-90 [ns]       : " << riseNs
            << "  target=" << targetRiseNs
            << "  status=" << (riseOk ? "OK" : "CHECK") << std::endl;
  std::cout << "  Fall 90-10 [ns]       : " << fallNs
            << "  target=" << targetFallNs
            << "  status=" << (fallOk ? "OK" : "CHECK") << std::endl;

  TCanvas canvas("c_timing_validation", "Timing Validation", 1100, 800);
  canvas.SetLogy();
  birthHistogram->SetLineColor(kBlue + 1);
  birthHistogram->SetLineWidth(3);
  birthHistogram->SetMinimum(1.0);
  birthHistogram->Draw("HIST");
  fitFunction.Draw("SAME");

  TLegend legend(0.56, 0.76, 0.88, 0.88);
  legend.AddEntry(birthHistogram.get(), "Scintillation birth times", "l");
  legend.AddEntry(&fitFunction, "Shifted-gamma fit", "l");
  legend.Draw();

  TLatex label;
  label.SetNDC(true);
  label.SetTextSize(0.031);
  label.DrawLatex(0.56, 0.68, Form("Fit mean = %.3f ns", fitMeanNs));
  label.DrawLatex(0.56, 0.64, Form("Fit MPV = %.3f ns", fitMpvNs));
  label.DrawLatex(0.56, 0.60, Form("Fit #sigma = %.3f ns", fitSigmaNs));
  label.DrawLatex(0.56, 0.56,
                  Form("FWHM = %.3f ns (target %.3f)", fwhmNs, targetFwhmNs));
  label.DrawLatex(0.56, 0.52,
                  Form("Rise 0.1-0.9 = %.3f ns (target %.3f)", riseNs,
                       targetRiseNs));
  label.DrawLatex(0.56, 0.48,
                  Form("Fall 0.9-0.1 = %.3f ns (target %.3f)", fallNs,
                       targetFallNs));

  SaveCanvas(canvas, outputDir, "timing_validation");
}
