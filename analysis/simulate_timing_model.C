#include "TCanvas.h"
#include "TF1.h"
#include "TH1D.h"
#include "TLatex.h"
#include "TLegend.h"
#include "TMath.h"
#include "TRandom3.h"
#include "TROOT.h"
#include "TString.h"
#include "TStyle.h"
#include "TSystem.h"

#include <algorithm>
#include <cmath>
#include <iostream>

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

double TimingPdf(double t, double riseNs, double decayNs) {
  if (t < 0.0 || decayNs <= riseNs || riseNs <= 0.0) {
    return 0.0;
  }
  const double norm = 1.0 / (decayNs - riseNs);
  return norm * (std::exp(-t / decayNs) - std::exp(-t / riseNs));
}

double TimingMode(double riseNs, double decayNs) {
  if (decayNs <= riseNs || riseNs <= 0.0) {
    return 0.0;
  }
  return (riseNs * decayNs / (decayNs - riseNs)) *
         std::log(decayNs / riseNs);
}

double SampleComponent(TRandom3& rng, double riseNs, double decayNs,
                       double tMaxNs) {
  const double mode = TimingMode(riseNs, decayNs);
  const double pdfMax = TimingPdf(mode, riseNs, decayNs);

  while (true) {
    const double t = rng.Uniform(0.0, tMaxNs);
    const double y = rng.Uniform(0.0, pdfMax);
    if (y <= TimingPdf(t, riseNs, decayNs)) {
      return t;
    }
  }
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

void simulate_timing_model(const char* outputDir = "analysis/plots",
                           int nSamples = 1000000, double riseNs = 0.85,
                           double fastDecayNs = 2.30,
                           double slowDecayNs = 8.50,
                           double fastWeight = 0.78,
                           double slowWeight = 0.22,
                           double tMaxNs = 80.0, int seed = 12345) {
  ConfigureStyle();
  gSystem->mkdir(outputDir, kTRUE);

  TRandom3 rng(seed);

  TH1D timingHist("timing_model_histogram",
                  "Standalone Scintillation Timing Model;time [ns];counts", 800,
                  0.0, tMaxNs);
  TH1D fastHist("timing_model_fast_component",
                "Standalone Scintillation Timing Model;time [ns];counts", 800,
                0.0, tMaxNs);
  TH1D slowHist("timing_model_slow_component",
                "Standalone Scintillation Timing Model;time [ns];counts", 800,
                0.0, tMaxNs);

  for (int i = 0; i < nSamples; ++i) {
    const bool useFast = rng.Uniform() < fastWeight / (fastWeight + slowWeight);
    const double timeNs =
        useFast ? SampleComponent(rng, riseNs, fastDecayNs, tMaxNs)
                : SampleComponent(rng, riseNs, slowDecayNs, tMaxNs);
    timingHist.Fill(timeNs);
    if (useFast) {
      fastHist.Fill(timeNs);
    } else {
      slowHist.Fill(timeNs);
    }
  }

  const double meanNs = timingHist.GetMean();
  const double fwhmNs = ComputeFwhm(timingHist);
  const double riseTimeNs = ComputeRiseTime10to90(timingHist);
  const double fallTimeNs = ComputeFallTime90to10(timingHist);

  const int peakBin = timingHist.GetMaximumBin();
  const double peakX = timingHist.GetBinCenter(peakBin);
  const double binWidth = timingHist.GetXaxis()->GetBinWidth(1);

  TF1 fitFunction(
      "timing_model_fit",
      [binWidth](double* x, double* p) {
        return binWidth * gammaComponent(x[0], p[0], p[1], p[2], p[3]);
      },
      std::max(0.0, binWidth), std::min(40.0, tMaxNs), 4);
  fitFunction.SetParNames("Area", "x0", "k", "theta");
  fitFunction.SetParameters(timingHist.Integral(), 0.02, 2.5,
                            std::max(0.05, peakX / 3.0));
  fitFunction.SetParLimits(0, 0.0, 10.0 * timingHist.Integral());
  fitFunction.SetParLimits(1, 0.0, 2.0);
  fitFunction.SetParLimits(2, 1.01, 30.0);
  fitFunction.SetParLimits(3, 0.02, 20.0);
  fitFunction.SetLineColor(kBlue + 3);
  fitFunction.SetLineWidth(3);
  timingHist.Fit(&fitFunction, "R0Q");

  const double x0 = fitFunction.GetParameter(1);
  const double k = fitFunction.GetParameter(2);
  const double theta = fitFunction.GetParameter(3);
  const double fitMeanNs = x0 + k * theta;
  const double fitMpvNs = k > 1.0 ? x0 + (k - 1.0) * theta : x0;
  const double fitSigmaNs = std::sqrt(k) * theta;

  std::cout << "Standalone timing-model validation" << std::endl;
  std::cout << "  Samples            : " << nSamples << std::endl;
  std::cout << "  Rise time [ns]     : " << riseNs << std::endl;
  std::cout << "  Fast decay [ns]    : " << fastDecayNs
            << " (weight " << fastWeight << ")" << std::endl;
  std::cout << "  Slow decay [ns]    : " << slowDecayNs
            << " (weight " << slowWeight << ")" << std::endl;
  std::cout << "  Histogram mean [ns]: " << meanNs << std::endl;
  std::cout << "  Fit mean [ns]      : " << fitMeanNs << std::endl;
  std::cout << "  Fit MPV [ns]       : " << fitMpvNs << std::endl;
  std::cout << "  Fit sigma [ns]     : " << fitSigmaNs << std::endl;
  std::cout << "  FWHM [ns]          : " << fwhmNs << std::endl;
  std::cout << "  Rise 0.1-0.9 [ns]  : " << riseTimeNs << std::endl;
  std::cout << "  Fall 0.9-0.1 [ns]  : " << fallTimeNs << std::endl;

  TCanvas canvas("c_timing_model", "Standalone Timing Model", 1100, 800);
  canvas.SetLogy();

  timingHist.SetLineColor(kBlue + 1);
  timingHist.SetLineWidth(3);
  timingHist.SetMinimum(1.0);
  fastHist.SetLineColor(kGreen + 2);
  fastHist.SetLineWidth(2);
  slowHist.SetLineColor(kRed + 1);
  slowHist.SetLineWidth(2);

  timingHist.Draw("HIST");
  fastHist.Draw("HIST SAME");
  slowHist.Draw("HIST SAME");
  fitFunction.Draw("SAME");

  TLegend legend(0.54, 0.70, 0.88, 0.88);
  legend.AddEntry(&timingHist, "Total timing model", "l");
  legend.AddEntry(&fastHist, "Fast component samples", "l");
  legend.AddEntry(&slowHist, "Slow component samples", "l");
  legend.AddEntry(&fitFunction, "Shifted-gamma fit", "l");
  legend.Draw();

  TLatex label;
  label.SetNDC(true);
  label.SetTextSize(0.031);
  label.DrawLatex(0.54, 0.62, Form("Fit mean = %.3f ns", fitMeanNs));
  label.DrawLatex(0.54, 0.58, Form("Fit MPV = %.3f ns", fitMpvNs));
  label.DrawLatex(0.54, 0.54, Form("Fit #sigma = %.3f ns", fitSigmaNs));
  label.DrawLatex(0.54, 0.50, Form("FWHM = %.3f ns", fwhmNs));
  label.DrawLatex(0.54, 0.46, Form("Rise 0.1-0.9 = %.3f ns", riseTimeNs));
  label.DrawLatex(0.54, 0.42, Form("Fall 0.9-0.1 = %.3f ns", fallTimeNs));

  SaveCanvas(canvas, outputDir, "timing_model_standalone");
}
