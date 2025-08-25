include <well.scad>         // hand-written well-building module

include <outlines.scad>     // path-returning function for each monster

include <tray.scad>         // coordinates for each monster

                        // polygon-building function for each monster,
                        // each with rotation from tray.tex

                         // one module per try, building wells only

difference() {
  tray_block(tray1_ll, tray1_dimens, "left");
  tray1_outlines();
}

