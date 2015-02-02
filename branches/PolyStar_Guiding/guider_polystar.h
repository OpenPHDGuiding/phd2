/*
 *  guider_onestar.h
 *  PHD Guiding
 *
 *  Created by Craig Stark.
 *  Copyright (c) 2006-2010 Craig Stark.
 *  All rights reserved.
 *
 *  Refactored by Bret McKee
 *  Copyright (c) 2012 Bret McKee
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
 *    Neither the name of Bret McKee, Dad Dog Development,
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
 *
 */

#ifndef GUIDER_POLYSTAR_H_INCLUDED
#define GUIDER_POLYSTAR_H_INCLUDED

#include "polystar.h"
// #include "guider_onestar.h"

class MassChecker;

//class GuiderPolyStar : public Guider
class GuiderPolyStar : public GuiderOneStar
{
private:
	enum {
		MIN_SNR = 3,		// Lowest value allowed in MinSNR and MaxSNR spinners
		DEF_SNR = 20,		// Default value for MinSNR spinner
		MAX_SNR = 80,		// Highest value allowed in MinSNR and MaxSNR spinners
		MIN_STARS = 2,		// Lowest value allowed in MaxStars spinner
		DEF_STARS = 10,		// Default value for MaxStars spinner
		MAX_STARS = 30,		// Highest value allowed in MaxStars spinner
		MIN_MASS = 200,		// Lowest value allowed MinMass spinner
		DEF_MASS = 200,		// Default value for MinMass spinner
		MAX_MASS = 16385,	// Highest value allowed in MinMass spinner
		MIN_BGS	= 2,		// Lowest value allowed in BGSigma spinner
		DEF_BGS = 3,		// Default value for BGSigma spinner
		MAX_BGS = 5,		// Highest value allowed in BGSigma spinner
	};

	// Multi-Star Guiding Advance Setup Panel parameters
	bool	m_failOneStar	= true;			// Fail over to OneStar guiding if we can't get enough stars for a polygon
	bool	m_autoSNR		= true;			// Automatically select stars in polygon based on the SNR of the best guide star candidate
	int		m_maxStars		= DEF_STARS;	// Maximum number of stars in the guiding polygon
	double	m_minSNR		= DEF_SNR;		// Minimum SNR for inclusion into guiding polygon
	double	m_maxSNR		= MAX_SNR;		// Maximum SNR for inclusion into guiding polygon (mostly for testing)
	double	m_minMass		= DEF_MASS;		// Minimum star mass for inclusion into guiding polygon
	int		m_BGSigma		= DEF_BGS;		// Sigma limit above image mean for noise

	// Private class member variables
	StarList	m_starList;					// Stars on the Image
	PolyStar	m_polyStar;					// Polygon of guide stars
	Star		m_virtualGuideStar;			// Fake star on which guiding should be done (at the centroid of the star polygon)
	bool		m_rotation;					// Will the field rotation significantly during the guiding session
	bool		m_guideOneStar;				// PolyStar not available--guide on one star

	void			SaveStarFITS2();			// We would really like to just call the one GuiderOneStar(), but it's private and it
												//   access m_star.
	virtual bool	UpdateCurrentPosition(usImage *pImage, FrameDroppedInfo *errorInfo);
	virtual bool	SetLockPosition(const PHD_Point& lockPos);    
	virtual bool	SetCurrentPosition(usImage *pImage, const PHD_Point& position);

	// Adding this to support testing the PolyStarLog.  If we keep it then we will need to update it for dithering, etc.
	//   Right now, I'm going to set it as a result of the autofind and use it when calling LogGuiding.
	PHD_Point	m_guideLockPosition;		// Position that will act as the guide lock (this should be the initial centroid of the star polygon)


#ifdef KOR_CONV_ABSTRACT_CLASS
    Star			m_star;
    MassChecker*	m_massChecker	= NULL;

    // parameters
    bool	m_massChangeThresholdEnabled;
    double	m_massChangeThreshold;
    int		m_searchRegion;				// how far u/d/l/r do we do the initial search for a star
	bool	m_rotation;					// Will the field rotation significantly during the guiding session
#endif
protected:

	class GuiderPolyStarConfigDialogSubPane : public ConfigDialogPane
	{
	private:

		GuiderPolyStar*	m_pGuiderPolyStar;

		wxCheckBox*		m_pFailOneStar;
		wxCheckBox*		m_pAutoSNR;
		wxSpinCtrl*		m_pMaxStars;
		wxSpinCtrl*		m_pMinSNR;
		wxSpinCtrl*		m_pMaxSNR;
		wxSpinCtrl*		m_pMinMass;
		wxSpinCtrl*		m_pBGSigma;

	protected:
	public:
		GuiderPolyStarConfigDialogSubPane(wxWindow* pParent, GuiderPolyStar *pGuider);
		~GuiderPolyStarConfigDialogSubPane(void);

		virtual void LoadValues(void);
		virtual void UnloadValues(void);

		void OnAutoSNRChecked(wxCommandEvent& event);
	};



	class GuiderPolyStarConfigDialogPane : public GuiderOneStarConfigDialogPane
	{
	private:
		GuiderPolyStarConfigDialogSubPane* m_polyStarParams;
	public:
        GuiderPolyStarConfigDialogPane(wxWindow *pParent, GuiderPolyStar *pGuider);
        ~GuiderPolyStarConfigDialogPane(void);

		virtual void LoadValues(void);
		virtual void UnloadValues(void);
    };

//	friend class GuiderPolyStarConfigDialogPane;

public:

    GuiderPolyStar(wxWindow *parent);
    virtual ~GuiderPolyStar(void);

	virtual bool	AutoSelect(void);
    virtual void	OnPaint(wxPaintEvent& evt);			//TODO - may be temporary--may be permanent, but definitely not the way it is now
    virtual const	PHD_Point& CurrentPosition(void);
	virtual bool	IsLocked(void);

    virtual ConfigDialogPane *GetConfigDialogPane(wxWindow *pParent);

	// Multi-Star Guiding Advanced Setup Panel access functions
	bool	GetFailOneStar(void)	{ return m_failOneStar; }
	int		GetMaxStars(void)		{ return m_maxStars; }
	bool	GetAutoSNR(void)		{ return m_autoSNR; }
	double	GetMinSNR(void)			{ return m_minSNR; }
	double	GetMaxSNR(void)			{ return m_maxSNR; }
	double	GetMinMass(void)		{ return m_minMass; }
	int		GetBGSigma(void)		{ return m_BGSigma; }

	void	SetFailOneStar(bool failOneStar);
	void	SetMaxStars(int maxStars);
	void	SetAutoSNR(bool autoSNR);
	void	SetMinSNR(double minSNR);
	void	SetMaxSNR(double maxSNR);
	void	SetMinMass(double minMass);
	void	SetBGSigma(int BGSigma);

    virtual void LoadProfileSettings(void);

    void			OnLClick(wxMouseEvent& evt);    
	virtual void	InvalidateCurrentPosition(bool fullReset = false);

    DECLARE_EVENT_TABLE()
#ifdef KOR_CONV_ABSTRACT_CLASS

    virtual bool IsLocked(void);

    virtual wxRect GetBoundingBox(void);
    virtual int GetMaxMovePixels(void);
    virtual double StarMass(void);
    virtual double SNR(void);
    virtual int StarError(void);
    virtual wxString GetSettingsSummary();


#endif

#ifdef KOR_CONV_ABSTRACT_CLASS
private:
    virtual bool IsValidLockPosition(const PHD_Point& pt);

    virtual bool UpdateCurrentPosition(usImage *pImage, FrameDroppedInfo *errorInfo);
    virtual bool SetCurrentPosition(usImage *pImage, const PHD_Point& position);





#endif
};

#endif /* GUIDER_POLYSTAR_H_INCLUDED */
