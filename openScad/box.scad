sizeX = 84;  
sizeY = 39;  

border = 2;
border2 = 2 * border;

height = 13;

picotHeight = 10;
picotStartZ = (height / 2) - 4;

picotR = 2.0;
footHole = 1.5;

offsetCutOutCenterX = (sizeX - border2) / 6 + 2.5; // not evenly centered usb port
offsetCutOutCenterZ = border + 5;

footHeight = 2.5;

y1 = -3;
y2 = 3;
yy1 = -14.2;
yy2 = 14.2;
x1 = 0;
x2 = 20;
z1 = -footHeight / 2;
z2 = -z1;

angle = 45;
offset = 3;
offsetZ = -(height - footHeight) / 2;

useFeet = true;
useFeetHoles = false;
// will cause problems when 3d printing (overhang...)
useFeetRubberHoles = false;

footRubberHoleHeight = 2;
footRubberHoleRadius = 5;

useSecondUsb = false;

CubePoints = [
  [  x1,  y1,  z1 ],  //0
  [ x2,  yy1,  z1 ],  //1
  [ x2,  yy2,  z1 ],  //2
  [  x1,  y2,  z1 ],  //3
  [  x1,  y1,  z2 ],  //4
  [ x2,  yy1,  z2 ],  //5
  [ x2,  yy2,  z2 ],  //6
  [  x1,  y2,  z2 ]]; //7
  
CubeFaces = [
  [0,1,2,3],  // bottom
  [4,5,1,0],  // front
  [7,6,5,4],  // top
  [5,6,2,1],  // right
  [6,7,3,2],  // back
  [7,4,0,3]]; // left

module foot() {  
    color("lightgreen")
    difference() {
        union() {
            polyhedron( CubePoints, CubeFaces );
            cylinder(h = footHeight, r = 3.0, $fn = 40, center = true);
        }
        {
            if (useFeetHoles)
                cylinder(h = footHeight + 2, r = footHole, $fn = 40, center = true);
            if (useFeetRubberHoles)
                translate([0, 0, -2])
                cylinder(h = footRubberHoleHeight, 
                         r = footRubberHoleRadius, 
                         $fn = 40, 
                         center = true);
        }
    }
}

difference() {
    union() {
     color("lightblue") {
       // box
        cube(size=[sizeX, sizeY, height], center = true);
        
        translate([sizeX / 2 + 2, 0, 0])
                cylinder(h = height, r1 = 3, r2 = 6, $fn = 40, center = true);
         translate([-(sizeX / 2 + 2), 0, 0])
                cylinder(h = height, r1 = 3, r2 = 6, $fn = 40, center = true);
     }
  //      translate([sizeX / 2, 0, 0])
  //          cube(size = [6, 12, height], center = true);
  //      translate([-sizeX / 2, 0, 0])
  //          cube(size = [6, 12, height], center = true);
        
    // feet
    if (useFeet) { 
        translate([-((sizeX / 2) + offset), -((sizeY / 2) + offset), offsetZ]) 
            rotate([0, 0, angle]) foot();
        translate([-((sizeX / 2) + offset), ((sizeY / 2) + offset), offsetZ])  
            rotate([0, 0, -angle]) foot();
        translate([((sizeX / 2) + offset), -((sizeY / 2) + offset), offsetZ])  
            rotate([0, 0, 180 - angle]) foot();
        translate([((sizeX / 2) + offset), ((sizeY / 2) + offset), offsetZ])  
            rotate([0, 0, 180 + angle]) foot();
    }  
    }
    {
        // inside
        translate([0, 0, border])
        cube(size=[sizeX - border2, sizeY - border2, height], center = true); 
        
        // hole 1
        #translate([sizeX / 2 + 2, 0, picotStartZ])
          cylinder(h = picotHeight, r = picotR, $fn = 40, center = true);
        
        // hole 2
        #translate([-(sizeX / 2 + 2), 0, picotStartZ])
          cylinder(h = picotHeight, r = picotR, $fn = 40, center = true);
        
        // usb cutout
        translate([(sizeX / 2) - offsetCutOutCenterX, (sizeY / 2), offsetCutOutCenterZ])
          cube(size=[12, 10, 10], center = true);
        
        // reset button
        translate([(sizeX / 2), (sizeY / 2) - 6, offsetCutOutCenterZ])
          cube(size=[5, 5, 10], center = true);
        
        if (useSecondUsb) {
            // usb cutout 2
            #translate([-(sizeX / 2) + offsetCutOutCenterX, -(sizeY / 2), offsetCutOutCenterZ])
                cube(size=[10, 10, 10], center = true);
        
        // reset button 2
        #translate([-(sizeX / 2), -(sizeY / 2) + 6, offsetCutOutCenterZ])
          cube(size=[5, 5, 10], center = true);
 
        }

    }
   
}