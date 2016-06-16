/**
 * PHD2 Guiding
 *
 * @file
 * @date      2014-2016
 * @copyright Max Planck Society
 *
 * @author    Edgar D. Klenske <edgar.klenske@tuebingen.mpg.de>
 * @author    Stephan Wenninger <stephan.wenninger@tuebingen.mpg.de>
 * @author    Raffi Enficiaud <raffi.enficiaud@tuebingen.mpg.de>
 *
 * @brief     Provides a Gaussian process based guiding algorithm
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

#include "guide_algorithm_gaussian_process.h"
#include <wx/stopwatch.h>
#include <ctime>

#include "math_tools.h"

#include "math_tools.h"
#include "gaussian_process.h"
#include "covariance_functions.h"

class GuideAlgorithmGaussianProcess::GuideAlgorithmGaussianProcessDialogPane : public ConfigDialogPane
{
    GuideAlgorithmGaussianProcess *m_pGuideAlgorithm;
    wxSpinCtrlDouble *m_pControlGain;
    wxSpinCtrl       *m_pNbPointsInference;
    wxSpinCtrl       *m_pNbPointsPeriodComputation;
    wxSpinCtrl       *m_pNbPointsApproximation;

    wxSpinCtrlDouble *m_pSE0KLengthScale;
    wxSpinCtrlDouble *m_pSE0KSignalVariance;
    wxSpinCtrlDouble *m_pPKLengthScale;
    wxSpinCtrlDouble *m_pPKPeriodLength;
    wxSpinCtrlDouble *m_pPKSignalVariance;
    wxSpinCtrlDouble *m_pSE1KLengthScale;
    wxSpinCtrlDouble *m_pSE1KSignalVariance;
    wxSpinCtrlDouble *m_pPredictionGain;

    wxCheckBox       *m_checkboxComputePeriod;

    wxCheckBox       *m_checkboxDarkMode;

public:
    GuideAlgorithmGaussianProcessDialogPane(wxWindow *pParent, GuideAlgorithmGaussianProcess *pGuideAlgorithm)
      : ConfigDialogPane(_("Gaussian Process Guide Algorithm"),pParent)
    {
        m_pGuideAlgorithm = pGuideAlgorithm;

        int width = StringWidth(_T("000.00"));
        m_pControlGain = pFrame->MakeSpinCtrlDouble(pParent, wxID_ANY, _T("foo2"),
            wxPoint(-1, -1), wxSize(width, -1),
                                              wxSP_ARROW_KEYS, 0.0, 1.0, 0.0, 0.05,
                                              _T("Control Gain"));
        m_pControlGain->SetDigits(2);

        m_pPredictionGain = new wxSpinCtrlDouble(pParent, wxID_ANY, wxEmptyString,
                                                  wxDefaultPosition,wxSize(width+30, -1),
                                                  wxSP_ARROW_KEYS, 0.0, 1.0, 0.8, 0.01);
        m_pPredictionGain->SetDigits(2);

        // number of elements before starting the inference
        m_pNbPointsInference = new wxSpinCtrl(pParent, wxID_ANY, wxEmptyString,
                                             wxDefaultPosition,wxSize(width+30, -1),
                                             wxSP_ARROW_KEYS, 0, 1000, 10);

        m_pNbPointsPeriodComputation = new wxSpinCtrl(pParent, wxID_ANY, wxEmptyString,
                                                 wxDefaultPosition,wxSize(width+30, -1),
                                                 wxSP_ARROW_KEYS, 0, 1000, 10);

        // number of points used for the approximate GP inference (subset of data)
        m_pNbPointsApproximation = new wxSpinCtrl(pParent, wxID_ANY, wxEmptyString,
                                                 wxDefaultPosition,wxSize(width+30, -1),
                                                 wxSP_ARROW_KEYS, 0, 2000, 10);

        // hyperparameters
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


        m_checkboxComputePeriod = new wxCheckBox(pParent, wxID_ANY, _T(""));

        m_checkboxDarkMode = new wxCheckBox(pParent, wxID_ANY, _T(""));

        DoAdd(_("Control Gain"), m_pControlGain,
              _("The control gain defines how aggressive the controller is. It is the amount of pointing error that is "
                "fed back to the system. Default = 0.8"));

        DoAdd(_("Prediction gain"), m_pPredictionGain,
              _("The prediction gain defines how much control signal is generated from the prediction. Default = 1.0"));

        DoAdd(_("Minimum data points (inference)"), m_pNbPointsInference,
              _("Minimal number of measurements to start using the Gaussian process. If there are too little data points, "
                "the result might be poor. Default = 25"));

        DoAdd(_("Minimum data points (period)"), m_pNbPointsPeriodComputation,
              _("Minimal number of measurements to start estimating the periodicity. If there are too little data points, "
                "the estimation might not work. Default = 100"));

        DoAdd(_("Used data points (approximation)"), m_pNbPointsApproximation,
              _("Number of data points used in the approximation. Both prediction accuracy as well as runtime rise with "
                "the number of datapoints. Default = 100"));

        // hyperparameters
        DoAdd(_("Length scale [long range]"), m_pSE0KLengthScale,
              _("The length scale of the large non-periodic structure in the error. This is essentially a high-pass "
                "filter and the length scale defines the corner frequency. Default = 500.0"));
        DoAdd(_("Signal Variance [long range]"), m_pSE0KSignalVariance,
              _("Signal Variance of the long-term variations. Default = 10.0"));
        DoAdd(_("Length scale [periodic]"), m_pPKLengthScale,
              _("The length scale defines the \"wigglyness\" of the function. The smaller the length scale, the more "
                "structure can be learned. If chosen too small, some non-periodic structure might be picked up as well. "
                "Default = 0.5"));
        DoAdd(_("Period length [periodic]"), m_pPKPeriodLength,
              _("The period length of the periodic error component that should be corrected. It turned out that the shorter "
                "period is more important for the performance than the long one, if a telescope mount shows both. Default = 500.0"));
        DoAdd(_("Signal variance [periodic]"), m_pPKSignalVariance,
              _("The width of the periodic error. Should be around the amplitude of the PE curve, but is not a critical parameter. "
                "Default = 10.0"));
        DoAdd(_("Length scale [short range]"), m_pSE1KLengthScale,
              _("The length scale of the short range non-periodic parts of the gear error. This is essentially a low-pass "
                "filter and the length scale defines the corner frequency. Default = 5.0"));
        DoAdd(_("Signal Variance [short range]"), m_pSE1KSignalVariance,
              _("Signal Variance of the short-term variations. Default = 1.0"));

        DoAdd(_("Compute period"), m_checkboxComputePeriod,
              _("Compute period length with FFT. Default  = on"));

        DoAdd(_("Force dark tracking"), m_checkboxDarkMode, _("This is just for debugging and disabled by default"));
    }

    virtual ~GuideAlgorithmGaussianProcessDialogPane(void)
    {
      // no need to destroy the widgets, this is done by the parent...
    }

    /* Fill the GUI with the parameters that are currently chosen in the
     * guiding algorithm.
     */
    virtual void LoadValues(void)
    {
        m_pControlGain->SetValue(m_pGuideAlgorithm->GetControlGain());
        m_pNbPointsInference->SetValue(m_pGuideAlgorithm->GetNbPointsInference());
        m_pNbPointsPeriodComputation->SetValue(m_pGuideAlgorithm->GetNbPointsPeriodComputation());
        m_pNbPointsApproximation->SetValue(m_pGuideAlgorithm->GetNbPointsForApproximation());

        std::vector<double> hyperparameters = m_pGuideAlgorithm->GetGPHyperparameters();
        assert(hyperparameters.size() == 8);

        m_pSE0KLengthScale->SetValue(hyperparameters[1]);
        m_pSE0KSignalVariance->SetValue(hyperparameters[2]);
        m_pPKLengthScale->SetValue(hyperparameters[3]);
        m_pPKSignalVariance->SetValue(hyperparameters[4]);
        m_pSE1KLengthScale->SetValue(hyperparameters[5]);
        m_pSE1KSignalVariance->SetValue(hyperparameters[6]);
        m_pPKPeriodLength->SetValue(hyperparameters[7]);

        m_pPredictionGain->SetValue(m_pGuideAlgorithm->GetPredictionGain());

        m_checkboxComputePeriod->SetValue(m_pGuideAlgorithm->GetBoolComputePeriod());
    }

    // Set the parameters chosen in the GUI in the actual guiding algorithm
    virtual void UnloadValues(void)
    {
        m_pGuideAlgorithm->SetControlGain(m_pControlGain->GetValue());
        m_pGuideAlgorithm->SetNbPointsInference(m_pNbPointsInference->GetValue());
        m_pGuideAlgorithm->SetNbPointsPeriodComputation(m_pNbPointsPeriodComputation->GetValue());
        m_pGuideAlgorithm->SetNbPointsForApproximation(m_pNbPointsApproximation->GetValue());

        std::vector<double> hyperparameters(8);

        hyperparameters[1] = m_pSE0KLengthScale->GetValue();
        hyperparameters[2] = m_pSE0KSignalVariance->GetValue();
        hyperparameters[3] = m_pPKLengthScale->GetValue();
        hyperparameters[4] = m_pPKSignalVariance->GetValue();
        hyperparameters[5] = m_pSE1KLengthScale->GetValue();
        hyperparameters[6] = m_pSE1KSignalVariance->GetValue();
        hyperparameters[7] = m_pPKPeriodLength->GetValue();

        m_pGuideAlgorithm->SetGPHyperparameters(hyperparameters);
        m_pGuideAlgorithm->SetPredictionGain(m_pPredictionGain->GetValue());
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
struct GuideAlgorithmGaussianProcess::gp_guide_parameters
{
    typedef gp_guiding_circular_datapoints data_points;
    circular_buffer<data_points> circular_buffer_parameters;

    wxStopWatch timer_;
    double control_signal_;
    double control_gain_;
    double last_timestamp_;
    double prediction_gain_;
    double prediction_;
    double last_prediction_end_;

    int min_nb_element_for_inference;
    int min_points_for_period_computation;
    int points_for_approximation;

    bool compute_period;

    bool dark_tracking_mode_;

    bool dithering_active_;
    int dither_steps_;

    covariance_functions::PeriodicSquareExponential2 covariance_function_;
    covariance_functions::PeriodicSquareExponential output_covariance_function_;
    GP gp_;

    gp_guide_parameters() :
      circular_buffer_parameters(CIRCULAR_BUFFER_SIZE),
      timer_(),
      control_signal_(0.0),
      control_gain_(0.0),
      last_timestamp_(0.0),
      prediction_gain_(0.0),
      prediction_(0.0),
      last_prediction_end_(0.0),
      min_nb_element_for_inference(0),
      min_points_for_period_computation(0),
      points_for_approximation(0),
      compute_period(false),
      dark_tracking_mode_(false),
      dithering_active_(false),
      dither_steps_(0),
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
        last_prediction_end_ = 0;
        gp_.clearData();
    }

};


static const double DefaultControlGain                   = 0.8; // control gain
static const int    DefaultNbMinPointsForInference       = 25; // minimal number of points for doing the inference

static const double DefaultGaussianNoiseHyperparameter   = 1.0; // default Gaussian measurement noise

static const double DefaultLengthScaleSE0Ker             = 500.0; // length-scale of the long-range SE-kernel
static const double DefaultSignalVarianceSE0Ker          = 10.0; // signal variance of the long-range SE-kernel
static const double DefaultLengthScalePerKer             = 0.5; // length-scale of the periodic kernel
static const double DefaultPeriodLengthPerKer            = 500; // P_p, period-length of the periodic kernel
static const double DefaultSignalVariancePerKer          = 10.0; // signal variance of the periodic kernel
static const double DefaultLengthScaleSE1Ker             = 5.0; // length-scale of the short-range SE-kernel
static const double DefaultSignalVarianceSE1Ker          = 1.0; // signal variance of the short range SE-kernel

static const int    DefaultNbMinPointsForPeriodComputation = 100; // minimal number of points for doing the period identification
static const int    DefaultNbPointsForApproximation        = 100; // number of points used in the GP approximation
static const double DefaultPredictionGain                  = 1.0; // amount of GP prediction to blend in

static const bool   DefaultComputePeriod                 = true;

GuideAlgorithmGaussianProcess::GuideAlgorithmGaussianProcess(Mount *pMount, GuideAxis axis)
    : GuideAlgorithm(pMount, axis),
      parameters(0)
{
    parameters = new gp_guide_parameters();
    wxString configPath = GetConfigPath();

    double control_gain = pConfig->Profile.GetDouble(configPath + "/gp_control_gain", DefaultControlGain);
    SetControlGain(control_gain);

    int nb_element_for_inference = pConfig->Profile.GetInt(configPath + "/gp_min_points_inference", DefaultNbMinPointsForInference);
    SetNbPointsInference(nb_element_for_inference);

    int nb_points_period_computation = pConfig->Profile.GetInt(configPath + "/gp_min_points_period_computation", DefaultNbMinPointsForPeriodComputation);
    SetNbPointsPeriodComputation(nb_points_period_computation);

    int nb_points_approximation = pConfig->Profile.GetInt(configPath + "/gp_points_for_approximation", DefaultNbPointsForApproximation);
    SetNbPointsForApproximation(nb_points_approximation);

    double prediction_gain = pConfig->Profile.GetDouble(configPath + "/gp_prediction_gain", DefaultPredictionGain);
    SetPredictionGain(prediction_gain);

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

    parameters->dark_tracking_mode_ = false;

    reset();
}

GuideAlgorithmGaussianProcess::~GuideAlgorithmGaussianProcess(void)
{
    delete parameters;
}


ConfigDialogPane *GuideAlgorithmGaussianProcess::GetConfigDialogPane(wxWindow *pParent)
{
    return new GuideAlgorithmGaussianProcessDialogPane(pParent, this);
}

bool GuideAlgorithmGaussianProcess::SetControlGain(double control_gain)
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

bool GuideAlgorithmGaussianProcess::SetNbPointsInference(int nb_elements)
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

bool GuideAlgorithmGaussianProcess::SetNbPointsPeriodComputation(int nb_points)
{
    bool error = false;

    try
    {
        if (nb_points < 0)
        {
            throw ERROR_INFO("invalid number of points");
        }

        parameters->min_points_for_period_computation = nb_points;
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        error = true;
        parameters->min_points_for_period_computation = DefaultNbMinPointsForPeriodComputation;
    }

    pConfig->Profile.SetInt(GetConfigPath() + "/gp_min_points_period_computation", parameters->min_points_for_period_computation);

    return error;
}

bool GuideAlgorithmGaussianProcess::SetNbPointsForApproximation(int nb_points)
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

bool GuideAlgorithmGaussianProcess::SetGPHyperparameters(std::vector<double> const &hyperparameters)
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

bool GuideAlgorithmGaussianProcess::SetPredictionGain(double prediction_gain)
{
    bool error = false;

    try
    {
        if (prediction_gain < 0)
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

    pConfig->Profile.SetDouble(GetConfigPath() + "/gp_prediction_gain", parameters->prediction_gain_);

    return error;
}

bool GuideAlgorithmGaussianProcess::SetBoolComputePeriod(bool active)
{
  parameters->compute_period = active;
  pConfig->Profile.SetBoolean(GetConfigPath() + "/gp_compute_period", parameters->compute_period);
  return true;
}

double GuideAlgorithmGaussianProcess::GetControlGain() const
{
    return parameters->control_gain_;
}

int GuideAlgorithmGaussianProcess::GetNbPointsInference() const
{
    return parameters->min_nb_element_for_inference;
}

int GuideAlgorithmGaussianProcess::GetNbPointsPeriodComputation() const
{
    return parameters->min_points_for_period_computation;
}

int GuideAlgorithmGaussianProcess::GetNbPointsForApproximation() const
{
    return parameters->points_for_approximation;
}

std::vector<double> GuideAlgorithmGaussianProcess::GetGPHyperparameters() const
{
    // since the GP class works in log space, we have to exp() the parameters first.
    Eigen::VectorXd hyperparameters = parameters->gp_.getHyperParameters().array().exp();

    // we need to map the Eigen::vector into a std::vector.
    return std::vector<double>(hyperparameters.data(), // the first element is at the array address
                               hyperparameters.data() + 8); // 8 parameters, therefore the last is at position 7
}

double GuideAlgorithmGaussianProcess::GetPredictionGain() const
{
    return parameters->prediction_gain_;
}

bool GuideAlgorithmGaussianProcess::GetBoolComputePeriod() const
{
    return parameters->compute_period;
}

bool GuideAlgorithmGaussianProcess::GetDarkTracking()
{
    return parameters->dark_tracking_mode_;
}

bool GuideAlgorithmGaussianProcess::SetDarkTracking(bool value)
{
    parameters->dark_tracking_mode_ = value;
    return false;
}

wxString GuideAlgorithmGaussianProcess::GetSettingsSummary()
{
    static const char* format =
      "Control Gain = %.3f\n"
      "Prediction gain = %.3f\n"
      "Hyperparameters\n"
      "\tLength scale long range SE kernel = %.3f\n"
      "\tSignal variance long range SE kernel = %.3f\n"
      "\tLength scale periodic kernel = %.3f\n"
      "\tSignal variance periodic kernel = %.3f\n"
      "\tLength scale short range SE kernel = %.3f\n"
      "\tSignal variance short range SE kernel = %.3f\n"
      "\tPeriod length periodic kernel = %.3f\n"
      "FFT called every = %.3d points\n"
    ;

    Eigen::VectorXd hyperparameters = parameters->gp_.getHyperParameters();

    return wxString::Format(
      format,
      GetControlGain(),
      parameters->prediction_gain_,
      std::exp(hyperparameters(0)), std::exp(hyperparameters(1)),
      std::exp(hyperparameters(2)), std::exp(hyperparameters(3)),
      std::exp(hyperparameters(4)), std::exp(hyperparameters(5)),
      std::exp(hyperparameters(6)), std::exp(hyperparameters(7)),
      parameters->min_points_for_period_computation);
}


GUIDE_ALGORITHM GuideAlgorithmGaussianProcess::Algorithm(void)
{
    return GUIDE_ALGORITHM_GAUSSIAN_PROCESS;
}

void GuideAlgorithmGaussianProcess::HandleTimestamps()
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
void GuideAlgorithmGaussianProcess::HandleMeasurements(double input)
{
    parameters->get_last_point().measurement = input;
}

void GuideAlgorithmGaussianProcess::HandleDarkGuiding()
{
    parameters->get_last_point().measurement = 0; // we didn't actually measure
    parameters->get_last_point().variance = 1e4; // add really high noise
}

void GuideAlgorithmGaussianProcess::HandleControls(double control_input)
{
    parameters->get_last_point().control = control_input;
}

void GuideAlgorithmGaussianProcess::HandleSNR(double SNR)
{
    SNR = std::max(SNR, 3.4); // limit the minimal SNR

    // this was determined by simulated experiments
    double standard_deviation = 2.1752 * 1 / (SNR - 3.3) + 0.5;// -0.0212;

    parameters->get_last_point().variance = standard_deviation * standard_deviation;
}

void GuideAlgorithmGaussianProcess::UpdateGP()
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
    for(size_t i = 0; i < N-1; i++)
    {
        sum_control += parameters->circular_buffer_parameters[i].control; // sum over the control signals
        timestamps(i) = parameters->circular_buffer_parameters[i].timestamp;
        measurements(i) = parameters->circular_buffer_parameters[i].measurement;
        variances(i) = parameters->circular_buffer_parameters[i].variance;
        sum_controls(i) = sum_control;
    }

    Eigen::VectorXd gear_error(N-1);
    Eigen::VectorXd linear_fit(N-1);

    gear_error = sum_controls + measurements; // for each time step, add the residual error

    clock_t end = std::clock();
    double time_init = double(end - begin) / CLOCKS_PER_SEC;
    begin = std::clock();

    // linear least squares regression for offset and drift
    Eigen::MatrixXd feature_matrix(2, timestamps.rows());
    feature_matrix.row(0) = Eigen::MatrixXd::Ones(1, timestamps.rows()); // timestamps.pow(0)
    feature_matrix.row(1) = timestamps.array(); // timestamps.pow(1)

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
    // calculate period length if we have enough points already
    if (parameters->compute_period && parameters->min_points_for_period_computation > 0
      && parameters->get_number_of_measurements() > parameters->min_points_for_period_computation)
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
          Debug.AddLine("Something is wrong: The average time step length is is negative!");
          Debug.AddLine(wxString::Format("timestamps: last: %f, first: %f, rows: %f", timestamps(timestamps.rows() - 1), timestamps(1), timestamps.rows()));
          return;
      }

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
    // inference of the GP with this new points, maximum accuracy should be reached around current time
    parameters->gp_.inferSD(timestamps, gear_error, parameters->points_for_approximation, variances, parameters->timer_.Time() / 1000.0);

    end = std::clock();
    double time_gp = double(end - begin) / CLOCKS_PER_SEC;
    Debug.AddLine(wxString::Format("timings: init: %f, detrend: %f, fft: %f, gp: %f", time_init, time_detrend, time_fft, time_gp));
}

double GuideAlgorithmGaussianProcess::PredictGearError()
{
    int delta_controller_time_ms = pFrame->RequestedExposureDuration();

    if ( parameters->last_prediction_end_ < 1.0 ) // check if this is near zero
    {
        parameters->last_prediction_end_ = parameters->timer_.Time() / 1000.0;
    }

    // prediction from the last endpoint to the prediction point
    Eigen::VectorXd next_location(2);
    next_location << parameters->last_prediction_end_,
    (parameters->timer_.Time() + delta_controller_time_ms) / 1000.0;
    Eigen::VectorXd prediction = parameters->gp_.predictProjected(next_location).first;

    double p1 = prediction(1);
    double p0 = prediction(0);
    assert(std::abs(p1 - p0) < 100);
    assert(!math_tools::isNaN(p1 - p0));

    parameters->last_prediction_end_ = next_location(1); // store current endpoint

    // the prediction is consisting of GP prediction and the linear drift
    return (p1 - p0);
}

double GuideAlgorithmGaussianProcess::result(double input)
{
    if (parameters->dark_tracking_mode_ == true)
    {
        return deduceResult();
    }

    /*
    * Dithering behaves differently from pausing. During dithering, the mount
    * is moved and thus we can assume that we applied the perfect control, but
    * we cannot trust the measurement. Once dithering has settled, we can trust
    * the measurement again and we can pretend nothing has happend.
    */
    if (parameters->dithering_active_ == true)
    {
        parameters->dither_steps_--;
        if (parameters->dither_steps_ <= 0)
        {
            parameters->dithering_active_ = false;
        }
        deduceResult(); // just pretend we would do dark guiding...
        return parameters->control_gain_*input; // ...but apply proportional control
    }


    HandleMeasurements(input);
    HandleTimestamps();
    HandleSNR(pFrame->pGuider->SNR());

    parameters->control_signal_ = parameters->control_gain_*input; // add the measured part of the controller

    // check if we are allowed to use the GP
    if (parameters->min_nb_element_for_inference > 0 &&
        parameters->get_number_of_measurements() > parameters->min_nb_element_for_inference)
    {
        UpdateGP(); // update the GP based on the new measurements
        parameters->control_signal_ = parameters->control_gain_*input;
        parameters->prediction_ = PredictGearError();
        parameters->control_signal_ += parameters->prediction_gain_*parameters->prediction_; // mix in the prediction
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
    parameters->gp_.inferSD(timestamps, gear_error, parameters->points_for_approximation, variances, parameters->timer_.Time() / 1000.0);

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
    Debug.AddLine(wxString::Format("GP Guider generated %f from input %f.", parameters->control_signal_, input));

    assert(std::abs(parameters->control_signal_) < 100);
    assert(!math_tools::isNaN(parameters->control_signal_));
    return parameters->control_signal_;
}

double GuideAlgorithmGaussianProcess::deduceResult()
{
    HandleDarkGuiding();
    HandleTimestamps();

    parameters->control_signal_ = 0;
    // check if we are allowed to use the GP
    if (parameters->min_nb_element_for_inference > 0 &&
        parameters->get_number_of_measurements() > parameters->min_nb_element_for_inference)
    {
        UpdateGP(); // update the GP to update the SD approximation
        parameters->prediction_ = PredictGearError();
        parameters->control_signal_ += parameters->prediction_; // control based on prediction
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
    parameters->gp_.inferSD(timestamps, gear_error, parameters->points_for_approximation, variances, parameters->timer_.Time() / 1000.0);

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

void GuideAlgorithmGaussianProcess::reset()
{
    parameters->clear();
}

void GuideAlgorithmGaussianProcess::GuidingStopped(void)
{
    reset(); // reset is only done on a complete stop
}

void GuideAlgorithmGaussianProcess::GuidingPaused(void)
{
}

void GuideAlgorithmGaussianProcess::GuidingResumed(void)
{
    /**
     * During a pause, the following happens: The no guiding commands are
     * issued, therefore the error builds up. Since we need this erorr to show
     * up in the data for the GP and for the FFT, we have to hallucinate the
     * error from the GP predictions. Note that this error needs to be
     * "measured" and not "controlled", since we didn't move the telescope.
     */

    int delta_controller_time_ms = pFrame->RequestedExposureDuration();

    // we need a fake measurement for every missed real measurement
    while (parameters->timer_.Time() - parameters->last_prediction_end_*1000
                > delta_controller_time_ms)
    {
        HandleMeasurements(PredictGearError());
        HandleTimestamps();
        HandleSNR(1e2); // assume high noise, since we didn't really measure

        parameters->control_signal_ = 0; // we didn't issue control signals

        parameters->add_one_point(); // add new point here, since the control is for the next point in time
        HandleControls(parameters->control_signal_);
    }
}

void GuideAlgorithmGaussianProcess::GuidingDithered(double amt)
{
    /*
     * We don't compensate for the dither amout (yet), but we need to know
     * that we are currently dithering.
     */
    parameters->dithering_active_ = true;
    parameters->dither_steps_ = 10;
}

void GuideAlgorithmGaussianProcess::GuidingDitherSettleDone(void)
{
    /*
     * Once dithering has settled, we can start regular guiding again.
     */
    parameters->dithering_active_ = false;
    parameters->dither_steps_ = 0;
}
