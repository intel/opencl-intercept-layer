/*
// Copyright (c) 2018 Intel Corporation
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
*/

#pragma once

// MFC include files
#include <afxwin.h>
#include <afxdlgs.h>
#include <afxcmn.h>
#include <afxtempl.h>

#include <vector>

#include "resource.h"

#define WM_RESIZEPAGE WM_USER + 111

const TCHAR* REGISTRY_KEY = L"SOFTWARE\\INTEL\\IGFX\\CLINTERCEPT";

#define CL_TARGET_OPENCL_VERSION 220
#include <CL\cl.h>
#include "envVars.h"

enum ICON_STATE
{
    ICON_STATE_NONDEFAULT           = 0,
    ICON_STATE_MODIFIED_NONDEFAULT  = 1,
    ICON_STATE_MODIFIED_DEFAULT     = 2,
    ICON_STATE_DEFAULT              = 3,
    ICON_STATE_SEPARATOR            = 4,
    NUM_ICON_STATES
};

struct CVariableState
{
    CVariableState();

    std::vector<ICON_STATE>     CurrentIconState;
    std::vector<int>            CurrentIntValue;
    std::vector<std::wstring>   CurrentStringValue;

    void SetDefaultStates();
    void UpdateIconStates();
};

class CControlsPage : public CPropertyPage
{
public:
    CControlsPage( CVariableState* pVariableState );

    void ApplyChanges();

private:
    CVariableState* m_pVariableState;

    CImageList  m_ImageList;
    CListCtrl*  m_pListCtrl;
    
    int         m_SelectedItem;

    void ResetDefaults();

    void ReadSettingsFromRegistry();
    void WriteSettingsToRegistry() const;
    void WriteSettingsToRegistryHelper( HKEY key ) const;

    void UpdateIcons() const;

    void UpdateVarState(int vIndex);
    void UpdateControl(int vIndex);
    void UpdateIcon(int vIndex) const;

    BOOL OnInitDialog();

    afx_msg void OnVarChangeBool();
    afx_msg void OnVarChangeEdit();
    afx_msg void OnSelectionChange();
    afx_msg void OnNotify( NMHDR * pNotifyStruct, LRESULT * result );

    DECLARE_MESSAGE_MAP()
};

class CAboutPage : public CPropertyPage
{
public:
    CAboutPage( CVariableState* pVariableState );

private:
    CVariableState* m_pVariableState;

    std::vector<cl_platform_id> m_Platforms;
    std::vector<cl_device_id>   m_Devices;
    
    BOOL OnInitDialog();
    BOOL OnSetActive();

    afx_msg void OnPlatformListChange();
    afx_msg void OnDeviceListChange();

    DECLARE_MESSAGE_MAP()
};

class CCLInterceptConfigSheet : public CPropertySheet
{
public:
    CVariableState  m_VariableState;

    CAboutPage      m_AboutPage;
    CControlsPage   m_UserPage;

    POINT m_baseSize;
    POINT m_lastSize;
    
    // constructor
    CCLInterceptConfigSheet() : 
        CPropertySheet( L"Intercept Layer for OpenCL Applications Configuration App"),
        m_AboutPage( &m_VariableState ),
        m_UserPage( &m_VariableState )
    {
        m_baseSize.x = m_baseSize.y = 0;

        AddPage( &m_UserPage );
        AddPage( &m_AboutPage );
    }
    
    BOOL OnInitDialog();
    void OnSize(UINT nType, int cx, int cy);
    BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);

    afx_msg void OnOK();
    afx_msg void OnApplyNow();
    afx_msg LRESULT OnResizePage(WPARAM wParam, LPARAM lParam);
    afx_msg void OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI);
    
    DECLARE_MESSAGE_MAP()
};

// the main configApp class is derived from a CWinApp
// application class
class CLInterceptConfigApp : public CWinApp
{
public:
    HANDLE  hMutex;      // to allow only one instance
    DWORD   mutexState;  // 
    
    CLInterceptConfigApp();
    ~CLInterceptConfigApp();
    
    BOOL IsAnotherInstanceRunning();
    
    virtual BOOL InitInstance();
};
