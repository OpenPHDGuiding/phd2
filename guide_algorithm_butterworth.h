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

class GuideAlgorithmButterworth : public GuideAlgorithm
{
    static const unsigned int max_order = 4;
    double m_xv[max_order + 1], m_yv[max_order + 1];

    double m_minMove;

protected:
    class GuideAlgorithmButterworthConfigDialogPane : public ConfigDialogPane
    {
        GuideAlgorithmButterworth *m_pGuideAlgorithm;
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

inline double GuideAlgorithmButterworth::GetMinMove(void)
{
    return m_minMove;
}

#endif /* GUIDE_ALGORITHM_BUTTERWORTH_H_INCLUDED */
