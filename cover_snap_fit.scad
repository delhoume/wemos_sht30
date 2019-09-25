sizeX = 84;  
sizeY = 39;  

height = 1.5;

border = 2; // some space for fitting
border2 = 2 * border;
offset = .2;
offset2 = 2 * offset;


snaphandlewidth = 20;
snaphandledepth = 1.5;
snaphandleheight = 3;

snapsize = 1.5;
snapwidth = snaphandlewidth - 5;
snapheight = snaphandleheight;

holewidth = 20;
holeheight = 15;
holeypos = 15;

cutoutholexstart = border2;
cutoutholex = sizeX - border2 * 2;
cutoutholeystart = border + offset + snaphandledepth;
cutoutholey = sizeY - border2 - offset2 - snaphandledepth * 2;


difference() {
    union() {
        cube(size=[sizeX, sizeY, height], center = false);
        

           translate([border + offset, border + offset, height])
            cube(size=[sizeX - (border2 + offset2), sizeY - border2, height / 2], 
                center = false);
        
        // snap1
        translate([(sizeX - snaphandlewidth) / 2, border + offset, 1.5 * height]) 
           cube(size=[snaphandlewidth, snaphandledepth, snaphandleheight], center = false);
        
       translate([(sizeX - snapwidth) / 2, border + offset, snapheight - offset]) 
         rotate([45, 0, 0])
          cube(size=[snapwidth, snapsize, snapsize], center = false);
 
         // snap2
        translate([(sizeX - snaphandlewidth) / 2, sizeY - border2, 1.5 * height]) 
           cube(size=[snaphandlewidth, snaphandledepth, snaphandleheight], center = false);
        
       translate([(sizeX - snapwidth) / 2, sizeY - border - offset2, snapheight - offset]) 
         rotate([45, 0, 0])
          cube(size=[snapwidth, snapsize, snapsize], center = false);
       
    }
    {
        translate([(sizeX - holewidth) / 2, holeypos, -1])
            cube(size=[holewidth, holeheight, height + 5], center = false); 
        
       // translate([cutoutholexstart, cutoutholeystart, height - offset])
        //    cube(size=[cutoutholex, cutoutholey, height + 5], center = false); 
  
    }
   
}