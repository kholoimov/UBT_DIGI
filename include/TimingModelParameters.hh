#ifndef TIMINGMODELPARAMETERS_HH
#define TIMINGMODELPARAMETERS_HH

namespace TimingModelParameters {

inline constexpr double kRiseTimeNs = 0.85;
inline constexpr double kFastDecayTimeNs = 2.30;
inline constexpr double kSlowDecayTimeNs = 8.50;
inline constexpr double kFastComponentYield = 0.78;
inline constexpr double kSlowComponentYield = 0.22;

}  // namespace TimingModelParameters

#endif
