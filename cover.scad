use <invaders_from_space.ttf>;
use <barrel.ttf>;

sizeX = 83;  
sizeY = 39;  

//linear_extrude(height = 2)

height = 1.8;
picotHeight = 6;
picotR = 1.8;

letter_height = 4.0;
letter_size = 6;

fontInvader = "invaders from space";
fontBarrel = "Barrel";

module letter(l, font) {
	// Use linear_extrude() to make the letters 3D objects as they
	// are only 2D shapes when only using text()
	linear_extrude(height = letter_height) {
		text(l, size = letter_size, font = font, halign = "center", 
             valign = "center", $fn = 32);
	}
}

difference() {
    union() {
        cube(size=[sizeX, sizeY, height], center = true);
        translate([sizeX / 2 + 2, 0, 0])
                cylinder(h = height, r = 6, $fn = 40, center = true);
         translate([-(sizeX / 2 + 2), 0, 0])
                cylinder(h = height, r = 6, $fn = 40, center = true);
        translate([sizeX / 2, 0, 0])
            cube(size = [6, 12, height], center = true);
        translate([-sizeX / 2, 0, 0])
            cube(size = [6, 12, height], center = true);
        
        translate([sizeX / 2 + 2, 0, 0])
          cylinder(h = picotHeight, r = picotR, $fn = 40, center = false);
        
        translate([-(sizeX / 2 + 2), 0, 0])
          cylinder(h = picotHeight, r = picotR, $fn = 40, center = false);
        

    }
    {
        translate([0, 3, 0])
       cube(size=[20, 15, height + 1], center = true); 


/*
       rotate([0, 180, 180])
           translate([0, 12, -1])
               letter("(c) 2019 - fred", fontBarrel);
 */
     /*   
       translate([sizeX / 2 + 4, 0, 0])
                circle(2, $fn = 40);
         translate([-(sizeX / 2 + 4), 0, 0])
                circle(2, $fn = 40);
        */
    }
   
}