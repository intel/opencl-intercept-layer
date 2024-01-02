/*
// Copyright (c) 2018-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
*/

#include "CLIConfig.h"

#include <wingdi.h>

#include <sstream>

std::wstring ToWString( const std::string& str )
{
    int size_needed = MultiByteToWideChar( CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0 );
    std::wstring wstrTo( size_needed, 0 );
    MultiByteToWideChar( CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed );
    return wstrTo;
}
std::wstring ToWString( const std::vector<char>& v )
{
    int size_needed = MultiByteToWideChar( CP_UTF8, 0, &v[0], (int)v.size(), NULL, 0 );
    std::wstring wstrTo( size_needed, 0 );
    MultiByteToWideChar( CP_UTF8, 0, &v[0], (int)v.size(), &wstrTo[0], size_needed );
    return wstrTo;
}
std::string ToString( const std::wstring &wstr)
{
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo( size_needed, 0 );
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

CVariableState::CVariableState()
{
    CurrentIconState.resize(cNumVars, ICON_STATE_DEFAULT);
    CurrentIntValue.resize(cNumVars);
    CurrentStringValue.resize(cNumVars);

    SetDefaultStates();
}

// Initializes Variable State and Icon State to defaults
void CVariableState::SetDefaultStates()
{
    for (int i = 0; i < cNumVars; i++)
    {
        CurrentIntValue[i] = cVars[i].defIntValue;
        CurrentStringValue[i] = ToWString(cVars[i].defStrValue);
        CurrentIconState[i] =
            cVars[i].Type == CONTROL_TYPE_SEPARATOR ?
            ICON_STATE_SEPARATOR :
            ICON_STATE_DEFAULT;
    }
}

// Updates all Icon States from Variable States
void CVariableState::UpdateIconStates()
{
    for (int i = 0; i < cNumVars; i++)
    {
        switch (cVars[i].Type)
        {
        case CONTROL_TYPE_BOOL:
        case CONTROL_TYPE_INT:
            if (CurrentIntValue[i] == cVars[i].defIntValue)
            {
                CurrentIconState[i] = ICON_STATE_DEFAULT;
            }
            else
            {
                CurrentIconState[i] = ICON_STATE_NONDEFAULT;
            }
            break;
        case CONTROL_TYPE_STRING:
            if (CurrentStringValue[i] == ToWString(cVars[i].defStrValue))
            {
                CurrentIconState[i] = ICON_STATE_DEFAULT;
            }
            else
            {
                CurrentIconState[i] = ICON_STATE_NONDEFAULT;
            }
            break;
        case CONTROL_TYPE_SEPARATOR:
            CurrentIconState[i] = ICON_STATE_SEPARATOR;
            break;
        }
    }
}


CControlsPage::CControlsPage( CVariableState* pVariableState ) :
    CPropertyPage(IDD_VARS_PAGE, IDS_USER),
    m_pVariableState( pVariableState )
{
}

BOOL CControlsPage::OnInitDialog()
{
    CPropertyPage::OnInitDialog();

    HICON hIcon[NUM_ICON_STATES];

    m_ImageList.Create(16, 16, 0, NUM_ICON_STATES, 0);
    hIcon[ICON_STATE_NONDEFAULT] = AfxGetApp()->LoadIcon(IDI_STATE_NONDEFAULT);
    hIcon[ICON_STATE_MODIFIED_NONDEFAULT] = AfxGetApp()->LoadIcon(IDI_STATE_MODIFIED_NONDEFAULT);
    hIcon[ICON_STATE_MODIFIED_DEFAULT] = AfxGetApp()->LoadIcon(IDI_STATE_MODIFIED_DEFAULT);
    hIcon[ICON_STATE_DEFAULT] = AfxGetApp()->LoadIcon(IDI_STATE_DEFAULT);
    hIcon[ICON_STATE_SEPARATOR] = AfxGetApp()->LoadIcon(IDI_STATE_SEPARATOR);

    for( int i = 0; i < NUM_ICON_STATES; i++ )
    {
        m_ImageList.Add(hIcon[i]);
    }

    m_pListCtrl = (CListCtrl*)GetDlgItem(IDC_VAR_LIST);
    m_pListCtrl->SetImageList(&m_ImageList, LVSIL_SMALL);

    m_pListCtrl->InsertColumn(0, L"Header");
    for( int i = 0; i < cNumVars; i++ )
    {
        m_pListCtrl->InsertItem(
            i,
            ToWString( cVars[i].Name ).c_str(),
            m_pVariableState->CurrentIconState[i] );
    }
    m_pListCtrl->SetColumnWidth(0, LVSCW_AUTOSIZE);

    m_SelectedItem = 0;
    m_pListCtrl->SetFocus();
    m_pListCtrl->SetItemState(m_SelectedItem, LVIS_SELECTED, LVIS_SELECTED);

    // Read previous Variable State from the registry
    ReadSettingsFromRegistry();

    // Sync Icon State with Variable State
    m_pVariableState->UpdateIconStates();
    UpdateIcons();

    SetModified(FALSE);

    return TRUE;
}

// Updates all Icons based on Icon State
void CControlsPage::UpdateIcons() const
{
    for( int i = 0; i < cNumVars; i++ )
    {
        UpdateIcon(i);
    }
}

// Updates one Icon based on Icon State
void CControlsPage::UpdateIcon( int vIndex ) const
{
    m_pListCtrl->SetItem(
        vIndex,
        0,
        LVIF_TEXT | LVIF_IMAGE,
        ToWString( cVars[vIndex].Name ).c_str(),
        m_pVariableState->CurrentIconState[vIndex],
        0,
        0,
        0 );
}

void CControlsPage::ResetDefaults()
{
    m_pVariableState->SetDefaultStates();
    UpdateIcons();

    WriteSettingsToRegistry();

    UpdateControl(m_SelectedItem);

    SetModified(FALSE);
}

// Updates Variable State from the Registry
void CControlsPage::ReadSettingsFromRegistry()
{
    HKEY    key;

    LSTATUS success = RegOpenKeyEx(
        HKEY_CURRENT_USER,
        REGISTRY_KEY,
        0,
        KEY_READ,
        &key );
    if( success == ERROR_SUCCESS )
    {
        for( int i = 0; i < cNumVars; i++ )
        {
            std::vector<wchar_t>    regQuery(128);

            DWORD regQuerySize = (DWORD)(regQuery.size() * sizeof(regQuery[0]));

            success = RegQueryValueEx(
                key,
                ToWString(cVars[i].Name).c_str(),
                NULL,
                NULL,
                (LPBYTE)regQuery.data(),
                &regQuerySize );

            if( success == ERROR_SUCCESS )
            {
                switch (cVars[i].Type)
                {
                case CONTROL_TYPE_BOOL:
                    {
                        int* pInt = (int*)regQuery.data();
                        m_pVariableState->CurrentIntValue[i] = pInt[0] ? TRUE : FALSE;
                    }
                    break;
                case CONTROL_TYPE_INT:
                    {
                        int* pInt = (int*)regQuery.data();
                        m_pVariableState->CurrentIntValue[i] = pInt[0];
                    }
                    break;
                case CONTROL_TYPE_STRING:
                    {
                        m_pVariableState->CurrentStringValue[i].assign( regQuery.begin(), regQuery.end() );
                    }
                    break;
                case CONTROL_TYPE_SEPARATOR:
                    break;
                }

                m_pVariableState->CurrentIconState[i] = ICON_STATE_NONDEFAULT;
            }

        }

        RegCloseKey(key);
    }
}

// Writes Variable State to the Registry
void CControlsPage::WriteSettingsToRegistry() const
{
    HKEY    key;

    // 32-bit registry keys.
    LSTATUS success = RegCreateKeyEx(
        HKEY_CURRENT_USER,
        REGISTRY_KEY,
        0,
        NULL,
        REG_OPTION_NON_VOLATILE,
        KEY_SET_VALUE,
        NULL,
        &key,
        NULL );
    if( success == ERROR_SUCCESS )
    {
        WriteSettingsToRegistryHelper(key);
        RegCloseKey(key);
    }

    // 64-bit registry keys.  This probably isn't needed, but better
    // safe than sorry.
    success = RegCreateKeyEx(
        HKEY_CURRENT_USER,
        REGISTRY_KEY,
        0,
        NULL,
        REG_OPTION_NON_VOLATILE,
        KEY_WOW64_64KEY | KEY_SET_VALUE,
        NULL,
        &key,
        NULL );
    if( success == ERROR_SUCCESS )
    {
        WriteSettingsToRegistryHelper(key);
        RegCloseKey(key);
    }
}

void CControlsPage::WriteSettingsToRegistryHelper( HKEY key ) const
{
    for (int i = 0; i < cNumVars; i++)
    {
        switch (cVars[i].Type) {
        case CONTROL_TYPE_BOOL:
        case CONTROL_TYPE_INT:
            if( m_pVariableState->CurrentIntValue[i] != cVars[i].defIntValue )
            {
                DWORD   dwValue = (DWORD)m_pVariableState->CurrentIntValue[i];

                RegSetValueEx(
                    key,
                    ToWString(cVars[i].Name).c_str(),
                    0,
                    REG_DWORD,
                    (CONST BYTE *)&dwValue,
                    sizeof(DWORD));
            }
            else
            {
                RegDeleteValue(
                    key,
                    ToWString(cVars[i].Name).c_str() );
            }
            break;
        case CONTROL_TYPE_STRING:
            if( m_pVariableState->CurrentStringValue[i] != ToWString(cVars[i].defStrValue) )
            {
                RegSetValueEx(
                    key,
                    ToWString(cVars[i].Name).c_str(),
                    0,
                    REG_SZ,
                    (const BYTE*)m_pVariableState->CurrentStringValue[i].data(),
                    (DWORD)(m_pVariableState->CurrentStringValue[i].length() * sizeof(wchar_t)));
            }
            else
            {
                RegDeleteValue(
                    key,
                    ToWString(cVars[i].Name).c_str() );
            }
            break;
        case CONTROL_TYPE_SEPARATOR:
            break;
        }
    }
}

// Updates a Control Based on Current State
void CControlsPage::UpdateControl( int vIndex )
{
    CButton *cCurEnabled, *cDefEnabled;
    CEdit *cCurEdit, *cDefEdit;

    cCurEnabled = (CButton *)GetDlgItem(IDC_CHECK_CUR_ENABLED);
    cDefEnabled = (CButton *)GetDlgItem(IDC_CHECK_DEF_ENABLED);
    cCurEdit = (CEdit *)GetDlgItem(IDC_CUR_EDIT);
    cDefEdit = (CEdit *)GetDlgItem(IDC_DEF_EDIT);

    switch (cVars[vIndex].Type)
    {
    case CONTROL_TYPE_BOOL:
        cCurEnabled->ShowWindow(SW_SHOW);
        cDefEnabled->ShowWindow(SW_SHOW);
        cCurEdit->ShowWindow(SW_HIDE);
        cDefEdit->ShowWindow(SW_HIDE);

        if (m_pVariableState->CurrentIntValue[vIndex])
        {
            cCurEnabled->SetCheck(TRUE);
        }
        else
        {
            cCurEnabled->SetCheck(FALSE);
        }

        if (cVars[vIndex].defIntValue)
        {
            cDefEnabled->SetCheck(TRUE);
        }
        else
        {
            cDefEnabled->SetCheck(FALSE);
        }
        break;

        // For int and String variables, setting the
        // current and default value causes the ON_EN_CHANGE
        // method to get called spuriously setting the varsChanged flag
        // to workaround this, we first hide the edit control
        // set the text to the required value, and then show the control
        // The varChange method checks for visibility before doing anything
    case CONTROL_TYPE_INT:
        cCurEdit->ShowWindow(SW_HIDE);
        SetDlgItemInt(IDC_CUR_EDIT, m_pVariableState->CurrentIntValue[vIndex]);
        cCurEdit->ShowWindow(SW_SHOW);
        cDefEdit->ShowWindow(SW_SHOW);
        SetDlgItemInt(IDC_DEF_EDIT, cVars[vIndex].defIntValue);
        cCurEnabled->ShowWindow(SW_HIDE);
        cDefEnabled->ShowWindow(SW_HIDE);
        break;
    case CONTROL_TYPE_STRING:
        cCurEdit->ShowWindow(SW_HIDE);
        SetDlgItemText(IDC_CUR_EDIT, m_pVariableState->CurrentStringValue[vIndex].c_str());
        cCurEdit->ShowWindow(SW_SHOW);
        cDefEdit->ShowWindow(SW_SHOW);
        SetDlgItemText(IDC_DEF_EDIT, ToWString(cVars[vIndex].defStrValue).c_str());
        cCurEnabled->ShowWindow(SW_HIDE);
        cDefEnabled->ShowWindow(SW_HIDE);
        break;
    case CONTROL_TYPE_SEPARATOR:
        cCurEnabled->ShowWindow(SW_HIDE);
        cDefEnabled->ShowWindow(SW_HIDE);
        cCurEdit->ShowWindow(SW_HIDE);
        cDefEdit->ShowWindow(SW_HIDE);
        break;
    }

    // Set the help text for the variable
    SetDlgItemText(IDC_EDIT_HELP, ToWString(cVars[vIndex].HelpText).c_str());
}

// Updates a Variable State and Icon State based on a Control
void CControlsPage::UpdateVarState(int vIndex)
{
    CButton *cCurEnabled = (CButton*)GetDlgItem(IDC_CHECK_CUR_ENABLED);

    switch (cVars[vIndex].Type)
    {
    case CONTROL_TYPE_BOOL:
        if (cCurEnabled->GetCheck() == TRUE)
        {
            m_pVariableState->CurrentIntValue[vIndex] = TRUE;
        }
        else
        {
            m_pVariableState->CurrentIntValue[vIndex] = FALSE;
        }
        break;

    case CONTROL_TYPE_INT:
        m_pVariableState->CurrentIntValue[vIndex] = GetDlgItemInt(IDC_CUR_EDIT);
        break;

    case CONTROL_TYPE_STRING:
        {
            int length = GetDlgItem( IDC_CUR_EDIT )->GetWindowTextLength();

            m_pVariableState->CurrentStringValue[vIndex].resize( length );

            GetDlgItemText(
                IDC_CUR_EDIT,
                &m_pVariableState->CurrentStringValue[vIndex][0],
                (int)(m_pVariableState->CurrentStringValue[vIndex].size() * sizeof(wchar_t)));
        }
        break;

    case CONTROL_TYPE_SEPARATOR:
        break;
    }

    switch (cVars[vIndex].Type)
    {
    case CONTROL_TYPE_BOOL:
    case CONTROL_TYPE_INT:
        if (m_pVariableState->CurrentIntValue[vIndex] == cVars[vIndex].defIntValue)
        {
            if (m_pVariableState->CurrentIconState[vIndex] == ICON_STATE_MODIFIED_NONDEFAULT)
            {
                m_pVariableState->CurrentIconState[vIndex] = ICON_STATE_DEFAULT;
            }
            else if (m_pVariableState->CurrentIconState[vIndex] == ICON_STATE_NONDEFAULT)
            {
                m_pVariableState->CurrentIconState[vIndex] = ICON_STATE_MODIFIED_DEFAULT;
            }
        }
        else
        {
            if (m_pVariableState->CurrentIconState[vIndex] == ICON_STATE_DEFAULT)
            {
                m_pVariableState->CurrentIconState[vIndex] = ICON_STATE_MODIFIED_NONDEFAULT;
            }
            else if (m_pVariableState->CurrentIconState[vIndex] == ICON_STATE_MODIFIED_DEFAULT)
            {
                m_pVariableState->CurrentIconState[vIndex] = ICON_STATE_NONDEFAULT;
            }
        }
        break;
    case CONTROL_TYPE_STRING:
        if (m_pVariableState->CurrentStringValue[vIndex] == ToWString(cVars[vIndex].defStrValue))
        {
            if (m_pVariableState->CurrentIconState[vIndex] == ICON_STATE_MODIFIED_NONDEFAULT)
            {
                m_pVariableState->CurrentIconState[vIndex] = ICON_STATE_DEFAULT;
            }
            else if (m_pVariableState->CurrentIconState[vIndex] == ICON_STATE_NONDEFAULT)
            {
                m_pVariableState->CurrentIconState[vIndex] = ICON_STATE_MODIFIED_DEFAULT;
            }
        }
        else
        {
            if (m_pVariableState->CurrentIconState[vIndex] == ICON_STATE_DEFAULT)
            {
                m_pVariableState->CurrentIconState[vIndex] = ICON_STATE_MODIFIED_NONDEFAULT;
            }
            else if (m_pVariableState->CurrentIconState[vIndex] == ICON_STATE_MODIFIED_DEFAULT)
            {
                m_pVariableState->CurrentIconState[vIndex] = ICON_STATE_NONDEFAULT;
            }
        }
        break;

    case CONTROL_TYPE_SEPARATOR:
        m_pVariableState->CurrentIconState[vIndex] = ICON_STATE_SEPARATOR;
        break;
    }
}

// Handle the Apply or OK Buttons
void CControlsPage::ApplyChanges()
{
    WriteSettingsToRegistry();

    m_pVariableState->UpdateIconStates();
    UpdateIcons();

    SetModified(FALSE);
}

// Called when a boolean Control changes
void CControlsPage::OnVarChangeBool()
{
    CButton *cCurEnabled = (CButton *)GetDlgItem(IDC_CHECK_CUR_ENABLED);
    if (cCurEnabled->IsWindowVisible() == FALSE) {
        // If the window is not visible, do nothing
        return;
    }

    UpdateVarState(m_SelectedItem);
    UpdateIcon(m_SelectedItem);

    SetModified(TRUE);
}

// Called when a int/str Control changes
void CControlsPage::OnVarChangeEdit()
{
    CEdit *cCurEdit = (CEdit *)GetDlgItem(IDC_CUR_EDIT);
    if (cCurEdit->IsWindowVisible() == FALSE) {
        // If the window is not visible, do nothing
        return;
    }

    UpdateVarState(m_SelectedItem);
    UpdateIcon(m_SelectedItem);
    SetModified(TRUE);
}

// Called when the user selects a different item in the list
void CControlsPage::OnSelectionChange()
{
    POSITION p = m_pListCtrl->GetFirstSelectedItemPosition();
    int vIndex;

    if (p == NULL) {
        return;
    } else {
        vIndex = m_pListCtrl->GetNextSelectedItem(p);
    }

    UpdateControl(vIndex);
    m_SelectedItem = vIndex;
}

// handles changes in selection of a listCtrl Item
void CControlsPage::OnNotify( NMHDR * pNMHDR, LRESULT * pResult )
{
    NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

    // call the OnSelectionChange function
    // only if the state has LVIS_SELECTED

    if (pNMListView->uNewState & LVIS_SELECTED) {
        OnSelectionChange();
    }

    *pResult = 0;
}

BEGIN_MESSAGE_MAP(CControlsPage, CPropertyPage)
    ON_BN_CLICKED(IDC_CMD_DEFAULTS, ResetDefaults)
    ON_BN_CLICKED(IDC_CHECK_CUR_ENABLED, OnVarChangeBool)
    ON_EN_CHANGE(IDC_CUR_EDIT, OnVarChangeEdit)
    ON_NOTIFY(LVN_ITEMCHANGED, IDC_VAR_LIST, OnNotify)
END_MESSAGE_MAP()

///////////////////////////////////////////////////////////////////////////////

// ==========================================================================
// About page

typedef cl_int  (CL_API_CALL *pfnGetPlatformIDs) (
            cl_uint num_entries,
            cl_platform_id* platforms,
            cl_uint* num_platforms );
typedef cl_int  (CL_API_CALL *pfnGetPlatformInfo) (
            cl_platform_id platform,
            cl_platform_info param_name,
            size_t param_value_size,
            void* param_value,
            size_t* param_value_size_ret );
typedef cl_int  (CL_API_CALL *pfnGetDeviceIDs) (
            cl_platform_id platform,
            cl_device_type device_type,
            cl_uint num_entries,
            cl_device_id* devices,
            cl_uint* num_devices );
typedef cl_int  (CL_API_CALL *pfnGetDeviceInfo) (
            cl_device_id device,
            cl_device_info param_name,
            size_t param_value_size,
            void* param_value,
            size_t* param_value_size_ret );

CAboutPage::CAboutPage( CVariableState* pVariableState ) :
    CPropertyPage(IDD_ABOUT_PAGE, IDS_INFO),
    m_pVariableState( pVariableState )
{
}

BOOL CAboutPage::OnInitDialog()
{
    CPropertyPage::OnInitDialog();

    CComboBox*  pPlatformComboBox = (CComboBox*)GetDlgItem(IDC_PLATFORM_LIST);

    HMODULE hModule = ::LoadLibraryA( "opencl.dll" );
    if( hModule )
    {
        pfnGetPlatformIDs   dclGetPlatformIDs = (pfnGetPlatformIDs)::GetProcAddress( hModule, "clGetPlatformIDs" );
        pfnGetPlatformInfo  dclGetPlatformInfo = (pfnGetPlatformInfo)::GetProcAddress( hModule, "clGetPlatformInfo" );

        cl_int  errorCode = CL_SUCCESS;
        cl_uint numPlatforms = 0;

        if( errorCode == CL_SUCCESS &&
            dclGetPlatformIDs )
        {
            errorCode = dclGetPlatformIDs(
                0,
                NULL,
                &numPlatforms );
        }
        if( errorCode != CL_SUCCESS ||
            numPlatforms == 0 )
        {
            pPlatformComboBox->AddString(L"No OpenCL platforms detected!");
        }

        m_Platforms.resize( numPlatforms );

        if( errorCode == CL_SUCCESS &&
            dclGetPlatformIDs &&
            dclGetPlatformInfo &&
            numPlatforms != 0 )
        {
            errorCode = dclGetPlatformIDs(
                numPlatforms,
                m_Platforms.data(),
                NULL );

            for( cl_uint p = 0; p < numPlatforms; p++ )
            {
                size_t  stringLength = 0;
                if( errorCode == CL_SUCCESS )
                {
                    errorCode = dclGetPlatformInfo(
                        m_Platforms[p],
                        CL_PLATFORM_NAME,
                        0,
                        NULL,
                        &stringLength );
                }
                if( errorCode == CL_SUCCESS )
                {
                    std::vector<char>   str( stringLength );
                    errorCode = dclGetPlatformInfo(
                        m_Platforms[p],
                        CL_PLATFORM_NAME,
                        str.size(),
                        str.data(),
                        NULL );
                    if( errorCode == CL_SUCCESS )
                    {
                        pPlatformComboBox->AddString(
                            ToWString(str).c_str() );
                    }
                }
            }
        }

        ::FreeLibrary( hModule );
    }

    pPlatformComboBox->SetCurSel(0);

    OnPlatformListChange();
    OnDeviceListChange();

    return TRUE;
}

BOOL CAboutPage::OnSetActive()
{
    CPropertyPage::OnSetActive();

    CListBox*   pConfigSummaryList = (CListBox*)GetDlgItem(IDC_CONFIG_SUMMARY);

    // First, delete any existing strings in the config summary list.
    for( int i = pConfigSummaryList->GetCount() - 1; i >= 0; i-- )
    {
        pConfigSummaryList->DeleteString( i );
    }

    int n = 0;
    for( int i = 0; i < cNumVars; i++ )
    {
        if( m_pVariableState->CurrentIconState[i] == ICON_STATE_NONDEFAULT )
        {
            std::ostringstream ss;
            ss << cVars[i].Name << " = ";
            switch( cVars[i].Type )
            {
            case CONTROL_TYPE_BOOL:
                ss << ( m_pVariableState->CurrentIntValue[i] ?
                    "true" :
                    "false" );
                break;
            case CONTROL_TYPE_INT:
                ss << m_pVariableState->CurrentIntValue[i];
                break;
            case CONTROL_TYPE_STRING:
                ss << ToString( m_pVariableState->CurrentStringValue[i] );
                break;
            default:
                ss << "<unexpected!>";
            }
            pConfigSummaryList->InsertString(n++, ToWString( ss.str() ).c_str() );
        }
    }
    if( n == 0 )
    {
        pConfigSummaryList->InsertString(n++, L"No non-default controls." );
    }

    return TRUE;
}

static cl_int GetDeviceInfoString(
    pfnGetDeviceInfo    dclGetDeviceInfo,
    cl_device_id        device,
    cl_device_info      param_name,
    std::vector<char>&  param_value )
{
    cl_int  errorCode = CL_SUCCESS;
    size_t  size = 0;

    if( errorCode == CL_SUCCESS )
    {
        errorCode = dclGetDeviceInfo(
            device,
            param_name,
            0,
            NULL,
            &size );
    }

    if( errorCode == CL_SUCCESS )
    {
        param_value.resize( size );
        errorCode = dclGetDeviceInfo(
            device,
            param_name,
            param_value.size(),
            param_value.data(),
            NULL );
    }

    return errorCode;
}

void CAboutPage::OnPlatformListChange()
{
    CComboBox*  pPlatformComboBox = (CComboBox*)GetDlgItem(IDC_PLATFORM_LIST);
    CComboBox*  pDeviceComboBox = (CComboBox*)GetDlgItem(IDC_DEVICE_LIST);

    // First, delete any existing strings in the device list.
    for( int i = pDeviceComboBox->GetCount() - 1; i >= 0; i-- )
    {
        pDeviceComboBox->DeleteString( i );
    }

    // Get the currently selected platform index.
    cl_uint platformIndex = pPlatformComboBox->GetCurSel();

    HMODULE hModule = ::LoadLibraryA( "opencl.dll" );
    if( hModule )
    {
        pfnGetDeviceIDs   dclGetDeviceIDs = (pfnGetDeviceIDs)::GetProcAddress( hModule, "clGetDeviceIDs" );
        pfnGetDeviceInfo  dclGetDeviceInfo = (pfnGetDeviceInfo)::GetProcAddress( hModule, "clGetDeviceInfo" );

        // Get the array of platforms.
        cl_int  errorCode = CL_SUCCESS;
        cl_uint numDevices = 0;

        if( errorCode == CL_SUCCESS &&
            dclGetDeviceIDs &&
            platformIndex < m_Platforms.size() )
        {
            errorCode = dclGetDeviceIDs(
                m_Platforms[platformIndex],
                CL_DEVICE_TYPE_ALL,
                0,
                NULL,
                &numDevices );
            if( errorCode != CL_SUCCESS ||
                numDevices == 0 )
            {
                pDeviceComboBox->AddString(L"No OpenCL devices detected!");
            }
        }
        else
        {
            pDeviceComboBox->AddString(L"No OpenCL platforms detected!");
        }

        m_Devices.resize( numDevices );

        if( errorCode == CL_SUCCESS &&
            dclGetDeviceIDs &&
            dclGetDeviceInfo &&
            platformIndex < m_Platforms.size() )
        {
            errorCode = dclGetDeviceIDs(
                m_Platforms[platformIndex],
                CL_DEVICE_TYPE_ALL,
                (cl_uint)m_Devices.size(),
                m_Devices.data(),
                NULL );
            for( cl_uint d = 0; d < numDevices; d++ )
            {
                std::vector<char>   str;
                if( errorCode == CL_SUCCESS )
                {
                    errorCode = GetDeviceInfoString(
                        dclGetDeviceInfo,
                        m_Devices[d],
                        CL_DEVICE_NAME,
                        str );
                }
                if( errorCode == CL_SUCCESS )
                {
                    pDeviceComboBox->AddString(
                        ToWString(str).c_str() );
                }
            }
        }

        ::FreeLibrary( hModule );
    }

    pDeviceComboBox->SetCurSel(0);

    OnDeviceListChange();
}

void CAboutPage::OnDeviceListChange()
{
    CComboBox*  pDeviceComboBox = (CComboBox*)GetDlgItem(IDC_DEVICE_LIST);
    CListBox*   pDeviceInfoList = (CListBox*)GetDlgItem(IDC_DEVICE_INFO);

    // First, delete any existing strings in the device info list.
    for( int i = pDeviceInfoList->GetCount() - 1; i >= 0; i-- )
    {
        pDeviceInfoList->DeleteString( i );
    }

    // Get the currently selected device index.
    cl_uint deviceIndex = pDeviceComboBox->GetCurSel();
    if( deviceIndex < m_Devices.size() )
    {
        HMODULE hModule = ::LoadLibraryA( "opencl.dll" );
        if( hModule )
        {
            pfnGetDeviceInfo  dclGetDeviceInfo = (pfnGetDeviceInfo)::GetProcAddress( hModule, "clGetDeviceInfo" );

            cl_int  errorCode = CL_SUCCESS;

            if( dclGetDeviceInfo )
            {
                std::vector<char>   vendor;
                errorCode |= GetDeviceInfoString(
                    dclGetDeviceInfo,
                    m_Devices[deviceIndex],
                    CL_DEVICE_VENDOR,
                    vendor );
                std::vector<char>   version;
                errorCode |= GetDeviceInfoString(
                    dclGetDeviceInfo,
                    m_Devices[deviceIndex],
                    CL_DEVICE_VERSION,
                    version );
                std::vector<char>   driverVersion;
                errorCode |= GetDeviceInfoString(
                    dclGetDeviceInfo,
                    m_Devices[deviceIndex],
                    CL_DRIVER_VERSION,
                    driverVersion );

                if( errorCode == CL_SUCCESS )
                {
                    int n = 0;
                    pDeviceInfoList->InsertString(n++, ToWString(vendor).c_str() );
                    pDeviceInfoList->InsertString(n++, ToWString(version).c_str() );
                    pDeviceInfoList->InsertString(n++, ToWString(driverVersion).c_str() );
                }
                else
                {
                    pDeviceInfoList->InsertString(0, L"Error getting device info!");
                }
            }
            else
            {
                pDeviceInfoList->InsertString(0, L"Error getting device info function pointer!");
            }

            ::FreeLibrary( hModule );
        }
    }
}

BEGIN_MESSAGE_MAP(CAboutPage, CPropertyPage)
    ON_CBN_SELCHANGE(IDC_PLATFORM_LIST, OnPlatformListChange)
    ON_CBN_SELCHANGE(IDC_DEVICE_LIST, OnDeviceListChange)
END_MESSAGE_MAP()

// ==========================================================================
// General utility routines

// This is the message handler for the Apply Button
// Calls the Apply Method for each enabled page

void CCLInterceptConfigSheet::OnApplyNow()
{
    m_UserPage.ApplyChanges();
    m_UserPage.SetModified(FALSE);

    PostMessage(WM_RESIZEPAGE);
}

void CCLInterceptConfigSheet::OnOK()
{
    // call the ApplyNow method since the user clicked OK
    OnApplyNow();

    // Signal the end of the dialog
    CPropertySheet::EndDialog(IDOK);
}

BOOL CCLInterceptConfigSheet::OnInitDialog( )
{
    CPropertySheet::OnInitDialog();

    ModifyStyle(0, WS_THICKFRAME);

    RECT r;
    GetWindowRect(&r);
    m_baseSize.x = r.right - r.left;
    m_baseSize.y = r.bottom - r.top;

    return TRUE;
}

void CCLInterceptConfigSheet::OnSize(UINT nType, int cx, int cy)
{
    ModifyStyle(0, WS_THICKFRAME);

    CPropertySheet::OnSize(nType, cx, cy);

    PostMessage(WM_RESIZEPAGE);
}

afx_msg LRESULT CCLInterceptConfigSheet::OnResizePage(WPARAM wParam, LPARAM lParam)
{
    Invalidate();
    return 0;
}

BOOL CCLInterceptConfigSheet::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
    NMHDR* pnmh = (LPNMHDR) lParam;

    // the sheet resizes the page whenever it is activated
    // so we need to resize it to what we want
    if (TCN_SELCHANGE == pnmh->code) {
        // user-defined message needs to be posted because page must
        // be resized after TCN_SELCHANGE has been processed
        PostMessage (WM_RESIZEPAGE);
    }

    return CPropertySheet::OnNotify(wParam, lParam, pResult);
}

void CCLInterceptConfigSheet::OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI)
{
    lpMMI->ptMinTrackSize.x = m_baseSize.x;
    lpMMI->ptMinTrackSize.y = m_baseSize.y;
    CPropertySheet::OnGetMinMaxInfo(lpMMI);
}

// Message map for the property sheet
BEGIN_MESSAGE_MAP( CCLInterceptConfigSheet, CPropertySheet )
    ON_MESSAGE (WM_RESIZEPAGE, OnResizePage)
    ON_WM_GETMINMAXINFO()
    ON_BN_CLICKED(ID_APPLY_NOW, OnApplyNow)
    ON_BN_CLICKED(IDOK, OnOK)
END_MESSAGE_MAP()

//========================================================================

LONG WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    return (LONG)DefWindowProc(hWnd, msg, wParam, lParam);
}

BOOL GetPlatformInfo()
{
    return TRUE;
}

CLInterceptConfigApp::CLInterceptConfigApp(void)
{
    // create a mutex to ensure only one instance of the OpenGL config app
    hMutex = CreateMutex(NULL, FALSE, L"CLInterceptConfig");
    mutexState = GetLastError();
}

CLInterceptConfigApp::~CLInterceptConfigApp(void)
{
    // release the mutex
    if (hMutex)
    {
        CloseHandle(hMutex);
        hMutex = NULL;
    }
}

BOOL CLInterceptConfigApp::IsAnotherInstanceRunning(void)
{
    return (ERROR_ALREADY_EXISTS == mutexState);
}

static BOOL CheckHKLMRegistryKey()
{
    HKEY    key;

    LSTATUS success = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,
        REGISTRY_KEY,
        0,
        KEY_READ,
        &key );
    if( success == ERROR_SUCCESS )
    {
        RegCloseKey(key);
    }

    return success == ERROR_SUCCESS;
}

BOOL CLInterceptConfigApp::InitInstance(void)
{
    if( IsAnotherInstanceRunning() == TRUE )
    {
        MessageBox(
            NULL,
            L"The Intercept Layer for OpenCL Applications Configuration App is already Running!",
            L"Error!",
            MB_OK);

        AfxGetApp()->ExitInstance();
        return FALSE;
    }

    if( CheckHKLMRegistryKey() == TRUE )
    {
        MessageBox(
            NULL,
            L"The Intercept Layer for OpenCL Applications now stores its registry keys "
            L"in HKEY_CURRENT_USER, but it appears as though there are registry keys in "
            L"HKEY_LOCAL_MACHINE.  To avoid confusion it is strongly recommended to "
            L"remove the old registry keys in HKEY_LOCAL_MACHINE!",
            L"Warning",
            MB_OK);
    }

    CCLInterceptConfigSheet cliSheet;
    m_pMainWnd = &cliSheet;

    cliSheet.DoModal();

    return FALSE;
}

CLInterceptConfigApp cliConfigApp;
