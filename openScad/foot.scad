picotR = 2.0;

footHeight = 5;

y1 = -4;
y2 = 4;
yy1 = -10;
yy2 = 10;
x1 = 0;
x2 = 20;
z1 = -footHeight / 2;
z2 = -z1;

cubex = 100;
cubey = 50;
cubez = 20;

// TODO: compute
angle = 45;

offset = 5;

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
difference() {
    union() {
        polyhedron( CubePoints, CubeFaces );

        cylinder(h = footHeight, r = 6, $fn = 50, center = true);
    }
    {
        cylinder(h = footHeight + 1, r = picotR, $fn = 50, center = true);
    }
}
}

foot();
/*
cube(size = [cubex, cubey, cubez], center = true);

translate([-((cubex / 2) + offset), -((cubey / 2) + offset), 0]) rotate([0, 0, angle]) foot();
translate([-((cubex / 2) + offset), ((cubey / 2) + offset), 0])  rotate([0, 0, -angle]) foot();
translate([((cubex / 2) + offset), -((cubey / 2) + offset), 0])  rotate([0, 0, 180 - angle]) foot();
translate([((cubex / 2) + offset), ((cubey / 2) + offset), 0])  rotate([0, 0, 180 + angle]) foot();
*/