$fs=0.5;

include <well.scad>         // hand-written well-building module

include <outlines.scad>     // path-returning function for each monster

include <tray.scad>         // coordinates for each monster

                        // polygon-building function for each monster,
                        // each with rotation from tray.tex

                         // one module per try, building wells only

module thumb1() {
  x = 250 - 9;
  y = 55.5;
    translate([250-9,55.5-9.5,caddy_height+epsilon])
    thumb(x, y, caddy_height-5);
}

module tray1() {
  translate([-tray1_ll.x,-tray1_ll.y,0])
  difference() {
    translate([tray1_ll.x,tray1_ll.y,0])
      union() {
        tray_block(tray1_dimens, "left");
        ears(tray1_dimens);
//        color("blue")
        translate([250, 53.5, caddy_height - epsilon])
          stop(10);
//        color("red")
        translate([250, 53.5-10-2*5.5, caddy_height - epsilon])
          stop(10);
      }
    tray1_outlines();
    fingers1();
    thumb1();
  }
}

sample1ll = [152,-2];
sample1ur = [220,42];
sample1dimens=[sample1ur.x-sample1ll.x,sample1ur.y-sample1ll.y];

module sample1() {
  // thickness = 6; // 11 minutes, 4.5g
  // thickness = 3; //  8 minutes, 3.1g
  thickness = 4;    //  9 minutes, 3.6g
  translate([sample1ll.x,sample1ll.y,caddy_height+epsilon-thickness])
  cube([sample1dimens.x, sample1dimens.y, thickness]);
}

module tray1_slice() {
  //thickness = 4;    //  1:34, 38.4g
  //thickness = 3;    //  1:23, 33.6g
  thickness = 4;
  translate([-10,-10,caddy_height+epsilon-thickness])
  cube([300, 300, thickness]);
}

module tray1_test() {
  translate([0,0,4-caddy_height])
  intersection() {
    tray1();
  //  sample1();
    tray1_slice();
  }
  ears(tray1_dimens);
}



tray1();


