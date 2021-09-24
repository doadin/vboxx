/* $Id$ */
/** @file
 * VBox Qt GUI - UIWizardExportAppPageBasic2 class declaration.
 */

/*
 * Copyright (C) 2009-2021 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef FEQT_INCLUDED_SRC_wizards_exportappliance_UIWizardExportAppPageBasic2_h
#define FEQT_INCLUDED_SRC_wizards_exportappliance_UIWizardExportAppPageBasic2_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* GUI includes: */
#include "UINativeWizardPage.h"
#include "UIWizardExportApp.h"

/* COM includes: */
#include "COMEnums.h"
#include "CAppliance.h"
#include "CCloudClient.h"
#include "CCloudProfile.h"
#include "CVirtualSystemDescription.h"
#include "CVirtualSystemDescriptionForm.h"

/* Forward declarations: */
class QAbstractButton;
class QButtonGroup;
class QCheckBox;
class QGridLayout;
class QLabel;
class QStackedWidget;
class QIComboBox;
class QIRichTextLabel;
class QIToolButton;
class UIEmptyFilePathSelector;

/** Format combo data fields. */
enum
{
    FormatData_Name            = Qt::UserRole + 1,
    FormatData_ShortName       = Qt::UserRole + 2,
    FormatData_IsItCloudFormat = Qt::UserRole + 3
};

/** Profile combo data fields. */
enum
{
    ProfileData_Name = Qt::UserRole + 1
};

/** Namespace for 2nd basic page of the Export Appliance wizard. */
namespace UIWizardExportAppPage2
{
    /** Populates formats. */
    void populateFormats(QIComboBox *pCombo, bool fExportToOCIByDefault);
    /** Populates MAC address policies. */
    void populateMACAddressPolicies(QIComboBox *pCombo);

    /** Returns current format of @a pCombo specified. */
    QString format(QIComboBox *pCombo);
    /** Returns whether format under certain @a iIndex is cloud one. */
    bool isFormatCloudOne(QIComboBox *pCombo, int iIndex = -1);

    /** Refresh stacked widget. */
    void refreshStackedWidget(QStackedWidget *pStackedWidget,
                              bool fIsFormatCloudOne);

    /** Refresh file selector name. */
    void refreshFileSelectorName(QString &strFileSelectorName,
                                 const QStringList &machineNames,
                                 const QString &strDefaultApplianceName,
                                 bool fIsFormatCloudOne);
    /** Refresh file selector extension. */
    void refreshFileSelectorExtension(QString &strFileSelectorExt,
                                      UIEmptyFilePathSelector *pFileSelector,
                                      bool fIsFormatCloudOne);
    /** Refresh file selector path. */
    void refreshFileSelectorPath(UIEmptyFilePathSelector *pFileSelector,
                                 const QString &strFileSelectorName,
                                 const QString &strFileSelectorExt,
                                 bool fIsFormatCloudOne);
    /** Refresh Manifest check-box access. */
    void refreshManifestCheckBoxAccess(QCheckBox *pCheckBox,
                                       bool fIsFormatCloudOne);
    /** Refresh Include ISOs check-box access. */
    void refreshIncludeISOsCheckBoxAccess(QCheckBox *pCheckBox,
                                          bool fIsFormatCloudOne);

    /** Refresh profile combo. */
    void refreshProfileCombo(QIComboBox *pCombo,
                             const QString &strFormat,
                             bool fIsFormatCloudOne);
    /** Refresh cloud profile. */
    void refreshCloudProfile(CCloudProfile &comCloudProfile,
                             const QString &strShortProviderName,
                             const QString &strProfileName,
                             bool fIsFormatCloudOne);
    /** Refresh cloud export mode. */
    void refreshCloudExportMode(const QMap<CloudExportMode, QAbstractButton*> &radios,
                                bool fIsFormatCloudOne);
    /** Refresh cloud stuff. */
    void refreshCloudStuff(CAppliance &comCloudAppliance,
                           CCloudClient &comCloudClient,
                           CVirtualSystemDescription &comCloudVsd,
                           CVirtualSystemDescriptionForm &comCloudVsdExportForm,
                           const CCloudProfile &comCloudProfile,
                           const QList<QUuid> &machineIDs,
                           const QString &strUri,
                           const CloudExportMode enmCloudExportMode);

    /** Returns current profile name of @a pCombo specified. */
    QString profileName(QIComboBox *pCombo);
    /** Returns current cloud export mode chosen in @a radioButtons specified. */
    CloudExportMode cloudExportMode(const QMap<CloudExportMode, QAbstractButton*> &radioButtons);

    /** Updates format combo tool-tips. */
    void updateFormatComboToolTip(QIComboBox *pCombo);
    /** Updates MAC address export policy combo tool-tips. */
    void updateMACAddressExportPolicyComboToolTip(QIComboBox *pCombo);
}

/** UINativeWizardPage extension for 2nd basic page of the Export Appliance wizard,
  * based on UIWizardAddCloudVMPage2 namespace functions. */
class UIWizardExportAppPageBasic2 : public UINativeWizardPage
{
    Q_OBJECT;

public:

    /** Constructs 2nd basic page. */
    UIWizardExportAppPageBasic2(bool fExportToOCIByDefault);

protected:

    /** Returns wizard this page belongs to. */
    UIWizardExportApp *wizard() const;

    /** Handles translation event. */
    virtual void retranslateUi() /* override */;

    /** Performs page initialization. */
    virtual void initializePage() /* override */;

    /** Returns whether page is complete. */
    virtual bool isComplete() const /* override */;

    /** Performs page validation. */
    virtual bool validatePage() /* override */;

private slots:

    /** Handles change in format combo-box. */
    void sltHandleFormatComboChange();

    /** Handles change in file-name selector. */
    void sltHandleFileSelectorChange();

    /** Handles change in MAC address export policy combo-box. */
    void sltHandleMACAddressExportPolicyComboChange();

    /** Handles change in manifest check-box. */
    void sltHandleManifestCheckBoxChange();

    /** Handles change in include ISOs check-box. */
    void sltHandleIncludeISOsCheckBoxChange();

    /** Handles change in profile combo-box. */
    void sltHandleProfileComboChange();

    /** Handles cloud export radio-button clicked. */
    void sltHandleRadioButtonToggled(QAbstractButton *pButton, bool fToggled);

    /** Handles profile tool-button click. */
    void sltHandleProfileButtonClick();

private:

    /** Updates format. */
    void updateFormat();
    /** Updates profile. */
    void updateProfile();
    /** Updates cloud stuff. */
    void updateCloudStuff();

    /** Holds whether default format should be Export to OCI. */
    bool  m_fExportToOCIByDefault;

    /** Holds the default appliance name. */
    QString  m_strDefaultApplianceName;
    /** Holds the file selector name. */
    QString  m_strFileSelectorName;
    /** Holds the file selector ext. */
    QString  m_strFileSelectorExt;

    /** Holds the Cloud Profile object instance. */
    CCloudProfile  m_comCloudProfile;


    /** Holds the format label instance. */
    QIRichTextLabel *m_pLabelFormat;
    /** Holds the settings label instance. */
    QIRichTextLabel *m_pLabelSettings;

    /** Holds the format layout. */
    QGridLayout *m_pFormatLayout;
    /** Holds the format combo-box label instance. */
    QLabel      *m_pFormatComboBoxLabel;
    /** Holds the format combo-box instance. */
    QIComboBox  *m_pFormatComboBox;

    /** Holds the settings widget instance. */
    QStackedWidget *m_pSettingsWidget;

    /** Holds the settings layout 1. */
    QGridLayout             *m_pSettingsLayout1;
    /** Holds the file selector label instance. */
    QLabel                  *m_pFileSelectorLabel;
    /** Holds the file selector instance. */
    UIEmptyFilePathSelector *m_pFileSelector;
    /** Holds the MAC address policy combo-box label instance. */
    QLabel                  *m_pMACComboBoxLabel;
    /** Holds the MAC address policy check-box instance. */
    QIComboBox              *m_pMACComboBox;
    /** Holds the additional label instance. */
    QLabel                  *m_pAdditionalLabel;
    /** Holds the manifest check-box instance. */
    QCheckBox               *m_pManifestCheckbox;
    /** Holds the include ISOs check-box instance. */
    QCheckBox               *m_pIncludeISOsCheckbox;

    /** Holds the settings layout 2. */
    QGridLayout   *m_pSettingsLayout2;
    /** Holds the profile label instance. */
    QLabel        *m_pProfileLabel;
    /** Holds the profile combo-box instance. */
    QIComboBox    *m_pProfileComboBox;
    /** Holds the profile management tool-button instance. */
    QIToolButton  *m_pProfileToolButton;

    /** Holds the export mode label instance. */
    QLabel                                  *m_pExportModeLabel;
    /** Holds the export mode button group instance. */
    QButtonGroup                            *m_pExportModeButtonGroup;
    /** Holds the map of export mode button instances. */
    QMap<CloudExportMode, QAbstractButton*>  m_exportModeButtons;
};

#endif /* !FEQT_INCLUDED_SRC_wizards_exportappliance_UIWizardExportAppPageBasic2_h */
