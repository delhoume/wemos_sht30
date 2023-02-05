//Humidity sensor project box

$fn = 50;

/* Comment or uncomment the following lines to see each piece on its own
 * You can also add a translate() to one line to move the plate next to the
 * cover for viewing both.  I did it this way to export both as separate 
 * .stl files
 */
cover();
//wall_plate();

module wall_plate() {
    //Create the base platform with mounting holes
    difference() {
        union() {
            translate([0, 0, -1]) cube([77, 57, 1]);
            translate([2.25, 2.25, 0]) cube([72.5, 52.5, 2]);
        }
        translate([15, 28.5, -2]) cylinder(d=4, h=5);
        translate([62, 28.5, -2]) cylinder(d=4, h=5);
        
        translate([15, 28.5, 1]) cylinder(d=8, h=2);
        translate([62, 28.5, 1]) cylinder(d=8, h=2);
    }
    
    //Create the snap tab uprights
    difference() {
        translate([30.5, 2.25, 0]) cube([16, 52.5, 10]);
        
        translate([30.5, 5.25, 0]) cube([16, 46.5, 10]);
    }
    
    //Create the latches
    translate([30.5, 2.25, 7.2]) rotate([45, 0, 0]) cube([16, 2, 2]);
    translate([30.5, 54.75, 7.2]) rotate([45, 0, 0]) cube([16, 2, 2]);
}

module cover() {
    //Create the main vented box
    difference() {
        //Create the outer box layers
        union() {
            translate([2, 2, 0]) cube([73, 53, 49]);
            cube([77, 57, 33]);
        }
        //Hollow out the box
        translate([4, 4, 0]) cube([69, 49, 47]);
        translate([2, 2, 0]) cube([73, 53, 28]);
        
        //Create the latch reliefs
        translate([29.5, 2.25, 7.2]) rotate([45, 0, 0]) cube([18, 2, 2]);
        translate([29.5, 54.75, 7.2]) rotate([45, 0, 0]) cube([18, 2, 2]);
        
        translate([29.5, 1.5, 0]) cube([18, 2, 8.2]);
        translate([29.5, 53.5, 0]) cube([18, 2, 8.2]);
        
        //Add the horizontal vent slots to the top layer
        translate([5, 2, 28]) cube([2, 53, 18]);
        translate([10, 2, 28]) cube([2, 53, 18]);
        translate([15, 2, 28]) cube([2, 53, 18]);
        translate([20, 2, 28]) cube([2, 53, 18]);
        translate([25, 2, 28]) cube([2, 53, 18]);
        translate([30, 2, 28]) cube([2, 53, 18]);
        translate([35, 2, 28]) cube([2, 53, 18]);
        translate([40, 2, 28]) cube([2, 53, 18]);
        translate([45, 2, 28]) cube([2, 53, 18]);
        translate([50, 2, 28]) cube([2, 53, 18]);
        translate([55, 2, 28]) cube([2, 53, 18]);
        translate([60, 2, 28]) cube([2, 53, 18]);
        translate([65, 2, 28]) cube([2, 53, 18]);
        translate([70, 2, 28]) cube([2, 53, 18]);
        //Add the vertical vent slots to the top layer
        translate([2, 5, 28]) cube([73, 2, 18]);
        translate([2, 10, 28]) cube([73, 2, 18]);
        translate([2, 15, 28]) cube([73, 2, 18]);
        translate([2, 20, 28]) cube([73, 2, 18]);
        translate([2, 25, 28]) cube([73, 2, 18]);
        translate([2, 30, 28]) cube([73, 2, 18]);
        translate([2, 35, 28]) cube([73, 2, 18]);
        translate([2, 40, 28]) cube([73, 2, 18]);
        translate([2, 45, 28]) cube([73, 2, 18]);
        translate([2, 50, 28]) cube([73, 2, 18]);
        
        //This is a slight relief in the side vents where the radio module sits  
        //to prevent the radio from restin on the side of the vents.  This makes
        //screwing the PCB on easier.
        translate([5, 3, 26]) cube([35, 4, 10]);
    }

    //Create the battery case holder
    difference() {
        translate([2, 7.75, 2]) cube([73, 41.5, 22.5]);
        
        translate([8.75, 7.75, 2]) cube([60, 41.5, 22.5]);
        translate([3.75, 11, 2]) cube([69.25, 34.25, 18.3]);
        //Cut out a slot for the power wires
        translate([2, 15, 0]) cube([10, 25.5, 25]);
        
    }

    //Create the mounting tabs for the PCB
    difference() {    
        translate([15, 2, 23]) cube([52.75, 55, 5]);
        
        translate([19.5, 6.5, 23]) cube([43.75, 44, 5]);
        translate([22, 2, 23]) cube([38.75, 55, 5]);
        translate([14, 9.5, 23]) cube([54.75, 38, 5]);
        
        translate([18.5, 5.5, 23]) cylinder(d=1, h=5);
        translate([64, 5.5, 23]) cylinder(d=1, h=5);
        translate([18.5, 51.5, 23]) cylinder(d=1, h=5);
        translate([64, 51.5, 23]) cylinder(d=1, h=5);
    }
}