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

#include "phd.h"
#include "guide_algorithm_trimmed_mean.h"

// A functor for special orderings
struct value_index_ordering
{
    value_index_ordering(Eigen::VectorXd const& val) : values_(val){}
    bool operator()(int a, int b) const
    {
        return (values_[a] > values_[b]);
    }
    Eigen::VectorXd const& values_;
};

class GuideAlgorithmTrimmedMean::GuideAlgorithmTrimmedMeanDialogPane : public ConfigDialogPane
{
    GuideAlgorithmTrimmedMean *m_pGuideAlgorithm;
    wxSpinCtrlDouble *m_pControlGain;
    wxSpinCtrlDouble *m_pPredictionGain;
    wxSpinCtrlDouble *m_pDifferentialGain;
    wxSpinCtrl       *m_pNbMeasurementMin;

    wxCheckBox       *m_checkboxDarkMode;

public:
    GuideAlgorithmTrimmedMeanDialogPane(wxWindow *pParent, GuideAlgorithmTrimmedMean *pGuideAlgorithm)
      : ConfigDialogPane(_("Trimmed Mean Guide Algorithm"),pParent)
    {
        m_pGuideAlgorithm = pGuideAlgorithm;

        int width = StringWidth(_T("00000.00"));

        m_pControlGain = new wxSpinCtrlDouble(pParent, wxID_ANY, wxEmptyString,
                                              wxDefaultPosition,wxSize(width+30, -1),
                                              wxSP_ARROW_KEYS, 0.0, 2.0, 0.5, 0.05);
        m_pControlGain->SetDigits(2);

        m_pPredictionGain = new wxSpinCtrlDouble(pParent, wxID_ANY, wxEmptyString,
                                              wxDefaultPosition,wxSize(width+30, -1),
                                              wxSP_ARROW_KEYS, 0.0, 2.0, 0.5, 0.05);
        m_pPredictionGain->SetDigits(2);

        m_pDifferentialGain = new wxSpinCtrlDouble(pParent, wxID_ANY, wxEmptyString,
                                                 wxDefaultPosition,wxSize(width+30, -1),
                                                 wxSP_ARROW_KEYS, 0.0, 20.0, 0.5, 0.5);
        m_pDifferentialGain->SetDigits(2);

        // nb elements before starting the inference
        m_pNbMeasurementMin = new wxSpinCtrl(pParent, wxID_ANY, wxEmptyString,
                                             wxDefaultPosition,wxSize(width+30, -1),
                                             wxSP_ARROW_KEYS, 0, 100, 25);

        m_checkboxDarkMode = new wxCheckBox(pParent, wxID_ANY, _T(""));

        DoAdd(_("Control Gain"), m_pControlGain,
              _("The control gain defines how aggressive the controller is. It is the amount of pointing error that is "
                "fed back to the system. Default = 0.5"));

        DoAdd(_("Prediction Gain"), m_pPredictionGain,
              _("The prediction gain defines how much of the prediction should be used to compensate the drift error. "
              "Default = 1.0"));

        DoAdd(_("Differential Gain"), m_pDifferentialGain,
              _("The differential gain is used to reduce overshoot. It tries to slow down the control system, but if set "
              " too high, it can lead to noise amplification. Default = 5.0"));

        DoAdd(_("Min data points (inference)"), m_pNbMeasurementMin,
              _("Minimal number of measurements to start using the Trimmed Mean. If there are too little data points, "
                "the result might be poor. Default = 50"));

        DoAdd(_("Force dark tracking"), m_checkboxDarkMode, _("This is just for debugging and disabled by default"));

    }

    virtual ~GuideAlgorithmTrimmedMeanDialogPane(void)
    {
      // no need to destroy the widgets, this is done by the parent...
    }

    /* Fill the GUI with the parameters that are currently chosen in the
      * guiding algorithm
      */
    virtual void LoadValues(void)
    {
      m_pControlGain->SetValue(m_pGuideAlgorithm->GetControlGain());
      m_pPredictionGain->SetValue(m_pGuideAlgorithm->GetPredictionGain());
      m_pDifferentialGain->SetValue(m_pGuideAlgorithm->GetDifferentialGain());
      m_pNbMeasurementMin->SetValue(m_pGuideAlgorithm->GetNbMeasurementsMin());

      m_checkboxDarkMode->SetValue(m_pGuideAlgorithm->GetDarkTracking());
    }

    // Set the parameters chosen in the GUI in the actual guiding algorithm
    virtual void UnloadValues(void)
    {
      m_pGuideAlgorithm->SetControlGain(m_pControlGain->GetValue());
      m_pGuideAlgorithm->SetPredictionGain(m_pPredictionGain->GetValue());
      m_pGuideAlgorithm->SetDifferentialGain(m_pDifferentialGain->GetValue());
      m_pGuideAlgorithm->SetNbElementForInference(m_pNbMeasurementMin->GetValue());

      m_pGuideAlgorithm->SetDarkTracking(m_checkboxDarkMode->GetValue());
    }

};


struct tm_data_point
{
    double timestamp;
    double measurement; // current pointing error
    double control; // control action
};


struct GuideAlgorithmTrimmedMean::tm_guide_parameters
{
    circular_buffer<tm_data_point> circular_buffer_parameters;

    wxStopWatch timer_;
    double control_signal_;
    double control_gain_;
    double prediction_gain_;
    double differential_gain_;
    double last_timestamp_;
    double mixing_parameter_;
    double stored_control_;
    double last_prediction_end_;

    bool dark_tracking_mode_;

    int min_nb_element_for_inference_;

    tm_guide_parameters() :
      circular_buffer_parameters(MW_BUFFER_SIZE),
      timer_(),
      control_signal_(0.0),
      control_gain_(0.0),
      prediction_gain_(0.0),
      differential_gain_(0.0),
      last_timestamp_(0.0),
      mixing_parameter_(0.0),
      stored_control_(0.0),
      last_prediction_end_(0.0),
      min_nb_element_for_inference_(0)
    {
        circular_buffer_parameters.push_front(tm_data_point());
        circular_buffer_parameters[0].control = 0; // the first control is always zero
    }

    tm_data_point& get_last_point()
    {
      return circular_buffer_parameters[circular_buffer_parameters.size() - 1];
    }

    tm_data_point& get_second_last_point()
    {
      return circular_buffer_parameters[circular_buffer_parameters.size() - 2];
    }

    size_t get_number_of_measurements() const
    {
      return circular_buffer_parameters.size();
    }

    void add_one_point()
    {
      circular_buffer_parameters.push_front(tm_data_point());
    }

    void clear()
    {
      circular_buffer_parameters.clear();
      circular_buffer_parameters.push_front(tm_data_point());
      circular_buffer_parameters[0].control = 0; // the first control is always zero
    }

};

static const double DefaultControlGain = 0.5;
static const double DefaultPredictionGain = 1.0;
static const double DefaultDifferentialGain = 5.0;
static const int    DefaultNumMinPointsForInference = 50;

GuideAlgorithmTrimmedMean::GuideAlgorithmTrimmedMean(Mount *pMount, GuideAxis axis)
    : GuideAlgorithm(pMount, axis),
      parameters(0)
{
    parameters = new tm_guide_parameters();
    wxString configPath = GetConfigPath();

    double control_gain = pConfig->Profile.GetDouble(configPath + "/tm_control_gain", DefaultControlGain);
    SetControlGain(control_gain);

    double prediction_gain = pConfig->Profile.GetDouble(configPath + "/tm_prediction_gain", DefaultPredictionGain);
    SetPredictionGain(prediction_gain);

    double differential_gain = pConfig->Profile.GetDouble(configPath + "/tm_differential_gain", DefaultDifferentialGain);
    SetDifferentialGain(differential_gain);

    int nb_element_for_inference = pConfig->Profile.GetInt(configPath + "/tm_nb_elements_for_prediction", DefaultNumMinPointsForInference);
    SetNbElementForInference(nb_element_for_inference);

    parameters->dark_tracking_mode_ = false;

    reset();
}

GuideAlgorithmTrimmedMean::~GuideAlgorithmTrimmedMean(void)
{
    delete parameters;
}


ConfigDialogPane *GuideAlgorithmTrimmedMean::GetConfigDialogPane(wxWindow *pParent)
{
    return new GuideAlgorithmTrimmedMeanDialogPane(pParent, this);
}


bool GuideAlgorithmTrimmedMean::SetControlGain(double control_gain)
{
    bool error = false;

    try
    {
        if (control_gain < 0 || control_gain > 2.0)
        {
            throw ERROR_INFO("invalid control gain");
        }

        parameters->control_gain_ = control_gain;
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        error = true;
        parameters->control_gain_ = DefaultControlGain;
    }

    pConfig->Profile.SetDouble(GetConfigPath() + "/tm_control_gain", parameters->control_gain_);

    return error;
}

bool GuideAlgorithmTrimmedMean::SetPredictionGain(double prediction_gain)
{
    bool error = false;

    try
    {
        if (prediction_gain < 0 || prediction_gain > 1.0)
        {
            throw ERROR_INFO("invalid prediction gain");
        }

        parameters->prediction_gain_ = prediction_gain;
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        error = true;
        parameters->prediction_gain_ = DefaultPredictionGain;
    }

    pConfig->Profile.SetDouble(GetConfigPath() + "/tm_prediction_gain", parameters->prediction_gain_);

    return error;
}

bool GuideAlgorithmTrimmedMean::SetDifferentialGain(double differential_gain)
{
    bool error = false;

    try
    {
        if (differential_gain < 0 || differential_gain > 20.0)
        {
            throw ERROR_INFO("invalid differential gain");
        }

        parameters->differential_gain_ = differential_gain;
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        error = true;
        parameters->differential_gain_ = DefaultDifferentialGain;
    }

    pConfig->Profile.SetDouble(GetConfigPath() + "/tm_differential_gain", parameters->differential_gain_);

    return error;
}

bool GuideAlgorithmTrimmedMean::SetNbElementForInference(int nb_elements)
{
    bool error = false;

    try
    {
        if (nb_elements < 0)
        {
            throw ERROR_INFO("invalid number of elements");
        }

        parameters->min_nb_element_for_inference_ = nb_elements;
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        error = true;
        parameters->min_nb_element_for_inference_ = DefaultNumMinPointsForInference;
    }

    pConfig->Profile.SetInt(GetConfigPath() + "/tm_nb_elements_for_prediction", parameters->min_nb_element_for_inference_);

    return error;
}

double GuideAlgorithmTrimmedMean::GetControlGain() const
{
    return parameters->control_gain_;
}

double GuideAlgorithmTrimmedMean::GetPredictionGain() const
{
    return parameters->prediction_gain_;
}

double GuideAlgorithmTrimmedMean::GetDifferentialGain() const
{
    return parameters->differential_gain_;
}

int GuideAlgorithmTrimmedMean::GetNbMeasurementsMin() const
{
    return parameters->min_nb_element_for_inference_;
}

bool GuideAlgorithmTrimmedMean::GetDarkTracking()
{
    return parameters->dark_tracking_mode_;
}

bool GuideAlgorithmTrimmedMean::SetDarkTracking(bool value)
{
    parameters->dark_tracking_mode_ = value;
    return false;
}

wxString GuideAlgorithmTrimmedMean::GetSettingsSummary()
{
    static const char* format =
    "Control Gain = %.3f\n"
    "Prediction Gain = %.3f\n"
    "Differential Gain = %.3f\n";

    return wxString::Format(format, GetControlGain(), GetPredictionGain(), GetDifferentialGain());
}


GUIDE_ALGORITHM GuideAlgorithmTrimmedMean::Algorithm(void)
{
    return GUIDE_ALGORITHM_TRIMMED_MEAN;
}

void GuideAlgorithmTrimmedMean::HandleTimestamps()
{
    if (parameters->get_number_of_measurements() == 0)
    {
        parameters->timer_.Start();
    }
    double time_now = parameters->timer_.Time();
    double delta_measurement_time_ms = time_now - parameters->last_timestamp_;
    parameters->last_timestamp_ = time_now;
    parameters->get_last_point().timestamp = (time_now - delta_measurement_time_ms / 2) / 1000;
}

// adds a new measurement to the circular buffer that holds the data.
void GuideAlgorithmTrimmedMean::HandleMeasurements(double input)
{
    parameters->get_last_point().measurement = input;
}

void GuideAlgorithmTrimmedMean::HandleControls(double control_input)
{
    // don't forget to apply the stored control signals from the dark period
    parameters->get_last_point().control = control_input + parameters->stored_control_;
    parameters->stored_control_ = 0; // reset stored control since we applied it
}

void GuideAlgorithmTrimmedMean::StoreControls(double control_input)
{
    // sum up control inputs over the dark period
    parameters->stored_control_ += control_input;
}

double GuideAlgorithmTrimmedMean::PredictDriftError()
{
    int N = parameters->get_number_of_measurements();

    // initialize the different vectors needed for the GP
    Eigen::VectorXd timestamps(N-1);
    Eigen::VectorXd measurements(N-1);
    Eigen::VectorXd sum_controls(N-1);
    Eigen::VectorXd gear_error(N-1);
    Eigen::VectorXd linear_fit(N-1);

    // transfer the data from the circular buffer to the Eigen::Vectors
    for(size_t i = 0; i < N-1; i++)
    {
        timestamps(i) = parameters->circular_buffer_parameters[i].timestamp;
        measurements(i) = parameters->circular_buffer_parameters[i].measurement;
        sum_controls(i) = parameters->circular_buffer_parameters[i].control;
        if(i > 0)
        {
            sum_controls(i) += sum_controls(i-1); // sum over the control signals
        }
    }
    gear_error = sum_controls + measurements; // for each time step, add the residual error

    Eigen::VectorXd diff_gear_error = gear_error.bottomRows(gear_error.rows()-1) - gear_error.topRows(gear_error.rows()-1);
    Eigen::ArrayXd diff_timestamps = timestamps.bottomRows(timestamps.rows()-1) - timestamps.topRows(timestamps.rows()-1);

    std::vector<int> index(diff_gear_error.size(), 0);
    for (int i = 0; i != index.size(); i++) {
        index[i] = i;
    }

    // sort indices with respect to covariance value
    std::sort(index.begin(), index.end(), value_index_ordering(diff_gear_error));

    int exclude = std::floor(N/4);

    double mean_slope = 0;

    Eigen::ArrayXd diff_gear_error_window(diff_gear_error.size() - 2 * exclude);
    Eigen::ArrayXd diff_timestamps_window(diff_gear_error.size() - 2 * exclude);

    for (int i = exclude; i < index.size() - exclude; ++i)
    {
        diff_gear_error_window[i - exclude] = diff_gear_error[i];
        diff_timestamps_window[i - exclude] = diff_timestamps[i];
    }

    mean_slope = (diff_gear_error_window / diff_timestamps_window).mean();

    int delta_controller_time_ms = pFrame->RequestedExposureDuration();

    if ( parameters->last_prediction_end_ < 1.0 )
    {
        parameters->last_prediction_end_ = parameters->timer_.Time() / 1000.0;
    }

    // prediction from the last endpoint to the prediction point
    double prediction_length = (parameters->timer_.Time() + delta_controller_time_ms) / 1000.0 - parameters->last_prediction_end_;

    parameters->last_prediction_end_ = (parameters->timer_.Time() + delta_controller_time_ms) / 1000.0; // store current endpoint

    assert(prediction_length < 100);
    assert(parameters->control_gain_ < 10);

    // the prediction is consisting of GP prediction and the linear drift
    return prediction_length * mean_slope;
}

double GuideAlgorithmTrimmedMean::result(double input)
{
    if (parameters->dark_tracking_mode_ == true)
    {
        return deduceResult();
    }

    HandleMeasurements(input);
    HandleTimestamps();

    parameters->control_signal_ = parameters->control_gain_*input; // add the measured part of the controller

    double difference = 0;

    if (parameters->get_number_of_measurements() > 1)
    {
        difference = (parameters->get_last_point().measurement - parameters->get_second_last_point().measurement)
            / (parameters->get_last_point().timestamp - parameters->get_second_last_point().timestamp);
    }

    double drift_prediction = 0;
    if (parameters->min_nb_element_for_inference_ > 0 &&
        parameters->get_number_of_measurements() > parameters->min_nb_element_for_inference_)
    {
        drift_prediction = PredictDriftError();
        parameters->control_signal_ += parameters->prediction_gain_ * drift_prediction; // add in the prediction
        parameters->control_signal_ += parameters->differential_gain_ * difference; // D-component of PD controller

        // check if the input points in the wrong direction, but only if the error isn't too big
        if (std::abs(input) < 10.0 && parameters->control_signal_ * drift_prediction < 0)
        {
            parameters->control_signal_ = 0; // prevent backlash overshooting
        }
    }
    else
    {
        parameters->control_signal_ += parameters->differential_gain_ * difference; // D-component of PD controller
        parameters->control_signal_ *= 0.5; // scale down if no prediction is available
    }

    parameters->add_one_point();
    HandleControls(parameters->control_signal_);

    Debug.AddLine(wxString::Format("Trimmed mean guider: input: %f, diff: %f, prediction: %f, control: %f",
        input, difference, drift_prediction, parameters->control_signal_));

    // write the data to a file for easy debugging
#if MW_DEBUG_FILE_
    int N = parameters->get_number_of_measurements();

    // initialize the different vectors needed for the GP
    Eigen::VectorXd timestamps(N - 1);
    Eigen::VectorXd measurements(N - 1);
    Eigen::VectorXd controls(N - 1);
    Eigen::VectorXd sum_controls(N - 1);
    Eigen::VectorXd gear_error(N - 1);

    // transfer the data from the circular buffer to the Eigen::Vectors
    for (size_t i = 0; i < N - 1; i++)
    {
        timestamps(i) = parameters->circular_buffer_parameters[i].timestamp;
        measurements(i) = parameters->circular_buffer_parameters[i].measurement;
        controls(i) = parameters->circular_buffer_parameters[i].control;
        sum_controls(i) = parameters->circular_buffer_parameters[i].control;
        if (i > 0)
        {
            sum_controls(i) += sum_controls(i - 1); // sum over the control signals
        }
    }
    gear_error = sum_controls + measurements; // for each time step, add the residual error

    std::ofstream outfile;
    outfile.open("tm_data.csv", std::ios_base::out);
    if (outfile.is_open()) {
        outfile << "location, output\n";
        for (int i = 0; i < timestamps.size(); ++i) {
            outfile << std::setw(8) << timestamps[i] << "," << std::setw(8) << gear_error[i] << "\n";
        }
    }
    else {
        std::cout << "unable to write to file" << std::endl;
    }
    outfile.close();
#endif

    return parameters->control_signal_;
}

double GuideAlgorithmTrimmedMean::deduceResult()
{
    double drift_prediction = 0;
    parameters->control_signal_ = 0;
    if (parameters->min_nb_element_for_inference_ > 0 &&
        parameters->get_number_of_measurements() > parameters->min_nb_element_for_inference_)
    {
        drift_prediction = PredictDriftError();
        parameters->control_signal_ += drift_prediction; // add in the prediction
    }

    StoreControls(parameters->control_signal_);

    Debug.AddLine(wxString::Format("Trimmed mean guider (deduced): gain: %f, prediction: %f, control: %f",
        parameters->control_gain_, drift_prediction, parameters->control_signal_));

    assert(parameters->control_gain_ < 10);

    return parameters->control_signal_;
}

void GuideAlgorithmTrimmedMean::reset()
{
    parameters->clear();
    return;
}
