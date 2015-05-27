//
//  guide_gaussian_process.cpp
//  PHD
//
//  Created by Stephan Wenninger
//  Copyright 2014, Max Planck Society.

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

#include "UDPGuidingInteraction.h"
#include "tools/circular_buffer.h"

#include "guide_algorithm_gaussian_process.h"
#include <wx/stopwatch.h>
#include "tools/math_tools.h"
#include "UDPGuidingInteraction.h"


class GuideGaussianProcess::GuideGaussianProcessDialogPane : public ConfigDialogPane
{
    GuideGaussianProcess *m_pGuideAlgorithm;
    wxSpinCtrlDouble *m_pControlGain;

public:
    GuideGaussianProcessDialogPane(wxWindow *pParent, GuideGaussianProcess *pGuideAlgorithm)
      : ConfigDialogPane(_("Gaussian Process Guide Algorithm"),pParent)
    {
        m_pGuideAlgorithm = pGuideAlgorithm;

        int width = StringWidth(_T("000.00"));
        m_pControlGain = new wxSpinCtrlDouble(pParent, wxID_ANY, _T("foo2"),
                                              wxPoint(-1,-1),wxSize(width+30, -1),
                                              wxSP_ARROW_KEYS, 0.0, 1.0, 0.0, 0.05,
                                              _T("Control Gain"));
        m_pControlGain->SetDigits(2);


        //
        // TODO Add description of the control gain!
        //
        DoAdd(_("Control Gain"), m_pControlGain,
              _("Description of the control gain. Default = 1.0"));
    }

    virtual ~GuideGaussianProcessDialogPane(void) 
    {}

    /* Fill the GUI with the parameters that are currently chosen in the
      * guiding algorithm
      */
    virtual void LoadValues(void)
    {
      m_pGuideAlgorithm->SetControlGain(m_pControlGain->GetValue());
    }

    // Set the parameters chosen in the GUI in the actual guiding algorithm
    virtual void UnloadValues(void)
    {
      m_pControlGain->SetValue(m_pGuideAlgorithm->GetControlGain());
    }

};



// parameters of the GP guiding algorithm
struct GuideGaussianProcess::gp_guide_parameters
{
    UDPGuidingInteraction udpInteraction;
    CircularDoubleBuffer timestamps_;
    CircularDoubleBuffer measurements_;
    CircularDoubleBuffer modified_measurements_;
    wxStopWatch timer_;
    double control_signal_;
    int number_of_measurements_;
    double control_gain_;
    double elapsed_time_ms_;

    gp_guide_parameters() :
      udpInteraction(_T("localhost"), _T("1308"), _T("1309")),
      timestamps_(100),
      measurements_(100),
      modified_measurements_(100),
      timer_(),
      control_signal_(0.0),
      number_of_measurements_(0),
      elapsed_time_ms_(0.0)
    {

    }



    void clear()
    {
        timestamps_.clear();
        measurements_.clear();
        modified_measurements_.clear();
        number_of_measurements_ = 0;
    }

};




static const double DefaultControlGain = 1.0;

GuideGaussianProcess::GuideGaussianProcess(Mount *pMount, GuideAxis axis)
    : GuideAlgorithm(pMount, axis),
      parameters(0)
{
    parameters = new gp_guide_parameters();
    wxString configPath = GetConfigPath();
    double control_gain = pConfig->Profile.GetDouble(configPath + "/controlGain", DefaultControlGain);
    SetControlGain(control_gain);

    reset();
}

GuideGaussianProcess::~GuideGaussianProcess(void)
{
  delete parameters;
}


ConfigDialogPane *GuideGaussianProcess::GetConfigDialogPane(wxWindow *pParent)
{
    return new GuideGaussianProcessDialogPane(pParent, this);
}



bool GuideGaussianProcess::SetControlGain(double control_gain)
{
    bool error = false;

    try 
    {
        if (control_gain < 0) 
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

    pConfig->Profile.SetDouble(GetConfigPath() + "/controlGain", parameters->control_gain_);

    return error;
}

double GuideGaussianProcess::GetControlGain()
{
    return parameters->control_gain_;
}

wxString GuideGaussianProcess::GetSettingsSummary()
{
    return wxString::Format("Control Gain = %.3f\n", GetControlGain());
}



GUIDE_ALGORITHM GuideGaussianProcess::Algorithm(void)
{
    return GUIDE_ALGORITHM_GAUSSIAN_PROCESS;
}

void GuideGaussianProcess::HandleTimestamps()
{
    if (parameters->number_of_measurements_ == 0)
    {
        parameters->timer_.Start();
    }
    double time_now = parameters->timer_.Time();
    double delta_measurement_time_ms = time_now - parameters->elapsed_time_ms_;
    parameters->elapsed_time_ms_ = time_now;
    parameters->timestamps_.append(parameters->elapsed_time_ms_ - delta_measurement_time_ms / 2);
}

void GuideGaussianProcess::HandleMeasurements(double input)
{
    parameters->measurements_.append(input);
}

void GuideGaussianProcess::HandleModifiedMeasurements(double input)
{
    // If there is no previous measurement, a random one is generated.
    if(parameters->number_of_measurements_ == 0)
    {
        //The daytime indoor measurement noise SD is 0.25-0.35
        double indoor_noise_standard_deviation = 0.25;
        double first_random_measurement = indoor_noise_standard_deviation *
            math_tools::generate_normal_random_double();
        double new_modified_measurement =
            parameters->control_signal_ +
            first_random_measurement * (1 - parameters->control_gain_) -
            input;
        parameters->modified_measurements_.append(new_modified_measurement);
    }
    else
    {
        double new_modified_measurement =
            parameters->control_signal_ +
            parameters->measurements_.getSecondLastElement() * (1 - parameters->control_gain_) -
            parameters->measurements_.getLastElement();
        parameters->modified_measurements_.append(new_modified_measurement);
    }
}

double GuideGaussianProcess::result(double input)
{
    HandleTimestamps();
    HandleMeasurements(input);
    HandleModifiedMeasurements(input);
    parameters->number_of_measurements_++;

    /*
     * Need to read this value here because it is not loaded at the construction
     * time of this object.
     * 
     *
     * Not used when testing the Code with Matlab, will be used in the actual GP
     */
    double delta_controller_time_ms = pFrame->RequestedExposureDuration();



    // This is the Code sending the circular buffers to Matlab:

    double* timestamp_data = parameters->timestamps_.getEigenVector()->data();
    double* modified_measurement_data = parameters->modified_measurements_.getEigenVector()->data();
    double result;
    double wait_time = 100;

    bool sent = false;
    bool received = false;

    // Send the input
    double input_buf[] = { input };
    sent = parameters->udpInteraction.SendToUDPPort(input_buf, 8);
    received = parameters->udpInteraction.ReceiveFromUDPPort(&result, 8);
    wxMilliSleep(wait_time);

    // Send the size of the buffer
    double size = parameters->timestamps_.getEigenVector()->size();
    double size_buf[] = { size };
    sent = parameters->udpInteraction.SendToUDPPort(size_buf, 8);
    received = parameters->udpInteraction.ReceiveFromUDPPort(&result, 8);
    wxMilliSleep(wait_time);

    // Send modified measurements
    sent = parameters->udpInteraction.SendToUDPPort(modified_measurement_data, size * 8);
    received = parameters->udpInteraction.ReceiveFromUDPPort(&result, 8);
    wxMilliSleep(wait_time);

    // Send timestamps
    sent = parameters->udpInteraction.SendToUDPPort(timestamp_data, size * 8);
    // Receive the final control signal
    received = parameters->udpInteraction.ReceiveFromUDPPort(&result, 8);

    return result;


    /*
     * This is the code running the actual GP
     *
     * TODO: 
     * - Let GP class be a member of this Guiding Class
     * - Call the GP´s BFGS Optimizer every once in a while (how often?)
     *
     */


    /*

    if (number_of_measurements_ > 5)
    {

        // Inference
        gp_->infer(*timestamps_.getEigenVector(),
                   *modified_measurements_.getEigenVector());
        // Prediction of new control_signal_
        Eigen::VectorXd prediction =
            gp_->predict(elapsed_time_ms_ + delta_controller_time_ms / 2) -
            control_gain_ * input / delta_controller_time_ms;
        control_signal_ = prediction(0);

    }
    else
    {
        // Simpler prediction when there are not enough data points for the GP
        control_signal_ = -input / delta_controller_time_ms;
    }

    return control_signal_;

     */
}


void GuideGaussianProcess::reset()
{
    parameters->clear();
    return;
}
