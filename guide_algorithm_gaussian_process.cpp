//
//  guide_gaussian_process.cpp
//  PHD2 Guiding
//
//  Created by Stephan Wenninger and Edgar Klenske.
//  Copyright 2014-2015, Max Planck Society.

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

#include "guide_algorithm_gaussian_process.h"
#include <wx/stopwatch.h>
#include <ctime>

#include "math_tools.h"

#include "math_tools.h"
#include "gaussian_process.h"
#include "covariance_functions.h"

class GuideGaussianProcess::GuideGaussianProcessDialogPane : public ConfigDialogPane
{
    GuideGaussianProcess *m_pGuideAlgorithm;
    wxSpinCtrlDouble *m_pControlGain;
    wxSpinCtrl       *m_pNbPointsInference;
    wxSpinCtrl       *m_pNbPointsOptimisation;
    wxSpinCtrl       *m_pNbPointsApproximation;

    wxSpinCtrlDouble *m_pHyperDiracNoise;
    wxSpinCtrlDouble *m_pSE0KLengthScale;
    wxSpinCtrlDouble *m_pSE0KSignalVariance;
    wxSpinCtrlDouble *m_pPKLengthScale;
    wxSpinCtrlDouble *m_pPKPeriodLength;
    wxSpinCtrlDouble *m_pPKSignalVariance;
    wxSpinCtrlDouble *m_pSE1KLengthScale;
    wxSpinCtrlDouble *m_pSE1KSignalVariance;
    wxSpinCtrlDouble *m_pMixingParameter;

    wxCheckBox       *m_checkboxOptimization;
    wxCheckBox       *m_checkboxComputePeriod;

public:
    GuideGaussianProcessDialogPane(wxWindow *pParent, GuideGaussianProcess *pGuideAlgorithm)
      : ConfigDialogPane(_("Gaussian Process Guide Algorithm"),pParent)
    {
        m_pGuideAlgorithm = pGuideAlgorithm;

        int width = StringWidth(_T("000.00"));
        m_pControlGain = pFrame->MakeSpinCtrlDouble(pParent, wxID_ANY, _T("foo2"),
            wxPoint(-1, -1), wxSize(width, -1),
                                              wxSP_ARROW_KEYS, 0.0, 1.0, 0.0, 0.05,
                                              _T("Control Gain"));
        m_pControlGain->SetDigits(2);

        m_pMixingParameter = new wxSpinCtrlDouble(pParent, wxID_ANY, wxEmptyString,
                                                  wxDefaultPosition,wxSize(width+30, -1),
                                                  wxSP_ARROW_KEYS, 0.0, 1.0, 0.8, 0.01);
        m_pMixingParameter->SetDigits(2);

        // number of elements before starting the inference
        m_pNbPointsInference = new wxSpinCtrl(pParent, wxID_ANY, wxEmptyString,
                                             wxDefaultPosition,wxSize(width+30, -1),
                                             wxSP_ARROW_KEYS, 0, 1000, 10);

        // number of elements before starting the optimization
        m_pNbPointsOptimisation = new wxSpinCtrl(pParent, wxID_ANY, wxEmptyString,
                                                 wxDefaultPosition,wxSize(width+30, -1),
                                                 wxSP_ARROW_KEYS, 0, 1000, 10);

        // number of points used for the approximate GP inference (subset of data)
        m_pNbPointsApproximation = new wxSpinCtrl(pParent, wxID_ANY, wxEmptyString,
                                                 wxDefaultPosition,wxSize(width+30, -1),
                                                 wxSP_ARROW_KEYS, 0, 2000, 10);

        // hyperparameters
        m_pHyperDiracNoise = new wxSpinCtrlDouble(pParent, wxID_ANY, wxEmptyString,
                                                  wxDefaultPosition,wxSize(width+30, -1),
                                                  wxSP_ARROW_KEYS, 0.0, 10.0, 1.0, 0.1);
        m_pHyperDiracNoise->SetDigits(2);

        m_pSE0KLengthScale = new wxSpinCtrlDouble(pParent, wxID_ANY, wxEmptyString,
                                                 wxDefaultPosition,wxSize(width+30, -1),
                                                 wxSP_ARROW_KEYS, 0.0, 5000.0, 500.0, 1.0);
        m_pSE0KLengthScale->SetDigits(2);

        m_pSE0KSignalVariance = new wxSpinCtrlDouble(pParent, wxID_ANY, wxEmptyString,
                                                    wxDefaultPosition,wxSize(width+30, -1),
                                                    wxSP_ARROW_KEYS, 0.0, 10, 1, 0.1);
        m_pSE0KSignalVariance->SetDigits(2);


        m_pPKLengthScale = new wxSpinCtrlDouble(pParent, wxID_ANY, wxEmptyString,
                                                wxDefaultPosition,wxSize(width+30, -1),
                                                wxSP_ARROW_KEYS, 0.0, 10, 1.0, 0.1);
        m_pPKLengthScale->SetDigits(2);

        m_pPKPeriodLength = new wxSpinCtrlDouble(pParent, wxID_ANY, wxEmptyString,
                                                 wxDefaultPosition,wxSize(width+30, -1),
                                                 wxSP_ARROW_KEYS, 50, 2000, 500, 1);
        m_pPKPeriodLength->SetDigits(2);


        m_pPKSignalVariance = new wxSpinCtrlDouble(pParent, wxID_ANY, wxEmptyString,
                                                   wxDefaultPosition,wxSize(width+30, -1),
                                                   wxSP_ARROW_KEYS, 0.0, 30, 10, 0.1);
        m_pPKSignalVariance->SetDigits(2);

        m_pSE1KLengthScale = new wxSpinCtrlDouble(pParent, wxID_ANY, wxEmptyString,
                                                 wxDefaultPosition,wxSize(width+30, -1),
                                                 wxSP_ARROW_KEYS, 0.0, 10.0, 5.0, 0.1);
        m_pSE1KLengthScale->SetDigits(2);

        m_pSE1KSignalVariance = new wxSpinCtrlDouble(pParent, wxID_ANY, wxEmptyString,
                                                 wxDefaultPosition,wxSize(width+30, -1),
                                                 wxSP_ARROW_KEYS, 0.0, 10, 1, 0.1);
        m_pSE1KSignalVariance->SetDigits(2);


        m_checkboxOptimization = new wxCheckBox(pParent, wxID_ANY, _T(""));
        m_checkboxComputePeriod = new wxCheckBox(pParent, wxID_ANY, _T(""));

        DoAdd(_("Control Gain"), m_pControlGain,
              _("The control gain defines how aggressive the controller is. It is the amount of pointing error that is "
                "fed back to the system. Default = 0.8"));

        DoAdd(_("Minimum data points (inference)"), m_pNbPointsInference,
              _("Minimal number of measurements to start using the Gaussian process. If there are too little data points, "
                "the result might be poor. Default = 25"));

        DoAdd(_("Minimum data points (optimization)"), m_pNbPointsOptimisation,
              _("Minimal number of measurements to start estimating the periodicity. If there are too little data points, "
                "the estimation might not work. Default = 100"));

        DoAdd(_("Used data points (approximation)"), m_pNbPointsApproximation,
              _("Number of data points used in the approximation. Both prediction accuracy as well as runtime rise with "
                "the number of datapoints. Default = 100"));

        // hyperparameters
        DoAdd(_("Measurement noise"), m_pHyperDiracNoise,
              _("The measurement noise is the expected uncertainty due to seeing and camera noise. "
                "If the measurement noise is too low, the Gaussian process might be too rigid. Try to upper bound your "
                "measurement uncertainty. Default = 1.0"));
        DoAdd(_("Length scale [SE]"), m_pSE0KLengthScale,
              _("The length scale of the large non-periodic structure in the error. This is essentially a high-pass "
                "filter and the length scale defines the corner frequency. Default = 500"));
        DoAdd(_("Signal Variance [SE]"), m_pSE0KSignalVariance,
              _("Signal Variance of the long-term variations. Default = 1"));
        DoAdd(_("Length scale [PER]"), m_pPKLengthScale,
              _("The length scale defines the \"wigglyness\" of the function. The smaller the length scale, the more "
                "structure can be learned. If chosen too small, some non-periodic structure might be picked up as well. "
                "Default = 5.0"));
        DoAdd(_("Period length [PER]"), m_pPKPeriodLength,
              _("The period length of the periodic error component that should be corrected. It turned out that the shorter "
                "period is more important for the performance than the long one, if a telescope mount shows both. Default = 200"));
        DoAdd(_("Signal variance [PER]"), m_pPKSignalVariance,
              _("The width of the periodic error. Should be around the amplitude of the PE curve, but is not a critical parameter. "
                "Default = 30"));
        DoAdd(_("Length scale [SE]"), m_pSE1KLengthScale,
              _("The length scale of the short range non-periodic parts of the gear error. This is essentially a low-pass "
                "filter and the length scale defines the corner frequency. Default = 5"));
        DoAdd(_("Signal Variance [SE]"), m_pSE1KSignalVariance,
              _("Signal Variance of the short-term variations. Default = 1"));
        DoAdd(_("Mixing"), m_pMixingParameter,
              _("The mixing defines how much control signal is generated from the prediction and how much. Default = 0.5"));

        DoAdd(_("Compute period"), m_checkboxComputePeriod,
              _("Compute period length with FFT"));
        DoAdd(_("Optimize parameters"), m_checkboxOptimization,
              _("Optimize parameters with Newton steps"));
    }

    virtual ~GuideGaussianProcessDialogPane(void)
    {
      // no need to destroy the widgets, this is done by the parent...
    }

    /* Fill the GUI with the parameters that are currently chosen in the
     * guiding algorithm.
     */
    virtual void LoadValues(void)
    {
        m_pControlGain->SetValue(m_pGuideAlgorithm->GetControlGain());
        m_pNbPointsInference->SetValue(m_pGuideAlgorithm->GetNbMeasurementsMin());
        m_pNbPointsOptimisation->SetValue(m_pGuideAlgorithm->GetNbPointsBetweenOptimisation());
        m_pNbPointsApproximation->SetValue(m_pGuideAlgorithm->GetNbPointsForApproximation());

        std::vector<double> hyperparameters = m_pGuideAlgorithm->GetGPHyperparameters();
        assert(hyperparameters.size() == 8);

        m_pHyperDiracNoise->SetValue(hyperparameters[0]);
        m_pSE0KLengthScale->SetValue(hyperparameters[1]);
        m_pSE0KSignalVariance->SetValue(hyperparameters[2]);
        m_pPKLengthScale->SetValue(hyperparameters[3]);
        m_pPKSignalVariance->SetValue(hyperparameters[4]);
        m_pSE1KLengthScale->SetValue(hyperparameters[5]);
        m_pSE1KSignalVariance->SetValue(hyperparameters[6]);
        m_pPKPeriodLength->SetValue(hyperparameters[7]);

        m_pMixingParameter->SetValue(m_pGuideAlgorithm->GetMixingParameter());

        m_checkboxOptimization->SetValue(m_pGuideAlgorithm->GetBoolOptimizeHyperparameters());
        m_checkboxComputePeriod->SetValue(m_pGuideAlgorithm->GetBoolComputePeriod());
    }

    // Set the parameters chosen in the GUI in the actual guiding algorithm
    virtual void UnloadValues(void)
    {
        m_pGuideAlgorithm->SetControlGain(m_pControlGain->GetValue());
        m_pGuideAlgorithm->SetNbElementForInference(m_pNbPointsInference->GetValue());
        m_pGuideAlgorithm->SetNbPointsOptimisation(m_pNbPointsOptimisation->GetValue());
        m_pGuideAlgorithm->SetNbPointsForApproximation(m_pNbPointsApproximation->GetValue());

        std::vector<double> hyperparameters(8);

        hyperparameters[0] = m_pHyperDiracNoise->GetValue();
        hyperparameters[1] = m_pSE0KLengthScale->GetValue();
        hyperparameters[2] = m_pSE0KSignalVariance->GetValue();
        hyperparameters[3] = m_pPKLengthScale->GetValue();
        hyperparameters[4] = m_pPKSignalVariance->GetValue();
        hyperparameters[5] = m_pSE1KLengthScale->GetValue();
        hyperparameters[6] = m_pSE1KSignalVariance->GetValue();
        hyperparameters[7] = m_pPKPeriodLength->GetValue();

        m_pGuideAlgorithm->SetGPHyperparameters(hyperparameters);
        m_pGuideAlgorithm->SetMixingParameter(m_pMixingParameter->GetValue());
        m_pGuideAlgorithm->SetBoolOptimizeHyperparameters(m_checkboxOptimization->GetValue());
        m_pGuideAlgorithm->SetBoolComputePeriod(m_checkboxComputePeriod->GetValue());
    }
};


struct gp_guiding_circular_datapoints
{
    double timestamp;
    double measurement;
    double modified_measurement;
    double control;
    double variance;
};


// parameters of the GP guiding algorithm
struct GuideGaussianProcess::gp_guide_parameters
{
    typedef gp_guiding_circular_datapoints data_points;
    circular_buffer<data_points> circular_buffer_parameters;

    wxStopWatch timer_;
    double control_signal_;
    double control_gain_;
    double last_timestamp_;
    double filtered_signal_;
    double mixing_parameter_;

    // Kalman filter state and variance
    double mean_kf_;
    double var_kf_;
    double prediction_;

    int min_nb_element_for_inference;
    int min_points_for_optimisation;
    int points_for_approximation;

    bool optimize_hyperparameters;
    bool compute_period;

    covariance_functions::PeriodicSquareExponential2 covariance_function_;
    covariance_functions::PeriodicSquareExponential output_covariance_function_;
    GP gp_;

    gp_guide_parameters() :
      circular_buffer_parameters(CIRCULAR_BUFFER_SIZE),
      timer_(),
      control_signal_(0.0),
      control_gain_(0.0),
      last_timestamp_(0.0),
      filtered_signal_(0.0),
      mixing_parameter_(0.0),
      mean_kf_(0.0),
      var_kf_(0.0),
      prediction_(0.0),
      min_nb_element_for_inference(0),
      min_points_for_optimisation(0),
      points_for_approximation(0),
      optimize_hyperparameters(false),
      compute_period(false),
      gp_(covariance_function_)
    {
        circular_buffer_parameters.push_front(data_points()); // add first point
        circular_buffer_parameters[0].control = 0; // set first control to zero
        gp_.enableOutputProjection(output_covariance_function_);
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
        circular_buffer_parameters.push_front(data_points()); // add first point
        circular_buffer_parameters[0].control = 0; // set first control to zero
        gp_.clear();
    }

};


static const double DefaultControlGain                   = 0.8; // control gain
static const int    DefaultNbMinPointsForInference       = 25; // minimal number of points for doing the inference

static const double DefaultGaussianNoiseHyperparameter   = 1.0; // default Gaussian measurement noise

static const double DefaultLengthScaleSE0Ker             = 500.0; // length-scale of the long-range SE-kernel
static const double DefaultSignalVarianceSE0Ker          = 1.0; // signal variance of the long-range SE-kernel
static const double DefaultLengthScalePerKer             = 0.3; // length-scale of the periodic kernel
static const double DefaultPeriodLengthPerKer            = 500; // P_p, period-length of the periodic kernel
static const double DefaultSignalVariancePerKer          = 10.0; // signal variance of the periodic kernel
static const double DefaultLengthScaleSE1Ker             = 5.0; // length-scale of the short-range SE-kernel
static const double DefaultSignalVarianceSE1Ker          = 1.0; // signal variance of the short range SE-kernel

static const int    DefaultNbMinPointsForOptimisation    = 100; // minimal number of points for doing the period identification
static const int    DefaultNbPointsForApproximation      = 100; // number of points used in the GP approximation
static const double DefaultMixing                        = 0.5; // amount of GP prediction to blend in

// by default optimization turned off
static const bool   DefaultOptimize                      = false;
static const bool   DefaultComputePeriod                 = true;

GuideGaussianProcess::GuideGaussianProcess(Mount *pMount, GuideAxis axis)
    : GuideAlgorithm(pMount, axis),
      parameters(0)
{
    parameters = new gp_guide_parameters();
    wxString configPath = GetConfigPath();

    double control_gain = pConfig->Profile.GetDouble(configPath + "/gp_control_gain", DefaultControlGain);
    SetControlGain(control_gain);

    int nb_element_for_inference = pConfig->Profile.GetInt(configPath + "/gp_min_points_inference", DefaultNbMinPointsForInference);
    SetNbElementForInference(nb_element_for_inference);

    int nb_points_optimisation = pConfig->Profile.GetInt(configPath + "/gp_min_points_optimization", DefaultNbMinPointsForOptimisation);
    SetNbPointsOptimisation(nb_points_optimisation);

    int nb_points_approximation = pConfig->Profile.GetInt(configPath + "/gp_points_for_approximation", DefaultNbPointsForApproximation);
    SetNbPointsForApproximation(nb_points_approximation);

    double mixing_parameter = pConfig->Profile.GetDouble(configPath + "/gp_mixing_parameter", DefaultMixing);
    SetMixingParameter(mixing_parameter);

    std::vector<double> v_hyperparameters(8);
    v_hyperparameters[0] = pConfig->Profile.GetDouble(configPath + "/gp_gaussian_noise",         DefaultGaussianNoiseHyperparameter);
    v_hyperparameters[1] = pConfig->Profile.GetDouble(configPath + "/gp_length_scale_se0_kern",  DefaultLengthScaleSE0Ker);
    v_hyperparameters[2] = pConfig->Profile.GetDouble(configPath + "/gp_sigvar_se0_kern",        DefaultSignalVarianceSE0Ker);
    v_hyperparameters[3] = pConfig->Profile.GetDouble(configPath + "/gp_length_scale_per_kern",  DefaultLengthScalePerKer);
    v_hyperparameters[4] = pConfig->Profile.GetDouble(configPath + "/gp_sigvar_per_kern",        DefaultSignalVariancePerKer);
    v_hyperparameters[5] = pConfig->Profile.GetDouble(configPath + "/gp_length_scale_se1_kern",  DefaultLengthScaleSE1Ker);
    v_hyperparameters[6] = pConfig->Profile.GetDouble(configPath + "/gp_sigvar_se1_kern",        DefaultSignalVarianceSE1Ker);
    v_hyperparameters[7] = pConfig->Profile.GetDouble(configPath + "/gp_period_per_kern",        DefaultPeriodLengthPerKer);

    SetGPHyperparameters(v_hyperparameters);

    bool compute_period = pConfig->Profile.GetBoolean(configPath + "/gp_compute_period", DefaultComputePeriod);
    SetBoolComputePeriod(compute_period);

    // enable the explicit basis function for the linear drift
    parameters->gp_.enableExplicitTrend();

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

    pConfig->Profile.SetDouble(GetConfigPath() + "/gp_control_gain", parameters->control_gain_);

    return error;
}

bool GuideGaussianProcess::SetNbElementForInference(int nb_elements)
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

    pConfig->Profile.SetInt(GetConfigPath() + "/gp_min_points_inference", parameters->min_nb_element_for_inference);

    return error;
}

bool GuideGaussianProcess::SetNbPointsOptimisation(int nb_points)
{
    bool error = false;

    try
    {
        if (nb_points < 0)
        {
            throw ERROR_INFO("invalid number of points");
        }

        parameters->min_points_for_optimisation = nb_points;
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        error = true;
        parameters->min_points_for_optimisation = DefaultNbMinPointsForOptimisation;
    }

    pConfig->Profile.SetInt(GetConfigPath() + "/gp_min_points_optimization", parameters->min_points_for_optimisation);

    return error;
}

bool GuideGaussianProcess::SetNbPointsForApproximation(int nb_points)
{
    bool error = false;

    try
    {
        if (nb_points < 0)
        {
            throw ERROR_INFO("invalid number of points");
        }

        parameters->points_for_approximation = nb_points;
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        error = true;
        parameters->points_for_approximation = DefaultNbPointsForApproximation;
    }

    pConfig->Profile.SetInt(GetConfigPath() + "/gp_points_approximation", parameters->points_for_approximation);

    return error;
}

bool GuideGaussianProcess::SetGPHyperparameters(std::vector<double> const &hyperparameters)
{
    if(hyperparameters.size() != 8)
        return false;

    Eigen::VectorXd hyperparameters_eig = Eigen::VectorXd::Map(&hyperparameters[0], hyperparameters.size());
    bool error = false;

    // we do this check in sequence: maybe there would be additional checks on this later.

    // gaussian process noise (dirac kernel)
    try
    {
        if (hyperparameters_eig(0) < 0)
        {
            throw ERROR_INFO("invalid noise for dirac kernel");
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        error = true;
        hyperparameters_eig(0) = DefaultGaussianNoiseHyperparameter;
    }

    pConfig->Profile.SetDouble(GetConfigPath() + "/gp_gaussian_noise", hyperparameters_eig(0));

    // length scale short SE kernel
    try
    {
      if (hyperparameters_eig(1) < 0)
      {
        throw ERROR_INFO("invalid length scale for short SE kernel");
      }
    }
    catch (wxString Msg)
    {
      POSSIBLY_UNUSED(Msg);
      error = true;
      hyperparameters_eig(1) = DefaultLengthScaleSE0Ker;
    }

    pConfig->Profile.SetDouble(GetConfigPath() + "/gp_length_scale_se0_kern", hyperparameters_eig(1));

    // signal variance short SE kernel
    try
    {
      if (hyperparameters_eig(2) < 0)
      {
        throw ERROR_INFO("invalid signal variance for the short SE kernel");
      }
    }
    catch (wxString Msg)
    {
      POSSIBLY_UNUSED(Msg);
      error = true;
      hyperparameters_eig(2) = DefaultSignalVarianceSE0Ker;
    }

    pConfig->Profile.SetDouble(GetConfigPath() + "/gp_sigvar_se0_kern", hyperparameters_eig(2));

    // length scale periodic kernel
    try
    {
        if (hyperparameters_eig(3) < 0)
        {
            throw ERROR_INFO("invalid length scale for periodic kernel");
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        error = true;
        hyperparameters_eig(3) = DefaultLengthScalePerKer;
    }

    pConfig->Profile.SetDouble(GetConfigPath() + "/gp_length_scale_per_kern", hyperparameters_eig(3));

    // signal variance periodic kernel
    try
    {
        if (hyperparameters_eig(4) < 0)
        {
            throw ERROR_INFO("invalid signal variance for the periodic kernel");
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        error = true;
        hyperparameters_eig(4) = DefaultSignalVariancePerKer;
    }

    pConfig->Profile.SetDouble(GetConfigPath() + "/gp_sigvar_per_kern", hyperparameters_eig(4));


    // length scale long SE kernel
    try
    {
        if (hyperparameters_eig(5) < 0)
        {
            throw ERROR_INFO("invalid length scale for SE kernel");
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        error = true;
        hyperparameters_eig(5) = DefaultLengthScaleSE1Ker;
    }

    pConfig->Profile.SetDouble(GetConfigPath() + "/gp_length_scale_se1_kern", hyperparameters_eig(5));

    // signal variance SE kernel
    try
    {
        if (hyperparameters_eig(6) < 0)
        {
            throw ERROR_INFO("invalid signal variance for the SE kernel");
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        error = true;
        hyperparameters_eig(6) = DefaultSignalVarianceSE1Ker;
    }

    pConfig->Profile.SetDouble(GetConfigPath() + "/gp_sigvar_se1_kern", hyperparameters_eig(6));

    // period length periodic kernel
    try
    {
      if (hyperparameters_eig(7) < 0)
      {
        throw ERROR_INFO("invalid period length for periodic kernel");
      }
    }
    catch (wxString Msg)
    {
      POSSIBLY_UNUSED(Msg);
      error = true;
      hyperparameters_eig(7) = DefaultPeriodLengthPerKer;
    }

    pConfig->Profile.SetDouble(GetConfigPath() + "/gp_period_per_kern", hyperparameters_eig(7));

    // note that the GP class works in log space! Thus, we have to convert the parameters to log.
    parameters->gp_.setHyperParameters(hyperparameters_eig.array().log());
    return error;
}

bool GuideGaussianProcess::SetMixingParameter(double mixing)
{
    bool error = false;

    try
    {
        if (mixing < 0)
        {
            throw ERROR_INFO("invalid mixing parameter");
        }

        parameters->mixing_parameter_ = mixing;
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        error = true;
        parameters->mixing_parameter_ = DefaultMixing;
    }

    pConfig->Profile.SetDouble(GetConfigPath() + "/gp_mixing_parameter", parameters->mixing_parameter_);

    return error;
}

double GuideGaussianProcess::GetControlGain() const
{
    return parameters->control_gain_;
}

int GuideGaussianProcess::GetNbMeasurementsMin() const
{
    return parameters->min_nb_element_for_inference;
}

int GuideGaussianProcess::GetNbPointsBetweenOptimisation() const
{
    return parameters->min_points_for_optimisation;
}

int GuideGaussianProcess::GetNbPointsForApproximation() const
{
    return parameters->points_for_approximation;
}

std::vector<double> GuideGaussianProcess::GetGPHyperparameters() const
{
    // since the GP class works in log space, we have to exp() the parameters first.
    Eigen::VectorXd hyperparameters = parameters->gp_.getHyperParameters().array().exp();

    // we need to map the Eigen::vector into a std::vector.
    return std::vector<double>(hyperparameters.data(), // the first element is at the array address
                               hyperparameters.data() + 8); // 8 parameters, therefore the last is at position 7
}

double GuideGaussianProcess::GetMixingParameter() const
{
    return parameters->mixing_parameter_;
}

bool GuideGaussianProcess::GetBoolComputePeriod() const
{
    return parameters->compute_period;
}

wxString GuideGaussianProcess::GetSettingsSummary()
{
    static const char* format =
      "Control Gain = %.3f\n"
      "Hyperparameters\n"
      "\tGP noise = %.3f\n"
      "\tLength scale short SE kernel = %.3f\n"
      "\tSignal variance short SE kernel = %.3f\n"
      "\tLength scale periodic kernel = %.3f\n"
      "\tPeriod Length periodic kernel = %.3f\n"
      "\tSignal variance periodic kernel = %.3f\n"
      "\tLength scale long SE kernel = %.3f\n"
      "\tSignal variance long SE kernel = %.3f\n"
      "Optimisation called every = %.3d points\n"
      "Mixing parameter = %.3d\n"
    ;

    Eigen::VectorXd hyperparameters = parameters->gp_.getHyperParameters();

    return wxString::Format(
      format,
      GetControlGain(),
      hyperparameters(0), hyperparameters(1),
      hyperparameters(2), hyperparameters(3),
      hyperparameters(4), hyperparameters(5),
      hyperparameters(6), hyperparameters(7),
      parameters->min_points_for_optimisation,
      parameters->mixing_parameter_);
}


GUIDE_ALGORITHM GuideGaussianProcess::Algorithm(void)
{
    return GUIDE_ALGORITHM_GAUSSIAN_PROCESS;
}

void GuideGaussianProcess::HandleTimestamps()
{
    if (parameters->get_number_of_measurements() == 0)
    {
        parameters->timer_.Start();
    }
    double time_now = parameters->timer_.Time();
    double delta_measurement_time_ms = time_now - parameters->last_timestamp_;
    parameters->last_timestamp_ = time_now;
    parameters->get_last_point().timestamp = (time_now - delta_measurement_time_ms / 2.0) / 1000.0;
}

// adds a new measurement to the circular buffer that holds the data.
void GuideGaussianProcess::HandleMeasurements(double input)
{
    parameters->get_last_point().measurement = input;
}

void GuideGaussianProcess::HandleControls(double control_input)
{
    parameters->get_last_point().control = control_input;
}

void GuideGaussianProcess::HandleSNR(double SNR)
{
    double standard_deviation = 1e6; // some large number
    if (SNR > 0.1)
    {
        SNR = std::max(SNR, 3.4); // limit the minimal SNR

        // this was determined by simulated experiments
        standard_deviation = 2.1752 * 1 / (SNR - 3.3) + 0.5;// -0.0212;
    }

    parameters->get_last_point().variance = standard_deviation * standard_deviation;
}

void GuideGaussianProcess::UpdateGP()
{
    clock_t begin = std::clock();

    int N = parameters->get_number_of_measurements();

    // initialize the different vectors needed for the GP
    Eigen::VectorXd timestamps(N-1);
    Eigen::VectorXd measurements(N-1);
    Eigen::VectorXd variances(N-1);
    Eigen::VectorXd sum_controls(N-1);


    double sum_control = 0;

    // transfer the data from the circular buffer to the Eigen::Vectors
    int j = 0;
    for(size_t i = 0; i < N-1; i++)
    {
        sum_control += parameters->circular_buffer_parameters[i].control; // sum over the control signals
        if (parameters->circular_buffer_parameters[i].variance < 10000.0) // discard very uncertain measurements
        {
            timestamps(j) = parameters->circular_buffer_parameters[i].timestamp;
            measurements(j) = parameters->circular_buffer_parameters[i].measurement;
            variances(j) = parameters->circular_buffer_parameters[i].variance;
            sum_controls(j) = sum_control;
            ++j;
        }
    }
    timestamps.conservativeResize(j);
    measurements.conservativeResize(j);
    variances.conservativeResize(j);
    sum_controls.conservativeResize(j);

    Eigen::VectorXd gear_error(j);
    Eigen::VectorXd linear_fit(j);

    gear_error = sum_controls + measurements; // for each time step, add the residual error

    clock_t end = std::clock();
    double time_init = double(end - begin) / CLOCKS_PER_SEC;
    begin = std::clock();

    // linear least squares regression for offset and drift
    Eigen::MatrixXd feature_matrix(2, timestamps.rows());
    feature_matrix.row(0) = timestamps.array().pow(0); // easier to understand than ones
    feature_matrix.row(1) = timestamps.array(); // .pow(1) would be kinda useless

    // this is the inference for linear regression
    Eigen::VectorXd weights = (feature_matrix*feature_matrix.transpose()
    + 1e-3*Eigen::Matrix<double, 2, 2>::Identity()).ldlt().solve(feature_matrix*gear_error);

    // calculate the linear regression for all datapoints
    linear_fit = weights.transpose()*feature_matrix;

    // correct the datapoints by the polynomial fit
    Eigen::VectorXd gear_error_detrend = gear_error - linear_fit;

    end = std::clock();
    double time_detrend = double(end - begin) / CLOCKS_PER_SEC;
    begin = std::clock();

    double time_fft = 0;
    // optimize the hyperparameters if we have enough points already
    if (parameters->min_points_for_optimisation > 0
      && parameters->get_number_of_measurements() > parameters->min_points_for_optimisation)
    {
      // find periodicity parameter with FFT

      // compute Hamming window to prevent too much spectral leakage
      Eigen::VectorXd windowed_gear_error = gear_error_detrend.array() * math_tools::hamming_window(gear_error_detrend.rows()).array();

      // compute the spectrum
      int N_fft = 2048;
      std::pair<Eigen::VectorXd, Eigen::VectorXd> result = math_tools::compute_spectrum(windowed_gear_error, N_fft);

      Eigen::ArrayXd amplitudes = result.first;
      Eigen::ArrayXd frequencies = result.second;

      double dt = (timestamps(timestamps.rows()-1) - timestamps(1))/timestamps.rows();
      if (dt < 0)
      {
          Debug.AddLine("timestamps: last: %f, first: %f, rows: %f", timestamps(timestamps.rows() - 1), timestamps(1), timestamps.rows());
      }
      assert(dt >= 0);
      frequencies /= dt; // correct for the average time step width

      Eigen::ArrayXd periods = 1/frequencies.array();
      amplitudes = (periods > 1500.0).select(0,amplitudes); // set amplitudes to zero for too large periods

      assert(amplitudes.size() == frequencies.size());

      Eigen::VectorXd::Index maxIndex;
      amplitudes.maxCoeff(&maxIndex);
      double period_length = 1 / frequencies(maxIndex);

      Eigen::VectorXd optim = parameters->gp_.getHyperParameters();
      optim[7] = std::log(period_length); // parameters are stored in log space
      parameters->gp_.setHyperParameters(optim);

      end = std::clock();
      time_fft = double(end - begin) / CLOCKS_PER_SEC;


#if GP_DEBUG_FILE_
      std::ofstream outfile;
      outfile.open("spectrum_data.csv", std::ios_base::out);
      if (outfile.is_open()) {
          outfile << "period, amplitude\n";
          for (int i = 0; i < amplitudes.size(); ++i) {
              outfile << std::setw(8) << periods[i] << "," << std::setw(8) << amplitudes[i] << "\n";
          }
      }
      else {
          std::cout << "unable to write to file" << std::endl;
      }
      outfile.close();
#endif
    }

    begin = std::clock();
    // inference of the GP with this new points
    parameters->gp_.inferSD(timestamps, gear_error, parameters->points_for_approximation, variances);

    end = std::clock();
    double time_gp = double(end - begin) / CLOCKS_PER_SEC;
    Debug.AddLine("timings: init: %f, detrend: %f, fft: %f, gp: %f", time_init, time_detrend, time_fft, time_gp);
}

double GuideGaussianProcess::FilterState(double input, double noise)
{
    double drift_variance = 1.0;

    int delta_controller_time_ms = pFrame->RequestedExposureDuration();

    // prediction for the next location
    Eigen::VectorXd old_location(2);
    long time = parameters->get_last_point().timestamp;
    old_location << time / 1000.0,
    (time + delta_controller_time_ms) / 1000.0;
    GP::VectorMatrixPair prediction = parameters->gp_.predictProjected(old_location);

    Eigen::VectorXd mean = prediction.first;
    Eigen::MatrixXd var  = prediction.second;

    // the prediction is consisting of GP prediction and the linear drift
    double gp_prediction = mean(1) - mean(0);
    double gp_variance = var(0,0) + var(1,1) - var(0,1);

    double predictive_mean = parameters->mean_kf_ - parameters->get_last_point().control + gp_prediction;
    double predictive_var = parameters->var_kf_ + gp_variance + drift_variance;

    double residual = input - predictive_mean;

    double updated_mean = predictive_mean + predictive_var / ( predictive_var + noise) * residual;
    double updated_var = predictive_var - predictive_var / ( predictive_var + noise) * predictive_var;

    Debug.AddLine("KF info: old mean: %f, pred mean: %f, measurement: %f, residual: %f, new mean: %f", parameters->mean_kf_, predictive_mean, input, residual, updated_mean);
    Debug.AddLine("KF info: old var: %f, pred var: %f, noise: %f, gp_var %f, new var: %f", parameters->var_kf_, predictive_var, noise, gp_variance, updated_var);

    parameters->mean_kf_ = updated_mean;
    parameters->var_kf_ = updated_var;

    return updated_mean;
}

double GuideGaussianProcess::PredictGearError()
{
    int delta_controller_time_ms = pFrame->RequestedExposureDuration();

    // prediction for the next location
    Eigen::VectorXd next_location(2);
    long current_time = parameters->timer_.Time();
    next_location << current_time / 1000.0,
    (current_time + delta_controller_time_ms) / 1000.0;
    Eigen::VectorXd prediction = parameters->gp_.predictProjected(next_location).first;

    double p1 = prediction(1);
    double p0 = prediction(0);
    assert(!math_tools::isNaN(p1 - p0));

    // the prediction is consisting of GP prediction and the linear drift
    return (p1 - p0);
}

double GuideGaussianProcess::result(double input)
{
    HandleMeasurements(input);
    HandleTimestamps();
    HandleSNR(pFrame->pGuider->SNR());

    parameters->control_signal_ = parameters->control_gain_*input; // add the measured part of the controller

    // check if we are allowed to use the GP
    if (parameters->min_nb_element_for_inference > 0 &&
        parameters->get_number_of_measurements() > parameters->min_nb_element_for_inference)
    {
        UpdateGP(); // update the GP based on the new measurements
        parameters->control_signal_ = parameters->control_gain_*FilterState(input, parameters->get_last_point().variance); // filter the state based on the GP
        parameters->prediction_ = PredictGearError();
        parameters->control_signal_ += parameters->mixing_parameter_*parameters->prediction_; // mix in the prediction
    }

    parameters->add_one_point(); // add new point here, since the control is for the next point in time
    HandleControls(parameters->control_signal_);

// write the GP output to a file for easy analyzation
#if GP_DEBUG_FILE_
    int N = parameters->get_number_of_measurements();

    // initialize the different vectors needed for the GP
    Eigen::VectorXd timestamps(N - 1);
    Eigen::VectorXd measurements(N - 1);
    Eigen::VectorXd variances(N - 1);
    Eigen::VectorXd controls(N - 1);
    Eigen::VectorXd sum_controls(N - 1);
    Eigen::VectorXd gear_error(N - 1);
    Eigen::VectorXd linear_fit(N - 1);

    // transfer the data from the circular buffer to the Eigen::Vectors
    for(size_t i = 0; i < N-1; i++)
    {
        timestamps(i) = parameters->circular_buffer_parameters[i].timestamp;
        measurements(i) = parameters->circular_buffer_parameters[i].measurement;
        variances(i) = parameters->circular_buffer_parameters[i].variance;
        controls(i) = parameters->circular_buffer_parameters[i].control;
        sum_controls(i) = parameters->circular_buffer_parameters[i].control;
        if(i > 0)
        {
            sum_controls(i) += sum_controls(i-1); // sum over the control signals
        }
    }
    gear_error = sum_controls + measurements; // for each time step, add the residual error

    // inference of the GP with these new points
    parameters->gp_.inferSD(timestamps, gear_error, parameters->points_for_approximation, variances);

    int M = 512; // number of prediction points
    Eigen::VectorXd locations = Eigen::VectorXd::LinSpaced(M, 0, parameters->get_second_last_point().timestamp + 1500);

    std::pair<Eigen::VectorXd, Eigen::MatrixXd> predictions = parameters->gp_.predictProjected(locations);

    Eigen::VectorXd means = predictions.first;
    Eigen::VectorXd stds = predictions.second.diagonal().array().sqrt();

    std::ofstream outfile;
    outfile.open("measurement_data.csv", std::ios_base::out);
    if(outfile.is_open()) {
        outfile << "location, output\n";
        for( int i = 0; i < timestamps.size(); ++i) {
            outfile << std::setw(8) << timestamps[i] << "," << std::setw(8) << gear_error[i] << "\n";
        }
    } else {
        std::cout << "unable to write to file" << std::endl;
    }
    outfile.close();

    outfile.open("gp_data.csv", std::ios_base::out);
    if(outfile.is_open()) {
        outfile << "location, mean, std\n";
        for( int i = 0; i < locations.size(); ++i) {
            outfile << std::setw(8) << locations[i] << "," << std::setw(8) << means[i] << "," << std::setw(8) << stds[i] << "\n";
        }
    } else {
        std::cout << "unable to write to file" << std::endl;
    }
    outfile.close();
#endif

    assert(!math_tools::isNaN(parameters->control_signal_));
    return parameters->control_signal_;
}

double GuideGaussianProcess::deduceResult()
{
    HandleMeasurements(0);
    HandleTimestamps();
    HandleSNR(0);

    parameters->control_signal_ = 0;
    // check if we are allowed to use the GP
    if (parameters->min_nb_element_for_inference > 0 &&
        parameters->get_number_of_measurements() > parameters->min_nb_element_for_inference)
    {
        parameters->control_signal_ = parameters->control_gain_*FilterState(0, 1e6); // filter the state based on the GP
        parameters->prediction_ = PredictGearError();
        parameters->control_signal_ += parameters->prediction_; // control based on prediction
    }

    parameters->add_one_point(); // add new point here, since the applied control is important as well
    HandleControls(parameters->control_signal_);

    // write the GP output to a file for easy analyzation
#if GP_DEBUG_FILE_
    int N = parameters->get_number_of_measurements();

    // initialize the different vectors needed for the GP
    Eigen::VectorXd timestamps(N - 1);
    Eigen::VectorXd measurements(N - 1);
    Eigen::VectorXd variances(N - 1);
    Eigen::VectorXd controls(N - 1);
    Eigen::VectorXd sum_controls(N - 1);
    Eigen::VectorXd gear_error(N - 1);
    Eigen::VectorXd linear_fit(N - 1);

    // transfer the data from the circular buffer to the Eigen::Vectors
    for (size_t i = 0; i < N - 1; i++)
    {
        timestamps(i) = parameters->circular_buffer_parameters[i].timestamp;
        measurements(i) = parameters->circular_buffer_parameters[i].measurement;
        variances(i) = parameters->circular_buffer_parameters[i].variance;
        controls(i) = parameters->circular_buffer_parameters[i].control;
        sum_controls(i) = parameters->circular_buffer_parameters[i].control;
        if (i > 0)
        {
            sum_controls(i) += sum_controls(i - 1); // sum over the control signals
        }
    }
    gear_error = sum_controls + measurements; // for each time step, add the residual error

    // inference of the GP with these new points
    parameters->gp_.inferSD(timestamps, gear_error, parameters->points_for_approximation, variances);

    int M = 512; // number of prediction points
    Eigen::VectorXd locations = Eigen::VectorXd::LinSpaced(M, 0, parameters->get_second_last_point().timestamp + 1500);

    std::pair<Eigen::VectorXd, Eigen::MatrixXd> predictions = parameters->gp_.predictProjected(locations);

    Eigen::VectorXd means = predictions.first;
    Eigen::VectorXd stds = predictions.second.diagonal().array().sqrt();

    std::ofstream outfile;
    outfile.open("measurement_data.csv", std::ios_base::out);
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

    outfile.open("gp_data.csv", std::ios_base::out);
    if (outfile.is_open()) {
        outfile << "location, mean, std\n";
        for (int i = 0; i < locations.size(); ++i) {
            outfile << std::setw(8) << locations[i] << "," << std::setw(8) << means[i] << "," << std::setw(8) << stds[i] << "\n";
        }
    }
    else {
        std::cout << "unable to write to file" << std::endl;
    }
    outfile.close();
#endif

    assert(!math_tools::isNaN(parameters->control_signal_));
    return parameters->control_signal_;
}

void GuideGaussianProcess::reset()
{
    parameters->clear();
    return;
}
