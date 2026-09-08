#!/usr/bin/env bash
# Discrete energy scan for mu-, e-, and gamma primaries.
#
# For each particle type, this shoots a fixed, mono-energetic beam
# (randomizeEnergy=false) at a set of energies distributed geometrically
# (a log-spaced / adaptive step) across that particle's energy range, so a
# wide dynamic range (e.g. the muon 0.5-100 GeV spectrum) is covered with
# few simulation points instead of a dense linear scan. Primary position is
# still randomized across the tile face (randomizeMuonPosition=true, i.e.
# particles are shot geometrically distributed over the tile), matching the
# general-studies convention.
#
# Each point is a short standalone run whose scintillator_digi.root is kept
# under UBT_ENERGY_SCAN_OUTPUT; a manifest CSV records (particle, energy,
# ROOT file) for the companion ROOT macro
# analysis/plot_adc_vs_particle_energy.C to aggregate into a mean-ADC-vs-energy
# plot per particle type.
set -euo pipefail

script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
project_dir=$(cd -- "${script_dir}/.." && pwd)

scan_output=${UBT_ENERGY_SCAN_OUTPUT:-"${project_dir}/output/energy_scan"}
analysis_output=${UBT_ENERGY_SCAN_ANALYSIS_OUTPUT:-"${project_dir}/analysis/plots/energy_scan"}
points=${UBT_ENERGY_SCAN_POINTS:-10}
executable="${project_dir}/build/ubt_scintillator"

mkdir -p "${scan_output}" "${analysis_output}"
manifest="${scan_output}/manifest.csv"
echo "particle,label,energy_mev,root_file" > "${manifest}"

muon_events=${UBT_ENERGY_SCAN_EVENTS_MUON:-1500}
electron_events=${UBT_ENERGY_SCAN_EVENTS_ELECTRON:-3000}
photon_events=${UBT_ENERGY_SCAN_EVENTS_PHOTON:-20000}
threads=${UBT_ENERGY_SCAN_THREADS:-80}

# name : G4 particle name : label for plots/report : min_energy_MeV : max_energy_MeV : events_per_point : threads
particle_configs=(
  "mu-|mu-|Muon|500|100000|${muon_events}|${threads}"
  "e-|e-|Electron|1|1000|${electron_events}|${threads}"
  "gamma|gamma|Photon|1|1000|${photon_events}|${threads}"
)

run_point() {
  # Tolerates the known shutdown-time segfault (exit 139) that occurs after
  # the ROOT file has already been written and closed (see
  # documentation/implementation.tex). The "|| rc=$?" form is required so
  # that `set -e` does not abort the script before this exit code is
  # inspected.
  local rc=0
  "$@" || rc=$?
  if [[ ${rc} -ne 0 && ${rc} -ne 139 ]]; then
    echo "Simulation failed with unexpected exit code ${rc}" >&2
    exit "${rc}"
  fi
}

for config in "${particle_configs[@]}"; do
  IFS='|' read -r safe_name g4_particle label min_mev max_mev n_events threads <<< "${config}"

  point_dirs=()
  for ((i = 0; i < points; i++)); do
    if [[ ${points} -eq 1 ]]; then
      fraction="0"
    else
      fraction=$(awk -v i="${i}" -v n="${points}" 'BEGIN { print i / (n - 1) }')
    fi
    energy_mev=$(awk -v lo="${min_mev}" -v hi="${max_mev}" -v f="${fraction}" \
      'BEGIN { print lo * exp(f * log(hi / lo)) }')

    point_dir="${scan_output}/${safe_name}/point_$(printf '%02d' "${i}")"
    mkdir -p "${point_dir}"

    macro="${point_dir}/run.mac"
    cat > "${macro}" <<EOF
/run/numberOfThreads ${threads}
/run/initialize
/ubt/gun/randomizeEnergy false
/ubt/gun/randomizeMuonPosition true
/gun/particle ${g4_particle}
/gun/energy ${energy_mev} MeV
/gun/position 0 0 -30 mm
/gun/direction 0 0 1
/run/beamOn ${n_events}
EOF

    echo "=== ${label}: point $((i + 1))/${points}, E = ${energy_mev} MeV ==="
    ( cd "${point_dir}" && run_point env \
        UBT_ENABLE_SCINTILLATOR_PHOTON_STUDIES=false \
        "${executable}" run.mac )

    root_file="${point_dir}/scintillator_digi.root"
    echo "${g4_particle},${label},${energy_mev},${root_file}" >> "${manifest}"
  done
done

echo "=== Aggregating energy-scan results ==="
root -l -b -q \
  "${script_dir}/plot_adc_vs_particle_energy.C(\"${manifest}\",\"${analysis_output}\")"

echo "Energy scan completed: ${analysis_output}"
