------------- Make It Stand: Balancing Shapes for 3D Fabrication -------------

------------------------------------------------------------------------------
(c) ETH-Zurick - INRIA - All rights reserved
------------------------------------------------------------------------------
This source code is distributed for research purposes only.
Commercial use is not allowed ; please contact us for licensing.
------------------------------------------------------------------------------

Romain Prévost (Interactive Geometry Lab, ETHZ Zürich)
Emily Whiting (Interactive Geometry Lab, ETHZ Zürich)
Sylvain Lefebvre (INRIA Nancy)
Olga Sorkine-Hornung (Interactive Geometry Lab, ETHZ Zürich)

ACM SIGGRAPH 2013

Website: http://igl.ethz.ch/projects/make-it-stand/

Email: rprevost@inf.ethz.ch

Caution: This is a research prototype; I have simplified a lot of things, 
there are probably bugs. But if you follow the instructions it should 
work nicely.

------------------------------------------------------------------------------

This archive contains the source code of our optimization software. This is a 
beta version without many comments. However the code is well-structured and 
names of variables, classes and functions are usually quite explicit.
It contains the Visual Studio 2010 solution and compile for Windows 32bits.
Also some examples are included with precomputed weights.

Requirements:
- Recent Nvidia graphics card (series 4 should be enough) with the driver up
  to date (current version 314) [for the voxelizer and transparency libraries]
- Mosek 7: http://mosek.com/resources/download/ (this is currently the last
  version but they do not ensure compatibility between versions, so you need
  to specifically use this one) [for the weights computation]
- CGAL: http://www.cgal.org/ [for the support polygon related computation]

Other dependencies included:
- Sylvain Lefebvre's library (GPU voxelizer and transparency rendering)
  based on his recent work: http://hal.inria.fr/hal-00811585/
- Embree 2.0: http://embree.github.io/
- Eigen 3.*: http://eigen.tuxfamily.org/
- AntTweakBar 1.16: http://anttweakbar.sourceforge.net/
- OpenGL, GLUT, ...

Basic Instructions:
- Put the model you want to balance in the folder "data/meshes/" (OBJ file)
- Launch the program
- Select the "mesh file" and click "Open/Create"
  [you should see your model]
- Choose the desired "Mesh rotation"
  [you should see the support polygon in orange]
- Place the some handles:
  - Press Alt+e
    [you should see a yellow dot as you move the mouse above the mesh]
  - Right click to add a new handle or removing an existing one
  - Left click and drag to move an existing handle
- Choose the "Voxel grid resolution" and click "Voxelize"
  [this will take a few minutes to compute the bounded biharmonic weights]
  [in the console you can see Mosek working]
  [when it is finished you see your model with the center of mass as a black
  dot and its projection on the ground in red, yellow or green depending on 
  the balance state]
- Choose a "Balancing algorithm":
  - Plane carving is the heuristic described in our paper
  - Full means no carving
  - Empty means everything is carved
  - You can also choose a different "Hull voxel thickness"
  [you should see the inner surface in yellow]
- Start/Pause the optimization by press key Space
- Choose a config name "project1.mis" and click "Save" to export everything 
  you have done (voxelization, BBW, inner/outer surfaces). You can find your
  final results in the folder "data/prints/"


Additional tips:
- Press F2 to enable/disable shadow rendering
- Press F3 to enable/disable transparency rendering
- Press Alt+t to enter translation mode and drag a handle with left mouse click
- Press Alt+z to enter scaling mode and drag a handle with left mouse click
- Press r to reset the mesh to its initial shape

------------------------------------------------------------------------------