#!/usr/bin/env bash
set -euo pipefail

script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
project_dir=$(cd -- "${script_dir}/.." && pwd)

root_output=${UBT_ROOT_OUTPUT:-"${project_dir}/scintillator_digi.root"}
analysis_output=${UBT_ANALYSIS_OUTPUT:-"${project_dir}/analysis/plots"}

if [[ ! -f "${root_output}" ]]; then
  echo "Input ROOT file not found: ${root_output}" >&2
  echo "Set UBT_ROOT_OUTPUT=/path/to/scintillator_digi.root if needed." >&2
  exit 1
fi

mkdir -p "${analysis_output}"
cd "${project_dir}"

root -l -b -q \
  "${script_dir}/plot_event_observables.C(\"${root_output}\",\"${analysis_output}\")"
root -l -b -q \
  "${script_dir}/study_adc_vs_edep_position.C(\"${root_output}\",\"${analysis_output}/adc\")"
root -l -b -q \
  "${script_dir}/plot_adc_per_edep_vs_position.C(\"${root_output}\",\"${analysis_output}/adc\")"

echo "UBT analysis completed: ${analysis_output}"
