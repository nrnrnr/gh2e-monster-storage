standee_thickness = 13.0 / 6;  
headroom = 0.5;
floor_thickness = 1.2; // perhaps could be thinner?

caddy_height = 6 * standee_thickness + floor_thickness + headroom;

epsilon = 0.001;

module well (n=count) { // store n standees
  depth = n * standee_thickness + headroom;
  
  translate([0,0,caddy_height + epsilon - depth])
    linear_extrude(h = depth)
    children();
}


//  Usage:
//  
//  translate([Inox_Guard.x, Inox_Guard.y, 0]) // place in tray
//    well(6) // convert to 3D shape of appropriate Z and height
//      rotate([0,0,small_angle]) // omit if small angle is zero
//         Inox_Guard(); // outline of the shape

module rail(length) {
    translate([0,length,0])
    rotate([90,0,0])
    linear_extrude(length) {
    polygon([[0,0], [0,3], [5,3], [2,0]]);
  }
}

module tray_block(ll, dimens, stop="left") {
  translate([ll.x, ll.y, 0])
    union() {
      cube([dimens.x, dimens.y, caddy_height]);
      translate([0, dimens.y, caddy_height - epsilon])
        rotate([0,0,-90])
        rail(dimens.x);
      translate([dimens.x, 0, caddy_height - epsilon])
        rotate([0,0,90])
        rail(dimens.x);
      if (stop == "left") {
        translate([0,0,caddy_height - epsilon])
          cube ([1, dimens.y, 3 + epsilon]);
      } else {
        translate([dimens.x,0,caddy_height - epsilon])
          cube ([1, dimens.y, 3 + epsilon]);
      }
  }
}
            

  
