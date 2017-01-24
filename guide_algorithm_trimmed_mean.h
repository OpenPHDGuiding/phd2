/**
 * PHD2 Guiding
 *
 * @file
 * @date      2014-2016
 * @copyright Max Planck Society
 *
 * @author    Edgar D. Klenske <edgar.klenske@tuebingen.mpg.de>
 * @author    Raffi Enficiaud <raffi.enficiaud@tuebingen.mpg.de>
 *
 * @brief     Provides a simple guider for declination based on a trimmed mean
 */

/*
 *  This source code is distributed under the following "BSD" license
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *    Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *    Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *    Neither the name of Bret McKee, Dad Dog Development, nor the names of its
 *     Craig Stark, Stark Labs nor the names of its
 *     contributors may be used to endorse or promote products derived from
 *     this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef GUIDE_ALGORITHM_TRIMMED_MEAN_H
#define GUIDE_ALGORITHM_TRIMMED_MEAN_H

#define TM_DEBUG_FILE_ 1

#define TM_BUFFER_SIZE 256

#include <Eigen/Dense>
#include "guide_algorithm.h"

class wxStopWatch;

/**
 * This is a robust guiding algorithm for the declination axis that provides
 * strong resistance against switching to the wrong direction and consistent
 * linear predictions for both tracking performance and dark tracking while
 * no measurement is available.
 */
class GuideAlgorithmTrimmedMean : public GuideAlgorithm
{
private:
    /**
     * Holds all data that is needed for the trimmed mean guiding.
     */
    struct tm_guide_parameters;

    /**
     * Provides the GUI configuration functionality.
     */
    class GuideAlgorithmTrimmedMeanDialogPane;

    /**
     * Pointer to the guiding parameters of this instance.
     */
    tm_guide_parameters* parameters;

    /**
     * Stores the current time and creates a timestamp for the GP.
     */
    void HandleTimestamps();

    /**
     * Stores the measurement to the last datapoint.
     */
    void HandleMeasurements(double input);

    /**
     * Stores the control value.
     */
    void HandleControls(double control_input);

    /**
     * Stores the control inputs from blind guiding.
     */
    void StoreControls(double control_input);

    /**
     * Calculates the difference in drift error for the time between the last
     * prediction point and the current prediction point, which lies one
     * exposure length in the future. For this, the diff of measurements and
     * time is calculated. The element-wise division provides average slope,
     * and a trimmed mean operation results in a robust estimate for the average
     * drift error.
     */
    double PredictDriftError();

protected:
    double GetControlGain() const;
    bool SetControlGain(double control_gain);

    double GetMinMove() const;
    bool SetMinMove(double min_move);

    double GetPredictionGain() const;
    bool SetPredictionGain(double prediction_gain);

    double GetDifferentialGain() const;
    bool SetDifferentialGain(double differential_gain);

    int GetNbMeasurementsMin() const;
    bool SetNbElementForInference(int nb_elements);

    bool GetDarkTracking();
    bool SetDarkTracking(bool value);

public:
    GuideAlgorithmTrimmedMean(Mount *pMount, GuideAxis axis);
    virtual ~GuideAlgorithmTrimmedMean(void);
    virtual GUIDE_ALGORITHM Algorithm(void);

    virtual ConfigDialogPane *GetConfigDialogPane(wxWindow *pParent);

    /**
     * Calculates the control value based on the current input. The control
     * signal consists of feedback and prediction parts.
     *
     */
    virtual double result(double input);

    /**
     * This method provides predictive control if no measurement could be made.
     */
    virtual double deduceResult(void);

    /**
     * Clears the data from the circular buffer.
     */
    virtual void reset();
    virtual wxString GetSettingsSummary();
    virtual wxString GetGuideAlgorithmClassName(void) const { return "Trimmed Mean"; }

};

#endif  // GUIDE_ALGORITHM_TRIMMED_MEAN_H
