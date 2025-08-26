standee_thickness = 13.0 / 6;  
headroom = 0.5;
floor_thickness = 0.8; // four layers, default floor
layer_height = 0.2;

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

module tray_block(dimens, stop="left") {
  cube([dimens.x, dimens.y, caddy_height]);
  translate([0, dimens.y, caddy_height - epsilon]) // top rail
    rotate([0,0,-90])
    rail(dimens.x);
  translate([dimens.x, 0, caddy_height - epsilon]) // bottom rail
    rotate([0,0,90])
    rail(dimens.x);
  if (stop == "left") {
    translate([0,0,caddy_height - epsilon])
      cube ([2, dimens.y, 3 + epsilon]);
  } else {
    translate([dimens.x,0,caddy_height - epsilon])
      cube ([2, dimens.y, 3 + epsilon]);
  }
}
            

  
module finger(x, y, radius=9.5) { // finger hole below monster stack
  translate([x, y, -epsilon])
    union () {
      cylinder(r=radius, h= caddy_height+ 2*epsilon);
      cylinder(h=3*layer_height, r1=radius+2*layer_height, r2=radius);
  }
}


module thumb(x, y, depth, radius=5.5) { // thumb hole for opening glass
  // bottom of opening is at -depth in z
  translate([0,0,radius - depth])
    union() {
//color("blue", 0.2)
      sphere(r=radius);
      rotate([0,90,0])
        cylinder(r=radius, h=250); // points toward plus x
      if (depth > radius) {
//color("green", 0.7)
        cylinder(r=radius, h = depth - radius + epsilon);
//color("red", 0.2)
    translate([0,-radius,0])
      cube([250, 2 * radius, depth - radius + epsilon]);
    }
  }
}

ear_radius = 15;

module ear() { // sits in the -y plane
  difference() {
    cylinder(r=ear_radius,h=layer_height);
    translate([-2*ear_radius,0,-1])
      cube([4*ear_radius, 4*ear_radius, layer_height+2]);
  }
}

module ears(dimens) {
  translate([ear_radius, 0, 0]) ear();
  translate([dimens.x - ear_radius, 0, 0]) ear();
  translate([ear_radius, dimens.y, 0]) rotate([0,0,180]) ear();
  translate([dimens.x - ear_radius, dimens.y, 0]) rotate([0,0,180]) ear();
}


module stop(length) { // low spot of one end at origin
                      // higher end extending to negative x
                      // length out to positive y
  translate([0,length,0])
  rotate([90,0,0])
    linear_extrude(length)
    polygon([[0,0],[-3,0],[-3,0.8]]);
}

