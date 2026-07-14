# UBT_DIGI

Standalone Geant4 example for a `40 x 40 x 10 mm^3` p-terphenyl scintillator with:

- a configurable particle gun for `mu-` or `gamma`
- optical scintillation enabled
- optical photon transport from scintillator to a centered `6 x 6 mm^2` SiPM
- SiPM photoelectron and charge calculation based on photons reaching the photocathode
- ROOT output including primary beam info, muon range, SiPM timing, and SiPM charge

## Layout

- `main.cc`: application entry point
- `include/`, `src/`: detector, gun, stepping, and digitization classes
- `macros/run_muons.mac`: example run for muons
- `macros/run_gammas.mac`: example run for gammas

## Build

Geant4 must be installed and discoverable by CMake.

```bash
cmake -S . -B build
cmake --build build -j
```

## Run

```bash
./build/ubt_scintillator macros/run_muons.mac
./build/ubt_scintillator macros/run_gammas.mac
```

If you start `./build/ubt_scintillator` without arguments, the Geant4 interactive UI is opened.

## Multithreading

The application now uses the Geant4 default run manager, so if your Geant4 build has multithreading enabled it can run events in parallel.

The example macros set:

```text
/run/numberOfThreads 4
```

This command must appear before `/run/initialize`.

Per-event accumulation is thread-local, and ROOT writing uses Geant4 analysis ntuple merging via `G4AnalysisManager::SetNtupleMerging(true)`, which is the safe way to write a single merged ROOT file from worker threads.

## Detector

The scintillator is centered at the origin and has half-sizes:

- `20 mm` in `x`
- `20 mm` in `y`
- `5 mm` in `z`

This corresponds to the requested full size of `40 x 40 x 10 mm^3`.

On the `+z` face, the model includes:

- `0.2 mm` optical grease
- `1.0 mm` SiPM window
- `0.1 mm` photocathode sensitive layer

The sensor stack is centered on the `+z` face and has a `6 x 6 mm^2` active footprint. Optical photons are produced in the scintillator, propagated through these media, and counted when they reach the photocathode.

The scintillation timing model now uses:

- rise time: `0.20 ns`
- fast decay: `0.60 ns`
- slow tail: `1.80 ns`
- fast/slow yield split: `92% / 8%`

The scintillator also has a diffuse reflective wrapping model on its surfaces:

- reflectivity: `96%`
- randomized reflection angles through a `groundfrontpainted` unified optical surface
- surface roughness parameter `sigma_alpha = 0.35`

## Digitization model

For every event, the code accumulates:

- `Edep`: total deposited energy in the scintillator
- `Nscint`: number of optical photons produced by the `Scintillation` process in the scintillator volume
- `Npmt`: number of optical photons accepted by the simplified light-collection model and passed to the sensor
- `Npe`: number of photoelectrons created at the photocathode after photon detection efficiency is applied

Then it computes a simple digital response:

```text
charge_pc  = Npe * PMT_gain * e
adc_counts = round(charge_pc * adc_counts_per_pc)
triggered  = (Edep >= 0.20 MeV) and (charge_pc >= 5 pC)
```

Current constants in `src/ScintillatorDigitizerModule.cc`:

- PMT gain = `2.8e6`
- ADC scale = `1 count / pC`
- trigger threshold = `0.20 MeV`
- threshold timing observable = time to `5` detected photoelectrons from the primary hit time
- threshold scan: timing sigma versus `{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 20, 30, 40}` detected-photoelectron thresholds

Current constant in `src/PMTSensitiveDetector.cc`:

- light collection efficiency = `30%`
- sensor photon detection efficiency = `30%`

This simplified sensor chain therefore gives an overall produced-photon to photoelectron efficiency of about `9%`.

## Output

Each run writes `scintillator_digi.root` with an `events` ntuple containing:

```text
event_id
primary_particle
primary_energy_mev
primary_momentum_mev_c
muon_range_mm
edep_mev
scintillation_photons
pmt_incident_photons
photoelectrons
pmt_first_hit_ns
pmt_charge_pc
adc_counts
triggered
scintillation_production_fwhm_ns
photoelectron_threshold_5_from_muon_ns
threshold_scan_pe
threshold_scan_mean_ns
threshold_scan_sigma_ns
photoelectron_arrival_1_from_muon_ns
...
photoelectron_arrival_200_from_muon_ns
```

For normal event rows:

- `scintillation_production_fwhm_ns` is set to `-1`
- `photoelectron_threshold_5_from_muon_ns` stores the event-by-event delay between the primary hit time and the moment the `5`th detected photoelectron arrives, or `-1` if the event never reaches `5` photoelectrons
- `threshold_scan_pe`, `threshold_scan_mean_ns`, and `threshold_scan_sigma_ns` are set to `-1`
- `photoelectron_arrival_1_from_muon_ns` through `photoelectron_arrival_200_from_muon_ns` store the arrival times of the `1`st through `200`th detected photoelectrons relative to the primary hit time, or `-1` when that photoelectron index is not reached in the event

At the end of each run, the code appends one extra row to the same `events` tree with:

- `event_id = -1`
- `primary_particle = RUN_SUMMARY`
- `scintillation_production_fwhm_ns` filled with the run-level FWHM of `scintillation_production_time_ns`
- `photoelectron_threshold_5_from_muon_ns` filled with the run-average of the valid event `t5` values
- `photoelectron_arrival_1_from_muon_ns` through `photoelectron_arrival_200_from_muon_ns` set to `-1`

The code also appends one `THRESHOLD_SCAN` row per threshold to the same `events` tree with:

- `event_id = -2`
- `primary_particle = THRESHOLD_SCAN`
- `threshold_scan_pe` set to the detected-photoelectron threshold
- `threshold_scan_mean_ns` set to the run-level mean threshold-crossing time
- `threshold_scan_sigma_ns` set to the run-level sigma of the threshold-crossing time
- `photoelectron_arrival_1_from_muon_ns` through `photoelectron_arrival_200_from_muon_ns` set to `-1`

The ROOT file also contains these run-level timing histograms:

- `scintillation_production_time_ns`: production-time distribution of scintillation photons created inside the scintillator
- `photoelectron_arrival_time_ns`: arrival-time distribution of detected photoelectrons at the sensor

The console printout contains the same event-level information.

## Analysis

The repository includes a ROOT C++ macro at [analysis/plot_event_observables.C](/Users/vkholoimov/Documents/SHIP/UBT_DIGI/analysis/plot_event_observables.C) that reads `scintillator_digi.root` and produces:

- light production vs muon energy
- photons arriving at the sensor vs produced scintillation photons
- detected photoelectrons vs produced scintillation photons
- timing sigma vs photoelectron threshold requirement
- arrival-time overlay histograms for selected photoelectron indices
- mean arrival time vs photoelectron index (`1..200`)

Run it with:

```bash
root -l -q 'analysis/plot_event_observables.C("scintillator_digi.root","analysis/plots")'
```

The plots are written to `analysis/plots/` as both `.png` and `.pdf`.

## Muon energy sampling

For `mu-` runs, the primary kinetic energy is randomized independently for each event in:

```text
1 GeV <= E_mu <= 20 GeV
```

That sampled value is what gets written to `primary_energy_mev` in the ROOT ntuple and shown in the console output.

## Muon beam spot

For `mu-` runs, the code also randomizes the initial transverse position event-by-event on the plane:

```text
z = -30 mm
-15 mm <= x <= 15 mm
-15 mm <= y <= 15 mm
```

So muons do not always pass through the detector center even though the macro keeps the nominal gun plane and forward direction.

## Changing the gun particle

The particle gun is configured through standard Geant4 gun commands in the macro:

```text
/gun/particle mu-
/gun/particle gamma
```

For `gamma`, the macro `'/gun/energy ...'` setting is used directly.
For `mu-`, the code overrides the macro energy and samples a random value from `1` to `20 GeV` for each event.
