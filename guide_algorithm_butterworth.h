/*
 *  guide_algorithm_butterworth.h
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

#ifndef GUIDE_ALGORITHM_BUTTERWORTH_H_INCLUDED
#define GUIDE_ALGORITHM_BUTTERWORTH_H_INCLUDED

#include "filterfactory.h"

class GuideAlgorithmButterworth : public GuideAlgorithm
{
    int    m_filter;
    std::vector<double> m_xv, m_yv;  // Historical values up to m_order
    std::vector<double> m_xcoeff, m_ycoeff;
    int m_order;
    double m_gain;
    double m_minMove;

    FilterFactory *m_pFactory;
    class Filter
    {
    public:
        std::string name;
        int order;
        double corner;
        double gain;
        std::vector<double> ycoeffs;
        Filter(const char* a, int o, double c, double g, std::vector<double> y) :name(a), order(o), corner(c), gain(g), ycoeffs(y) {};
    };
    std::vector<Filter> c_Filter; // Filter options
    double calcxcoeff(int order, int n);

protected:
    class GuideAlgorithmButterworthConfigDialogPane : public ConfigDialogPane
    {
        GuideAlgorithmButterworth *m_pGuideAlgorithm;
        wxChoice         *m_pFilter;  // Listbox for filters
        wxSpinCtrlDouble *m_pMinMove;

    public:
        GuideAlgorithmButterworthConfigDialogPane(wxWindow *pParent, GuideAlgorithmButterworth *pGuideAlgorithm);
        virtual ~GuideAlgorithmButterworthConfigDialogPane(void);

        virtual void LoadValues(void);
        virtual void UnloadValues(void);
        virtual void HandleBinningChange(int oldBinVal, int newBinVal);
    };

    class GuideAlgorithmButterworthGraphControlPane : public GraphControlPane
    {
    public:
        GuideAlgorithmButterworthGraphControlPane(wxWindow *pParent, GuideAlgorithmButterworth *pGuideAlgorithm, const wxString& label);
        ~GuideAlgorithmButterworthGraphControlPane(void);

    private:
        GuideAlgorithmButterworth *m_pGuideAlgorithm;
        wxSpinCtrlDouble *m_pMinMove;

        void OnMinMoveSpinCtrlDouble(wxSpinDoubleEvent& evt);
    };

    int GetFilter(void);
    bool SetFilter(int Order);
    double GetMinMove(void);
    bool SetMinMove(double minMove);

    friend class GuideAlgorithmButterworthConfigDialogPane;

public:
    GuideAlgorithmButterworth(Mount *pMount, GuideAxis axis);
    virtual ~GuideAlgorithmButterworth(void);
    virtual GUIDE_ALGORITHM Algorithm(void);

    virtual void reset(void);
    virtual double result(double input);
    virtual ConfigDialogPane *GetConfigDialogPane(wxWindow *pParent);
    virtual GraphControlPane *GetGraphControlPane(wxWindow *pParent, const wxString& label);
    virtual wxString GetSettingsSummary();
    virtual wxString GetGuideAlgorithmClassName(void) const { return "Butterworth"; }
    virtual void GetParamNames(wxArrayString& names) const;
    virtual bool GetParam(const wxString& name, double *val);
    virtual bool SetParam(const wxString& name, double val);
};

inline int GuideAlgorithmButterworth::GetFilter(void)
{
    return m_filter;
}

inline double GuideAlgorithmButterworth::GetMinMove(void)
{
    return m_minMove;
}

inline double GuideAlgorithmButterworth::calcxcoeff(int order, int n)
{
    if (n == 0 || n == order) return 1.0;
    if (n > order) return 0.0;
    return calcxcoeff(order - 1, n - 1) + calcxcoeff(order - 1, n);
}

#endif /* GUIDE_ALGORITHM_BUTTERWORTH_H_INCLUDED */
