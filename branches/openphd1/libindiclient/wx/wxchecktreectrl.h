/////////////////////////////////////////////////////////////////////////////
// Name:        checktreectrl.h
// Purpose:     Checkable tree control
// Author:      Julian Smart
// Modified by:
// Created:     2007-06-04
// RCS-ID:      $Id: checktreectrl.h,v 1.3 2007/08/17 23:23:43 anthemion Exp $
// Copyright:   (c) Julian Smart
// Licence:
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_CHECKTREECTRL_H_
#define _WX_CHECKTREECTRL_H_

#include "wx/treectrl.h"

// Type of item

#define wxCHECKTREE_ICON_CHILD       0x01
#define wxCHECKTREE_ICON_FOLDER      0x02

// Identifier of the icon
#define wxCHECKTREE_IMAGE_CHILD_CHECK_ENABLED     0
#define wxCHECKTREE_IMAGE_CHILD_CHECK_DISABLED    1
#define wxCHECKTREE_IMAGE_CHILD_UNCHECKED_ENABLED   2
#define wxCHECKTREE_IMAGE_CHILD_UNCHECKED_DISABLED  3

#if 0
#define wxCHECKTREE_IMAGE_FOLDER_CHECK_ENABLED    4
#define wxCHECKTREE_IMAGE_FOLDER_CHECK_DISABLED   5
#define wxCHECKTREE_IMAGE_FOLDER_UNCHECKED_ENABLED  6
#define wxCHECKTREE_IMAGE_FOLDER_UNCHECKED_DISABLED 7
#endif

/*!
 * wxCheckTreeItemData
 * Holds the data for each tree item.
 */

class wxCheckTreeItemData: public wxTreeItemData
{
public:
    wxCheckTreeItemData() { m_checked = false; m_enabled = true; m_iconType = wxCHECKTREE_ICON_CHILD; }
    ~wxCheckTreeItemData()  {}

    void SetChecked(bool checked) { m_checked = checked; }
    bool GetChecked() const { return m_checked; }

    void SetEnabled(bool enabled) { m_enabled = enabled; }
    bool GetEnabled() const { return m_enabled; }

    void SetIconType(int iconType) { m_iconType = iconType; }
    int GetIconType() const { return m_iconType; }

    void SetTranslatedLabel(const wxString& label) { m_translatedLabel = label; }
    const wxString& GetTranslatedLabel() const { return m_translatedLabel; }

    void SetUntranslatedLabel(const wxString& label) { m_untranslatedLabel = label; }
    const wxString& GetUntranslatedLabel() const { return m_untranslatedLabel; }

private:
    bool    m_checked;
    bool    m_enabled;
    int     m_iconType;

    wxString    m_untranslatedLabel;
    wxString    m_translatedLabel;
};


/*!
 * wxCheckTreeCtrl
 * The options hierarchy viewer.
 */

class wxCheckTreeCtrl: public wxTreeCtrl
{
    DECLARE_CLASS(wxCheckTreeCtrl)
public:
    wxCheckTreeCtrl(wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pt = wxDefaultPosition,
        const wxSize& sz = wxDefaultSize, long style = wxTR_HAS_BUTTONS);
    ~wxCheckTreeCtrl();

//// Event handlers    
    void OnMouseEvent(wxMouseEvent& event);
    void OnKeyDown(wxKeyEvent& event);

//// Accessors

    wxImageList* GetImageList() const { return m_imageList; }

//// Operations

    /// Add an item
    wxTreeItemId AddCheckedItem(wxTreeItemId& parent, const wxString& label, bool checked = false);

    /// Add an item with separate translated and untranslated labels
    wxTreeItemId AddCheckedItem(wxTreeItemId& parent, const wxString& translatedLabel, const wxString& untranslatedLabel, bool checked = false);

    /// Check/uncheck the item
    bool CheckItem(wxTreeItemId& item, bool check);

    /// Enable/disable the item
    bool EnableItem(wxTreeItemId& item, bool enable);

    /// Load the icons
    bool LoadIcons();

    /// Set the appropriate icon
    bool SetIcon(wxTreeItemId& item);

    /// Get the data for the item
    wxCheckTreeItemData* GetData(wxTreeItemId& item);

protected:
    wxImageList*        m_imageList;

    DECLARE_EVENT_TABLE()
};

class wxCheckTreeEvent: public wxNotifyEvent
{
public:
    wxCheckTreeEvent(wxEventType commandType = wxEVT_NULL, int id = 0):
                 wxNotifyEvent(commandType, id)
    {
        m_checked = false;
        m_data = NULL;
    }

    void SetChecked(bool checked) { m_checked = checked; }
    bool IsChecked() const { return m_checked; }

    wxTreeItemId GetTreeItemId() const { return m_treeItemId; }
    void SetTreeItemId(wxTreeItemId id) { m_treeItemId = id; }

    wxCheckTreeItemData* GetData() const { return m_data; }
    void SetData(wxCheckTreeItemData* data) { m_data = data; }

private:

    bool                    m_checked;
    wxTreeItemId            m_treeItemId;
    wxCheckTreeItemData*    m_data;

    DECLARE_DYNAMIC_CLASS(wxCheckTreeEvent);
};

typedef void (wxEvtHandler::*wxCheckTreeEventFunction)(wxCheckTreeEvent&);

// ----------------------------------------------------------------------------
// swatch control events and macros for handling them
// ----------------------------------------------------------------------------

BEGIN_DECLARE_EVENT_TYPES()
    DECLARE_EVENT_TYPE(wxEVT_COMMAND_CHECKTREECTRL_TOGGLED, 900)
END_DECLARE_EVENT_TYPES()

#define EVT_CHECKTREECTRL_TOGGLED(id, fn) DECLARE_EVENT_TABLE_ENTRY( wxEVT_COMMAND_CHECKTREECTRL_TOGGLED, id, -1, (wxObjectEventFunction) (wxEventFunction) (wxCheckTreeEventFunction) & fn, (wxObject *) NULL ),

#endif
// _WB_CHECKTREECTRL_H_
