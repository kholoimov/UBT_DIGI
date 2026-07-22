// fit_scintillation.C
//
// Run with:
//   root -l fit_scintillation.C
//
// Or compile:
//   root -l
//   .L fit_scintillation.C+
//   fit_scintillation();

#include <TH1F.h>
#include <TF1.h>
#include <TCanvas.h>
#include <TFile.h>
#include <TFitResultPtr.h>
#include <TLegend.h>
#include <TLatex.h>
#include <TMath.h>
#include <TRandom3.h>
#include <TStyle.h>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

// ------------------------------------------------------------
// Two-component scintillation model plus constant background.
//
// Parameter definitions:
//
// [0] = t0              : pulse start time
// [1] = N_fast          : integral/yield of fast component
// [2] = tau_fast        : fast decay time
// [3] = N_slow          : integral/yield of slow component
// [4] = tau_slow        : slow decay time
// [5] = tau_rise        : common rise time
// [6] = background      : constant background per bin
//
// The exponential components are normalized so that N_fast and
// N_slow approximately represent the total numbers of counts in
// each component.
// ------------------------------------------------------------
Double_t ScintillationCommonRise(Double_t* x, Double_t* p)
{
    // p[0] = t0
    // p[1] = Nfast, integrated fast-component counts
    // p[2] = tauFast
    // p[3] = Nslow, integrated slow-component counts
    // p[4] = tauSlow
    // p[5] = tauRise
    // p[6] = background

    const Double_t t          = x[0];
    const Double_t t0         = p[0];
    const Double_t nFast      = p[1];
    const Double_t tauFast    = p[2];
    const Double_t nSlow      = p[3];
    const Double_t tauSlow    = p[4];
    const Double_t tauRise    = p[5];
    const Double_t background = p[6];

    if (t < t0) {
        return background;
    }

    if (tauRise <= 0.0 ||
        tauFast <= tauRise ||
        tauSlow <= tauRise) {
        return background;
    }

    const Double_t dt = t - t0;

    const Double_t fast =
        nFast / (tauFast - tauRise) *
        (TMath::Exp(-dt / tauFast) -
         TMath::Exp(-dt / tauRise));

    const Double_t slow =
        nSlow / (tauSlow - tauRise) *
        (TMath::Exp(-dt / tauSlow) -
         TMath::Exp(-dt / tauRise));

    return background + fast + slow;
}

// ============================================================
// Fast component only
// ============================================================
Double_t FastWithCommonRise(Double_t *x, Double_t *par)
{
    const Double_t t       = x[0];
    const Double_t t0      = par[0];
    const Double_t nFast   = par[1];
    const Double_t tauFast = par[2];
    const Double_t tauRise = par[5];

    if (t < t0 ||
        tauRise <= 0.0 ||
        tauFast <= tauRise) {
        return 0.0;
    }

    const Double_t dt = t - t0;

    return nFast / (tauFast - tauRise) *
           (
               TMath::Exp(-dt / tauFast) -
               TMath::Exp(-dt / tauRise)
           );
}

// ============================================================
// Slow component only
// ============================================================
Double_t SlowWithCommonRise(Double_t *x, Double_t *par)
{
    const Double_t t       = x[0];
    const Double_t t0      = par[0];
    const Double_t nSlow   = par[3];
    const Double_t tauSlow = par[4];
    const Double_t tauRise = par[5];

    if (t < t0 ||
        tauRise <= 0.0 ||
        tauSlow <= tauRise) {
        return 0.0;
    }

    const Double_t dt = t - t0;

    return nSlow / (tauSlow - tauRise) *
           (
               TMath::Exp(-dt / tauSlow) -
               TMath::Exp(-dt / tauRise)
           );
}

std::vector<Double_t> LoadInputArray(const std::string& inputPath)
{
    std::ifstream input(inputPath);
    if (!input.is_open()) {
        std::cerr << "Error: unable to open input spectrum file: "
                  << inputPath << std::endl;
        return {};
    }

    std::vector<Double_t> values;
    std::string line;
    while (std::getline(input, line)) {
        const std::size_t commentPosition = line.find('#');
        if (commentPosition != std::string::npos) {
            line.erase(commentPosition);
        }

        std::replace(line.begin(), line.end(), ',', ' ');

        std::istringstream stream(line);
        Double_t value = 0.0;
        while (stream >> value) {
            values.push_back(value);
        }
    }

    return values;
}

void fit_scintillation(const char* inputPath = "analysis/input/fit_scintillation_input_default.dat")
{
    gStyle->SetOptStat(0);
    gStyle->SetOptFit(0);

    // --------------------------------------------------------
    // Histogram definition
    // --------------------------------------------------------
    const Int_t numberOfBins = 11494 / 6;
    const Double_t xMinimum = 100.0;
    const Double_t xMaximum = 200.0;

    TH1F *hist_dnl_inl__1 = new TH1F(
        "time_spectrum",
        "Scintillation Time Spectrum;Time [ns];Counts",
        numberOfBins,
        xMinimum,
        xMaximum
    );

    hist_dnl_inl__1->Sumw2();

    // --------------------------------------------------------
    // Load the full input spectrum from a configuration file.
    // --------------------------------------------------------
    std::vector<Double_t> hist_dnl_inl__1_vect0 =
        LoadInputArray(inputPath);

    constexpr std::size_t kSliceOffset = 1915;
    constexpr std::size_t kSliceLength = 1915;
    if (hist_dnl_inl__1_vect0.size() < kSliceOffset + kSliceLength) {
        std::cerr << "Error: input spectrum file contains "
                  << hist_dnl_inl__1_vect0.size()
                  << " values, but at least "
                  << (kSliceOffset + kSliceLength)
                  << " are required." << std::endl;
        return;
    }

    std::vector<Double_t> hist_dnl_inl__1_vect0_processed(
        hist_dnl_inl__1_vect0.begin() + kSliceOffset,
        hist_dnl_inl__1_vect0.begin() + kSliceOffset + kSliceLength
    );

    if (hist_dnl_inl__1_vect0_processed.empty()) {
        std::cerr
            << "Error: hist_dnl_inl__1_vect0_processed contains no data."
            << std::endl;
        return;
    }

    if (hist_dnl_inl__1_vect0_processed.size() !=
        static_cast<std::size_t>(numberOfBins)) {
        std::cerr
            << "Warning: vector contains "
            << hist_dnl_inl__1_vect0_processed.size()
            << " values, but the histogram contains "
            << numberOfBins
            << " bins."
            << std::endl;
    }

    // Do not write to underflow or overflow bins.
    const Int_t binsToCopy = std::min(
        numberOfBins,
        static_cast<Int_t>(hist_dnl_inl__1_vect0_processed.size())
    );

    for (Int_t bin = 1; bin <= binsToCopy; ++bin) {
        const Double_t value =
            hist_dnl_inl__1_vect0_processed.at(static_cast<std::size_t>(bin - 1));

        hist_dnl_inl__1->SetBinContent(bin, value);

        // Appropriate when the vector stores unweighted event counts.
        // Replace this if you have externally calculated uncertainties.
        hist_dnl_inl__1->SetBinError(
            bin,
            value > 0.0 ? std::sqrt(value) : 1.0
        );
    }

    // --------------------------------------------------------
    // Determine approximate starting values
    // --------------------------------------------------------
    const Int_t maximumBin = hist_dnl_inl__1->GetMaximumBin();
    const Double_t peakTime = hist_dnl_inl__1->GetBinCenter(maximumBin);
    const Double_t peakHeight = hist_dnl_inl__1->GetBinContent(maximumBin);

    // Estimate background from the first and last 5% of the spectrum.
    const Int_t sidebandBins =
        std::max(10, static_cast<Int_t>(0.05 * numberOfBins));

    Double_t backgroundSum = 0.0;
    Int_t backgroundEntries = 0;

    for (Int_t bin = 1; bin <= sidebandBins; ++bin) {
        backgroundSum += hist_dnl_inl__1->GetBinContent(bin);
        ++backgroundEntries;
    }

    for (Int_t bin = numberOfBins - sidebandBins + 1;
         bin <= numberOfBins;
         ++bin) {
        backgroundSum += hist_dnl_inl__1->GetBinContent(bin);
        ++backgroundEntries;
    }

    const Double_t backgroundEstimate =
        backgroundEntries > 0
            ? backgroundSum / backgroundEntries
            : 0.0;

    const Double_t totalCounts =
        std::max(1.0, hist_dnl_inl__1->Integral());

    const Double_t onsetThreshold =
        backgroundEstimate + 0.05 * std::max(0.0, peakHeight - backgroundEstimate);
    Double_t onsetTime = peakTime - 2.0;
    for (Int_t bin = 1; bin <= maximumBin; ++bin) {
        if (hist_dnl_inl__1->GetBinContent(bin) > onsetThreshold) {
            onsetTime = hist_dnl_inl__1->GetBinCenter(bin);
            break;
        }
    }

    // --------------------------------------------------------
    // Select the fitting interval.
    //
    // Adjust these values to exclude pre-trigger structure or
    // regions unrelated to the scintillation pulse.
    // --------------------------------------------------------
    const Double_t fitMinimum = std::max(xMinimum, onsetTime - 0.8);
    const Double_t fitMaximum = xMaximum;

    const Double_t fastYieldInit =
        std::max(0.05 * totalCounts, peakHeight * 2.0);
    const Double_t slowYieldInit =
        std::max(0.01 * totalCounts, peakHeight * 12.0);
    const Double_t backgroundUpper =
        std::max(200.0, 3.0 * backgroundEstimate);

    TF1* fitFunction = new TF1(
        "fitFunction",
        ScintillationCommonRise,
        fitMinimum,
        fitMaximum,
        7
    );

    fitFunction->SetParNames(
        "t0",
        "Nfast",
        "tauFast",
        "Nslow",
        "tauSlow",
        "tauRise",
        "Background"
    );

    fitFunction->SetParameters(
        onsetTime,           // t0
        fastYieldInit,       // Nfast
        2.3,                 // tauFast
        slowYieldInit,       // Nslow
        20.0,                // tauSlow
        0.18,                // tauRise
        backgroundEstimate
    );

    fitFunction->SetParLimits(0, onsetTime - 0.8, onsetTime + 0.8);

    fitFunction->SetParLimits(1, 0.0, 100.0 * peakHeight);
    fitFunction->SetParLimits(2, 0.8, 8.0);

    fitFunction->SetParLimits(3, 0.0, 100.0 * peakHeight);
    fitFunction->SetParLimits(4, 6.0, 80.0);

    fitFunction->SetParLimits(5, 0.02, 0.7);
    fitFunction->SetParLimits(6, std::max(0.0, 0.5 * backgroundEstimate),
                              backgroundUpper);

    // R: use the TF1 fitting range.
    // S: return TFitResultPtr.
    // I: integrate the function over each bin.
    // M: improve minimization for a multi-parameter model.
    //
    // A chi2 fit is more stable here because the imported spectrum has
    // corrected floating-bin contents rather than raw integer Poisson counts.
    hist_dnl_inl__1->Fit(fitFunction, "QRSIM");
    TFitResultPtr fitResult =
        hist_dnl_inl__1->Fit(fitFunction, "RSIM");

    if (!fitResult.Get() || fitResult->Status() != 0) {
        std::cerr << "The fit did not return a valid result." << std::endl;
        if (fitResult.Get()) {
            std::cerr << "Fit status code: " << fitResult->Status()
                      << std::endl;
        }
    }

    // --------------------------------------------------------
    // Extract results
    // --------------------------------------------------------
    const double tauFast = fitFunction->GetParameter(2);
    const double tauSlow = fitFunction->GetParameter(4);
    const double tauRise = fitFunction->GetParameter(5);

    const double nFast = fitFunction->GetParameter(1);
    const double nSlow = fitFunction->GetParameter(3);

    const double totalYield = std::max(1e-12, nFast + nSlow);
    const double fastFraction = nFast / totalYield;
    const double slowFraction = nSlow / totalYield;

    std::cout << "kFastDecayTimeNs = " << tauFast << ";\n";
    std::cout << "kSlowDecayTimeNs = " << tauSlow << ";\n";
    std::cout << "kRiseTimeNs = " << tauRise << ";\n";
    std::cout << "kFastComponentYield = " << fastFraction << ";\n";
    std::cout << "kSlowComponentYield = " << slowFraction << ";\n";

    // --------------------------------------------------------
    // Draw and save the final fit plot
    // --------------------------------------------------------
    TCanvas *canvas = new TCanvas(
        "scintillation_fit_canvas",
        "Scintillation fit",
        1200,
        800
    );
    canvas->SetLogy();

    hist_dnl_inl__1->SetMarkerStyle(20);
    hist_dnl_inl__1->SetMarkerSize(0.5);
    hist_dnl_inl__1->SetMarkerColor(kBlack);
    hist_dnl_inl__1->SetLineColor(kBlack);
    hist_dnl_inl__1->SetMinimum(10.0);
    hist_dnl_inl__1->SetMaximum(1e5);
    hist_dnl_inl__1->Draw("E");

    fitFunction->SetLineColor(kRed + 1);
    fitFunction->SetLineStyle(1);
    fitFunction->SetLineWidth(3);
    fitFunction->SetNpx(2000);
    fitFunction->Draw("SAME");

    TF1 *fastComponent = new TF1(
        "fastComponent",
        FastWithCommonRise,
        fitMinimum,
        fitMaximum,
        7
    );
    TF1 *slowComponent = new TF1(
        "slowComponent",
        SlowWithCommonRise,
        fitMinimum,
        fitMaximum,
        7
    );
    fastComponent->SetParameters(fitFunction->GetParameters());
    slowComponent->SetParameters(fitFunction->GetParameters());
    fastComponent->SetLineColor(kBlue + 1);
    fastComponent->SetLineStyle(2);
    fastComponent->SetLineWidth(2);
    slowComponent->SetLineColor(kGreen + 2);
    slowComponent->SetLineStyle(2);
    slowComponent->SetLineWidth(2);
    fastComponent->Draw("SAME");
    slowComponent->Draw("SAME");

    TLegend *legend = new TLegend(0.55, 0.72, 0.88, 0.88);
    legend->SetBorderSize(0);
    legend->SetFillStyle(0);
    legend->AddEntry(hist_dnl_inl__1, "Input spectrum", "lep");
    legend->AddEntry(fitFunction, "Total fit", "l");
    legend->AddEntry(fastComponent, "Fast component", "l");
    legend->AddEntry(slowComponent, "Slow component", "l");
    legend->Draw();

    TLatex label;
    label.SetNDC(true);
    label.SetTextSize(0.03);

    const auto x_label_position = 0.3;
    const auto y_label_top_position = 0.85;
    const auto y_label_step_value = 0.04;
    label.DrawLatex(x_label_position, y_label_top_position, Form("t_{0} = %.3f ns", fitFunction->GetParameter(0)));
    label.DrawLatex(x_label_position, y_label_top_position - y_label_step_value * 1, Form("#tau_{rise} = %.3f ns", tauRise));
    label.DrawLatex(x_label_position, y_label_top_position - y_label_step_value * 2, Form("#tau_{fast} = %.3f ns", tauFast));
    label.DrawLatex(x_label_position, y_label_top_position - y_label_step_value * 3, Form("#tau_{slow} = %.3f ns", tauSlow));
    label.DrawLatex(x_label_position, y_label_top_position - y_label_step_value * 4, Form("fast yield = %.3f", fastFraction));
    label.DrawLatex(x_label_position, y_label_top_position - y_label_step_value * 5, Form("slow yield = %.3f", slowFraction));

    canvas->SaveAs("analysis/scintillation_fit.pdf");

    // Use exactly the interval selected for the fit for the data-only and
    // data-versus-generated timing plots.
    std::unique_ptr<TH1F> measuredSpectrum(
        static_cast<TH1F*>(hist_dnl_inl__1->Clone("measured_timing_spectrum")));
    measuredSpectrum->SetDirectory(nullptr);
    measuredSpectrum->GetXaxis()->SetRangeUser(fitMinimum, fitMaximum);
    measuredSpectrum->SetTitle(
        "Measured scintillation timing;Time [ns];Counts");

    TCanvas measuredCanvas("measured_timing_canvas", "Measured timing", 1200,
                           800);
    measuredCanvas.SetLogy();
    measuredSpectrum->SetMinimum(1.0);
    measuredSpectrum->Draw("E");
    measuredCanvas.SaveAs("analysis/scintillation_real_data.pdf");

    TF1 generatedTimingPdf("generated_timing_pdf", ScintillationCommonRise,
                           fitMinimum, fitMaximum, 7);
    generatedTimingPdf.SetParameters(fitFunction->GetParameters());
    generatedTimingPdf.SetParameter(6, 0.0);

    std::unique_ptr<TH1F> generatedSpectrum(static_cast<TH1F*>(
        measuredSpectrum->Clone("generated_timing_spectrum")));
    generatedSpectrum->Reset("ICES");
    generatedSpectrum->SetDirectory(nullptr);
    generatedSpectrum->SetTitle(
        "Measured and generated scintillation timing;Time [ns];Normalized "
        "entries");

    constexpr int kGeneratedSamples = 1000000;
    TRandom3 timingRandom(12345);
    for (int sample = 0; sample < kGeneratedSamples; ++sample) {
        generatedSpectrum->Fill(
            generatedTimingPdf.GetRandom(fitMinimum, fitMaximum, &timingRandom));
    }

    const int fitFirstBin = measuredSpectrum->FindBin(fitMinimum);
    const int fitLastBin = measuredSpectrum->FindBin(fitMaximum);
    const double measuredIntegral =
        measuredSpectrum->Integral(fitFirstBin, fitLastBin);
    const double generatedIntegral =
        generatedSpectrum->Integral(fitFirstBin, fitLastBin);
    if (measuredIntegral > 0.0) {
        measuredSpectrum->Scale(1.0 / measuredIntegral);
    }
    if (generatedIntegral > 0.0) {
        generatedSpectrum->Scale(1.0 / generatedIntegral);
    }

    TCanvas comparisonCanvas("timing_comparison_canvas",
                             "Measured and generated timing", 1200, 800);
    comparisonCanvas.SetLogy();
    measuredSpectrum->SetMinimum(1e-7);
    measuredSpectrum->SetMaximum(
        2.0 * std::max(measuredSpectrum->GetMaximum(),
                       generatedSpectrum->GetMaximum()));
    measuredSpectrum->SetMarkerStyle(20);
    measuredSpectrum->SetMarkerSize(0.45);
    measuredSpectrum->SetMarkerColor(kBlack);
    measuredSpectrum->SetLineColor(kBlack);
    generatedSpectrum->SetLineColor(kRed + 1);
    generatedSpectrum->SetLineWidth(3);
    measuredSpectrum->Draw("E");
    generatedSpectrum->Draw("HIST SAME");

    TLegend comparisonLegend(0.58, 0.76, 0.88, 0.88);
    comparisonLegend.SetBorderSize(0);
    comparisonLegend.SetFillStyle(0);
    comparisonLegend.AddEntry(measuredSpectrum.get(), "Measured data", "lep");
    comparisonLegend.AddEntry(generatedSpectrum.get(),
                              "Generated fitted timing model", "l");
    comparisonLegend.Draw();
    comparisonCanvas.SaveAs(
        "analysis/scintillation_real_vs_generated.pdf");

    TFile outputFile("analysis/scintillation_fit.root", "RECREATE");
    hist_dnl_inl__1->Write();
    fitFunction->Write("scintillation_fit_function");
    canvas->Write();
    outputFile.Close();
}
