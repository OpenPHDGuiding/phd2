Gaussian-Process Based Guiding
==============================

Due to mechanical imprecisions, many telescope mounts show periodic tracking
errors. This work uses the Gaussian process regression framework to predict and
reduce the periodoc error.

Compile Instructions
--------------------

So far, this feature is optional and needs to be activated with CMake to be
compiled. The GP guiding feature is activated by calling `cmake` with the
additional flag `-DGUIDING_GAUSSIAN_PROCESS=true`. The rest of the build process
remains the same.

Files Overview
--------------

The GP guiding project introduced new files to PHD2 guiding. These files are
listed, together with a short description, in the following.

### Files in the Project Root

|File name | Description|
|----------|------------|
|`guide_algorithm_gaussian_process.cpp` | Provides the UI for the GP-based guider.|
|`guide_algorithm_gaussian_process.h` | Header for GP guider UI.|

### Files in `contributions/MPI_IS_gaussian_process`

|File name | Description|
|----------|------------|
|CMakeLists.txt | CMake file for this subtree.|
|README.md | This file.|
|`src/covariance_functions.cpp` | Provides the necessary covariance functions / kernels for the Gaussian process.|
|`src/covariance_functions.h` | Header for covariance functions.|
|`src/gaussian_process.cpp` | Provides the Gaussian process inference and prediction functionality.|
|`src/gaussian_process.h` | Header for the Gaussian process.|
|`src/gaussian_process_guider.cpp` | Provides the GP-based control algorithm for the right ascension axis.|
|`src/gaussian_process_guider.h` | Header for the Gaussian process guider.|
|`tools/math_tools.cpp` | Mathematical tools for the GP implementation.|
|`tools/math_tools.h` | Header for the math tools.|
|`tools/plot_dec_data.py` | Python plotting for debugging.|
|`tools/plot_gp_data.py` | Python plotting for debugging.|
|`tools/plot_spectrum.py` | Python plotting for debugging.|
|`tools/analyze_data.py` | Python script that analyzes and plots data from testers.|
|`tools/optimize_params.py` | Python script for rudimentary parameter optimization.|
|`tests/gaussian_process/gaussian_process_test.cpp` | Unittests for the GP.|
|`tests/gaussian_process/math_tools_test.cpp` | Unittests for the math tools.|
|`tests/gaussian_process/dataset01.csv` | Real-world dataset for certain tests.|
|`tests/gaussian_process/dataset02.csv` | Real-world dataset for certain tests.|
|`tests/gaussian_process/dataset03.csv` | Real-world dataset for certain tests.|
|`tests/gaussian_process/performance_dataset01.csv` | Real-world dataset for performance tests.|
|`tests/gaussian_process/performance_dataset02.csv` | Real-world dataset for performance tests.|
|`tests/gaussian_process/performance_dataset03.csv` | Real-world dataset for performance tests.|
|`tests/gaussian_process/performance_dataset04.csv` | Real-world dataset for performance tests.|
|`tests/gaussian_process/performance_dataset05.csv` | Real-world dataset for performance tests.|
|`tests/gaussian_process/performance_dataset06.csv` | Real-world dataset for performance tests.|
|`tests/gaussian_process/performance_dataset07.csv` | Real-world dataset for performance tests.|
|`tests/gaussian_process/performance_dataset08.csv` | Real-world dataset for performance tests.|

Copyright and Licensing
-----------------------

Copyright 2014-2017, Max Planck Society.
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
