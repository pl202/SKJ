# Indian Ocean skipjack tuna model

[![Travis CI build](https://travis-ci.org/iotcwpm/SKJ.svg)](https://travis-ci.org/iotcwpm/SKJ)
[![GitHub issues](https://img.shields.io/github/issues/iotcwpm/SKJ.svg)](https://github.com/iotcwpm/SKJ/issues)

The Indian Ocean Tuna Commission's (IOTC) Working Party on Methods' (WPM) simulation model of the Indian Ocean skipjack (SKJ) tuna fishery for the evaluation of management procedures (MP).

> ![](other/images/skipjack.jpg)

> Skipjack tuna (_Katsuwonus pelamis_)

## Documentation

The following document the model structure and code,

- [model/description](http://iotcwpm.github.io/SKJ/model/description) : a description of the model, including equations
- [parameters/description](http://iotcwpm.github.io/SKJ/parameters/description) : a description of the parameter priors and how they were derived
- [procedures/description](http://iotcwpm.github.io/SKJ/procedures/description) : a description of the candidate management procedures
- [doxygen](http://iotcwpm.github.io/SKJ/doxygen) : C++ code documentation generated by Doxygen

## Reports

Several reports associated with this project are available (in reverse chronological order):

- [Bentley, N and M.S. Adam  (2015). An operating model for the Indian Ocean skipjack tuna fishery. IOTC-2015-WPTT17-35. Paper submitted to the 17th Meeting of the Working Party on Tropical Tunas, Montpellier, France, 23-28 October 2015](http://www.iotc.org/sites/default/files/documents/2015/10/IOTC-2015-WPTT17-35_-_SKJ_MSE.pdf)

- [Bentley, N and M.S. Adam  (2015). An operating model for the Indian Ocean skipjack tuna fishery. IOTC-2015-WPM06-09. Paper submitted to the 6th Meeting of the Working Party on Methods, Montpellier, France, 19-21 October 2015](http://www.iotc.org/sites/default/files/documents/2015/10/IOTC-2015-WPM06-09_-_SKJ_OM.pdf)

- [Bentley, N and M.S. Adam  (2014). Management procedure evaluation for Indian Ocean skipjack tuna fishery: model description and conditioning. IOTC-2014-WPM05-08. Paper submitted to the 5th Meeting of the Working Party on Methods, Seychelles, 05-06 December 2014](http://www.iotc.org/documents/management-procedure-evaluation-indian-ocean-skipjack-tuna-fishery-simulation-model)

- [Bentley, N and M.S. Adam  (2014). Management strategy evaluation for Indian Ocean skipjack tuna fishery: first steps. IOTC-2014-WPTT16-39. Paper submitted to the 16th Meeting of the Working Party on Tropical Tuna, Bali, Indonesia, 15-19 November 2014](http://www.iotc.org/documents/management-strategy-evaluation-indian-ocean-skipjack-tuna-first-steps)

- [ISSF (2014). ISSF Report of the 2014 Meeting of the Indian Ocean Skipjack MSE Advisory Committee Meeting. 14-16 October 2014, Media One Hotel, Dubai, United Arab Emirates. 13 pp.](http://iss-foundation.org/resources/downloads/?did=548)

- [Adam, M. S., R. Sharma and N. Bentley (2013). Progress and arrangements for management strategy evaluation work of Indian Ocean skipjack tuna. IOTC-2013-WPTT15-12, Paper submitted to Working Party on Tropical Tuna, San Sebastian, Spain.](http://www.iotc.org/documents/process-and-arrangement-management-strategy-evaluation-indian-ocean-skipjack-tuna )


## Support

This work has been funded by the [International Pole and Line Foundation (IPNLF)](http://ipnlf.org/), the Maldives Seafood Processors and Exporters Association (MSPEA), the [Areas Beyond National Jurisdiction (ABNJ aka Common Oceans) Program](http://www.commonoceans.org/) and [WWF](http://wwf.panda.org) with support and oversignt of the Maldives [Marine Research Centre (MRC)](http://www.mrc.gov.mv/). Support for the project's Advisory Committee has been provided by the [International Seafood Sustainability Foundation (ISSF)](http://iss-foundation.org/).

<div align="center">
	<img src="other/images/ipnlf.png" style="max-width:125px">
	<img src="other/images/mspea.png" style="max-width:125px">
	<img src="other/images/common-oceans.png" style="max-width:125px">
	<img src="other/images/mrc.png" style="max-width:125px">
	<img src="other/images/wwf.png" style="max-width:125px">
	<img src="other/images/issf.png" style="max-width:125px">
</div>

## Status

The model is still under active development. As the model matures we hope to make it available as precompiled executables for Windows and Linux and/or a package for R.

Automated builds are done on [Travis CI](https://travis-ci.org/iotcwpm/SKJ) when changes are pushed to this repository.


## Usage

To run the executable, `ioskj.exe`, supply a task name and, for certain tasks, additional arguments,

```
./ioskj.exe <task> [<arg1>],[<arg2>]...
```

Tasks include,

- `run`
- `priors`
- `feasible <trials>`
- `evaluate <replicates>`

For example, to evaluate the defined set of management procedures using 1000 replicates run,

```
./ioskj.exe evaluate 1000
```

## Building

The project `Makefile` includes a task (`make requires`) which will download and compile required C++ libraries. Use `make compile` to compile a production version of the executable.

On most Linux systems you should have the required build tools installed already and can just run:

```shell
make requires compile
```

On Windows, the recommended approach is to build using MSYS2 and MinGW64:

 1. Download and install MYSYS2 following the instructions at https://msys2.github.io/

 2. Open the MYSY2 shell and install build tools:

	```shell
	pacman -S make mingw-w64-x86_64-toolchain zip
	```

 3. Open the MinGW64 shell and run:

	```shell
	make requires compile
	```

To build a debug version of the executable for use with [GDB](http://www.gnu.org/software/gdb/) run,

```
make debug
```

To update all documentation run,

```
make docs
```

To publish documentation to http://iotcwpm.github.io/SKJ run,

```
make publish
```

## Organisation

The C++ code is divided into several files:

- `imports.hpp` - `#include`s to import classes and functions from external libraries
- `dimensions.hpp` - defines the dimensions used in various model arrays e.g. `Region`, `Age`, `Method`
- `model.hpp` - contains the `Model` class which specifies the structure and dynamics of the fishery
- `parameters.hpp` - contains the `Parameters` class which defines the parameters and their priors used for model conditioning
- `data.hpp` - contains the `Data` class which defines the data sets and their likelihoods used for model conditioning
- `ioskj.cpp` - the primary C++ file for compiling the executable
- `tests.cpp` - unit tests of the C++ code

Some of these files have corresponding folders,

- `parameters` : R scripts for background analyses for developing model prior probability distributions.
- `data` : R and Python scripts for processing source data. See the documentation in those files for more details.

In addition there is a folder related to documentation,

- `doxygen` : a Doxygen project for automatically generating documentation from C++ source code.
