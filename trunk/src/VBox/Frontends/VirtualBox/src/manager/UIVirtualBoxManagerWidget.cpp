/* $Id$ */
/** @file
 * VBox Qt GUI - UIVirtualBoxManagerWidget class implementation.
 */

/*
 * Copyright (C) 2006-2018 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifdef VBOX_WITH_PRECOMPILED_HEADERS
# include <precomp.h>
#else  /* !VBOX_WITH_PRECOMPILED_HEADERS */

/* Qt includes: */
# include <QStyle>
# include <QVBoxLayout>

/* GUI includes: */
# include "QISplitter.h"
# include "UIActionPoolSelector.h"
# include "UIErrorString.h"
# include "UIExtraDataManager.h"
# include "UIChooser.h"
# include "UIVirtualBoxManager.h"
# include "UIVirtualBoxManagerWidget.h"
# include "UISlidingWidget.h"
# include "UITabBar.h"
# include "UIToolBar.h"
# include "UIVirtualMachineItem.h"
# include "UIToolbarTools.h"
# ifndef VBOX_WS_MAC
#  include "UIMenuBar.h"
# endif

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


UIVirtualBoxManagerWidget::UIVirtualBoxManagerWidget(UIVirtualBoxManager *pParent)
    : m_fPolished(false)
    , m_pActionPool(pParent->actionPool())
    , m_pSlidingWidget(0)
    , m_pSplitter(0)
    , m_pToolBar(0)
    , m_pTabBarMachine(0)
    , m_pTabBarGlobal(0)
    , m_pActionTabBarMachine(0)
    , m_pActionTabBarGlobal(0)
    , m_pToolbarTools(0)
    , m_pPaneChooser(0)
    , m_pPaneToolsMachine(0)
    , m_pPaneToolsGlobal(0)
{
    prepare();
}

UIVirtualBoxManagerWidget::~UIVirtualBoxManagerWidget()
{
    cleanup();
}

UIVirtualMachineItem *UIVirtualBoxManagerWidget::currentItem() const
{
    return m_pPaneChooser->currentItem();
}

QList<UIVirtualMachineItem*> UIVirtualBoxManagerWidget::currentItems() const
{
    return m_pPaneChooser->currentItems();
}

bool UIVirtualBoxManagerWidget::isGroupSavingInProgress() const
{
    return m_pPaneChooser->isGroupSavingInProgress();
}

bool UIVirtualBoxManagerWidget::isAllItemsOfOneGroupSelected() const
{
    return m_pPaneChooser->isAllItemsOfOneGroupSelected();
}

bool UIVirtualBoxManagerWidget::isSingleGroupSelected() const
{
    return m_pPaneChooser->isSingleGroupSelected();
}

bool UIVirtualBoxManagerWidget::isToolOpened(ToolTypeMachine enmType) const
{
    return m_pPaneToolsMachine->isToolOpened(enmType);
}

bool UIVirtualBoxManagerWidget::isToolOpened(ToolTypeGlobal enmType) const
{
    return m_pPaneToolsGlobal->isToolOpened(enmType);
}

void UIVirtualBoxManagerWidget::switchToTool(ToolTypeMachine enmType)
{
    sltHandleToolOpenedMachine(enmType);
}

void UIVirtualBoxManagerWidget::switchToTool(ToolTypeGlobal enmType)
{
    sltHandleToolOpenedGlobal(enmType);
}

void UIVirtualBoxManagerWidget::sltHandleContextMenuRequest(const QPoint &position)
{
    /* Populate toolbar actions: */
    QList<QAction*> actions;
    /* Add 'Show Toolbar Text' action: */
    QAction *pShowToolBarText = new QAction(tr("Show Toolbar Text"), 0);
    AssertPtrReturnVoid(pShowToolBarText);
    {
        /* Configure action: */
        pShowToolBarText->setCheckable(true);
        pShowToolBarText->setChecked(m_pToolBar->toolButtonStyle() == Qt::ToolButtonTextUnderIcon);

        /* Add into action list: */
        actions << pShowToolBarText;
    }

    /* Prepare the menu position: */
    QPoint globalPosition = position;
    QWidget *pSender = static_cast<QWidget*>(sender());
    if (pSender)
        globalPosition = pSender->mapToGlobal(position);

    /* Execute the menu: */
    QAction *pResult = QMenu::exec(actions, globalPosition);

    /* Handle the menu execution result: */
    if (pResult == pShowToolBarText)
    {
        m_pToolBar->setToolButtonStyle(  pResult->isChecked()
                                       ? Qt::ToolButtonTextUnderIcon
                                       : Qt::ToolButtonIconOnly);
        m_pToolbarTools->setToolButtonStyle(  pResult->isChecked()
                                            ? Qt::ToolButtonTextUnderIcon
                                            : Qt::ToolButtonIconOnly);
    }
}

void UIVirtualBoxManagerWidget::retranslateUi()
{
    /* Make sure chosen item fetched: */
    sltHandleChooserPaneIndexChange(false /* update details? */, false /* update snapshots? */, false /* update the logviewer? */);

#ifdef VBOX_WS_MAC
    // WORKAROUND:
    // There is a bug in Qt Cocoa which result in showing a "more arrow" when
    // the necessary size of the toolbar is increased. Also for some languages
    // the with doesn't match if the text increase. So manually adjust the size
    // after changing the text.
    m_pToolBar->updateLayout();
#endif
}

void UIVirtualBoxManagerWidget::showEvent(QShowEvent *pEvent)
{
    /* Call to base-class: */
    QIWithRetranslateUI<QWidget>::showEvent(pEvent);

    /* Is polishing required? */
    if (!m_fPolished)
    {
        /* Pass the show-event to polish-event: */
        polishEvent(pEvent);
        /* Mark as polished: */
        m_fPolished = true;
    }
}

void UIVirtualBoxManagerWidget::polishEvent(QShowEvent *)
{
    /* Call for async polishing finally: */
    QMetaObject::invokeMethod(this, "sltHandlePolishEvent", Qt::QueuedConnection);
}

void UIVirtualBoxManagerWidget::sltHandlePolishEvent()
{
    /* Get current item: */
    UIVirtualMachineItem *pItem = currentItem();

    /* Make sure there is accessible VM item chosen: */
    if (pItem && pItem->accessible())
    {
        // WORKAROUND:
        // By some reason some of X11 DEs unable to update() tab-bars on startup.
        // Let's just _create_ them later, asynchronously after the showEvent().
        /* Restore previously opened Machine tools at startup: */
        QMap<ToolTypeMachine, QAction*> mapActionsMachine;
        mapActionsMachine[ToolTypeMachine_Details] = actionPool()->action(UIActionIndexST_M_Tools_M_Machine_S_Details);
        mapActionsMachine[ToolTypeMachine_Snapshots] = actionPool()->action(UIActionIndexST_M_Tools_M_Machine_S_Snapshots);
        mapActionsMachine[ToolTypeMachine_LogViewer] = actionPool()->action(UIActionIndexST_M_Tools_M_Machine_S_LogViewer);
        for (int i = m_orderMachine.size() - 1; i >= 0; --i)
            if (m_orderMachine.at(i) != ToolTypeMachine_Invalid)
                mapActionsMachine.value(m_orderMachine.at(i))->trigger();
        /* Make sure further action triggering cause tool type switch as well: */
        actionPool()->action(UIActionIndexST_M_Tools_T_Machine)->setProperty("watch_child_activation", true);
    }
}

void UIVirtualBoxManagerWidget::sltHandleChooserPaneIndexChange(bool fUpdateDetails /* = true */,
                                                                bool fUpdateSnapshots /* = true */,
                                                                bool fUpdateLogViewer /* = true */)
{
    /* Let the parent know: */
    emit sigChooserPaneIndexChange();

    /* Get current item: */
    UIVirtualMachineItem *pItem = currentItem();

    /* Update Tools-pane: */
    m_pPaneToolsMachine->setCurrentItem(pItem);

    /* Update Machine tab-bar visibility */
    m_pTabBarMachine->setEnabled(pItem && pItem->accessible());

    /* If current item exists & accessible: */
    if (pItem && pItem->accessible())
    {
        /* If Desktop pane is chosen currently: */
        if (m_pPaneToolsMachine->currentTool() == ToolTypeMachine_Desktop)
        {
            /* Make sure Details or Snapshot pane is chosen if opened: */
            if (m_pPaneToolsMachine->isToolOpened(ToolTypeMachine_Details))
                actionPool()->action(UIActionIndexST_M_Tools_M_Machine_S_Details)->trigger();
            else
            if (m_pPaneToolsMachine->isToolOpened(ToolTypeMachine_Snapshots))
                actionPool()->action(UIActionIndexST_M_Tools_M_Machine_S_Snapshots)->trigger();
            else
            if (m_pPaneToolsMachine->isToolOpened(ToolTypeMachine_LogViewer))
                actionPool()->action(UIActionIndexST_M_Tools_M_Machine_S_LogViewer)->trigger();
        }

        /* Update Details-pane (if requested): */
        if (   fUpdateDetails
            && m_pPaneToolsMachine->isToolOpened(ToolTypeMachine_Details))
            m_pPaneToolsMachine->setItems(currentItems());
        /* Update the Snapshots-pane or/and Logviewer-pane (if requested): */
        if (fUpdateSnapshots || fUpdateLogViewer)
            m_pPaneToolsMachine->setMachine(pItem->machine());
    }
    else
    {
        /* Make sure Desktop-pane raised: */
        m_pPaneToolsMachine->openTool(ToolTypeMachine_Desktop);

        /* Note that the machine becomes inaccessible (or if the last VM gets
         * deleted), we have to update all fields, ignoring input arguments. */
        if (pItem)
        {
            /* The VM is inaccessible: */
            m_pPaneToolsMachine->setDetailsError(UIErrorString::formatErrorInfo(pItem->accessError()));
        }

        /* Update Details-pane (in any case): */
        if (m_pPaneToolsMachine->isToolOpened(ToolTypeMachine_Details))
            m_pPaneToolsMachine->setItems(currentItems());
        /* Update Snapshots-pane and Logviewer-pane (in any case): */
        m_pPaneToolsMachine->setMachine(CMachine());
    }
}

void UIVirtualBoxManagerWidget::sltHandleToolsTypeSwitch()
{
    /* If Machine tool button is checked => go backward: */
    if (actionPool()->action(UIActionIndexST_M_Tools_T_Machine)->isChecked())
        m_pSlidingWidget->moveBackward();

    else

    /* If Global tool button is checked => go forward: */
    if (actionPool()->action(UIActionIndexST_M_Tools_T_Global)->isChecked())
        m_pSlidingWidget->moveForward();

    /* Update action visibility: */
    emit sigToolsTypeSwitch();

    /* Make sure chosen item fetched: */
    sltHandleChooserPaneIndexChange(false /* update details? */, false /* update snapshots? */, false /* update the logviewer? */);
}

void UIVirtualBoxManagerWidget::sltHandleShowTabBarMachine()
{
    m_pActionTabBarGlobal->setVisible(false);
    m_pActionTabBarMachine->setVisible(true);
}

void UIVirtualBoxManagerWidget::sltHandleShowTabBarGlobal()
{
    m_pActionTabBarMachine->setVisible(false);
    m_pActionTabBarGlobal->setVisible(true);
}

void UIVirtualBoxManagerWidget::sltHandleToolOpenedMachine(ToolTypeMachine enmType)
{
    /* First, make sure corresponding tool set opened: */
    if (   !actionPool()->action(UIActionIndexST_M_Tools_T_Machine)->isChecked()
        && actionPool()->action(UIActionIndexST_M_Tools_T_Machine)->property("watch_child_activation").toBool())
        actionPool()->action(UIActionIndexST_M_Tools_T_Machine)->setChecked(true);

    /* Open corresponding tool: */
    m_pPaneToolsMachine->openTool(enmType);
    /* If that was 'Details' => pass there current items: */
    if (   enmType == ToolTypeMachine_Details
        && m_pPaneToolsMachine->isToolOpened(ToolTypeMachine_Details))
        m_pPaneToolsMachine->setItems(currentItems());
    /* If that was 'Snapshot' or 'LogViewer' => pass there current or null machine: */
    if (enmType == ToolTypeMachine_Snapshots || enmType == ToolTypeMachine_LogViewer)
    {
        UIVirtualMachineItem *pItem = currentItem();
        m_pPaneToolsMachine->setMachine(pItem ? pItem->machine() : CMachine());
    }
}

void UIVirtualBoxManagerWidget::sltHandleToolOpenedGlobal(ToolTypeGlobal enmType)
{
    /* First, make sure corresponding tool set opened: */
    if (   !actionPool()->action(UIActionIndexST_M_Tools_T_Global)->isChecked()
        && actionPool()->action(UIActionIndexST_M_Tools_T_Global)->property("watch_child_activation").toBool())
        actionPool()->action(UIActionIndexST_M_Tools_T_Global)->setChecked(true);

    /* Open corresponding tool: */
    m_pPaneToolsGlobal->openTool(enmType);
}

void UIVirtualBoxManagerWidget::sltHandleToolClosedMachine(ToolTypeMachine enmType)
{
    /* Close corresponding tool: */
    m_pPaneToolsMachine->closeTool(enmType);
}

void UIVirtualBoxManagerWidget::sltHandleToolClosedGlobal(ToolTypeGlobal enmType)
{
    /* Close corresponding tool: */
    m_pPaneToolsGlobal->closeTool(enmType);
}

void UIVirtualBoxManagerWidget::prepare()
{
    /* Prepare: */
    prepareToolbar();
    prepareWidgets();
    prepareConnections();

    /* Load settings: */
    loadSettings();

    /* Translate UI: */
    retranslateUi();

    /* Make sure current Chooser-pane index fetched: */
    sltHandleChooserPaneIndexChange();
}

void UIVirtualBoxManagerWidget::prepareToolbar()
{
    /* Create Main toolbar: */
    m_pToolBar = new UIToolBar(this);
    if (m_pToolBar)
    {
        /* Configure toolbar: */
        const int iIconMetric = QApplication::style()->pixelMetric(QStyle::PM_LargeIconSize);
        m_pToolBar->setIconSize(QSize(iIconMetric, iIconMetric));
        m_pToolBar->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
        m_pToolBar->setContextMenuPolicy(Qt::CustomContextMenu);
        m_pToolBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);

        /* Add main actions block: */
        m_pToolBar->addAction(actionPool()->action(UIActionIndexST_M_Machine_S_New));
        m_pToolBar->addAction(actionPool()->action(UIActionIndexST_M_Machine_S_Settings));
        m_pToolBar->addAction(actionPool()->action(UIActionIndexST_M_Machine_S_Discard));
        m_pToolBar->addAction(actionPool()->action(UIActionIndexST_M_Machine_M_StartOrShow));
#ifdef VBOX_WS_MAC
        // WORKAROUND:
        // Actually Qt should do that itself but by some unknown reason it sometimes
        // forget to update toolbar after changing its actions on cocoa platform.
        connect(actionPool()->action(UIActionIndexST_M_Machine_S_New), &UIAction::changed,
                m_pToolBar, static_cast<void(UIToolBar::*)(void)>(&UIToolBar::update));
        connect(actionPool()->action(UIActionIndexST_M_Machine_S_Settings), &UIAction::changed,
                m_pToolBar, static_cast<void(UIToolBar::*)(void)>(&UIToolBar::update));
        connect(actionPool()->action(UIActionIndexST_M_Machine_S_Discard), &UIAction::changed,
                m_pToolBar, static_cast<void(UIToolBar::*)(void)>(&UIToolBar::update));
        connect(actionPool()->action(UIActionIndexST_M_Machine_M_StartOrShow), &UIAction::changed,
                m_pToolBar, static_cast<void(UIToolBar::*)(void)>(&UIToolBar::update));
#endif /* VBOX_WS_MAC */

        /* Create Machine tab-bar: */
        m_pTabBarMachine = new UITabBar;
        if (m_pTabBarMachine)
        {
            /* Configure tab-bar: */
            const int iL = qApp->style()->pixelMetric(QStyle::PM_LayoutLeftMargin);
            const int iR = qApp->style()->pixelMetric(QStyle::PM_LayoutRightMargin);
            m_pTabBarMachine->setContentsMargins(iL, 0, iR, 0);

            /* Add into toolbar: */
            m_pActionTabBarMachine = m_pToolBar->addWidget(m_pTabBarMachine);
        }

        /* Create Global tab-bar: */
        m_pTabBarGlobal = new UITabBar;
        if (m_pTabBarGlobal)
        {
            /* Configure tab-bar: */
            const int iL = qApp->style()->pixelMetric(QStyle::PM_LayoutLeftMargin);
            const int iR = qApp->style()->pixelMetric(QStyle::PM_LayoutRightMargin);
            m_pTabBarGlobal->setContentsMargins(iL, 0, iR, 0);

            /* Add into toolbar: */
            m_pActionTabBarGlobal = m_pToolBar->addWidget(m_pTabBarGlobal);
        }

        /* Create Tools toolbar: */
        m_pToolbarTools = new UIToolbarTools(actionPool());
        if (m_pToolbarTools)
        {
            /* Configure toolbar: */
            m_pToolbarTools->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::MinimumExpanding);
            connect(m_pToolbarTools, &UIToolbarTools::sigShowTabBarMachine,
                    this, &UIVirtualBoxManagerWidget::sltHandleShowTabBarMachine);
            connect(m_pToolbarTools, &UIToolbarTools::sigShowTabBarGlobal,
                    this, &UIVirtualBoxManagerWidget::sltHandleShowTabBarGlobal);
            m_pToolbarTools->setTabBars(m_pTabBarMachine, m_pTabBarGlobal);

            /* Create exclusive action-group: */
            QActionGroup *pActionGroupTools = new QActionGroup(m_pToolbarTools);
            if (pActionGroupTools)
            {
                /* Configure action-group: */
                pActionGroupTools->setExclusive(true);

                /* Add 'Tools' actions into action-group: */
                pActionGroupTools->addAction(actionPool()->action(UIActionIndexST_M_Tools_T_Machine));
                pActionGroupTools->addAction(actionPool()->action(UIActionIndexST_M_Tools_T_Global));
            }

            /* Add into toolbar: */
            m_pToolBar->addWidget(m_pToolbarTools);
        }

#ifdef VBOX_WS_MAC
        // WORKAROUND:
        // There is a bug in Qt Cocoa which result in showing a "more arrow" when
        // the necessary size of the toolbar is increased. Also for some languages
        // the with doesn't match if the text increase. So manually adjust the size
        // after changing the text.
        m_pToolBar->updateLayout();
#endif
    }
}

void UIVirtualBoxManagerWidget::prepareWidgets()
{
    /* Create central-layout: */
    QVBoxLayout *pLayout = new QVBoxLayout(this);
    if (pLayout)
    {
        /* Configure layout: */
        pLayout->setSpacing(0);
        pLayout->setContentsMargins(0, 0, 0, 0);

        /* Add into layout: */
        pLayout->addWidget(m_pToolBar);

        /* Create sliding-widget: */
        m_pSlidingWidget = new UISlidingWidget;
        if (m_pSlidingWidget)
        {
            /* Create splitter: */
            m_pSplitter = new QISplitter;
            if (m_pSplitter)
            {
                /* Configure splitter: */
#ifdef VBOX_WS_X11
                m_pSplitter->setHandleType(QISplitter::Native);
#endif

                /* Create Chooser-pane: */
                m_pPaneChooser = new UIChooser(this);
                if (m_pPaneChooser)
                {
                    /* Add into splitter: */
                    m_pSplitter->addWidget(m_pPaneChooser);
                }

                /* Create Machine Tools-pane: */
                m_pPaneToolsMachine = new UIToolPaneMachine(actionPool());
                if (m_pPaneToolsMachine)
                {
                    /* Add into splitter: */
                    m_pSplitter->addWidget(m_pPaneToolsMachine);
                }

                /* Adjust splitter colors according to main widgets it splits: */
                m_pSplitter->configureColors(m_pPaneChooser->palette().color(QPalette::Active, QPalette::Window),
                                             m_pPaneToolsMachine->palette().color(QPalette::Active, QPalette::Window));
                /* Set the initial distribution. The right site is bigger. */
                m_pSplitter->setStretchFactor(0, 2);
                m_pSplitter->setStretchFactor(1, 3);
            }

            /* Create Global Tools-pane: */
            m_pPaneToolsGlobal = new UIToolPaneGlobal(actionPool());
            if (m_pPaneToolsGlobal)

            /* Add left/right widgets into sliding widget: */
            m_pSlidingWidget->setWidgets(m_pSplitter, m_pPaneToolsGlobal);

            /* Add into layout: */
            pLayout->addWidget(m_pSlidingWidget);
        }
    }

    /* Bring the VM list to the focus: */
    m_pPaneChooser->setFocus();
}

void UIVirtualBoxManagerWidget::prepareConnections()
{
    /* Tool-bar connections: */
    connect(m_pToolBar, &UIToolBar::customContextMenuRequested,
            this, &UIVirtualBoxManagerWidget::sltHandleContextMenuRequest);
    connect(m_pToolbarTools, &UIToolbarTools::sigToolOpenedMachine,
            this, &UIVirtualBoxManagerWidget::sltHandleToolOpenedMachine);
    connect(m_pToolbarTools, &UIToolbarTools::sigToolOpenedGlobal,
            this, &UIVirtualBoxManagerWidget::sltHandleToolOpenedGlobal);
    connect(m_pToolbarTools, &UIToolbarTools::sigToolClosedMachine,
            this, &UIVirtualBoxManagerWidget::sltHandleToolClosedMachine);
    connect(m_pToolbarTools, &UIToolbarTools::sigToolClosedGlobal,
            this, &UIVirtualBoxManagerWidget::sltHandleToolClosedGlobal);

    /* 'Tools' actions connections: */
    connect(actionPool()->action(UIActionIndexST_M_Tools_T_Machine), &UIAction::toggled,
            this, &UIVirtualBoxManagerWidget::sltHandleToolsTypeSwitch);
    connect(actionPool()->action(UIActionIndexST_M_Tools_T_Global), &UIAction::toggled,
            this, &UIVirtualBoxManagerWidget::sltHandleToolsTypeSwitch);

    /* Chooser-pane connections: */
    connect(m_pPaneChooser, &UIChooser::sigSelectionChanged,
            this, &UIVirtualBoxManagerWidget::sltHandleChooserPaneIndexChangeDefault);
    connect(m_pPaneChooser, &UIChooser::sigSlidingStarted,
            m_pPaneToolsMachine, &UIToolPaneMachine::sigSlidingStarted);
    connect(m_pPaneChooser, &UIChooser::sigToggleStarted,
            m_pPaneToolsMachine, &UIToolPaneMachine::sigToggleStarted);
    connect(m_pPaneChooser, &UIChooser::sigToggleFinished,
            m_pPaneToolsMachine, &UIToolPaneMachine::sigToggleFinished);
    connect(m_pPaneChooser, &UIChooser::sigGroupSavingStateChanged,
            this, &UIVirtualBoxManagerWidget::sigGroupSavingStateChanged);

    /* Details-pane connections: */
    connect(m_pPaneToolsMachine, &UIToolPaneMachine::sigLinkClicked,
            this, &UIVirtualBoxManagerWidget::sigMachineSettingsLinkClicked);
}

void UIVirtualBoxManagerWidget::loadSettings()
{
    /* Restore splitter handle position: */
    {
        /* Read splitter hints: */
        QList<int> sizes = gEDataManager->selectorWindowSplitterHints();
        /* If both hints are zero, we have the 'default' case: */
        if (sizes[0] == 0 && sizes[1] == 0)
        {
            /* Propose some 'default' based on current dialog width: */
            sizes[0] = (int)(width() * .9 * (1.0 / 3));
            sizes[1] = (int)(width() * .9 * (2.0 / 3));
        }
        /* Pass hints to the splitter: */
        m_pSplitter->setSizes(sizes);
    }

    /* Restore toolbar settings: */
    {
        m_pToolBar->setToolButtonStyle(gEDataManager->selectorWindowToolBarTextVisible()
                                       ? Qt::ToolButtonTextUnderIcon : Qt::ToolButtonIconOnly);
        m_pToolbarTools->setToolButtonStyle(gEDataManager->selectorWindowToolBarTextVisible()
                                            ? Qt::ToolButtonTextUnderIcon : Qt::ToolButtonIconOnly);
    }

    /* Restore toolbar Machine/Global tools orders:  */
    {
        m_orderMachine = gEDataManager->selectorWindowToolsOrderMachine();
        m_orderGlobal = gEDataManager->selectorWindowToolsOrderGlobal();

        /* We can restore previously opened Global tools right here: */
        QMap<ToolTypeGlobal, QAction*> mapActionsGlobal;
        mapActionsGlobal[ToolTypeGlobal_VirtualMedia] = actionPool()->action(UIActionIndexST_M_Tools_M_Global_S_VirtualMediaManager);
        mapActionsGlobal[ToolTypeGlobal_HostNetwork] = actionPool()->action(UIActionIndexST_M_Tools_M_Global_S_HostNetworkManager);
        for (int i = m_orderGlobal.size() - 1; i >= 0; --i)
            if (m_orderGlobal.at(i) != ToolTypeGlobal_Invalid)
                mapActionsGlobal.value(m_orderGlobal.at(i))->trigger();
        /* Make sure further action triggering cause tool type switch as well: */
        actionPool()->action(UIActionIndexST_M_Tools_T_Global)->setProperty("watch_child_activation", true);

        /* But we can't restore previously opened Machine tools here,
         * see the reason in corresponding async sltHandlePolishEvent slot. */
    }
}

void UIVirtualBoxManagerWidget::saveSettings()
{
    /* Save toolbar Machine/Global tools orders: */
    {
        gEDataManager->setSelectorWindowToolsOrderMachine(m_pToolbarTools->tabOrderMachine());
        gEDataManager->setSelectorWindowToolsOrderGlobal(m_pToolbarTools->tabOrderGlobal());
    }

    /* Save toolbar visibility: */
    {
        gEDataManager->setSelectorWindowToolBarVisible(!m_pToolBar->isHidden());
        gEDataManager->setSelectorWindowToolBarTextVisible(m_pToolBar->toolButtonStyle() == Qt::ToolButtonTextUnderIcon);
    }

    /* Save splitter handle position: */
    {
        gEDataManager->setSelectorWindowSplitterHints(m_pSplitter->sizes());
    }
}

void UIVirtualBoxManagerWidget::cleanup()
{
    /* Save settings: */
    saveSettings();
}
