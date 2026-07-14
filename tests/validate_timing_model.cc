#include "TimingModelParameters.hh"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <numeric>
#include <random>
#include <string>
#include <vector>

namespace {

struct Options {
  int samples = 200000;
  double targetFwhmNs = -1.0;
  double toleranceFwhmNs = 0.30;
  double targetRiseNs = -1.0;
  double toleranceRiseNs = 0.20;
  double targetFallNs = -1.0;
  double toleranceFallNs = 0.80;
  bool verbose = true;
};

double TimingPdf(double timeNs, double riseNs, double decayNs) {
  if (timeNs < 0.0 || riseNs <= 0.0 || decayNs <= riseNs) {
    return 0.0;
  }
  return (std::exp(-timeNs / decayNs) - std::exp(-timeNs / riseNs)) /
         (decayNs - riseNs);
}

double TimingMode(double riseNs, double decayNs) {
  if (riseNs <= 0.0 || decayNs <= riseNs) {
    return 0.0;
  }
  return (riseNs * decayNs / (decayNs - riseNs)) *
         std::log(decayNs / riseNs);
}

double SampleComponent(std::mt19937_64& rng, double riseNs, double decayNs,
                       double timeMaxNs) {
  std::uniform_real_distribution<double> timeDist(0.0, timeMaxNs);
  const double modeNs = TimingMode(riseNs, decayNs);
  const double pdfMax = TimingPdf(modeNs, riseNs, decayNs);
  std::uniform_real_distribution<double> acceptDist(0.0, pdfMax);

  while (true) {
    const double timeNs = timeDist(rng);
    if (acceptDist(rng) <= TimingPdf(timeNs, riseNs, decayNs)) {
      return timeNs;
    }
  }
}

double QuantileFromSorted(const std::vector<double>& values, double q) {
  if (values.empty()) {
    return -1.0;
  }
  const double index = q * static_cast<double>(values.size() - 1);
  const std::size_t lower = static_cast<std::size_t>(std::floor(index));
  const std::size_t upper = static_cast<std::size_t>(std::ceil(index));
  if (lower == upper) {
    return values[lower];
  }
  const double fraction = index - static_cast<double>(lower);
  return values[lower] + fraction * (values[upper] - values[lower]);
}

double EstimateMode(const std::vector<double>& values, double binWidthNs,
                    double timeMaxNs) {
  if (values.empty() || binWidthNs <= 0.0) {
    return -1.0;
  }
  const int nBins = static_cast<int>(std::ceil(timeMaxNs / binWidthNs));
  std::vector<int> counts(nBins, 0);
  for (double value : values) {
    if (value < 0.0 || value >= timeMaxNs) {
      continue;
    }
    const int bin = static_cast<int>(value / binWidthNs);
    if (bin >= 0 && bin < nBins) {
      ++counts[bin];
    }
  }
  const auto peakIt = std::max_element(counts.begin(), counts.end());
  if (peakIt == counts.end() || *peakIt <= 0) {
    return -1.0;
  }
  const int peakBin = static_cast<int>(std::distance(counts.begin(), peakIt));
  return (peakBin + 0.5) * binWidthNs;
}

double EstimateFwhm(const std::vector<double>& values, double binWidthNs,
                    double timeMaxNs) {
  if (values.empty() || binWidthNs <= 0.0) {
    return -1.0;
  }
  const int nBins = static_cast<int>(std::ceil(timeMaxNs / binWidthNs));
  std::vector<int> counts(nBins, 0);
  for (double value : values) {
    if (value < 0.0 || value >= timeMaxNs) {
      continue;
    }
    const int bin = static_cast<int>(value / binWidthNs);
    if (bin >= 0 && bin < nBins) {
      ++counts[bin];
    }
  }
  const auto peakIt = std::max_element(counts.begin(), counts.end());
  if (peakIt == counts.end() || *peakIt <= 0) {
    return -1.0;
  }

  const int peakBin = static_cast<int>(std::distance(counts.begin(), peakIt));
  const double halfMax = 0.5 * (*peakIt);

  int leftBin = peakBin;
  while (leftBin > 0 && counts[leftBin] >= halfMax) {
    --leftBin;
  }
  int rightBin = peakBin;
  while (rightBin + 1 < nBins && counts[rightBin] >= halfMax) {
    ++rightBin;
  }

  return std::max(0.0, (rightBin - leftBin) * binWidthNs);
}

Options ParseOptions(int argc, char** argv) {
  Options options;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    auto requireValue = [&](const std::string& flag) {
      if (i + 1 >= argc) {
        std::cerr << "Missing value for " << flag << std::endl;
        std::exit(2);
      }
      return std::string(argv[++i]);
    };

    if (arg == "--samples") {
      options.samples = std::stoi(requireValue(arg));
    } else if (arg == "--target-fwhm-ns") {
      options.targetFwhmNs = std::stod(requireValue(arg));
    } else if (arg == "--tolerance-fwhm-ns") {
      options.toleranceFwhmNs = std::stod(requireValue(arg));
    } else if (arg == "--target-rise-ns") {
      options.targetRiseNs = std::stod(requireValue(arg));
    } else if (arg == "--tolerance-rise-ns") {
      options.toleranceRiseNs = std::stod(requireValue(arg));
    } else if (arg == "--target-fall-ns") {
      options.targetFallNs = std::stod(requireValue(arg));
    } else if (arg == "--tolerance-fall-ns") {
      options.toleranceFallNs = std::stod(requireValue(arg));
    } else if (arg == "--quiet") {
      options.verbose = false;
    } else {
      std::cerr << "Unknown argument: " << arg << std::endl;
      std::exit(2);
    }
  }
  return options;
}

}  // namespace

int main(int argc, char** argv) {
  const Options options = ParseOptions(argc, argv);

  constexpr double riseNs = TimingModelParameters::kRiseTimeNs;
  constexpr double fastDecayNs = TimingModelParameters::kFastDecayTimeNs;
  constexpr double slowDecayNs = TimingModelParameters::kSlowDecayTimeNs;
  constexpr double fastWeight = TimingModelParameters::kFastComponentYield;
  constexpr double slowWeight = TimingModelParameters::kSlowComponentYield;
  constexpr double timeMaxNs = 80.0;
  constexpr double binWidthNs = 0.1;

  std::mt19937_64 rng(12345);
  std::uniform_real_distribution<double> componentDist(
      0.0, fastWeight + slowWeight);

  std::vector<double> samples;
  samples.reserve(static_cast<std::size_t>(options.samples));

  for (int i = 0; i < options.samples; ++i) {
    const bool useFast = componentDist(rng) < fastWeight;
    samples.push_back(useFast ? SampleComponent(rng, riseNs, fastDecayNs, timeMaxNs)
                              : SampleComponent(rng, riseNs, slowDecayNs, timeMaxNs));
  }

  std::sort(samples.begin(), samples.end());

  const double meanNs =
      std::accumulate(samples.begin(), samples.end(), 0.0) / samples.size();
  const double modeNs = EstimateMode(samples, binWidthNs, timeMaxNs);
  const double sigmaNs = std::sqrt(
      std::accumulate(samples.begin(), samples.end(), 0.0,
                      [meanNs](double sum, double value) {
                        const double diff = value - meanNs;
                        return sum + diff * diff;
                      }) /
      samples.size());
  const double fwhmNs = EstimateFwhm(samples, binWidthNs, timeMaxNs);
  const double rise10to90Ns =
      QuantileFromSorted(samples, 0.90) - QuantileFromSorted(samples, 0.10);
  const double fall90to10Ns =
      QuantileFromSorted(samples, 0.95) - QuantileFromSorted(samples, 0.50);

  if (options.verbose) {
    std::cout << "Timing-only validation using shared framework constants\n";
    std::cout << "  rise_ns=" << riseNs << '\n';
    std::cout << "  fast_decay_ns=" << fastDecayNs
              << " weight=" << fastWeight << '\n';
    std::cout << "  slow_decay_ns=" << slowDecayNs
              << " weight=" << slowWeight << '\n';
    std::cout << "  samples=" << options.samples << '\n';
    std::cout << "  mean_ns=" << meanNs << '\n';
    std::cout << "  mode_ns=" << modeNs << '\n';
    std::cout << "  sigma_ns=" << sigmaNs << '\n';
    std::cout << "  fwhm_ns=" << fwhmNs << '\n';
    std::cout << "  rise10to90_ns=" << rise10to90Ns << '\n';
    std::cout << "  fall_proxy_ns=" << fall90to10Ns << '\n';
  }

  bool success = true;
  if (options.targetFwhmNs >= 0.0) {
    success = success &&
              (std::abs(fwhmNs - options.targetFwhmNs) <=
               options.toleranceFwhmNs);
  }
  if (options.targetRiseNs >= 0.0) {
    success = success &&
              (std::abs(rise10to90Ns - options.targetRiseNs) <=
               options.toleranceRiseNs);
  }
  if (options.targetFallNs >= 0.0) {
    success = success &&
              (std::abs(fall90to10Ns - options.targetFallNs) <=
               options.toleranceFallNs);
  }

  return success ? 0 : 1;
}
