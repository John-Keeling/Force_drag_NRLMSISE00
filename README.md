# Force_drag_NRLMSISE00

This is the implementation file of a new class written for the UCL SGNL Orbit Prediction Software (OPS) in 2021. The purpose of the class is to prepare the requisite inputs for, and then run, the NRLMSISE-00 atmosphere model, in order that a value for thermospheric mass density can be returned. This value is used in the calculation of atmospheric drag on a satellite whose characteristics are specified by the user at the outset. The programme numerically integrates along each step in the satellite's orbit, with mass density being calculated at each, based on spatial and temporal coordinates. The details of the satellite track is recorded in an ephemeris output file.

This code is an extract only, published here as an example of work completed as part of the master's project. The file will not compile or run in isolation from the numerous other OPS source files. 
