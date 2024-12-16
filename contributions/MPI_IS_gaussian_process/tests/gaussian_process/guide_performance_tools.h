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

#include <iterator>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>

class CSVRow
{
public:
    std::string const& operator[](std::size_t index) const { return m_data[index]; }
    std::size_t size() const { return m_data.size(); }
    void readNextRow(std::istream& str)
    {
        std::string line;
        std::getline(str, line);

        std::stringstream lineStream(line);
        std::string cell;

        m_data.clear();
        while (std::getline(lineStream, cell, ','))
        {
            m_data.push_back(cell);
        }
        // This checks for a trailing comma with no data after it.
        if (!lineStream && cell.empty())
        {
            // If there was a trailing comma then add an empty element.
            m_data.push_back("");
        }
    }

private:
    std::vector<std::string> m_data;
};

inline std::istream& operator>>(std::istream& str, CSVRow& data)
{
    data.readNextRow(str);
    return str;
}

inline Eigen::ArrayXXd read_data_from_file(std::string filename)
{
    std::ifstream file(filename);

    int i = 0;
    CSVRow row;
    while (file >> row)
    {
        // ignore special lines
        if (row[0][0] == 'F' || row.size() < 18 || row[5].size() == 0)
        {
            continue;
        }
        else
        {
            ++i;
        }
    }

    size_t N = i;
    i = -1;

    // initialize the different vectors needed for the GP
    Eigen::VectorXd times(N);
    Eigen::VectorXd measurements(N);
    Eigen::VectorXd controls(N);
    Eigen::VectorXd SNRs(N);

    file.close();
    file.clear();
    file.open(filename);
    double dither = 0.0;
    while (file >> row)
    {
        if (row[0][0] == 'I')
        {
            std::string infoline(row[0]);
            if (infoline.substr(6, 6) == "DITHER")
            {
                dither = std::stod(infoline.substr(15, 10));
            }
        }

        // ignore special lines
        if (row[0][0] == 'F' || row.size() < 18 || row[5].size() == 0)
        {
            continue;
        }
        else
        {
            ++i;
        }
        times(i) = std::stod(row[1]);
        measurements(i) = std::stod(row[5]);
        controls(i) = std::stod(row[7]) + dither;
        dither = 0;
        SNRs(i) = std::stod(row[16]);
    }

    Eigen::ArrayXXd result(4, N);
    result.row(0) = times;
    result.row(1) = measurements;
    result.row(2) = controls;
    result.row(3) = SNRs;

    return result;
}

inline double get_exposure_from_file(std::string filename)
{
    std::ifstream file(filename);

    CSVRow row;
    double exposure = 3.0; // initialize a default value
    while (file >> row)
    {
        if (row[0][0] == 'E')
        {
            std::string infoline(row[0]);
            if (infoline.substr(0, 8) == "Exposure")
            {
                exposure = std::stoi(infoline.substr(11, 4)) / 1000.0;
            }
        }
    }
    return exposure;
}

/*
 * Replicates the behavior of the standard Hysteresis algorithm.
 */
class GAHysteresis
{
public:
    double m_hysteresis;
    double m_aggression;
    double m_minMove;
    double m_lastMove;

    GAHysteresis() : m_hysteresis(0.1), m_aggression(0.7), m_minMove(0.2), m_lastMove(0.0) { }

    double result(double input)
    {
        double dReturn = (1.0 - m_hysteresis) * input + m_hysteresis * m_lastMove;

        dReturn *= m_aggression;

        if (fabs(input) < m_minMove)
        {
            dReturn = 0.0;
        }

        m_lastMove = dReturn;

        return dReturn;
    }
};

/*
 * Calculates the improvement of the GP Guider over Hysteresis on a dataset.
 */
inline double calculate_improvement(std::string filename, GAHysteresis GAH, GaussianProcessGuider *GPG)
{
    Eigen::ArrayXXd data = read_data_from_file(filename);
    double exposure = get_exposure_from_file(filename);

    Eigen::ArrayXd times = data.row(0);
    Eigen::ArrayXd measurements = data.row(1);
    Eigen::ArrayXd controls = data.row(2);
    Eigen::ArrayXd SNRs = data.row(3);

    double hysteresis_control = 0.0;
    double hysteresis_state = measurements(0);
    Eigen::ArrayXd hysteresis_states(times.size() - 2);

    double gp_guider_control = 0.0;
    double gp_guider_state = measurements(0);
    Eigen::ArrayXd gp_guider_states(times.size() - 2);

    for (int i = 0; i < times.size() - 2; ++i)
    {
        hysteresis_control = GAH.result(hysteresis_state);

        // this is a simple telescope "simulator"
        hysteresis_state = hysteresis_state + (measurements(i + 1) - (measurements(i) - controls(i))) - hysteresis_control;
        hysteresis_states(i) = hysteresis_state;

        GPG->reset();
        for (int j = 0; j < i; ++j)
        {
            GPG->inject_data_point(times(j), measurements(j), SNRs(j), controls(j));
        }
        gp_guider_control = GPG->result(gp_guider_state, SNRs(i), exposure);
        gp_guider_state = gp_guider_state + (measurements(i + 1) - (measurements(i) - controls(i))) - gp_guider_control;
        assert(fabs(gp_guider_state) < 100);

        gp_guider_states(i) = gp_guider_state;
    }

    double gp_guider_rms = std::sqrt(gp_guider_states.pow(2).sum() / gp_guider_states.size());
    double hysteresis_rms = std::sqrt(hysteresis_states.array().pow(2).sum() / hysteresis_states.size());

    return 1 - gp_guider_rms / hysteresis_rms;
}
