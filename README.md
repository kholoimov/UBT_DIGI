# UBT_DIGI

Standalone Geant4 simulation and digitization example for a p-terphenyl
scintillator read out by a SiPM.

Detailed geometry, material, optical-physics, timing, digitization, ROOT-schema,
and FairShip-integration descriptions are maintained in
[`documentation/main.tex`](documentation/main.tex).

## Main features

- configurable `mu-` and gamma particle gun
- Geant4 scintillation and optical-photon transport
- selectable 20 x 20 or 40 x 40 mm scintillator with a fixed 6 x 6 mm SiPM
- configurable distributed or central muon beam position
- event-level deposited energy, photon, photoelectron, timing, charge, ADC, and
  trigger observables
- optional row-per-photon ROOT output, disabled by default
- multithreaded event processing and merged ROOT output
- ROOT analysis macros for timing, event observables, and ADC response
- FairShip digitization maps for position-dependent 20 and 40 mm tile
  responses, with a default 100-ADC trigger threshold

## Repository layout

- `main.cc`: application entry point
- `include/`, `src/`: simulation and digitization implementation
- `macros/`: example Geant4 run macros
- `analysis/`: ROOT analysis macros and the analysis-only driver
- `tests/`: compiled timing-model validation
- `documentation/`: full technical note
- `geometry/`: full and simplified UBT detector maps

## Build

Geant4 and ROOT must be discoverable by CMake.

```bash
cmake -S . -B build
cmake --build build -j
```

Run the registered test with:

```bash
ctest --test-dir build --output-on-failure
```

## Run the simulation

Example muon and gamma runs:

```bash
./build/ubt_scintillator macros/run_muons.mac
./build/ubt_scintillator macros/run_muons_center.mac
./build/ubt_scintillator macros/run_gammas.mac
```

The scintillator is 40 x 40 mm by default. Select the 20 x 20 mm tile with
the environment variable below; the SiPM active area remains 6 x 6 mm:

```bash
UBT_SCINTILLATOR_SIZE_MM=20 \
  ./build/ubt_scintillator macros/run_muons_20mm.mac
```

Only `20` and `40` are accepted. The 20 mm example restricts the randomized
beam to the physical tile. The selected size is saved as `tile_size_mm` in
every event-tree row so the analysis macros choose their spatial range
automatically.

Starting without a macro opens the interactive Geant4 UI:

```bash
./build/ubt_scintillator
```

The example macros set the number of worker threads before
`/run/initialize`. Adjust `/run/numberOfThreads` in the selected macro when
needed.

The simulation writes `scintillator_digi.root` in the working directory.

### Optional photon-level output

Detailed row-per-photon trees and timing histograms are disabled by default.
Enable them for photon timing studies with:

```bash
UBT_ENABLE_SCINTILLATOR_PHOTON_STUDIES=true \
  ./build/ubt_scintillator macros/run_muons.mac
```

Leave the variable unset, or set it to `false`, for general event and ADC
studies.

### Particle-gun controls

Select the particle and nominal gun configuration in a Geant4 macro:

```text
/gun/particle mu-
/gun/position 0 0 -30 mm
/gun/direction 0 0 1
```

Muon position randomization is configured after `/run/initialize`:

```text
/ubt/gun/randomizeMuonPosition true
/ubt/gun/muonBeamSpotHalfSize 20 mm
```

Use `randomizeMuonPosition false` for a fixed central trajectory. See the
files in `macros/` for complete examples.

## Run analyses

All general analyses can be run on an existing `scintillator_digi.root` with:

```bash
analysis/run_general_studies.sh
```

This script does not run Geant4. Input and output paths can be overridden:

```bash
UBT_ROOT_OUTPUT=/path/to/scintillator_digi.root \
UBT_ANALYSIS_OUTPUT=/path/to/plots \
  analysis/run_general_studies.sh
```

Individual general-study macros can also be run directly:

```bash
root -l -q 'analysis/plot_event_observables.C("scintillator_digi.root","analysis/plots")'
root -l -q 'analysis/study_adc_vs_edep_position.C("scintillator_digi.root","analysis/adc_studies")'
root -l -q 'analysis/plot_adc_per_edep_vs_position.C("scintillator_digi.root","analysis/adc_studies")'
```

The energy-normalized position analysis additionally writes
`adc/adc_per_edep_vs_position.csv`, a 1 x 1 mm-bin table containing the bin
center, event count, mean ADC/MeV, and standard error.

Photon timing analysis requires a simulation produced with photon-level output
enabled:

```bash
root -l -q 'analysis/validate_timing_distribution.C("scintillator_digi.root","analysis/plots")'
```

The timing model can be studied without a Geant4 output file:

```bash
root -l -q 'analysis/simulate_timing_model.C("analysis/plots")'
```

The compiled validator accepts optional numerical targets:

```bash
./build/ubt_timing_model_validation --samples 500000 \
  --target-fwhm-ns 2.8 --tolerance-fwhm-ns 0.5
```
