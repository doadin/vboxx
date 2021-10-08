/* $Id$ */
/** @file
 * VBox Qt GUI - UIWizardImportAppPageBasic1 class implementation.
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

/* Qt includes: */
#include <QGridLayout>
#include <QLabel>
#include <QListWidget>
#include <QStackedWidget>

/* GUI includes: */
#include "QIComboBox.h"
#include "QIRichTextLabel.h"
#include "QIToolButton.h"
#include "UICloudNetworkingStuff.h"
#include "UIEmptyFilePathSelector.h"
#include "UIIconPool.h"
#include "UIMessageCenter.h"
#include "UIVirtualBoxEventHandler.h"
#include "UIVirtualBoxManager.h"
#include "UIWizardImportApp.h"
#include "UIWizardImportAppPageBasic1.h"

/* COM includes: */
#include "CAppliance.h"
#include "CVirtualSystemDescription.h"
#include "CVirtualSystemDescriptionForm.h"

/* Namespaces: */
using namespace UIWizardImportAppPage1;


/*********************************************************************************************************************************
*   Class UIWizardImportAppPage1 implementation.                                                                                 *
*********************************************************************************************************************************/

void UIWizardImportAppPage1::populateSources(QIComboBox *pCombo, bool fImportFromOCIByDefault)
{
    /* Sanity check: */
    AssertPtrReturnVoid(pCombo);
    /* We need top-level parent as well: */
    QWidget *pParent = pCombo->window();
    AssertPtrReturnVoid(pParent);

    /* Remember current item data to be able to restore it: */
    QString strOldData;
    if (pCombo->currentIndex() != -1)
        strOldData = pCombo->currentData(SourceData_ShortName).toString();
    else
    {
        /* Otherwise "OCI" or "local" should be the default one: */
        if (fImportFromOCIByDefault)
            strOldData = "OCI";
        else
            strOldData = "local";
    }

    /* Block signals while updating: */
    pCombo->blockSignals(true);

    /* Clear combo initially: */
    pCombo->clear();

    /* Compose hardcoded sources list: */
    QStringList sources;
    sources << "local";
    /* Add that list to combo: */
    foreach (const QString &strShortName, sources)
    {
        /* Compose empty item, fill it's data: */
        pCombo->addItem(QString());
        pCombo->setItemData(pCombo->count() - 1, strShortName, SourceData_ShortName);
    }

    /* Iterate through existing providers: */
    foreach (const CCloudProvider &comProvider, listCloudProviders(pParent))
    {
        /* Skip if we have nothing to populate (file missing?): */
        if (comProvider.isNull())
            continue;
        /* Acquire provider name: */
        QString strProviderName;
        if (!cloudProviderName(comProvider, strProviderName, pParent))
            continue;
        /* Acquire provider short name: */
        QString strProviderShortName;
        if (!cloudProviderShortName(comProvider, strProviderShortName, pParent))
            continue;

        /* Compose empty item, fill it's data: */
        pCombo->addItem(QString());
        pCombo->setItemData(pCombo->count() - 1, strProviderName,      SourceData_Name);
        pCombo->setItemData(pCombo->count() - 1, strProviderShortName, SourceData_ShortName);
        pCombo->setItemData(pCombo->count() - 1, true,                 SourceData_IsItCloudFormat);
    }

    /* Set previous/default item if possible: */
    int iNewIndex = -1;
    if (   iNewIndex == -1
        && !strOldData.isNull())
        iNewIndex = pCombo->findData(strOldData, SourceData_ShortName);
    if (   iNewIndex == -1
        && pCombo->count() > 0)
        iNewIndex = 0;
    if (iNewIndex != -1)
        pCombo->setCurrentIndex(iNewIndex);

    /* Unblock signals after update: */
    pCombo->blockSignals(false);
}

QString UIWizardImportAppPage1::source(QIComboBox *pCombo)
{
    /* Sanity check: */
    AssertPtrReturn(pCombo, QString());

    /* Give the actual result: */
    return pCombo->currentData(SourceData_ShortName).toString();
}

bool UIWizardImportAppPage1::isSourceCloudOne(QIComboBox *pCombo, int iIndex /* = -1 */)
{
    /* Sanity check: */
    AssertPtrReturn(pCombo, false);

    /* Handle special case, -1 means "current one": */
    if (iIndex == -1)
        iIndex = pCombo->currentIndex();

    /* Give the actual result: */
    return pCombo->itemData(iIndex, SourceData_IsItCloudFormat).toBool();
}

void UIWizardImportAppPage1::refreshStackedWidget(QStackedWidget *pStackedWidget,
                                                  bool fIsSourceCloudOne)
{
    /* Sanity check: */
    AssertPtrReturnVoid(pStackedWidget);

    /* Update stack appearance according to chosen source: */
    pStackedWidget->setCurrentIndex((int)fIsSourceCloudOne);
}

void UIWizardImportAppPage1::refreshProfileCombo(QIComboBox *pCombo,
                                                 const QString &strSource,
                                                 bool fIsSourceCloudOne)
{
    /* Sanity check: */
    AssertPtrReturnVoid(pCombo);

    /* If source is cloud one: */
    if (fIsSourceCloudOne)
    {
        /* We need top-level parent as well: */
        QWidget *pParent = pCombo->window();
        AssertPtrReturnVoid(pParent);
        /* Acquire provider: */
        CCloudProvider comProvider = cloudProviderByShortName(strSource, pParent);
        AssertReturnVoid(comProvider.isNotNull());

        /* Remember current item data to be able to restore it: */
        QString strOldData;
        if (pCombo->currentIndex() != -1)
            strOldData = pCombo->currentData(ProfileData_Name).toString();

        /* Block signals while updating: */
        pCombo->blockSignals(true);

        /* Clear combo initially: */
        pCombo->clear();

        /* Iterate through existing profile names: */
        foreach (const CCloudProfile &comProfile, listCloudProfiles(comProvider, pParent))
        {
            /* Skip if we have nothing to populate (wtf happened?): */
            if (comProfile.isNull())
                continue;
            /* Acquire profile name: */
            QString strProfileName;
            if (!cloudProfileName(comProfile, strProfileName, pParent))
                continue;

            /* Compose item, fill it's data: */
            pCombo->addItem(strProfileName);
            pCombo->setItemData(pCombo->count() - 1, strProfileName, ProfileData_Name);
        }

        /* Set previous/default item if possible: */
        int iNewIndex = -1;
        if (   iNewIndex == -1
            && !strOldData.isNull())
            iNewIndex = pCombo->findData(strOldData, ProfileData_Name);
        if (   iNewIndex == -1
            && pCombo->count() > 0)
            iNewIndex = 0;
        if (iNewIndex != -1)
            pCombo->setCurrentIndex(iNewIndex);

        /* Unblock signals after update: */
        pCombo->blockSignals(false);
    }
    /* If source is local one: */
    else
    {
        /* Block signals while updating: */
        pCombo->blockSignals(true);

        /* Clear combo initially: */
        pCombo->clear();

        /* Unblock signals after update: */
        pCombo->blockSignals(false);
    }
}

void UIWizardImportAppPage1::refreshCloudProfileInstances(QListWidget *pListWidget,
                                                          const QString &strSource,
                                                          const QString &strProfileName,
                                                          bool fIsSourceCloudOne)
{
    /* Sanity check: */
    AssertPtrReturnVoid(pListWidget);

    /* If source is cloud one: */
    if (fIsSourceCloudOne)
    {
        /* We need top-level parent as well: */
        QWidget *pParent = pListWidget->window();
        AssertPtrReturnVoid(pParent);
        /* Acquire client: */
        CCloudClient comClient = cloudClientByName(strSource, strProfileName, pParent);
        AssertReturnVoid(comClient.isNotNull());

        /* Block signals while updating: */
        pListWidget->blockSignals(true);

        /* Clear list initially: */
        pListWidget->clear();

        /* Gather VM names, ids and states.
         * Currently we are interested in Running and Stopped VMs only. */
        QString strErrorMessage;
        const QMap<QString, QString> instances = listInstances(comClient, strErrorMessage, pParent);

        /* Push acquired names to list rows: */
        foreach (const QString &strId, instances.keys())
        {
            /* Create list item: */
            QListWidgetItem *pItem = new QListWidgetItem(instances.value(strId), pListWidget);
            if (pItem)
            {
                pItem->setFlags(pItem->flags() & ~Qt::ItemIsEditable);
                pItem->setData(Qt::UserRole, strId);
            }
        }

        /* Choose the 1st one by default if possible: */
        if (pListWidget->count())
            pListWidget->setCurrentRow(0);

        /* Unblock signals after update: */
        pListWidget->blockSignals(false);
    }
    /* If source is local one: */
    else
    {
        /* Block signals while updating: */
        pListWidget->blockSignals(true);

        /* Clear combo initially: */
        pListWidget->clear();

        /* Unblock signals after update: */
        pListWidget->blockSignals(false);
    }
}

void UIWizardImportAppPage1::refreshCloudStuff(CAppliance &comCloudAppliance,
                                               CVirtualSystemDescriptionForm &comCloudVsdImportForm,
                                               QWidget *pParent,
                                               const QString &strMachineId,
                                               const QString &strSource,
                                               const QString &strProfileName,
                                               bool fIsSourceCloudOne)
{
    /* Clear stuff: */
    comCloudAppliance = CAppliance();
    comCloudVsdImportForm = CVirtualSystemDescriptionForm();

    /* If source is NOT cloud one: */
    if (!fIsSourceCloudOne)
        return;

    /* We need top-level parent as well: */
    AssertPtrReturnVoid(pParent);
    /* Acquire client: */
    CCloudClient comClient = cloudClientByName(strSource, strProfileName, pParent);
    AssertReturnVoid(comClient.isNotNull());

    /* Create appliance: */
    CVirtualBox comVBox = uiCommon().virtualBox();
    CAppliance comAppliance = comVBox.CreateAppliance();
    if (!comVBox.isOk())
    {
        msgCenter().cannotCreateAppliance(comVBox, pParent);
        return;
    }

    /* Remember appliance: */
    comCloudAppliance = comAppliance;

    /* Read cloud instance info: */
    CProgress comReadProgress = comCloudAppliance.Read(QString("OCI://%1/%2").arg(strProfileName, strMachineId));
    if (!comCloudAppliance.isOk())
    {
        msgCenter().cannotImportAppliance(comCloudAppliance, pParent);
        return;
    }

    /* Show "Read appliance" progress: */
    msgCenter().showModalProgressDialog(comReadProgress, UIWizardImportApp::tr("Read appliance ..."),
                                        ":/progress_reading_appliance_90px.png", pParent, 0);
    if (!comReadProgress.isOk() || comReadProgress.GetResultCode() != 0)
    {
        msgCenter().cannotImportAppliance(comReadProgress, comCloudAppliance.GetPath(), pParent);
        return;
    }

    /* Acquire virtual system description: */
    QVector<CVirtualSystemDescription> descriptions = comCloudAppliance.GetVirtualSystemDescriptions();
    if (!comCloudAppliance.isOk())
    {
        msgCenter().cannotAcquireVirtualSystemDescription(comCloudAppliance, pParent);
        return;
    }

    /* Make sure there is at least one virtual system description created: */
    AssertReturnVoid(!descriptions.isEmpty());
    CVirtualSystemDescription comDescription = descriptions.at(0);

    /* Read Cloud Client description form: */
    CVirtualSystemDescriptionForm comVsdImportForm;
    bool fSuccess = importDescriptionForm(comClient, comDescription, comVsdImportForm, pParent);
    if (!fSuccess)
        return;

    /* Remember form: */
    comCloudVsdImportForm = comVsdImportForm;
}

QString UIWizardImportAppPage1::path(UIEmptyFilePathSelector *pFileSelector)
{
    /* Sanity check: */
    AssertPtrReturn(pFileSelector, QString());

    /* Give the actual result: */
    return pFileSelector->path().toLower();
}

QString UIWizardImportAppPage1::profileName(QIComboBox *pCombo)
{
    /* Sanity check: */
    AssertPtrReturn(pCombo, QString());

    /* Give the actual result: */
    return pCombo->currentData(ProfileData_Name).toString();
}

QString UIWizardImportAppPage1::machineId(QListWidget *pListWidget)
{
    /* Sanity check: */
    AssertPtrReturn(pListWidget, QString());

    /* Give the actual result: */
    QListWidgetItem *pItem = pListWidget->currentItem();
    return pItem ? pItem->data(Qt::UserRole).toString() : QString();
}

void UIWizardImportAppPage1::updateSourceComboToolTip(QIComboBox *pCombo)
{
    /* Sanity check: */
    AssertPtrReturnVoid(pCombo);

    /* Update tool-tip: */
    const QString strCurrentToolTip = pCombo->currentData(Qt::ToolTipRole).toString();
    pCombo->setToolTip(strCurrentToolTip);
}


/*********************************************************************************************************************************
*   Class UIWizardImportAppPageBasic1 implementation.                                                                            *
*********************************************************************************************************************************/

UIWizardImportAppPageBasic1::UIWizardImportAppPageBasic1(bool fImportFromOCIByDefault)
    : m_fImportFromOCIByDefault(fImportFromOCIByDefault)
    , m_pLabelMain(0)
    , m_pLabelDescription(0)
    , m_pSourceLayout(0)
    , m_pSourceLabel(0)
    , m_pSourceComboBox(0)
    , m_pSettingsWidget1(0)
    , m_pLocalContainerLayout(0)
    , m_pFileLabel(0)
    , m_pFileSelector(0)
    , m_pCloudContainerLayout(0)
    , m_pProfileLabel(0)
    , m_pProfileComboBox(0)
    , m_pProfileToolButton(0)
    , m_pProfileInstanceLabel(0)
    , m_pProfileInstanceList(0)
{
    /* Prepare main layout: */
    QVBoxLayout *pMainLayout = new QVBoxLayout(this);
    if (pMainLayout)
    {
        /* Prepare main label: */
        m_pLabelMain = new QIRichTextLabel(this);
        if (m_pLabelMain)
            pMainLayout->addWidget(m_pLabelMain);

        /* Prepare source layout: */
        m_pSourceLayout = new QGridLayout;
        if (m_pSourceLayout)
        {
            m_pSourceLayout->setContentsMargins(0, 0, 0, 0);
            m_pSourceLayout->setColumnStretch(0, 0);
            m_pSourceLayout->setColumnStretch(1, 1);

            /* Prepare source label: */
            m_pSourceLabel = new QLabel(this);
            if (m_pSourceLabel)
                m_pSourceLayout->addWidget(m_pSourceLabel, 0, 0, Qt::AlignRight);
            /* Prepare source selector: */
            m_pSourceComboBox = new QIComboBox(this);
            if (m_pSourceComboBox)
            {
                m_pSourceLabel->setBuddy(m_pSourceComboBox);
                m_pSourceLayout->addWidget(m_pSourceComboBox, 0, 1);
            }

            /* Add into layout: */
            pMainLayout->addLayout(m_pSourceLayout);
        }

        /* Prepare description label: */
        m_pLabelDescription = new QIRichTextLabel(this);
        if (m_pLabelDescription)
            pMainLayout->addWidget(m_pLabelDescription);

        /* Prepare settings widget: */
        m_pSettingsWidget1 = new QStackedWidget(this);
        if (m_pSettingsWidget1)
        {
            /* Prepare local container: */
            QWidget *pContainerLocal = new QWidget(m_pSettingsWidget1);
            if (pContainerLocal)
            {
                /* Prepare local container layout: */
                m_pLocalContainerLayout = new QGridLayout(pContainerLocal);
                if (m_pLocalContainerLayout)
                {
                    m_pLocalContainerLayout->setContentsMargins(0, 0, 0, 0);
                    m_pLocalContainerLayout->setColumnStretch(0, 0);
                    m_pLocalContainerLayout->setColumnStretch(1, 1);
                    m_pLocalContainerLayout->setRowStretch(1, 1);

                    /* Prepare file label: */
                    m_pFileLabel = new QLabel(pContainerLocal);
                    if (m_pFileLabel)
                        m_pLocalContainerLayout->addWidget(m_pFileLabel, 0, 0, Qt::AlignRight);

                    /* Prepare file-path selector: */
                    m_pFileSelector = new UIEmptyFilePathSelector(pContainerLocal);
                    if (m_pFileSelector)
                    {
                        m_pFileLabel->setBuddy(m_pFileSelector);
                        m_pFileSelector->setHomeDir(uiCommon().documentsPath());
                        m_pFileSelector->setMode(UIEmptyFilePathSelector::Mode_File_Open);
                        m_pFileSelector->setButtonPosition(UIEmptyFilePathSelector::RightPosition);
                        m_pFileSelector->setEditable(true);
                        m_pLocalContainerLayout->addWidget(m_pFileSelector, 0, 1);
                    }
                }

                /* Add into widget: */
                m_pSettingsWidget1->addWidget(pContainerLocal);
            }

            /* Prepare cloud container: */
            QWidget *pContainerCloud = new QWidget(m_pSettingsWidget1);
            if (pContainerCloud)
            {
                /* Prepare cloud container layout: */
                m_pCloudContainerLayout = new QGridLayout(pContainerCloud);
                if (m_pCloudContainerLayout)
                {
                    m_pCloudContainerLayout->setContentsMargins(0, 0, 0, 0);
                    m_pCloudContainerLayout->setColumnStretch(0, 0);
                    m_pCloudContainerLayout->setColumnStretch(1, 1);
                    m_pCloudContainerLayout->setRowStretch(1, 0);
                    m_pCloudContainerLayout->setRowStretch(2, 1);

                    /* Prepare profile label: */
                    m_pProfileLabel = new QLabel(pContainerCloud);
                    if (m_pProfileLabel)
                        m_pCloudContainerLayout->addWidget(m_pProfileLabel, 0, 0, Qt::AlignRight);

                    /* Prepare sub-layout: */
                    QHBoxLayout *pSubLayout = new QHBoxLayout;
                    if (pSubLayout)
                    {
                        pSubLayout->setContentsMargins(0, 0, 0, 0);
                        pSubLayout->setSpacing(1);

                        /* Prepare profile combo-box: */
                        m_pProfileComboBox = new QIComboBox(pContainerCloud);
                        if (m_pProfileComboBox)
                        {
                            m_pProfileLabel->setBuddy(m_pProfileComboBox);
                            pSubLayout->addWidget(m_pProfileComboBox);
                        }

                        /* Prepare profile tool-button: */
                        m_pProfileToolButton = new QIToolButton(pContainerCloud);
                        if (m_pProfileToolButton)
                        {
                            m_pProfileToolButton->setIcon(UIIconPool::iconSet(":/cloud_profile_manager_16px.png",
                                                                              ":/cloud_profile_manager_disabled_16px.png"));
                            pSubLayout->addWidget(m_pProfileToolButton);
                        }

                        /* Add into layout: */
                        m_pCloudContainerLayout->addLayout(pSubLayout, 0, 1);
                    }

                    /* Prepare profile instance label: */
                    m_pProfileInstanceLabel = new QLabel(pContainerCloud);
                    if (m_pProfileInstanceLabel)
                        m_pCloudContainerLayout->addWidget(m_pProfileInstanceLabel, 1, 0, Qt::AlignRight);

                    /* Prepare profile instances table: */
                    m_pProfileInstanceList = new QListWidget(pContainerCloud);
                    if (m_pProfileInstanceList)
                    {
                        m_pProfileInstanceLabel->setBuddy(m_pProfileInstanceLabel);
                        const QFontMetrics fm(m_pProfileInstanceList->font());
                        const int iFontWidth = fm.width('x');
                        const int iTotalWidth = 50 * iFontWidth;
                        const int iFontHeight = fm.height();
                        const int iTotalHeight = 4 * iFontHeight;
                        m_pProfileInstanceList->setMinimumSize(QSize(iTotalWidth, iTotalHeight));
//                        m_pProfileInstanceList->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
                        m_pProfileInstanceList->setAlternatingRowColors(true);
                        m_pCloudContainerLayout->addWidget(m_pProfileInstanceList, 1, 1, 2, 1);
                    }
                }

                /* Add into widget: */
                m_pSettingsWidget1->addWidget(pContainerCloud);
            }

            /* Add into layout: */
            pMainLayout->addWidget(m_pSettingsWidget1);
        }
    }

    /* Setup connections: */
    connect(gVBoxEvents, &UIVirtualBoxEventHandler::sigCloudProfileRegistered,
            this, &UIWizardImportAppPageBasic1::sltHandleSourceComboChange);
    connect(gVBoxEvents, &UIVirtualBoxEventHandler::sigCloudProfileChanged,
            this, &UIWizardImportAppPageBasic1::sltHandleSourceComboChange);
    connect(m_pSourceComboBox, static_cast<void(QIComboBox::*)(int)>(&QIComboBox::currentIndexChanged),
            this, &UIWizardImportAppPageBasic1::sltHandleSourceComboChange);
    connect(m_pFileSelector, &UIEmptyFilePathSelector::pathChanged,
            this, &UIWizardImportAppPageBasic1::completeChanged);
    connect(m_pProfileComboBox, static_cast<void(QIComboBox::*)(int)>(&QIComboBox::currentIndexChanged),
            this, &UIWizardImportAppPageBasic1::sltHandleProfileComboChange);
    connect(m_pProfileToolButton, &QIToolButton::clicked,
            this, &UIWizardImportAppPageBasic1::sltHandleProfileButtonClick);
    connect(m_pProfileInstanceList, &QListWidget::currentRowChanged,
            this, &UIWizardImportAppPageBasic1::completeChanged);
}

UIWizardImportApp *UIWizardImportAppPageBasic1::wizard() const
{
    return qobject_cast<UIWizardImportApp*>(UINativeWizardPage::wizard());
}

void UIWizardImportAppPageBasic1::retranslateUi()
{
    /* Translate page: */
    setTitle(UIWizardImportApp::tr("Appliance to import"));

    /* Translate main label: */
    if (m_pLabelMain)
        m_pLabelMain->setText(UIWizardImportApp::tr("Please choose the source to import appliance from.  This can be a "
                                                    "local file system to import OVF archive or one of known cloud "
                                                    "service providers to import cloud VM from."));

    /* Translate description label: */
    if (m_pLabelDescription)
    {
        if (wizard()->isSourceCloudOne())
            m_pLabelDescription->setText(UIWizardImportApp::
                                         tr("<p>Please choose one of cloud service profiles you have registered to import virtual "
                                            "machine from.  Corresponding machines list will be updated.  To continue, "
                                            "select one of machines to import below.</p>"));
        else
            m_pLabelDescription->setText(UIWizardImportApp::
                                         tr("<p>Please choose a file to import the virtual appliance from.  VirtualBox currently "
                                            "supports importing appliances saved in the Open Virtualization Format (OVF).  "
                                            "To continue, select the file to import below.</p>"));
    }

    /* Translate source label: */
    if (m_pSourceLabel)
        m_pSourceLabel->setText(UIWizardImportApp::tr("&Source:"));
    if (m_pSourceComboBox)
    {
        /* Translate hardcoded values of Source combo-box: */
        m_pSourceComboBox->setItemText(0, UIWizardImportApp::tr("Local File System"));
        m_pSourceComboBox->setItemData(0, UIWizardImportApp::tr("Import from local file system."), Qt::ToolTipRole);

        /* Translate received values of Source combo-box.
         * We are enumerating starting from 0 for simplicity: */
        for (int i = 0; i < m_pSourceComboBox->count(); ++i)
            if (isSourceCloudOne(m_pSourceComboBox, i))
            {
                m_pSourceComboBox->setItemText(i, m_pSourceComboBox->itemData(i, SourceData_Name).toString());
                m_pSourceComboBox->setItemData(i, UIWizardImportApp::tr("Import from cloud service provider."), Qt::ToolTipRole);
            }
    }

    /* Translate local stuff: */
    if (m_pFileLabel)
        m_pFileLabel->setText(UIWizardImportApp::tr("&File:"));
    if (m_pFileSelector)
    {
        m_pFileSelector->setChooseButtonToolTip(UIWizardImportApp::tr("Choose a virtual appliance file to import..."));
        m_pFileSelector->setFileDialogTitle(UIWizardImportApp::tr("Please choose a virtual appliance file to import"));
        m_pFileSelector->setFileFilters(UIWizardImportApp::tr("Open Virtualization Format (%1)").arg("*.ova *.ovf"));
    }

    /* Translate profile stuff: */
    if (m_pProfileLabel)
        m_pProfileLabel->setText(UIWizardImportApp::tr("&Profile:"));
    if (m_pProfileToolButton)
        m_pProfileToolButton->setToolTip(UIWizardImportApp::tr("Open Cloud Profile Manager..."));
    if (m_pProfileInstanceLabel)
        m_pProfileInstanceLabel->setText(UIWizardImportApp::tr("&Machines:"));

    /* Adjust label widths: */
    QList<QWidget*> labels;
    if (m_pFileLabel)
        labels << m_pFileLabel;
    if (m_pSourceLabel)
        labels << m_pSourceLabel;
    if (m_pProfileLabel)
        labels << m_pProfileLabel;
    if (m_pProfileInstanceLabel)
        labels << m_pProfileInstanceLabel;
    int iMaxWidth = 0;
    foreach (QWidget *pLabel, labels)
        iMaxWidth = qMax(iMaxWidth, pLabel->minimumSizeHint().width());
    if (m_pSourceLayout)
        m_pSourceLayout->setColumnMinimumWidth(0, iMaxWidth);
    if (m_pLocalContainerLayout)
    {
        m_pLocalContainerLayout->setColumnMinimumWidth(0, iMaxWidth);
        m_pCloudContainerLayout->setColumnMinimumWidth(0, iMaxWidth);
    }

    /* Update tool-tips: */
    updateSourceComboToolTip(m_pSourceComboBox);
}

void UIWizardImportAppPageBasic1::initializePage()
{
    /* Populate sources: */
    populateSources(m_pSourceComboBox, m_fImportFromOCIByDefault);
    /* Translate page: */
    retranslateUi();

    /* Choose initially focused widget: */
    if (wizard()->isSourceCloudOne())
        m_pProfileInstanceList->setFocus();
    else
        m_pFileSelector->setFocus();

    /* Fetch it, asynchronously: */
    QMetaObject::invokeMethod(this, "sltHandleSourceComboChange", Qt::QueuedConnection);
}

bool UIWizardImportAppPageBasic1::isComplete() const
{
    /* Initial result: */
    bool fResult = true;

    /* Check whether there was cloud source selected: */
    if (wizard()->isSourceCloudOne())
        fResult = !machineId(m_pProfileInstanceList).isEmpty();
    else
        fResult =    UICommon::hasAllowedExtension(path(m_pFileSelector), OVFFileExts)
                  && QFile::exists(path(m_pFileSelector));

    /* Return result: */
    return fResult;
}

bool UIWizardImportAppPageBasic1::validatePage()
{
    /* Initial result: */
    bool fResult = true;

    /* Check whether there was cloud source selected: */
    if (wizard()->isSourceCloudOne())
    {
        /* Update cloud stuff: */
        updateCloudStuff();
        /* Which is required to continue to the next page: */
        fResult =    wizard()->cloudAppliance().isNotNull()
                  && wizard()->vsdImportForm().isNotNull();
    }
    else
    {
        /* Update local stuff (only if something changed): */
        if (m_pFileSelector->isModified())
        {
            updateLocalStuff();
            m_pFileSelector->resetModified();
        }
        /* Which is required to continue to the next page: */
        fResult = wizard()->localAppliance().isNotNull();
    }

    /* Return result: */
    return fResult;
}

void UIWizardImportAppPageBasic1::sltHandleSourceComboChange()
{
    /* Update combo tool-tip: */
    updateSourceComboToolTip(m_pSourceComboBox);

    /* Update wizard fields: */
    wizard()->setSourceCloudOne(isSourceCloudOne(m_pSourceComboBox));

    /* Refresh page widgets: */
    refreshStackedWidget(m_pSettingsWidget1,
                         wizard()->isSourceCloudOne());
    refreshProfileCombo(m_pProfileComboBox,
                        source(m_pSourceComboBox),
                        wizard()->isSourceCloudOne());

    /* Update profile instances: */
    sltHandleProfileComboChange();

    /* Notify about changes: */
    emit completeChanged();
}

void UIWizardImportAppPageBasic1::sltHandleProfileComboChange()
{
    /* Refresh required settings: */
    refreshCloudProfileInstances(m_pProfileInstanceList,
                                 source(m_pSourceComboBox),
                                 profileName(m_pProfileComboBox),
                                 wizard()->isSourceCloudOne());

    /* Notify about changes: */
    emit completeChanged();
}

void UIWizardImportAppPageBasic1::sltHandleProfileButtonClick()
{
    /* Open Cloud Profile Manager: */
    if (gpManager)
        gpManager->openCloudProfileManager();
}

void UIWizardImportAppPageBasic1::updateLocalStuff()
{
    /* Create local appliance: */
    wizard()->setFile(path(m_pFileSelector));
}

void UIWizardImportAppPageBasic1::updateCloudStuff()
{
    /* Create cloud appliance and VSD import form: */
    CAppliance comAppliance;
    CVirtualSystemDescriptionForm comForm;
    refreshCloudStuff(comAppliance,
                      comForm,
                      wizard(),
                      machineId(m_pProfileInstanceList),
                      source(m_pSourceComboBox),
                      profileName(m_pProfileComboBox),
                      wizard()->isSourceCloudOne());
    wizard()->setCloudAppliance(comAppliance);
    wizard()->setVsdImportForm(comForm);
}
