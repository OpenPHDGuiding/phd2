/*
 * Copyright 2017, Max Planck Society.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 *    may be used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/* Created by Edgar Klenske <edgar.klenske@tuebingen.mpg.de>
 */

#include "gaussian_process_guider.h"
#include "guide_performance_tools.h"

int main(int argc, char **argv)
{
    if (argc < 15)
        return -1;

    GaussianProcessGuider *GPG;

    GaussianProcessGuider::guide_parameters parameters;

    // use parameters from command line
    parameters.control_gain_ = std::stod(argv[2]);
    parameters.min_periods_for_inference_ = std::stod(argv[3]);
    parameters.min_move_ = std::stod(argv[4]);
    parameters.SE0KLengthScale_ = std::stod(argv[5]);
    parameters.SE0KSignalVariance_ = std::stod(argv[6]);
    parameters.PKLengthScale_ = std::stod(argv[7]);
    parameters.PKPeriodLength_ = std::stod(argv[8]);
    parameters.PKSignalVariance_ = std::stod(argv[9]);
    parameters.SE1KLengthScale_ = std::stod(argv[10]);
    parameters.SE1KSignalVariance_ = std::stod(argv[11]);
    parameters.min_periods_for_period_estimation_ = std::stod(argv[12]);
    parameters.points_for_approximation_ = static_cast<int>(std::floor(std::stod(argv[13])));
    parameters.prediction_gain_ = std::stod(argv[14]);
    parameters.compute_period_ = true;

    GPG = new GaussianProcessGuider(parameters);

    GAHysteresis GAH;

    std::string filename;

    filename = argv[1];
    double improvement = calculate_improvement(filename, GAH, GPG);

    std::cout << improvement << std::endl;

    return 0;
}
