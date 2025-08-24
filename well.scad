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

  
