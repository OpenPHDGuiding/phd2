/////////////////////////////////////////////////////////////////////////////
// Name:        checktreectrl.cpp
// Purpose:     Checkable tree control
// Author:      Julian Smart
// Modified by:
// Created:     2007-06-04
// RCS-ID:      $Id: checktreectrl.cpp,v 1.3 2007/08/17 23:23:43 anthemion Exp $
// Copyright:   (c) Julian Smart
// Licence:
/////////////////////////////////////////////////////////////////////////////

#include "wx/icon.h"
#include "wx/imaglist.h"

// Include XPM icons
#include "bitmaps/checked.xpm"
#include "bitmaps/checked_dis.xpm"
#include "bitmaps/unchecked.xpm"
#include "bitmaps/unchecked_dis.xpm"

#include "wxchecktreectrl.h"

/*
 * wxCheckTreeCtrl
 */

IMPLEMENT_CLASS(wxCheckTreeCtrl, wxTreeCtrl)

BEGIN_EVENT_TABLE(wxCheckTreeCtrl, wxTreeCtrl)
    EVT_MOUSE_EVENTS(wxCheckTreeCtrl::OnMouseEvent)
    EVT_CHAR(wxCheckTreeCtrl::OnKeyDown)
END_EVENT_TABLE()

DEFINE_EVENT_TYPE(wxEVT_COMMAND_CHECKTREECTRL_TOGGLED)
IMPLEMENT_DYNAMIC_CLASS(wxCheckTreeEvent, wxNotifyEvent)

wxCheckTreeCtrl::wxCheckTreeCtrl(wxWindow* parent, wxWindowID id, const wxPoint& pt,
                                   const wxSize& sz, long style):
wxTreeCtrl(parent, id, pt, sz, style)
{
    LoadIcons();
}

/// Load the icons
bool wxCheckTreeCtrl::LoadIcons()
{
    m_imageList = new wxImageList(16, 16, true);
    AssignImageList(m_imageList);

    m_imageList->Add(wxIcon(checked_xpm));
    m_imageList->Add(wxIcon(checked_dis_xpm));
    m_imageList->Add(wxIcon(unchecked_xpm));
    m_imageList->Add(wxIcon(unchecked_dis_xpm));

#if 0
    m_imageList->Add(wxIcon(closedfolder_xpm), 0, true);
    m_imageList->Add(wxIcon(closedfolder_dis_xpm), 0, false);
#endif
    
    return true;
}

wxCheckTreeCtrl::~wxCheckTreeCtrl()
{
}

/// Set the appropriate icon
bool wxCheckTreeCtrl::SetIcon(wxTreeItemId& item)
{
    wxCheckTreeItemData* data = (wxCheckTreeItemData*) GetItemData(item);
    if (data)
    {
        int imageIndex = 0;
        if (data->GetChecked())
        {
            if (data->GetEnabled())
                imageIndex = wxCHECKTREE_IMAGE_CHILD_CHECK_ENABLED;
            else
                imageIndex = wxCHECKTREE_IMAGE_CHILD_CHECK_DISABLED;
        }
        else
        {
            if (data->GetEnabled())
                imageIndex = wxCHECKTREE_IMAGE_CHILD_UNCHECKED_ENABLED;
            else
                imageIndex = wxCHECKTREE_IMAGE_CHILD_UNCHECKED_DISABLED;
        }
        SetItemImage(item, imageIndex);

        return true;
    }
    else
        return false;
}


void wxCheckTreeCtrl::OnMouseEvent(wxMouseEvent& event)
{
    int flags = 0;
    wxTreeItemId item = HitTest(wxPoint(event.GetX(), event.GetY()), flags);
    
    if (event.LeftDown())
    {
        if (flags & wxTREE_HITTEST_ONITEMICON)
        {
            wxCheckTreeItemData* data = (wxCheckTreeItemData*) GetItemData(item);

            if (data && data->GetEnabled())
            {
                data->SetChecked(!data->GetChecked());
                SetIcon(item);

                wxCheckTreeEvent commandEvent(wxEVT_COMMAND_CHECKTREECTRL_TOGGLED, GetId());
                commandEvent.SetEventObject(this);
                commandEvent.SetTreeItemId(item);
                commandEvent.SetChecked(data->GetChecked());
                commandEvent.SetData(data);
                GetEventHandler()->ProcessEvent(commandEvent);
            }
        }
    }

    event.Skip();
}

void wxCheckTreeCtrl::OnKeyDown(wxKeyEvent& event)
{
    wxTreeItemId item = GetSelection();
    if (event.GetKeyCode() == WXK_SPACE)
    {
        if (item.IsOk())
        {
            wxCheckTreeItemData* data = (wxCheckTreeItemData*) GetItemData(item);

            if (data && data->GetEnabled())
            {
                data->SetChecked(!data->GetChecked());
                SetIcon(item);

                wxCheckTreeEvent commandEvent(wxEVT_COMMAND_CHECKTREECTRL_TOGGLED, GetId());
                commandEvent.SetEventObject(this);
                commandEvent.SetTreeItemId(item);
                commandEvent.SetChecked(data->GetChecked());
                commandEvent.SetData(data);
                GetEventHandler()->ProcessEvent(commandEvent);
            }
        }
    }
    else
    {
        event.Skip();
    }
}

/// Check/uncheck the item
bool wxCheckTreeCtrl::CheckItem(wxTreeItemId& item, bool check)
{
    wxCheckTreeItemData* data = (wxCheckTreeItemData*) GetItemData(item);
    
    if (data)
    {
        data->SetChecked(check);
        SetIcon(item);
    }
    return true;
}

/// Enable/disable the item
bool wxCheckTreeCtrl::EnableItem(wxTreeItemId& item, bool enable)
{
    wxCheckTreeItemData* data = (wxCheckTreeItemData*) GetItemData(item);
    
    if (data)
    {
        data->SetEnabled(enable);
        SetIcon(item);
    }
    return true;
}

/// Add an item
wxTreeItemId wxCheckTreeCtrl::AddCheckedItem(wxTreeItemId& parent, const wxString& label, bool checked)
{
    wxCheckTreeItemData* data = new wxCheckTreeItemData;
    data->SetChecked(checked);
    data->SetTranslatedLabel(label);
    data->SetUntranslatedLabel(label);
    wxTreeItemId id = AppendItem(parent, label, -1, -1, data);
    SetIcon(id);

    return id;
}

/// Add an item with separate translated and untranslated labels
wxTreeItemId wxCheckTreeCtrl::AddCheckedItem(wxTreeItemId& parent, const wxString& translatedLabel, const wxString& untranslatedLabel, bool checked)
{
    wxCheckTreeItemData* data = new wxCheckTreeItemData;
    data->SetChecked(checked);
    data->SetTranslatedLabel(translatedLabel);
    data->SetUntranslatedLabel(untranslatedLabel);
    wxTreeItemId id = AppendItem(parent, translatedLabel, -1, -1, data);
    SetIcon(id);

    return id;
}

/// Get the data for the item
wxCheckTreeItemData* wxCheckTreeCtrl::GetData(wxTreeItemId& item)
{
    wxCheckTreeItemData* data = (wxCheckTreeItemData*) GetItemData(item);
    return data;
}

