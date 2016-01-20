//
//  guide_algorithm_median_window.cpp
//  PHD2 Guiding
//
//  Created by Edgar Klenske.
//  Copyright 2015, Max Planck Society.

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
#include "guide_algorithm_median_window.h"

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

class GuideAlgorithmMedianWindow::GuideAlgorithmMedianWindowDialogPane : public ConfigDialogPane
{
    GuideAlgorithmMedianWindow *m_pGuideAlgorithm;
    wxSpinCtrlDouble *m_pControlGain;
    wxSpinCtrl       *m_pNbMeasurementMin;

public:
    GuideAlgorithmMedianWindowDialogPane(wxWindow *pParent, GuideAlgorithmMedianWindow *pGuideAlgorithm)
      : ConfigDialogPane(_("Median Window Guide Algorithm"),pParent)
    {
        m_pGuideAlgorithm = pGuideAlgorithm;

        int width = StringWidth(_T("00000.00"));
        m_pControlGain = new wxSpinCtrlDouble(pParent, wxID_ANY, wxEmptyString,
                                              wxDefaultPosition,wxSize(width+30, -1),
                                              wxSP_ARROW_KEYS, 0.0, 1.0, 0.8, 0.05);
        m_pControlGain->SetDigits(2);

        // nb elements before starting the inference
        m_pNbMeasurementMin = new wxSpinCtrl(pParent, wxID_ANY, wxEmptyString,
                                             wxDefaultPosition,wxSize(width+30, -1),
                                             wxSP_ARROW_KEYS, 0, 100, 25);

        DoAdd(_("Control Gain"), m_pControlGain,
              _("The control gain defines how aggressive the controller is. It is the amount of pointing error that is "
                "fed back to the system. Default = 0.8"));

        DoAdd(_("Min data points (inference)"), m_pNbMeasurementMin,
              _("Minimal number of measurements to start using the Median Window. If there are too little data points, "
                "the result might be poor. Default = 25"));

    }

    virtual ~GuideAlgorithmMedianWindowDialogPane(void)
    {
      // no need to destroy the widgets, this is done by the parent...
    }

    /* Fill the GUI with the parameters that are currently chosen in the
      * guiding algorithm
      */
    virtual void LoadValues(void)
    {
      m_pControlGain->SetValue(m_pGuideAlgorithm->GetControlGain());
      m_pNbMeasurementMin->SetValue(m_pGuideAlgorithm->GetNbMeasurementsMin());
    }

    // Set the parameters chosen in the GUI in the actual guiding algorithm
    virtual void UnloadValues(void)
    {
      m_pGuideAlgorithm->SetControlGain(m_pControlGain->GetValue());
      m_pGuideAlgorithm->SetNbElementForInference(m_pNbMeasurementMin->GetValue());
    }

};


struct mw_guiding_circular_datapoints
{
  double timestamp;
  double measurement;
  double modified_measurement;
  double control;
};


// parameters of the LR guiding algorithm
struct GuideAlgorithmMedianWindow::mw_guide_parameters
{
    typedef mw_guiding_circular_datapoints data_points;
    circular_buffer<data_points> circular_buffer_parameters;

    wxStopWatch timer_;
    double control_signal_;
    double control_gain_;
    double last_timestamp_;
    double filtered_signal_;
    double mixing_parameter_;
    double stored_control_;

    int min_nb_element_for_inference;

    mw_guide_parameters() :
      circular_buffer_parameters(1024),
      timer_(),
      control_signal_(0.0),
      control_gain_(0.0),
      last_timestamp_(0.0),
      filtered_signal_(0.0),
      mixing_parameter_(0.0),
      stored_control_(0.0),
      min_nb_element_for_inference(0)
    {
        circular_buffer_parameters.push_front(data_points());
        circular_buffer_parameters[0].control = 0; // the first control is always zero
    }

    data_points& get_last_point()
    {
      return circular_buffer_parameters[circular_buffer_parameters.size() - 1];
    }

    data_points& get_second_last_point()
    {
      return circular_buffer_parameters[circular_buffer_parameters.size() - 2];
    }

    size_t get_number_of_measurements() const
    {
      return circular_buffer_parameters.size();
    }

    void add_one_point()
    {
      circular_buffer_parameters.push_front(data_points());
    }

    void clear()
    {
      circular_buffer_parameters.clear();
      circular_buffer_parameters.push_front(data_points());
      circular_buffer_parameters[0].control = 0; // the first control is always zero
    }

};


static const double DefaultControlGain = 0.5;           // control gain
static const int    DefaultNbMinPointsForInference = 25; // minimal number of points for doing the inference

GuideAlgorithmMedianWindow::GuideAlgorithmMedianWindow(Mount *pMount, GuideAxis axis)
    : GuideAlgorithm(pMount, axis),
      parameters(0)
{
    parameters = new mw_guide_parameters();
    wxString configPath = GetConfigPath();

    double control_gain = pConfig->Profile.GetDouble(configPath + "/mw_control_gain", DefaultControlGain);
    SetControlGain(control_gain);

    int nb_element_for_inference = pConfig->Profile.GetInt(configPath + "/mw_nb_elements_for_prediction", DefaultNbMinPointsForInference);
    SetNbElementForInference(nb_element_for_inference);

    reset();
}

GuideAlgorithmMedianWindow::~GuideAlgorithmMedianWindow(void)
{
    delete parameters;
}


ConfigDialogPane *GuideAlgorithmMedianWindow::GetConfigDialogPane(wxWindow *pParent)
{
    return new GuideAlgorithmMedianWindowDialogPane(pParent, this);
}


bool GuideAlgorithmMedianWindow::SetControlGain(double control_gain)
{
    bool error = false;

    try
    {
        if (control_gain < 0 || control_gain > 1)
        {
            throw ERROR_INFO("invalid controlGain");
        }

        parameters->control_gain_ = control_gain;
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        error = true;
        parameters->control_gain_ = DefaultControlGain;
    }

    pConfig->Profile.SetDouble(GetConfigPath() + "/mw_control_gain", parameters->control_gain_);

    return error;
}

bool GuideAlgorithmMedianWindow::SetNbElementForInference(int nb_elements)
{
    bool error = false;

    try
    {
        if (nb_elements < 0)
        {
            throw ERROR_INFO("invalid number of elements");
        }

        parameters->min_nb_element_for_inference = nb_elements;
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        error = true;
        parameters->min_nb_element_for_inference = DefaultNbMinPointsForInference;
    }

    pConfig->Profile.SetInt(GetConfigPath() + "/mw_nb_elements_for_prediction", parameters->min_nb_element_for_inference);

    return error;
}

double GuideAlgorithmMedianWindow::GetControlGain() const
{
    return parameters->control_gain_;
}

int GuideAlgorithmMedianWindow::GetNbMeasurementsMin() const
{
    return parameters->min_nb_element_for_inference;
}

wxString GuideAlgorithmMedianWindow::GetSettingsSummary()
{
    static const char* format =
      "Control Gain = %.3f\n";

    return wxString::Format(
      format,
      GetControlGain());
}


GUIDE_ALGORITHM GuideAlgorithmMedianWindow::Algorithm(void)
{
    return GUIDE_ALGORITHM_MEDIAN_WINDOW;
}

void GuideAlgorithmMedianWindow::HandleTimestamps()
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
void GuideAlgorithmMedianWindow::HandleMeasurements(double input)
{
    parameters->get_last_point().measurement = input;
}

void GuideAlgorithmMedianWindow::HandleControls(double control_input)
{
    // don't forget to apply the stored control signals from the dark period
    parameters->get_last_point().control = control_input + parameters->stored_control_;
    parameters->stored_control_ = 0; // reset stored control since we applied it
}

void GuideAlgorithmMedianWindow::StoreControls(double control_input)
{
    // sum up control inputs over the dark period
    parameters->stored_control_ += control_input;
}

double GuideAlgorithmMedianWindow::PredictDriftError()
{
    int delta_controller_time_ms = pFrame->RequestedExposureDuration();

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

    // the prediction is consisting of GP prediction and the linear drift
    return (delta_controller_time_ms / 1000.0)*mean_slope;
}

double GuideAlgorithmMedianWindow::result(double input)
{
    HandleMeasurements(input);
    HandleTimestamps();

    parameters->control_signal_ = parameters->control_gain_*input; // add the measured part of the controller

    double drift_prediction = 0;
	if (parameters->min_nb_element_for_inference > 0 &&
        parameters->get_number_of_measurements() > parameters->min_nb_element_for_inference)
    {
        drift_prediction = PredictDriftError();
		parameters->control_signal_ += drift_prediction; // add in the prediction
        if (parameters->control_signal_ * drift_prediction < 0) // check if the input points in the wrong direction
        {
            parameters->control_signal_ = 0; // prevent backlash overshooting
        }
	}
    else
    {
        parameters->control_signal_ *= 0.1; // scale down if no prediction is available
    }

    parameters->add_one_point();
    HandleControls(parameters->control_signal_);

    Debug.AddLine("Median window guider: input: %f, gain: %f, prediction: %f, control: %f",
        input, parameters->control_gain_, drift_prediction, parameters->control_signal_);

    return parameters->control_signal_;
}

double GuideAlgorithmMedianWindow::deduceResult()
{
    double drift_prediction = 0;
    parameters->control_signal_ = 0;
	if (parameters->min_nb_element_for_inference > 0 &&
        parameters->get_number_of_measurements() > parameters->min_nb_element_for_inference)
    {
        drift_prediction = PredictDriftError();
        parameters->control_signal_ += drift_prediction; // add in the prediction
	}

    StoreControls(parameters->control_signal_);

    Debug.AddLine("Median window guider: input: %f, gain: %f, prediction: %f, control: %f",
        0, parameters->control_gain_, drift_prediction, parameters->control_signal_);

    return parameters->control_signal_;
}

void GuideAlgorithmMedianWindow::reset()
{
    parameters->clear();
    return;
}
