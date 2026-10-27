#!/bin/bash
# =============================================================================
# make_figures.sh - renders the Part 2 figures of the report (Section 4.9)
#   ./make_figures.sh              -> writes into ../report/images
#   ./make_figures.sh <folder>     -> writes into <folder>
# Each line opens the application for a few frames, saves one PNG and quits.
# fig_p2_edit_h2s.png is made by hand (see the report, Section 4.4.5).
# =============================================================================
set -e
cd "$(dirname "$0")"
OUT="${1:-../report/images}"
mkdir -p "$OUT"
make -s -j4 > /dev/null

shot() {   # shot <file.png> <options...>
    local file="$1"; shift
    ./build/molecules "$@" --screenshot "$OUT/$file" > /dev/null
    echo "  $OUT/$file"
}

# ---- interface (whole window, GUI included)
shot fig_p2_gui_bohr.png   --element C --with-gui
shot fig_p2_gui_mol.png    --molecule H2O --with-gui --editor --no-vibrate

# ---- Bohr models with 1 to 4 shells
for e in H C Ne Na Ca Cu; do
    shot fig_p2_bohr_$e.png --element $e
done

# ---- electron motion: sodium at t = 0, 0.5 and 1 s
shot fig_p2_orbit_t0.png   --element Na
shot fig_p2_orbit_t1.png   --element Na --time 0.5
shot fig_p2_orbit_t2.png   --element Na --time 1.0

# ---- the eight molecules (equilibrium geometry)
for m in H2O CO2 NH3 CH4 O2 N2 HCl NaCl; do
    shot fig_p2_mol_$m.png --molecule $m --no-vibrate
done

# ---- vibration, A = 0.2, at 1/4 and 3/4 of a period
#      CO2: f = 1.333 Hz (T = 0.7502 s)   HCl: f = 2.886 Hz (T = 0.3465 s)
shot fig_p2_vib_co2_max.png --molecule CO2 --amplitude 0.2 --time 0.1876
shot fig_p2_vib_co2_min.png --molecule CO2 --amplitude 0.2 --time 0.5627
shot fig_p2_vib_hcl_max.png --molecule HCl --amplitude 0.2 --time 0.0866
shot fig_p2_vib_hcl_min.png --molecule HCl --amplitude 0.2 --time 0.2599

# ---- rotation of methane (30 deg/s) at t = 0, 1 and 2 s
shot fig_p2_rot_0.png      --molecule CH4 --no-vibrate --rotate
shot fig_p2_rot_1.png      --molecule CH4 --no-vibrate --rotate --time 1
shot fig_p2_rot_2.png      --molecule CH4 --no-vibrate --rotate --time 2

# ---- camera control on ammonia
shot fig_p2_cam_front.png  --molecule NH3 --no-vibrate
shot fig_p2_cam_side.png   --molecule NH3 --no-vibrate --yaw 0 --pitch 0
shot fig_p2_cam_top.png    --molecule NH3 --no-vibrate --pitch 89
shot fig_p2_cam_zoom.png   --molecule NH3 --no-vibrate --distance 3.5

# ---- wireframe and light position
shot fig_p2_wire_bohr.png  --element Na --wireframe
shot fig_p2_wire_mol.png   --molecule CH4 --no-vibrate --wireframe
shot fig_p2_light_a.png    --molecule CH4 --no-vibrate
shot fig_p2_light_b.png    --molecule CH4 --no-vibrate --light -10,-2,4

echo "Done. Still to do by hand: fig_p2_edit_h2s.png (water edited into H2S, Shift+F12)."
