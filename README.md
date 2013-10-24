# `ioskj`

`ioskj` is a simulation model of the Indian Ocean skipjack tuna fishery for management strategy evaluation.

## Status

`ioskj` is still under active development. It requires several third party C++ libraries and a modern C++ compiler which supports the C++11 standard. At this stage we do not recommend trying to compile it yourself. As the model matures we hope to make it available as precompiled executables for Windows and Linux and/or a package for R.

## Documentation

Documentation is at http://trophia.github.io/ioskj/

## Organisation

### C++ files

The main C++ files are:

- `dimensions.hpp` - defines the dimensions used in various model arrays e.g. `Region`, `Age`, `Method`
- `model.hpp` - contains the `Model` class which specifies the model parameters and dynamics
- `data.hpp` - reads in and holds data for use in driving and conditioning the model
- `main.cpp` - the primary C++ file for compiling a `Model` executable

### `priors` folder

The `data` folder includes R scripts of background analyses for developing model prior probability distributions.

### `data` folder

The `data` folder includes R and Python scripts for processing source data. See the documentation in those files for more details. The resulting, processed data is outputted to the folder `data\processed-data`.

### `docs` folder

The `docs` folder includes hand written documentation and a Doxygen project for automatically generating documentation from C++ source code/
