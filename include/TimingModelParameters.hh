#ifndef TIMINGMODELPARAMETERS_HH
#define TIMINGMODELPARAMETERS_HH

namespace TimingModelParameters {

inline constexpr double kRiseTimeNs = 0.54;
inline constexpr double kFastDecayTimeNs = 1.05;
inline constexpr double kSlowDecayTimeNs = 6.50;
inline constexpr double kFastComponentYield = 0.86;
inline constexpr double kSlowComponentYield = 0.14;

}  // namespace TimingModelParameters

#endif
