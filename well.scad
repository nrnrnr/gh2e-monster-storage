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
    
  
