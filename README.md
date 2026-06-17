# UBT_DIGI

Standalone Geant4 example for a `40 x 40 x 15 mm^3` plastic scintillator with:

- a configurable particle gun for `mu-` or `gamma`
- optical scintillation enabled
- digitization based on deposited energy and the number of primary scintillation photons created inside the scintillator
- ROOT output including primary beam info and muon range through the scintillator

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

## Detector

The scintillator is centered at the origin and has half-sizes:

- `20 mm` in `x`
- `20 mm` in `y`
- `7.5 mm` in `z`

This corresponds to the requested full size of `40 x 40 x 15 mm^3`.

## Digitization model

For every event, the code accumulates:

- `Edep`: total deposited energy in the scintillator
- `Nscint`: number of optical photons produced by the `Scintillation` process in the scintillator volume

Then it computes a simple digital response:

```text
photoelectrons = Nscint * transport_efficiency * PDE
adc_counts     = round(photoelectrons * gain_adc_per_pe)
triggered      = (Edep >= 0.20 MeV) and (Nscint > 0)
```

Current constants in `src/ScintillatorDigitizerModule.cc`:

- transport efficiency = `0.22`
- photon detection efficiency = `0.35`
- ADC gain = `8 counts / photoelectron`
- trigger threshold = `0.20 MeV`

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
photoelectrons
adc_counts
triggered
```

The console printout contains the same event-level information.

## Muon energy sampling

For `mu-` runs, the primary kinetic energy is randomized independently for each event in:

```text
1 GeV <= E_mu <= 20 GeV
```

That sampled value is what gets written to `primary_energy_mev` in the ROOT ntuple and shown in the console output.

## Changing the gun particle

The particle gun is configured through standard Geant4 gun commands in the macro:

```text
/gun/particle mu-
/gun/particle gamma
```

For `gamma`, the macro `'/gun/energy ...'` setting is used directly.
For `mu-`, the code overrides the macro energy and samples a random value from `1` to `20 GeV` for each event.
