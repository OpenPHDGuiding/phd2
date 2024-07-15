/*
 *  guide_algorithm_zfilter.h
 *  PHD Guiding
 *
 *  Created by Ken Self
 *  Copyright (c) 2018 Ken Self
 *  All rights reserved.
 *
 *  This source code is distributed under the following "BSD" license
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *    Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *    Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *    Neither the name of openphdguiding.org nor the names of its
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
 *
 */

#ifndef GUIDE_ALGORITHM_ZFILTER_H_INCLUDED
#define GUIDE_ALGORITHM_ZFILTER_H_INCLUDED

#include "zfilterfactory.h"

class GuideAlgorithmZFilter : public GuideAlgorithm
{
    FILTER_DESIGN m_design;
    std::vector<double> m_xv, m_yv;  // Historical values up to m_order
    std::vector<double> m_xcoeff, m_ycoeff;
    int m_order;
    double m_gain;
    double m_minMove;
    double m_sumCorr; // Sum of all corrections issued
    double m_expFactor;

    ZFilterFactory *m_pFactory;
protected:
    class GuideAlgorithmZFilterConfigDialogPane : public ConfigDialogPane
    {
        GuideAlgorithmZFilter *m_pGuideAlgorithm;
        wxSpinCtrlDouble *m_pExpFactor;
        wxSpinCtrlDouble *m_pMinMove;

    public:
        GuideAlgorithmZFilterConfigDialogPane(wxWindow *pParent, GuideAlgorithmZFilter *pGuideAlgorithm);
        virtual ~GuideAlgorithmZFilterConfigDialogPane();

        void LoadValues() override;
        void UnloadValues() override;
        void OnImageScaleChange() override;
        void EnableDecControls(bool enable) override;
    };

    class GuideAlgorithmZFilterGraphControlPane : public GraphControlPane
    {
    public:
        GuideAlgorithmZFilterGraphControlPane(wxWindow *pParent, GuideAlgorithmZFilter *pGuideAlgorithm, const wxString& label);
        ~GuideAlgorithmZFilterGraphControlPane();
        void EnableDecControls(bool enable) override;

    private:
        GuideAlgorithmZFilter *m_pGuideAlgorithm;
        wxSpinCtrlDouble *m_pMinMove;
        wxSpinCtrlDouble *m_pExpFactor;

        void OnMinMoveSpinCtrlDouble(wxSpinDoubleEvent& evt);
        void OnExpFactorSpinCtrlDouble(wxSpinDoubleEvent& evt);
    };

    bool BuildFilter();
    double GetMinMove() const override;
    bool SetMinMove(double minMove) override;
    double GetExpFactor() const;
    bool SetExpFactor(double expfactor);

    friend class GuideAlgorithmZFilterConfigDialogPane;

public:
    GuideAlgorithmZFilter(Mount *pMount, GuideAxis axis);
    virtual ~GuideAlgorithmZFilter();
    GUIDE_ALGORITHM Algorithm() const override;

    void reset() override;
    double result(double input) override;
    ConfigDialogPane *GetConfigDialogPane(wxWindow *pParent) override;
    GraphControlPane *GetGraphControlPane(wxWindow *pParent, const wxString& label) override;
    wxString GetSettingsSummary() const override;
    wxString GetGuideAlgorithmClassName() const override { return "ZFilter"; }
    void GetParamNames(wxArrayString& names) const override;
    bool GetParam(const wxString& name, double *val) const override;
    bool SetParam(const wxString& name, double val) override;
};

inline double GuideAlgorithmZFilter::GetMinMove() const
{
    return m_minMove;
}

inline double GuideAlgorithmZFilter::GetExpFactor() const
{
    return m_expFactor;
}

#endif /* GUIDE_ALGORITHM_ZFILTER_H_INCLUDED */
