# MPDATA Shallow Water #

This program computes the shallow water equations on a sphere,
using the edge-based unstructured mesh discretisation with the
MPDATA algorithm.

The discretisation is based on the article
"An edge-based unstructured mesh discretisation in geospherical framework",
Szmelter, Smolarkiewicz, JCP (2010)

The shallow water equations are initialised with the Rossby-Haurwitz initial
condition, and simulated for 15 days

## Notes ##
The main program can be found in src/shallow_water.F90. 
The actual solver routines can be found in the src/shallow_water_module.F90

All other files, especially in the src/datastructure, and src/io directory are
really the underlying data-structure, which is under development and 
can change at any moment.
It is not necessary to understand the underlying data-structure to follow
the workings of the solver.


## Installation ##
- Copy the meshvol.d file generated from Joanna Szmelter's DualMesh.f90 program,
  provided elsewhere, to the data directory. Note: this is the file generated by
  a early version without the experimental partitioning algorithm.
- Make sure you have cmake installed. On our Cray: ```module load cmake```

```
mkdir build
cd build
cmake .. -DCMAKE_Fortran_COMPILER=mpif90
make -j3
```
- Optionally, also pass ```-DGRIB_API_DIR=path/to/grib``` to the cmake command, to enable grib output. In order to run with this option, a gribfield must be copied in the data directory
- To run:

```
mpirun -np 4 shallow_water
```

On our Cray cluster it is done slightly differently:

```
cmake .. -DCMAKE_Fortran_COMPILER=ftn -DGRIB_API_DIR=/lus/scratch/ecmwf/esm/grib_api/1.11.0/cray/82
make -j8
aprun -n 24 shallow_water
```


## Inspecting results ##
In the data directory you will find various output files:

- results.d, giving the results for depth, and wind velocity at the end of 15 days, in the same format of Joanna Szmelter's coding.
- fields00.msh ... fields15.msh : Gmsh field files containing depth and momentum for every day
- depth00.grib ... depth15.grib: Grib field files containing the depth for every day

#### Gmsh Visualisation ###
- Download the free open source software Gmsh (http://geuz.org/gmsh/)
- Convert the mesh.d file, generated by Joanna Szmelter's PrimaryGrid.f90 program to mesh_latlon.msh

``` 
python    primary_mesh_to_gmsh.py   path/to/mesh.d    data/rtable_lin_T255.d
```

- View the output in Gmsh:

```
gmsh mesh_latlon.msh data/fields*.msh &
```

- Right-click on any field in the Gmsh viewer, and select "Combine Time Steps > By View Name"
