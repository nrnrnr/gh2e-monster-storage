include <well.scad>         // hand-written well-building module

include <outlines.scad>     // path-returning function for each monster

include <tray.scad>         // coordinates for each monster

                        // polygon-building function for each monster,
                        // each with rotation from tray.tex

                         // one module per try, building wells only

module tray1() {
  translate([-tray1_ll.x,-tray1_ll.y,0])
  difference() {
    translate([tray1_ll.x,tray1_ll.y,0])
      union() {
        tray_block(tray1_dimens, "left");
        ears(tray1_dimens);
      }
    tray1_outlines();
    fingers1();
  }
}

sample1ll = [152,-2];
sample1ur = [220,42];
sample1dimens=[sample1ur.x-sample1ll.x,sample1ur.y-sample1ll.y];

module sample1() {
  thickness = 6;
  translate([sample1ll.x,sample1ll.y,caddy_height+epsilon-thickness])
  cube([sample1dimens.x, sample1dimens.y, thickness]);
}

intersection() {
  tray1();
  sample1();
}

