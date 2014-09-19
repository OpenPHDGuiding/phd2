//
//  guide_gaussian_process.h
//  PHD
//
//  Created by Stephan Wenninger on 11/07/14.
//  Copyright (c) 2014 open-phd-guiding. All rights reserved.
//

#ifndef __PHD__guide_gaussian_process__
#define __PHD__guide_gaussian_process__


class GuideGaussianProcess : public GuideAlgorithm
{
protected:
    class GuideGaussianProcessDialogPane : public ConfigDialogPane
    {
        GuideGaussianProcess *m_pGuideAlgorithm;
    public:
        GuideGaussianProcessDialogPane(wxWindow *pParent, GuideGaussianProcess *pGuideAlgorithm);
        virtual ~GuideGaussianProcessDialogPane(void);
        
        virtual void LoadValues(void);
        virtual void UnloadValues(void);
    };
    
    
    
public:
    GuideGaussianProcess(Mount *pMount, GuideAxis axis);
    virtual ~GuideGaussianProcess(void);
    virtual GUIDE_ALGORITHM Algorithm(void);
    
    virtual ConfigDialogPane *GetConfigDialogPane(wxWindow *pParent);
    virtual double result(double input);
    virtual void reset();
    virtual wxString GetSettingsSummary() { return "\n"; }
    virtual wxString GetGuideAlgorithmClassName(void) const { return "Gaussian Process"; }
    
};

#endif /* defined(__PHD__guide_gaussian_process__) */
