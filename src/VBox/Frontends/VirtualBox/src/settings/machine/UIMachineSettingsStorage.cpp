/* $Id$ */
/** @file
 * VBox Qt GUI - UIMachineSettingsStorage class implementation.
 */

/*
 * Copyright (C) 2006-2019 Oracle Corporation
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
#include <QDrag>
#include <QDragMoveEvent>
#include <QHeaderView>
#include <QItemEditorFactory>
#include <QMenu>
#include <QMimeData>
#include <QMouseEvent>
#include <QScrollBar>
#include <QStylePainter>
#include <QTimer>
#include <QCommonStyle>
#include <QMetaProperty>

/* GUI includes: */
#include "QIFileDialog.h"
#include "QIMessageBox.h"
#include "QIWidgetValidator.h"
#include "UICommon.h"
#include "UIIconPool.h"
#include "UIWizardNewVD.h"
#include "UIErrorString.h"
#include "UIMessageCenter.h"
#include "UIMachineSettingsStorage.h"
#include "UIMediumSelector.h"
#include "UIConverter.h"
#include "UIMedium.h"
#include "UIExtraDataManager.h"
#include "UIModalWindowManager.h"

/* COM includes: */
#include "CStorageController.h"
#include "CMediumAttachment.h"


QString compressText (const QString &aText)
{
    return QString ("<nobr><compact elipsis=\"end\">%1</compact></nobr>").arg (aText);
}


/** Machine settings: Storage Attachment data structure. */
struct UIDataSettingsMachineStorageAttachment
{
    /** Constructs data. */
    UIDataSettingsMachineStorageAttachment()
        : m_attachmentType(KDeviceType_Null)
        , m_iAttachmentPort(-1)
        , m_iAttachmentDevice(-1)
        , m_uAttachmentMediumId(QUuid())
        , m_fAttachmentPassthrough(false)
        , m_fAttachmentTempEject(false)
        , m_fAttachmentNonRotational(false)
        , m_fAttachmentHotPluggable(false)
    {}

    /** Returns whether the @a other passed data is equal to this one. */
    bool equal(const UIDataSettingsMachineStorageAttachment &other) const
    {
        return true
               && (m_attachmentType == other.m_attachmentType)
               && (m_iAttachmentPort == other.m_iAttachmentPort)
               && (m_iAttachmentDevice == other.m_iAttachmentDevice)
               && (m_uAttachmentMediumId == other.m_uAttachmentMediumId)
               && (m_fAttachmentPassthrough == other.m_fAttachmentPassthrough)
               && (m_fAttachmentTempEject == other.m_fAttachmentTempEject)
               && (m_fAttachmentNonRotational == other.m_fAttachmentNonRotational)
               && (m_fAttachmentHotPluggable == other.m_fAttachmentHotPluggable)
               ;
    }

    /** Returns whether the @a other passed data is equal to this one. */
    bool operator==(const UIDataSettingsMachineStorageAttachment &other) const { return equal(other); }
    /** Returns whether the @a other passed data is different from this one. */
    bool operator!=(const UIDataSettingsMachineStorageAttachment &other) const { return !equal(other); }

    /** Holds the attachment type. */
    KDeviceType  m_attachmentType;
    /** Holds the attachment port. */
    LONG         m_iAttachmentPort;
    /** Holds the attachment device. */
    LONG         m_iAttachmentDevice;
    /** Holds the attachment medium ID. */
    QUuid        m_uAttachmentMediumId;
    /** Holds whether the attachment being passed through. */
    bool         m_fAttachmentPassthrough;
    /** Holds whether the attachment being temporarily eject. */
    bool         m_fAttachmentTempEject;
    /** Holds whether the attachment is solid-state. */
    bool         m_fAttachmentNonRotational;
    /** Holds whether the attachment is hot-pluggable. */
    bool         m_fAttachmentHotPluggable;
};


/** Machine settings: Storage Controller data structure. */
struct UIDataSettingsMachineStorageController
{
    /** Constructs data. */
    UIDataSettingsMachineStorageController()
        : m_strControllerName(QString())
        , m_controllerBus(KStorageBus_Null)
        , m_controllerType(KStorageControllerType_Null)
        , m_uPortCount(0)
        , m_fUseHostIOCache(false)
    {}

    /** Returns whether the @a other passed data is equal to this one. */
    bool equal(const UIDataSettingsMachineStorageController &other) const
    {
        return true
               && (m_strControllerName == other.m_strControllerName)
               && (m_controllerBus == other.m_controllerBus)
               && (m_controllerType == other.m_controllerType)
               && (m_uPortCount == other.m_uPortCount)
               && (m_fUseHostIOCache == other.m_fUseHostIOCache)
               ;
    }

    /** Returns whether the @a other passed data is equal to this one. */
    bool operator==(const UIDataSettingsMachineStorageController &other) const { return equal(other); }
    /** Returns whether the @a other passed data is different from this one. */
    bool operator!=(const UIDataSettingsMachineStorageController &other) const { return !equal(other); }

    /** Holds the controller name. */
    QString                 m_strControllerName;
    /** Holds the controller bus. */
    KStorageBus             m_controllerBus;
    /** Holds the controller type. */
    KStorageControllerType  m_controllerType;
    /** Holds the controller port count. */
    uint                    m_uPortCount;
    /** Holds whether the controller uses host IO cache. */
    bool                    m_fUseHostIOCache;
};


/** Machine settings: Storage page data structure. */
struct UIDataSettingsMachineStorage
{
    /** Constructs data. */
    UIDataSettingsMachineStorage() {}

    /** Returns whether the @a other passed data is equal to this one. */
    bool operator==(const UIDataSettingsMachineStorage & /* other */) const { return true; }
    /** Returns whether the @a other passed data is different from this one. */
    bool operator!=(const UIDataSettingsMachineStorage & /* other */) const { return false; }
};


/** UIIconPool interface extension used as Storage Settings page icon-pool. */
class UIIconPoolStorageSettings : public UIIconPool
{
public:

    /** Icon-pool instance access method. */
    static UIIconPoolStorageSettings* instance();
    /** Create icon-pool instance. */
    static void create();
    /** Destroy icon-pool instance. */
    static void destroy();

    /** Returns pixmap corresponding to passed @a pixmapType. */
    QPixmap pixmap(PixmapType pixmapType) const;
    /** Returns icon (probably merged) corresponding to passed @a pixmapType and @a pixmapDisabledType. */
    QIcon icon(PixmapType pixmapType, PixmapType pixmapDisabledType = InvalidPixmap) const;

private:

    /** Icon-pool constructor. */
    UIIconPoolStorageSettings();
    /** Icon-pool destructor. */
    ~UIIconPoolStorageSettings();

    /** Icon-pool instance. */
    static UIIconPoolStorageSettings *m_spInstance;
    /** Icon-pool names cache. */
    QMap<PixmapType, QString> m_names;
    /** Icon-pool icons cache. */
    mutable QMap<PixmapType, QIcon> m_icons;
};
UIIconPoolStorageSettings* iconPool() { return UIIconPoolStorageSettings::instance(); }


/* static */
UIIconPoolStorageSettings* UIIconPoolStorageSettings::m_spInstance = 0;
UIIconPoolStorageSettings* UIIconPoolStorageSettings::instance() { return m_spInstance; }
void UIIconPoolStorageSettings::create() { new UIIconPoolStorageSettings; }
void UIIconPoolStorageSettings::destroy() { delete m_spInstance; }

QPixmap UIIconPoolStorageSettings::pixmap(PixmapType pixmapType) const
{
    /* Prepare fallback pixmap: */
    static QPixmap nullPixmap;

    /* If we do NOT have that 'pixmap type' icon cached already: */
    if (!m_icons.contains(pixmapType))
    {
        /* Compose proper icon if we have that 'pixmap type' known: */
        if (m_names.contains(pixmapType))
            m_icons[pixmapType] = iconSet(m_names[pixmapType]);
        /* Assign fallback icon if we do NOT have that 'pixmap type' known: */
        else
            m_icons[pixmapType] = iconSet(nullPixmap);
    }

    /* Retrieve corresponding icon: */
    const QIcon &icon = m_icons[pixmapType];
    AssertMsgReturn(!icon.isNull(),
                    ("Undefined icon for type '%d'.", (int)pixmapType),
                    nullPixmap);

    /* Retrieve available sizes for that icon: */
    const QList<QSize> availableSizes = icon.availableSizes();
    AssertMsgReturn(!availableSizes.isEmpty(),
                    ("Undefined icon for type '%s'.", (int)pixmapType),
                    nullPixmap);

    /* Determine icon metric: */
    const QStyle *pStyle = QApplication::style();
    const int iIconMetric = pStyle->pixelMetric(QStyle::PM_SmallIconSize);

    /* Return pixmap of first available size: */
    return icon.pixmap(QSize(iIconMetric, iIconMetric));
}

QIcon UIIconPoolStorageSettings::icon(PixmapType pixmapType,
                                      PixmapType pixmapDisabledType /* = InvalidPixmap */) const
{
    /* Prepare fallback pixmap: */
    static QPixmap nullPixmap;
    /* Prepare fallback icon: */
    static QIcon nullIcon;

    /* If we do NOT have that 'pixmap type' icon cached already: */
    if (!m_icons.contains(pixmapType))
    {
        /* Compose proper icon if we have that 'pixmap type' known: */
        if (m_names.contains(pixmapType))
            m_icons[pixmapType] = iconSet(m_names[pixmapType]);
        /* Assign fallback icon if we do NOT have that 'pixmap type' known: */
        else
            m_icons[pixmapType] = iconSet(nullPixmap);
    }

    /* Retrieve normal icon: */
    const QIcon &icon = m_icons[pixmapType];
    AssertMsgReturn(!icon.isNull(),
                    ("Undefined icon for type '%d'.", (int)pixmapType),
                    nullIcon);

    /* If 'disabled' icon is invalid => just return 'normal' icon: */
    if (pixmapDisabledType == InvalidPixmap)
        return icon;

    /* If we do NOT have that 'pixmap disabled type' icon cached already: */
    if (!m_icons.contains(pixmapDisabledType))
    {
        /* Compose proper icon if we have that 'pixmap disabled type' known: */
        if (m_names.contains(pixmapDisabledType))
            m_icons[pixmapDisabledType] = iconSet(m_names[pixmapDisabledType]);
        /* Assign fallback icon if we do NOT have that 'pixmap disabled type' known: */
        else
            m_icons[pixmapDisabledType] = iconSet(nullPixmap);
    }

    /* Retrieve disabled icon: */
    const QIcon &iconDisabled = m_icons[pixmapDisabledType];
    AssertMsgReturn(!iconDisabled.isNull(),
                    ("Undefined icon for type '%d'.", (int)pixmapDisabledType),
                    nullIcon);

    /* Return icon composed on the basis of two above: */
    QIcon resultIcon = icon;
    foreach (const QSize &size, iconDisabled.availableSizes())
        resultIcon.addPixmap(iconDisabled.pixmap(size), QIcon::Disabled);
    return resultIcon;
}

UIIconPoolStorageSettings::UIIconPoolStorageSettings()
{
    /* Connect instance: */
    m_spInstance = this;

    /* Controller file-names: */
    m_names.insert(ControllerAddEn,              ":/controller_add_16px.png");
    m_names.insert(ControllerAddDis,             ":/controller_add_disabled_16px.png");
    m_names.insert(ControllerDelEn,              ":/controller_remove_16px.png");
    m_names.insert(ControllerDelDis,             ":/controller_remove_disabled_16px.png");
    /* Attachment file-names: */
    m_names.insert(AttachmentAddEn,              ":/attachment_add_16px.png");
    m_names.insert(AttachmentAddDis,             ":/attachment_add_disabled_16px.png");
    m_names.insert(AttachmentDelEn,              ":/attachment_remove_16px.png");
    m_names.insert(AttachmentDelDis,             ":/attachment_remove_disabled_16px.png");
    /* Specific controller default/expand/collapse file-names: */
    m_names.insert(IDEControllerNormal,          ":/ide_16px.png");
    m_names.insert(IDEControllerExpand,          ":/ide_expand_16px.png");
    m_names.insert(IDEControllerCollapse,        ":/ide_collapse_16px.png");
    m_names.insert(SATAControllerNormal,         ":/sata_16px.png");
    m_names.insert(SATAControllerExpand,         ":/sata_expand_16px.png");
    m_names.insert(SATAControllerCollapse,       ":/sata_collapse_16px.png");
    m_names.insert(SCSIControllerNormal,         ":/scsi_16px.png");
    m_names.insert(SCSIControllerExpand,         ":/scsi_expand_16px.png");
    m_names.insert(SCSIControllerCollapse,       ":/scsi_collapse_16px.png");
    m_names.insert(SASControllerNormal,          ":/sata_16px.png");
    m_names.insert(SASControllerExpand,          ":/sata_expand_16px.png");
    m_names.insert(SASControllerCollapse,        ":/sata_collapse_16px.png");
    m_names.insert(USBControllerNormal,          ":/usb_16px.png");
    m_names.insert(USBControllerExpand,          ":/usb_expand_16px.png");
    m_names.insert(USBControllerCollapse,        ":/usb_collapse_16px.png");
    m_names.insert(NVMeControllerNormal,         ":/ide_16px.png");
    m_names.insert(NVMeControllerExpand,         ":/ide_expand_16px.png");
    m_names.insert(NVMeControllerCollapse,       ":/ide_collapse_16px.png");
    m_names.insert(VirtioSCSIControllerNormal,   ":/scsi_16px.png");
    m_names.insert(VirtioSCSIControllerExpand,   ":/scsi_expand_16px.png");
    m_names.insert(VirtioSCSIControllerCollapse, ":/scsi_collapse_16px.png");
    m_names.insert(FloppyControllerNormal,       ":/floppy_16px.png");
    m_names.insert(FloppyControllerExpand,       ":/floppy_expand_16px.png");
    m_names.insert(FloppyControllerCollapse,     ":/floppy_collapse_16px.png");
    /* Specific controller add file-names: */
    m_names.insert(IDEControllerAddEn,           ":/ide_add_16px.png");
    m_names.insert(IDEControllerAddDis,          ":/ide_add_disabled_16px.png");
    m_names.insert(SATAControllerAddEn,          ":/sata_add_16px.png");
    m_names.insert(SATAControllerAddDis,         ":/sata_add_disabled_16px.png");
    m_names.insert(SCSIControllerAddEn,          ":/scsi_add_16px.png");
    m_names.insert(SCSIControllerAddDis,         ":/scsi_add_disabled_16px.png");
    m_names.insert(SASControllerAddEn,           ":/sata_add_16px.png");
    m_names.insert(SASControllerAddDis,          ":/sata_add_disabled_16px.png");
    m_names.insert(USBControllerAddEn,           ":/usb_add_16px.png");
    m_names.insert(USBControllerAddDis,          ":/usb_add_disabled_16px.png");
    m_names.insert(NVMeControllerAddEn,          ":/ide_add_16px.png");
    m_names.insert(NVMeControllerAddDis,         ":/ide_add_disabled_16px.png");
    m_names.insert(VirtioSCSIControllerAddEn,    ":/scsi_add_16px.png");
    m_names.insert(VirtioSCSIControllerAddDis,   ":/scsi_add_disabled_16px.png");
    m_names.insert(FloppyControllerAddEn,        ":/floppy_add_16px.png");
    m_names.insert(FloppyControllerAddDis,       ":/floppy_add_disabled_16px.png");
    /* Specific attachment file-names: */
    m_names.insert(HDAttachmentNormal,           ":/hd_16px.png");
    m_names.insert(CDAttachmentNormal,           ":/cd_16px.png");
    m_names.insert(FDAttachmentNormal,           ":/fd_16px.png");
    /* Specific attachment add file-names: */
    m_names.insert(HDAttachmentAddEn,            ":/hd_add_16px.png");
    m_names.insert(HDAttachmentAddDis,           ":/hd_add_disabled_16px.png");
    m_names.insert(CDAttachmentAddEn,            ":/cd_add_16px.png");
    m_names.insert(CDAttachmentAddDis,           ":/cd_add_disabled_16px.png");
    m_names.insert(FDAttachmentAddEn,            ":/fd_add_16px.png");
    m_names.insert(FDAttachmentAddDis,           ":/fd_add_disabled_16px.png");
    /* Specific attachment custom file-names: */
    m_names.insert(ChooseExistingEn,             ":/select_file_16px.png");
    m_names.insert(ChooseExistingDis,            ":/select_file_disabled_16px.png");
    m_names.insert(HDNewEn,                      ":/hd_new_16px.png");
    m_names.insert(HDNewDis,                     ":/hd_new_disabled_16px.png");
    m_names.insert(CDUnmountEnabled,             ":/cd_unmount_16px.png");
    m_names.insert(CDUnmountDisabled,            ":/cd_unmount_disabled_16px.png");
    m_names.insert(FDUnmountEnabled,             ":/fd_unmount_16px.png");
    m_names.insert(FDUnmountDisabled,            ":/fd_unmount_disabled_16px.png");
}

UIIconPoolStorageSettings::~UIIconPoolStorageSettings()
{
    /* Disconnect instance: */
    m_spInstance = 0;
}


/* Abstract Controller Type */
AbstractControllerType::AbstractControllerType (KStorageBus aBusType, KStorageControllerType aCtrType)
    : mBusType (aBusType)
    , mCtrType (aCtrType)
{
    AssertMsg (mBusType != KStorageBus_Null, ("Wrong Bus Type {%d}!\n", mBusType));
    AssertMsg (mCtrType != KStorageControllerType_Null, ("Wrong Controller Type {%d}!\n", mCtrType));

    updateBusInfo();
    updateTypeInfo();
    updatePixmaps();
}

KStorageBus AbstractControllerType::busType() const
{
    return mBusType;
}

ControllerBusList AbstractControllerType::busTypes() const
{
    return m_buses;
}

KStorageControllerType AbstractControllerType::ctrType() const
{
    return mCtrType;
}

ControllerTypeList AbstractControllerType::ctrTypes() const
{
    return m_types;
}

PixmapType AbstractControllerType::pixmap(ItemState aState) const
{
    return m_pixmaps.at(aState);
}

void AbstractControllerType::setCtrBusType(KStorageBus enmCtrBusType)
{
    mBusType = enmCtrBusType;

    updateBusInfo();
    updateTypeInfo();
    updatePixmaps();
}

void AbstractControllerType::setCtrType (KStorageControllerType aCtrType)
{
    mCtrType = aCtrType;
}

DeviceTypeList AbstractControllerType::deviceTypeList() const
{
    return uiCommon().virtualBox().GetSystemProperties().GetDeviceTypesForStorageBus (mBusType).toList();
}

void AbstractControllerType::updateBusInfo()
{
    m_buses.clear();

    switch (mBusType)
    {
        case KStorageBus_IDE:
        case KStorageBus_SATA:
        case KStorageBus_SCSI:
        case KStorageBus_SAS:
        case KStorageBus_USB:
        case KStorageBus_PCIe:
        case KStorageBus_VirtioSCSI:
        {
            m_buses << KStorageBus_IDE << KStorageBus_SATA << KStorageBus_SCSI << KStorageBus_SAS
                    << KStorageBus_USB << KStorageBus_PCIe << KStorageBus_VirtioSCSI;
            m_buses.removeAll(mBusType);
        }
        RT_FALL_THRU();
        default:
        {
            m_buses.prepend(mBusType);
            break;
        }
    }
}

void AbstractControllerType::updateTypeInfo()
{
    m_types.clear();

    KStorageControllerType enmFirstType = KStorageControllerType_Null;
    switch (mBusType)
    {
        case KStorageBus_IDE:        enmFirstType = KStorageControllerType_PIIX3; break;
        case KStorageBus_SATA:       enmFirstType = KStorageControllerType_IntelAhci; break;
        case KStorageBus_SCSI:       enmFirstType = KStorageControllerType_LsiLogic; break;
        case KStorageBus_Floppy:     enmFirstType = KStorageControllerType_I82078; break;
        case KStorageBus_SAS:        enmFirstType = KStorageControllerType_LsiLogicSas; break;
        case KStorageBus_USB:        enmFirstType = KStorageControllerType_USB; break;
        case KStorageBus_PCIe:       enmFirstType = KStorageControllerType_NVMe; break;
        case KStorageBus_VirtioSCSI: enmFirstType = KStorageControllerType_VirtioSCSI; break;
        default:                     break;
    }
    AssertMsg(enmFirstType != KStorageControllerType_Null, ("Invalid item type!\n"));

    uint uTypeAmount = 0;
    switch (mBusType)
    {
        case KStorageBus_IDE:        uTypeAmount = 3; break;
        case KStorageBus_SATA:       uTypeAmount = 1; break;
        case KStorageBus_SCSI:       uTypeAmount = 2; break;
        case KStorageBus_Floppy:     uTypeAmount = 1; break;
        case KStorageBus_SAS:        uTypeAmount = 1; break;
        case KStorageBus_USB:        uTypeAmount = 1; break;
        case KStorageBus_PCIe:       uTypeAmount = 1; break;
        case KStorageBus_VirtioSCSI: uTypeAmount = 1; break;
        default:                     break;
    }
    AssertMsg(uTypeAmount != 0, ("Invalid item type count!\n"));

    for (uint i = enmFirstType; i < enmFirstType + uTypeAmount; ++i)
        m_types << static_cast<KStorageControllerType>(i);
}

void AbstractControllerType::updatePixmaps()
{
    m_pixmaps.clear();

    for (int i = 0; i < State_MAX; ++i)
    {
        m_pixmaps << InvalidPixmap;
        switch (mBusType)
        {
            case KStorageBus_IDE:        m_pixmaps[i] = static_cast<PixmapType>(IDEControllerNormal + i); break;
            case KStorageBus_SATA:       m_pixmaps[i] = static_cast<PixmapType>(SATAControllerNormal + i); break;
            case KStorageBus_SCSI:       m_pixmaps[i] = static_cast<PixmapType>(SCSIControllerNormal + i); break;
            case KStorageBus_Floppy:     m_pixmaps[i] = static_cast<PixmapType>(FloppyControllerNormal + i); break;
            case KStorageBus_SAS:        m_pixmaps[i] = static_cast<PixmapType>(SASControllerNormal + i); break;
            case KStorageBus_USB:        m_pixmaps[i] = static_cast<PixmapType>(USBControllerNormal + i); break;
            case KStorageBus_PCIe:       m_pixmaps[i] = static_cast<PixmapType>(NVMeControllerNormal + i); break;
            case KStorageBus_VirtioSCSI: m_pixmaps[i] = static_cast<PixmapType>(VirtioSCSIControllerNormal + i); break;
            default: break;
        }
        AssertMsg(m_pixmaps[i] != InvalidPixmap, ("Invalid item state pixmap!\n"));
    }
}


/* Abstract Item */
AbstractItem::AbstractItem(QITreeView *pParent)
    : QITreeViewItem(pParent)
    , m_pParentItem(0)
    , mId(QUuid::createUuid())
{
    if (m_pParentItem)
        m_pParentItem->addChild(this);
}

AbstractItem::AbstractItem(AbstractItem *pParentItem)
    : QITreeViewItem(pParentItem)
    , m_pParentItem(pParentItem)
    , mId(QUuid::createUuid())
{
    if (m_pParentItem)
        m_pParentItem->addChild(this);
}

AbstractItem::~AbstractItem()
{
    if (m_pParentItem)
        m_pParentItem->delChild(this);
}

AbstractItem* AbstractItem::parent() const
{
    return m_pParentItem;
}

QUuid AbstractItem::id() const
{
    return mId;
}

QUuid AbstractItem::machineId() const
{
    return mMachineId;
}

void AbstractItem::setMachineId (const QUuid &uMachineId)
{
    mMachineId = uMachineId;
}


/* Root Item */
RootItem::RootItem(QITreeView *pParent)
    : AbstractItem(pParent)
{
}

RootItem::~RootItem()
{
    while (!mControllers.isEmpty())
        delete mControllers.first();
}

ULONG RootItem::childCount (KStorageBus aBus) const
{
    ULONG result = 0;
    foreach (AbstractItem *item, mControllers)
    {
        ControllerItem *ctrItem = qobject_cast<ControllerItem*>(item);
        if (ctrItem->ctrBusType() == aBus)
            ++ result;
    }
    return result;
}

AbstractItem::ItemType RootItem::rtti() const
{
    return Type_RootItem;
}

AbstractItem* RootItem::childItem (int aIndex) const
{
    return mControllers [aIndex];
}

AbstractItem* RootItem::childItemById (const QUuid &uId) const
{
    for (int i = 0; i < childCount(); ++ i)
        if (mControllers [i]->id() == uId)
            return mControllers [i];
    return 0;
}

int RootItem::posOfChild (AbstractItem *aItem) const
{
    return mControllers.indexOf (aItem);
}

int RootItem::childCount() const
{
    return mControllers.size();
}

QString RootItem::text() const
{
    return QString();
}

QString RootItem::tip() const
{
    return QString();
}

QPixmap RootItem::pixmap (ItemState /* aState */)
{
    return QPixmap();
}

void RootItem::addChild (AbstractItem *aItem)
{
    mControllers << aItem;
}

void RootItem::delChild (AbstractItem *aItem)
{
    mControllers.removeAll (aItem);
}


/* Controller Item */
ControllerItem::ControllerItem (AbstractItem *aParent, const QString &aName,
                                KStorageBus aBusType, KStorageControllerType aControllerType)
    : AbstractItem (aParent)
    , mOldCtrName (aName)
    , mCtrName (aName)
    , mCtrType (0)
    , mPortCount (0)
    , mUseIoCache (false)
{
    /* Check for proper parent type */
    AssertMsg(m_pParentItem->rtti() == AbstractItem::Type_RootItem, ("Incorrect parent type!\n"));

    /* Select default type */
    switch (aBusType)
    {
        case KStorageBus_IDE:
            mCtrType = new AbstractControllerType(KStorageBus_IDE, aControllerType);
            break;
        case KStorageBus_SATA:
            mCtrType = new AbstractControllerType(KStorageBus_SATA, aControllerType);
            break;
        case KStorageBus_SCSI:
            mCtrType = new AbstractControllerType(KStorageBus_SCSI, aControllerType);
            break;
        case KStorageBus_Floppy:
            mCtrType = new AbstractControllerType(KStorageBus_Floppy, aControllerType);
            break;
        case KStorageBus_SAS:
            mCtrType = new AbstractControllerType(KStorageBus_SAS, aControllerType);
            break;
        case KStorageBus_USB:
            mCtrType = new AbstractControllerType(KStorageBus_USB, aControllerType);
            break;
        case KStorageBus_PCIe:
            mCtrType = new AbstractControllerType(KStorageBus_PCIe, aControllerType);
            break;
        case KStorageBus_VirtioSCSI:
            mCtrType = new AbstractControllerType(KStorageBus_VirtioSCSI, aControllerType);
            break;

        default:
            AssertMsgFailed (("Wrong Controller Type {%d}!\n", aBusType));
            break;
    }

    mUseIoCache = uiCommon().virtualBox().GetSystemProperties().GetDefaultIoCacheSettingForStorageController (aControllerType);
}

ControllerItem::~ControllerItem()
{
    delete mCtrType;
    while (!mAttachments.isEmpty())
        delete mAttachments.first();
}

KStorageBus ControllerItem::ctrBusType() const
{
    return mCtrType->busType();
}

ControllerBusList ControllerItem::ctrBusTypes() const
{
    return mCtrType->busTypes();
}

QString ControllerItem::oldCtrName() const
{
    return mOldCtrName;
}

QString ControllerItem::ctrName() const
{
    return mCtrName;
}

KStorageControllerType ControllerItem::ctrType() const
{
    return mCtrType->ctrType();
}

ControllerTypeList ControllerItem::ctrTypes() const
{
    return mCtrType->ctrTypes();
}

uint ControllerItem::portCount()
{
    /* Recalculate actual port count: */
    for (int i = 0; i < mAttachments.size(); ++i)
    {
        AttachmentItem *pItem = qobject_cast<AttachmentItem*>(mAttachments.at(i));
        if (mPortCount < (uint)pItem->attSlot().port + 1)
            mPortCount = (uint)pItem->attSlot().port + 1;
    }
    return mPortCount;
}

uint ControllerItem::maxPortCount()
{
    return (uint)uiCommon().virtualBox().GetSystemProperties().GetMaxPortCountForStorageBus(ctrBusType());
}

bool ControllerItem::ctrUseIoCache() const
{
    return mUseIoCache;
}

void ControllerItem::setCtrBusType(KStorageBus enmCtrBusType)
{
    mCtrType->setCtrBusType(enmCtrBusType);
}

void ControllerItem::setCtrName (const QString &aCtrName)
{
    mCtrName = aCtrName;
}

void ControllerItem::setCtrType (KStorageControllerType aCtrType)
{
    mCtrType->setCtrType (aCtrType);
}

void ControllerItem::setPortCount (uint aPortCount)
{
    /* Limit maximum port count: */
    mPortCount = qMin(aPortCount, (uint)uiCommon().virtualBox().GetSystemProperties().GetMaxPortCountForStorageBus(ctrBusType()));
}

void ControllerItem::setCtrUseIoCache (bool aUseIoCache)
{
    mUseIoCache = aUseIoCache;
}

SlotsList ControllerItem::ctrAllSlots() const
{
    SlotsList allSlots;
    CSystemProperties sp = uiCommon().virtualBox().GetSystemProperties();
    for (ULONG i = 0; i < sp.GetMaxPortCountForStorageBus (mCtrType->busType()); ++ i)
        for (ULONG j = 0; j < sp.GetMaxDevicesPerPortForStorageBus (mCtrType->busType()); ++ j)
            allSlots << StorageSlot (mCtrType->busType(), i, j);
    return allSlots;
}

SlotsList ControllerItem::ctrUsedSlots() const
{
    SlotsList usedSlots;
    for (int i = 0; i < mAttachments.size(); ++ i)
        usedSlots << qobject_cast<AttachmentItem*>(mAttachments.at(i))->attSlot();
    return usedSlots;
}

DeviceTypeList ControllerItem::ctrDeviceTypeList() const
{
     return mCtrType->deviceTypeList();
}

QList<QUuid> ControllerItem::attachmentIDs(KDeviceType enmType /* = KDeviceType_Null */) const
{
    QList<QUuid> ids;
    foreach (AbstractItem *pItem, mAttachments)
    {
        AttachmentItem *pItemAttachment = qobject_cast<AttachmentItem*>(pItem);
        if (   enmType == KDeviceType_Null
            || pItemAttachment->attDeviceType() == enmType)
            ids << pItem->id();
    }
    return ids;
}

AbstractItem::ItemType ControllerItem::rtti() const
{
    return Type_ControllerItem;
}

AbstractItem* ControllerItem::childItem (int aIndex) const
{
    return mAttachments [aIndex];
}

AbstractItem* ControllerItem::childItemById (const QUuid &uId) const
{
    for (int i = 0; i < childCount(); ++ i)
        if (mAttachments [i]->id() == uId)
            return mAttachments [i];
    return 0;
}

int ControllerItem::posOfChild (AbstractItem *aItem) const
{
    return mAttachments.indexOf (aItem);
}

int ControllerItem::childCount() const
{
    return mAttachments.size();
}

QString ControllerItem::text() const
{
    return UIMachineSettingsStorage::tr("Controller: %1").arg(ctrName());
}

QString ControllerItem::tip() const
{
    return UIMachineSettingsStorage::tr ("<nobr><b>%1</b></nobr><br>"
                                 "<nobr>Bus:&nbsp;&nbsp;%2</nobr><br>"
                                 "<nobr>Type:&nbsp;&nbsp;%3</nobr>")
                                 .arg (mCtrName)
                                 .arg (gpConverter->toString (mCtrType->busType()))
                                 .arg (gpConverter->toString (mCtrType->ctrType()));
}

QPixmap ControllerItem::pixmap (ItemState aState)
{
    return iconPool()->pixmap(mCtrType->pixmap(aState));
}

void ControllerItem::addChild (AbstractItem *aItem)
{
    mAttachments << aItem;
}

void ControllerItem::delChild (AbstractItem *aItem)
{
    mAttachments.removeAll (aItem);
}


/* Attachment Item */
AttachmentItem::AttachmentItem (AbstractItem *aParent, KDeviceType aDeviceType)
    : AbstractItem (aParent)
    , mAttDeviceType (aDeviceType)
    , mAttIsHostDrive (false)
    , mAttIsPassthrough (false)
    , mAttIsTempEject (false)
    , mAttIsNonRotational (false)
    , m_fIsHotPluggable(false)
{
    /* Check for proper parent type */
    AssertMsg(m_pParentItem->rtti() == AbstractItem::Type_ControllerItem, ("Incorrect parent type!\n"));

    /* Select default slot */
    AssertMsg (!attSlots().isEmpty(), ("There should be at least one available slot!\n"));
    mAttSlot = attSlots() [0];
}

StorageSlot AttachmentItem::attSlot() const
{
    return mAttSlot;
}

SlotsList AttachmentItem::attSlots() const
{
    ControllerItem *ctr = qobject_cast<ControllerItem*>(m_pParentItem);

    /* Filter list from used slots */
    SlotsList allSlots (ctr->ctrAllSlots());
    SlotsList usedSlots (ctr->ctrUsedSlots());
    foreach (StorageSlot usedSlot, usedSlots)
        if (usedSlot != mAttSlot)
            allSlots.removeAll (usedSlot);

    return allSlots;
}

KDeviceType AttachmentItem::attDeviceType() const
{
    return mAttDeviceType;
}

DeviceTypeList AttachmentItem::attDeviceTypes() const
{
    return qobject_cast<ControllerItem*>(m_pParentItem)->ctrDeviceTypeList();
}

QUuid AttachmentItem::attMediumId() const
{
    return mAttMediumId;
}

bool AttachmentItem::attIsHostDrive() const
{
    return mAttIsHostDrive;
}

bool AttachmentItem::attIsPassthrough() const
{
    return mAttIsPassthrough;
}

bool AttachmentItem::attIsTempEject() const
{
    return mAttIsTempEject;
}

bool AttachmentItem::attIsNonRotational() const
{
    return mAttIsNonRotational;
}

bool AttachmentItem::attIsHotPluggable() const
{
    return m_fIsHotPluggable;
}

void AttachmentItem::setAttSlot (const StorageSlot &aAttSlot)
{
    mAttSlot = aAttSlot;
}

void AttachmentItem::setAttDevice (KDeviceType aAttDeviceType)
{
    mAttDeviceType = aAttDeviceType;
}

void AttachmentItem::setAttMediumId (const QUuid &uAttMediumId)
{
    /// @todo is this required?
    //AssertMsg(!aAttMediumId.isNull(), ("Medium ID value can't be null!\n"));
    mAttMediumId = uiCommon().medium(uAttMediumId).id();
    cache();
}

void AttachmentItem::setAttIsPassthrough (bool aIsAttPassthrough)
{
    mAttIsPassthrough = aIsAttPassthrough;
}

void AttachmentItem::setAttIsTempEject (bool aIsAttTempEject)
{
    mAttIsTempEject = aIsAttTempEject;
}

void AttachmentItem::setAttIsNonRotational (bool aIsAttNonRotational)
{
    mAttIsNonRotational = aIsAttNonRotational;
}

void AttachmentItem::setAttIsHotPluggable(bool fIsHotPluggable)
{
    m_fIsHotPluggable = fIsHotPluggable;
}

QString AttachmentItem::attSize() const
{
    return mAttSize;
}

QString AttachmentItem::attLogicalSize() const
{
    return mAttLogicalSize;
}

QString AttachmentItem::attLocation() const
{
    return mAttLocation;
}

QString AttachmentItem::attFormat() const
{
    return mAttFormat;
}

QString AttachmentItem::attDetails() const
{
    return mAttDetails;
}

QString AttachmentItem::attUsage() const
{
    return mAttUsage;
}

QString AttachmentItem::attEncryptionPasswordID() const
{
    return m_strAttEncryptionPasswordID;
}

void AttachmentItem::cache()
{
    UIMedium medium = uiCommon().medium(mAttMediumId);

    /* Cache medium information */
    mAttName = medium.name (true);
    mAttTip = medium.toolTipCheckRO (true, mAttDeviceType != KDeviceType_HardDisk);
    mAttPixmap = medium.iconCheckRO (true);
    mAttIsHostDrive = medium.isHostDrive();

    /* Cache additional information */
    mAttSize = medium.size (true);
    mAttLogicalSize = medium.logicalSize (true);
    mAttLocation = medium.location (true);
    m_strAttEncryptionPasswordID = QString("--");
    if (medium.isNull())
    {
        mAttFormat = QString("--");
    }
    else
    {
        switch (mAttDeviceType)
        {
            case KDeviceType_HardDisk:
            {
                mAttFormat = QString("%1 (%2)").arg(medium.hardDiskType(true)).arg(medium.hardDiskFormat(true));
                mAttDetails = medium.storageDetails();
                const QString strAttEncryptionPasswordID = medium.encryptionPasswordID();
                if (!strAttEncryptionPasswordID.isNull())
                    m_strAttEncryptionPasswordID = strAttEncryptionPasswordID;
                break;
            }
            case KDeviceType_DVD:
            case KDeviceType_Floppy:
            {
                mAttFormat = mAttIsHostDrive ? UIMachineSettingsStorage::tr("Host Drive") : UIMachineSettingsStorage::tr("Image", "storage image");
                break;
            }
            default:
                break;
        }
    }
    mAttUsage = medium.usage (true);

    /* Fill empty attributes */
    if (mAttUsage.isEmpty())
        mAttUsage = QString ("--");
}

AbstractItem::ItemType AttachmentItem::rtti() const
{
    return Type_AttachmentItem;
}

AbstractItem* AttachmentItem::childItem (int /* aIndex */) const
{
    return 0;
}

AbstractItem* AttachmentItem::childItemById (const QUuid& /* uId */) const
{
    return 0;
}

int AttachmentItem::posOfChild (AbstractItem* /* aItem */) const
{
    return 0;
}

int AttachmentItem::childCount() const
{
    return 0;
}

QString AttachmentItem::text() const
{
    return mAttName;
}

QString AttachmentItem::tip() const
{
    return mAttTip;
}

QPixmap AttachmentItem::pixmap (ItemState /* aState */)
{
    if (mAttPixmap.isNull())
    {
        switch (mAttDeviceType)
        {
            case KDeviceType_HardDisk:
                mAttPixmap = iconPool()->pixmap(HDAttachmentNormal);
                break;
            case KDeviceType_DVD:
                mAttPixmap = iconPool()->pixmap(CDAttachmentNormal);
                break;
            case KDeviceType_Floppy:
                mAttPixmap = iconPool()->pixmap(FDAttachmentNormal);
                break;
            default:
                break;
        }
    }
    return mAttPixmap;
}

void AttachmentItem::addChild (AbstractItem* /* aItem */)
{
}

void AttachmentItem::delChild (AbstractItem* /* aItem */)
{
}


/* Storage model */
StorageModel::StorageModel(QITreeView *pParent)
    : QAbstractItemModel(pParent)
    , mRootItem(new RootItem(pParent))
    , mToolTipType(DefaultToolTip)
    , m_chipsetType(KChipsetType_PIIX3)
    , m_configurationAccessLevel(ConfigurationAccessLevel_Null)
{
}

StorageModel::~StorageModel()
{
    delete mRootItem;
}

int StorageModel::rowCount (const QModelIndex &aParent) const
{
    return !aParent.isValid() ? 1 /* only root item has invalid parent */ :
           static_cast <AbstractItem*> (aParent.internalPointer())->childCount();
}

int StorageModel::columnCount (const QModelIndex & /* aParent */) const
{
    return 1;
}

QModelIndex StorageModel::root() const
{
    return index (0, 0);
}

QModelIndex StorageModel::index (int aRow, int aColumn, const QModelIndex &aParent) const
{
    if (!hasIndex (aRow, aColumn, aParent))
        return QModelIndex();

    AbstractItem *item = !aParent.isValid() ? mRootItem :
                         static_cast <AbstractItem*> (aParent.internalPointer())->childItem (aRow);

    return item ? createIndex (aRow, aColumn, item) : QModelIndex();
}

QModelIndex StorageModel::parent (const QModelIndex &aIndex) const
{
    if (!aIndex.isValid())
        return QModelIndex();

    AbstractItem *item = static_cast <AbstractItem*> (aIndex.internalPointer());
    AbstractItem *parentOfItem = item->parent();
    AbstractItem *parentOfParent = parentOfItem ? parentOfItem->parent() : 0;
    int position = parentOfParent ? parentOfParent->posOfChild (parentOfItem) : 0;

    if (parentOfItem)
        return createIndex (position, 0, parentOfItem);
    else
        return QModelIndex();
}

QVariant StorageModel::data (const QModelIndex &aIndex, int aRole) const
{
    if (!aIndex.isValid())
        return QVariant();

    switch (aRole)
    {
        /* Basic Attributes: */
        case Qt::FontRole:
        {
            return QVariant (qApp->font());
        }
        case Qt::SizeHintRole:
        {
            QFontMetrics fm (data (aIndex, Qt::FontRole).value <QFont>());
            int minimumHeight = qMax (fm.height(), data (aIndex, R_IconSize).toInt());
            int margin = data (aIndex, R_Margin).toInt();
            return QSize (1 /* ignoring width */, 2 * margin + minimumHeight);
        }
        case Qt::ToolTipRole:
        {
            if (AbstractItem *item = static_cast <AbstractItem*> (aIndex.internalPointer()))
            {
                if (item->rtti() == AbstractItem::Type_ControllerItem)
                {
                    QString tip (item->tip());
                    switch (mToolTipType)
                    {
                        case ExpanderToolTip:
                            if (aIndex.child (0, 0).isValid())
                                tip = UIMachineSettingsStorage::tr("<nobr>Expands/Collapses&nbsp;item.</nobr>");
                            break;
                        case HDAdderToolTip:
                            tip = UIMachineSettingsStorage::tr("<nobr>Adds&nbsp;hard&nbsp;disk.</nobr>");
                            break;
                        case CDAdderToolTip:
                            tip = UIMachineSettingsStorage::tr("<nobr>Adds&nbsp;optical&nbsp;drive.</nobr>");
                            break;
                        case FDAdderToolTip:
                            tip = UIMachineSettingsStorage::tr("<nobr>Adds&nbsp;floppy&nbsp;drive.</nobr>");
                            break;
                        default:
                            break;
                    }
                    return tip;
                }
                return item->tip();
            }
            return QString();
        }

        /* Advanced Attributes: */
        case R_ItemId:
        {
            if (AbstractItem *item = static_cast <AbstractItem*> (aIndex.internalPointer()))
                return item->id();
            return QUuid();
        }
        case R_ItemPixmap:
        {
            if (AbstractItem *item = static_cast <AbstractItem*> (aIndex.internalPointer()))
            {
                ItemState state = State_DefaultItem;
                if (hasChildren (aIndex))
                    if (QTreeView *view = qobject_cast<QTreeView*>(QObject::parent()))
                        state = view->isExpanded (aIndex) ? State_ExpandedItem : State_CollapsedItem;
                return item->pixmap (state);
            }
            return QPixmap();
        }
        case R_ItemPixmapRect:
        {
            int margin = data (aIndex, R_Margin).toInt();
            int width = data (aIndex, R_IconSize).toInt();
            return QRect (margin, margin, width, width);
        }
        case R_ItemName:
        {
            if (AbstractItem *item = static_cast <AbstractItem*> (aIndex.internalPointer()))
                return item->text();
            return QString();
        }
        case R_ItemNamePoint:
        {
            int margin = data (aIndex, R_Margin).toInt();
            int spacing = data (aIndex, R_Spacing).toInt();
            int width = data (aIndex, R_IconSize).toInt();
            QFontMetrics fm (data (aIndex, Qt::FontRole).value <QFont>());
            QSize sizeHint = data (aIndex, Qt::SizeHintRole).toSize();
            return QPoint (margin + width + 2 * spacing,
                           sizeHint.height() / 2 + fm.ascent() / 2 - 1 /* base line */);
        }
        case R_ItemType:
        {
            QVariant result (QVariant::fromValue (AbstractItem::Type_InvalidItem));
            if (AbstractItem *item = static_cast <AbstractItem*> (aIndex.internalPointer()))
                result.setValue (item->rtti());
            return result;
        }
        case R_IsController:
        {
            if (AbstractItem *item = static_cast <AbstractItem*> (aIndex.internalPointer()))
                return item->rtti() == AbstractItem::Type_ControllerItem;
            return false;
        }
        case R_IsAttachment:
        {
            if (AbstractItem *item = static_cast <AbstractItem*> (aIndex.internalPointer()))
                return item->rtti() == AbstractItem::Type_AttachmentItem;
            return false;
        }

        case R_ToolTipType:
        {
            return QVariant::fromValue (mToolTipType);
        }
        case R_IsMoreIDEControllersPossible:
        {
            return (m_configurationAccessLevel == ConfigurationAccessLevel_Full) &&
                   (qobject_cast<RootItem*>(mRootItem)->childCount(KStorageBus_IDE) <
                    uiCommon().virtualBox().GetSystemProperties().GetMaxInstancesOfStorageBus(chipsetType(), KStorageBus_IDE));
        }
        case R_IsMoreSATAControllersPossible:
        {
            return (m_configurationAccessLevel == ConfigurationAccessLevel_Full) &&
                   (qobject_cast<RootItem*>(mRootItem)->childCount(KStorageBus_SATA) <
                    uiCommon().virtualBox().GetSystemProperties().GetMaxInstancesOfStorageBus(chipsetType(), KStorageBus_SATA));
        }
        case R_IsMoreSCSIControllersPossible:
        {
            return (m_configurationAccessLevel == ConfigurationAccessLevel_Full) &&
                   (qobject_cast<RootItem*>(mRootItem)->childCount(KStorageBus_SCSI) <
                    uiCommon().virtualBox().GetSystemProperties().GetMaxInstancesOfStorageBus(chipsetType(), KStorageBus_SCSI));
        }
        case R_IsMoreFloppyControllersPossible:
        {
            return (m_configurationAccessLevel == ConfigurationAccessLevel_Full) &&
                   (qobject_cast<RootItem*>(mRootItem)->childCount(KStorageBus_Floppy) <
                    uiCommon().virtualBox().GetSystemProperties().GetMaxInstancesOfStorageBus(chipsetType(), KStorageBus_Floppy));
        }
        case R_IsMoreSASControllersPossible:
        {
            return (m_configurationAccessLevel == ConfigurationAccessLevel_Full) &&
                   (qobject_cast<RootItem*>(mRootItem)->childCount(KStorageBus_SAS) <
                    uiCommon().virtualBox().GetSystemProperties().GetMaxInstancesOfStorageBus(chipsetType(), KStorageBus_SAS));
        }
        case R_IsMoreUSBControllersPossible:
        {
            return (m_configurationAccessLevel == ConfigurationAccessLevel_Full) &&
                   (qobject_cast<RootItem*>(mRootItem)->childCount(KStorageBus_USB) <
                    uiCommon().virtualBox().GetSystemProperties().GetMaxInstancesOfStorageBus(chipsetType(), KStorageBus_USB));
        }
        case R_IsMoreNVMeControllersPossible:
        {
            return (m_configurationAccessLevel == ConfigurationAccessLevel_Full) &&
                   (qobject_cast<RootItem*>(mRootItem)->childCount(KStorageBus_PCIe) <
                    uiCommon().virtualBox().GetSystemProperties().GetMaxInstancesOfStorageBus(chipsetType(), KStorageBus_PCIe));
        }
        case R_IsMoreVirtioSCSIControllersPossible:
        {
            return (m_configurationAccessLevel == ConfigurationAccessLevel_Full) &&
                   (qobject_cast<RootItem*>(mRootItem)->childCount(KStorageBus_VirtioSCSI) <
                    uiCommon().virtualBox().GetSystemProperties().GetMaxInstancesOfStorageBus(chipsetType(), KStorageBus_VirtioSCSI));
        }
        case R_IsMoreAttachmentsPossible:
        {
            if (AbstractItem *item = static_cast <AbstractItem*> (aIndex.internalPointer()))
            {
                if (item->rtti() == AbstractItem::Type_ControllerItem)
                {
                    ControllerItem *ctr = qobject_cast<ControllerItem*>(item);
                    CSystemProperties sp = uiCommon().virtualBox().GetSystemProperties();
                    const bool fIsMoreAttachmentsPossible = (ULONG)rowCount(aIndex) <
                                                            (sp.GetMaxPortCountForStorageBus(ctr->ctrBusType()) *
                                                             sp.GetMaxDevicesPerPortForStorageBus(ctr->ctrBusType()));
                    if (fIsMoreAttachmentsPossible)
                    {
                        switch (m_configurationAccessLevel)
                        {
                            case ConfigurationAccessLevel_Full:
                                return true;
                            case ConfigurationAccessLevel_Partial_Running:
                            {
                                switch (ctr->ctrBusType())
                                {
                                    case KStorageBus_USB:
                                        return true;
                                    case KStorageBus_SATA:
                                        return (uint)rowCount(aIndex) < ctr->portCount();
                                    default:
                                        break;
                                }
                            }
                            default:
                                break;
                        }
                    }
                }
            }
            return false;
        }

        case R_CtrOldName:
        {
            if (AbstractItem *item = static_cast <AbstractItem*> (aIndex.internalPointer()))
                if (item->rtti() == AbstractItem::Type_ControllerItem)
                    return qobject_cast<ControllerItem*>(item)->oldCtrName();
            return QString();
        }
        case R_CtrName:
        {
            if (AbstractItem *item = static_cast <AbstractItem*> (aIndex.internalPointer()))
                if (item->rtti() == AbstractItem::Type_ControllerItem)
                    return qobject_cast<ControllerItem*>(item)->ctrName();
            return QString();
        }
        case R_CtrType:
        {
            QVariant result (QVariant::fromValue (KStorageControllerType_Null));
            if (AbstractItem *item = static_cast <AbstractItem*> (aIndex.internalPointer()))
                if (item->rtti() == AbstractItem::Type_ControllerItem)
                    result.setValue (qobject_cast<ControllerItem*>(item)->ctrType());
            return result;
        }
        case R_CtrTypes:
        {
            QVariant result (QVariant::fromValue (ControllerTypeList()));
            if (AbstractItem *item = static_cast <AbstractItem*> (aIndex.internalPointer()))
                if (item->rtti() == AbstractItem::Type_ControllerItem)
                    result.setValue (qobject_cast<ControllerItem*>(item)->ctrTypes());
            return result;
        }
        case R_CtrDevices:
        {
            QVariant result (QVariant::fromValue (DeviceTypeList()));
            if (AbstractItem *item = static_cast <AbstractItem*> (aIndex.internalPointer()))
                if (item->rtti() == AbstractItem::Type_ControllerItem)
                    result.setValue (qobject_cast<ControllerItem*>(item)->ctrDeviceTypeList());
            return result;
        }
        case R_CtrBusType:
        {
            QVariant result (QVariant::fromValue (KStorageBus_Null));
            if (AbstractItem *item = static_cast <AbstractItem*> (aIndex.internalPointer()))
                if (item->rtti() == AbstractItem::Type_ControllerItem)
                    result.setValue (qobject_cast<ControllerItem*>(item)->ctrBusType());
            return result;
        }
        case R_CtrBusTypes:
        {
            QVariant result(QVariant::fromValue(ControllerBusList()));
            if (AbstractItem *pItem = static_cast<AbstractItem*>(aIndex.internalPointer()))
                if (pItem->rtti() == AbstractItem::Type_ControllerItem)
                    result.setValue(qobject_cast<ControllerItem*>(pItem)->ctrBusTypes());
            return result;
        }
        case R_CtrPortCount:
        {
            if (AbstractItem *item = static_cast <AbstractItem*> (aIndex.internalPointer()))
                if (item->rtti() == AbstractItem::Type_ControllerItem)
                    return qobject_cast<ControllerItem*>(item)->portCount();
            return 0;
        }
        case R_CtrMaxPortCount:
        {
            if (AbstractItem *item = static_cast <AbstractItem*> (aIndex.internalPointer()))
                if (item->rtti() == AbstractItem::Type_ControllerItem)
                    return qobject_cast<ControllerItem*>(item)->maxPortCount();
            return 0;
        }
        case R_CtrIoCache:
        {
            if (AbstractItem *item = static_cast <AbstractItem*> (aIndex.internalPointer()))
                if (item->rtti() == AbstractItem::Type_ControllerItem)
                    return qobject_cast<ControllerItem*>(item)->ctrUseIoCache();
            return false;
        }

        case R_AttSlot:
        {
            QVariant result (QVariant::fromValue (StorageSlot()));
            if (AbstractItem *item = static_cast <AbstractItem*> (aIndex.internalPointer()))
                if (item->rtti() == AbstractItem::Type_AttachmentItem)
                    result.setValue(qobject_cast<AttachmentItem*>(item)->attSlot());
            return result;
        }
        case R_AttSlots:
        {
            QVariant result (QVariant::fromValue (SlotsList()));
            if (AbstractItem *item = static_cast <AbstractItem*> (aIndex.internalPointer()))
                if (item->rtti() == AbstractItem::Type_AttachmentItem)
                    result.setValue(qobject_cast<AttachmentItem*>(item)->attSlots());
            return result;
        }
        case R_AttDevice:
        {
            QVariant result (QVariant::fromValue (KDeviceType_Null));
            if (AbstractItem *item = static_cast <AbstractItem*> (aIndex.internalPointer()))
                if (item->rtti() == AbstractItem::Type_AttachmentItem)
                    result.setValue(qobject_cast<AttachmentItem*>(item)->attDeviceType());
            return result;
        }
        case R_AttMediumId:
        {
            if (AbstractItem *item = static_cast <AbstractItem*> (aIndex.internalPointer()))
                if (item->rtti() == AbstractItem::Type_AttachmentItem)
                    return qobject_cast<AttachmentItem*>(item)->attMediumId();
            return QUuid();
        }
        case R_AttIsHostDrive:
        {
            if (AbstractItem *item = static_cast <AbstractItem*> (aIndex.internalPointer()))
                if (item->rtti() == AbstractItem::Type_AttachmentItem)
                    return qobject_cast<AttachmentItem*>(item)->attIsHostDrive();
            return false;
        }
        case R_AttIsPassthrough:
        {
            if (AbstractItem *item = static_cast <AbstractItem*> (aIndex.internalPointer()))
                if (item->rtti() == AbstractItem::Type_AttachmentItem)
                    return qobject_cast<AttachmentItem*>(item)->attIsPassthrough();
            return false;
        }
        case R_AttIsTempEject:
        {
            if (AbstractItem *item = static_cast <AbstractItem*> (aIndex.internalPointer()))
                if (item->rtti() == AbstractItem::Type_AttachmentItem)
                    return qobject_cast<AttachmentItem*>(item)->attIsTempEject();
            return false;
        }
        case R_AttIsNonRotational:
        {
            if (AbstractItem *item = static_cast <AbstractItem*> (aIndex.internalPointer()))
                if (item->rtti() == AbstractItem::Type_AttachmentItem)
                    return qobject_cast<AttachmentItem*>(item)->attIsNonRotational();
            return false;
        }
        case R_AttIsHotPluggable:
        {
            if (AbstractItem *item = static_cast<AbstractItem*>(aIndex.internalPointer()))
                if (item->rtti() == AbstractItem::Type_AttachmentItem)
                    return qobject_cast<AttachmentItem*>(item)->attIsHotPluggable();
            return false;
        }
        case R_AttSize:
        {
            if (AbstractItem *item = static_cast <AbstractItem*> (aIndex.internalPointer()))
                if (item->rtti() == AbstractItem::Type_AttachmentItem)
                    return qobject_cast<AttachmentItem*>(item)->attSize();
            return QString();
        }
        case R_AttLogicalSize:
        {
            if (AbstractItem *item = static_cast <AbstractItem*> (aIndex.internalPointer()))
                if (item->rtti() == AbstractItem::Type_AttachmentItem)
                    return qobject_cast<AttachmentItem*>(item)->attLogicalSize();
            return QString();
        }
        case R_AttLocation:
        {
            if (AbstractItem *item = static_cast <AbstractItem*> (aIndex.internalPointer()))
                if (item->rtti() == AbstractItem::Type_AttachmentItem)
                    return qobject_cast<AttachmentItem*>(item)->attLocation();
            return QString();
        }
        case R_AttFormat:
        {
            if (AbstractItem *item = static_cast <AbstractItem*> (aIndex.internalPointer()))
                if (item->rtti() == AbstractItem::Type_AttachmentItem)
                    return qobject_cast<AttachmentItem*>(item)->attFormat();
            return QString();
        }
        case R_AttDetails:
        {
            if (AbstractItem *item = static_cast <AbstractItem*> (aIndex.internalPointer()))
                if (item->rtti() == AbstractItem::Type_AttachmentItem)
                    return qobject_cast<AttachmentItem*>(item)->attDetails();
            return QString();
        }
        case R_AttUsage:
        {
            if (AbstractItem *item = static_cast <AbstractItem*> (aIndex.internalPointer()))
                if (item->rtti() == AbstractItem::Type_AttachmentItem)
                    return qobject_cast<AttachmentItem*>(item)->attUsage();
            return QString();
        }
        case R_AttEncryptionPasswordID:
        {
            if (AbstractItem *pItem = static_cast<AbstractItem*>(aIndex.internalPointer()))
                if (pItem->rtti() == AbstractItem::Type_AttachmentItem)
                    return qobject_cast<AttachmentItem*>(pItem)->attEncryptionPasswordID();
            return QString();
        }
        case R_Margin:
        {
            return 4;
        }
        case R_Spacing:
        {
            return 4;
        }
        case R_IconSize:
        {
            return QApplication::style()->pixelMetric(QStyle::PM_SmallIconSize);
        }

        case R_HDPixmapEn:
        {
            return iconPool()->pixmap(HDAttachmentNormal);
        }
        case R_CDPixmapEn:
        {
            return iconPool()->pixmap(CDAttachmentNormal);
        }
        case R_FDPixmapEn:
        {
            return iconPool()->pixmap(FDAttachmentNormal);
        }

        case R_HDPixmapAddEn:
        {
            return iconPool()->pixmap(HDAttachmentAddEn);
        }
        case R_HDPixmapAddDis:
        {
            return iconPool()->pixmap(HDAttachmentAddDis);
        }
        case R_CDPixmapAddEn:
        {
            return iconPool()->pixmap(CDAttachmentAddEn);
        }
        case R_CDPixmapAddDis:
        {
            return iconPool()->pixmap(CDAttachmentAddDis);
        }
        case R_FDPixmapAddEn:
        {
            return iconPool()->pixmap(FDAttachmentAddEn);
        }
        case R_FDPixmapAddDis:
        {
            return iconPool()->pixmap(FDAttachmentAddDis);
        }
        case R_HDPixmapRect:
        {
            int margin = data (aIndex, R_Margin).toInt();
            int width = data (aIndex, R_IconSize).toInt();
            return QRect (0 - width - margin, margin, width, width);
        }
        case R_CDPixmapRect:
        {
            int margin = data (aIndex, R_Margin).toInt();
            int spacing = data (aIndex, R_Spacing).toInt();
            int width = data (aIndex, R_IconSize).toInt();
            return QRect (0 - width - spacing - width - margin, margin, width, width);
        }
        case R_FDPixmapRect:
        {
            int margin = data (aIndex, R_Margin).toInt();
            int width = data (aIndex, R_IconSize).toInt();
            return QRect (0 - width - margin, margin, width, width);
        }

        default:
            break;
    }
    return QVariant();
}

bool StorageModel::setData (const QModelIndex &aIndex, const QVariant &aValue, int aRole)
{
    if (!aIndex.isValid())
        return QAbstractItemModel::setData (aIndex, aValue, aRole);

    switch (aRole)
    {
        case R_ToolTipType:
        {
            mToolTipType = aValue.value <ToolTipType>();
            emit dataChanged (aIndex, aIndex);
            return true;
        }
        case R_CtrName:
        {
            if (AbstractItem *item = static_cast <AbstractItem*> (aIndex.internalPointer()))
                if (item->rtti() == AbstractItem::Type_ControllerItem)
                {
                    qobject_cast<ControllerItem*>(item)->setCtrName(aValue.toString());
                    emit dataChanged (aIndex, aIndex);
                    return true;
                }
            return false;
        }
        case R_CtrBusType:
        {
            if (AbstractItem *pItem = static_cast<AbstractItem*>(aIndex.internalPointer()))
                if (pItem->rtti() == AbstractItem::Type_ControllerItem)
                {
                    /* Acquire controller item and requested storage bus type: */
                    ControllerItem *pItemController = qobject_cast<ControllerItem*>(pItem);
                    const KStorageBus enmNewCtrBusType = aValue.value<KStorageBus>();

                    /* PCIe devices allows for hard-drives attachments only,
                     * no optical devices. So, lets make sure that rule is fulfilled. */
                    if (enmNewCtrBusType == KStorageBus_PCIe)
                    {
                        const QList<QUuid> opticalIds = pItemController->attachmentIDs(KDeviceType_DVD);
                        if (!opticalIds.isEmpty())
                        {
                            if (!msgCenter().confirmStorageBusChangeWithOpticalRemoval(qobject_cast<QWidget*>(QObject::parent())))
                                return false;
                            foreach (const QUuid &uId, opticalIds)
                                delAttachment(pItemController->id(), uId);
                        }
                    }

                    /* Lets make sure there is enough of place for all the remaining attachments: */
                    const uint uMaxPortCount =
                        (uint)uiCommon().virtualBox().GetSystemProperties().GetMaxPortCountForStorageBus(enmNewCtrBusType);
                    const uint uMaxDevicePerPortCount =
                        (uint)uiCommon().virtualBox().GetSystemProperties().GetMaxDevicesPerPortForStorageBus(enmNewCtrBusType);
                    const QList<QUuid> ids = pItemController->attachmentIDs();
                    if (uMaxPortCount * uMaxDevicePerPortCount < (uint)ids.size())
                    {
                        if (!msgCenter().confirmStorageBusChangeWithExcessiveRemoval(qobject_cast<QWidget*>(QObject::parent())))
                            return false;
                        for (int i = uMaxPortCount * uMaxDevicePerPortCount; i < ids.size(); ++i)
                            delAttachment(pItemController->id(), ids.at(i));
                    }

                    /* Push new bus/controller type: */
                    pItemController->setCtrBusType(enmNewCtrBusType);
                    pItemController->setCtrType(pItemController->ctrTypes().first());
                    emit dataChanged(aIndex, aIndex);

                    /* Make sure each of remaining attachments has valid slot: */
                    foreach (AbstractItem *pChildItem, pItemController->attachments())
                    {
                        AttachmentItem *pChildItemAttachment = qobject_cast<AttachmentItem*>(pChildItem);
                        const SlotsList availableSlots = pChildItemAttachment->attSlots();
                        const StorageSlot currentSlot = pChildItemAttachment->attSlot();
                        if (!availableSlots.isEmpty() && !availableSlots.contains(currentSlot))
                            pChildItemAttachment->setAttSlot(availableSlots.first());
                    }

                    /* This means success: */
                    return true;
                }
            return false;
        }
        case R_CtrType:
        {
            if (AbstractItem *item = static_cast <AbstractItem*> (aIndex.internalPointer()))
                if (item->rtti() == AbstractItem::Type_ControllerItem)
                {
                    qobject_cast<ControllerItem*>(item)->setCtrType(aValue.value<KStorageControllerType>());
                    emit dataChanged (aIndex, aIndex);
                    return true;
                }
            return false;
        }
        case R_CtrPortCount:
        {
            if (AbstractItem *item = static_cast <AbstractItem*> (aIndex.internalPointer()))
                if (item->rtti() == AbstractItem::Type_ControllerItem)
                {
                    qobject_cast<ControllerItem*>(item)->setPortCount(aValue.toUInt());
                    emit dataChanged (aIndex, aIndex);
                    return true;
                }
            return false;
        }
        case R_CtrIoCache:
        {
            if (AbstractItem *item = static_cast <AbstractItem*> (aIndex.internalPointer()))
                if (item->rtti() == AbstractItem::Type_ControllerItem)
                {
                    qobject_cast<ControllerItem*>(item)->setCtrUseIoCache(aValue.toBool());
                    emit dataChanged (aIndex, aIndex);
                    return true;
                }
            return false;
        }
        case R_AttSlot:
        {
            if (AbstractItem *item = static_cast <AbstractItem*> (aIndex.internalPointer()))
                if (item->rtti() == AbstractItem::Type_AttachmentItem)
                {
                    qobject_cast<AttachmentItem*>(item)->setAttSlot(aValue.value<StorageSlot>());
                    emit dataChanged (aIndex, aIndex);
                    sort();
                    return true;
                }
            return false;
        }
        case R_AttDevice:
        {
            if (AbstractItem *item = static_cast <AbstractItem*> (aIndex.internalPointer()))
                if (item->rtti() == AbstractItem::Type_AttachmentItem)
                {
                    qobject_cast<AttachmentItem*>(item)->setAttDevice(aValue.value<KDeviceType>());
                    emit dataChanged (aIndex, aIndex);
                    return true;
                }
            return false;
        }
        case R_AttMediumId:
        {
            if (AbstractItem *item = static_cast <AbstractItem*> (aIndex.internalPointer()))
                if (item->rtti() == AbstractItem::Type_AttachmentItem)
                {
                    qobject_cast<AttachmentItem*>(item)->setAttMediumId(aValue.toUuid());
                    emit dataChanged (aIndex, aIndex);
                    return true;
                }
            return false;
        }
        case R_AttIsPassthrough:
        {
            if (AbstractItem *item = static_cast <AbstractItem*> (aIndex.internalPointer()))
                if (item->rtti() == AbstractItem::Type_AttachmentItem)
                {
                    qobject_cast<AttachmentItem*>(item)->setAttIsPassthrough(aValue.toBool());
                    emit dataChanged (aIndex, aIndex);
                    return true;
                }
            return false;
        }
        case R_AttIsTempEject:
        {
            if (AbstractItem *item = static_cast <AbstractItem*> (aIndex.internalPointer()))
                if (item->rtti() == AbstractItem::Type_AttachmentItem)
                {
                    qobject_cast<AttachmentItem*>(item)->setAttIsTempEject(aValue.toBool());
                    emit dataChanged (aIndex, aIndex);
                    return true;
                }
            return false;
        }
        case R_AttIsNonRotational:
        {
            if (AbstractItem *item = static_cast <AbstractItem*> (aIndex.internalPointer()))
                if (item->rtti() == AbstractItem::Type_AttachmentItem)
                {
                    qobject_cast<AttachmentItem*>(item)->setAttIsNonRotational(aValue.toBool());
                    emit dataChanged (aIndex, aIndex);
                    return true;
                }
            return false;
        }
        case R_AttIsHotPluggable:
        {
            if (AbstractItem *item = static_cast<AbstractItem*>(aIndex.internalPointer()))
                if (item->rtti() == AbstractItem::Type_AttachmentItem)
                {
                    qobject_cast<AttachmentItem*>(item)->setAttIsHotPluggable(aValue.toBool());
                    emit dataChanged(aIndex, aIndex);
                    return true;
                }
            return false;
        }
        default:
            break;
    }

    return false;
}

QModelIndex StorageModel::addController (const QString &aCtrName, KStorageBus aBusType, KStorageControllerType aCtrType)
{
    beginInsertRows (root(), mRootItem->childCount(), mRootItem->childCount());
    new ControllerItem (mRootItem, aCtrName, aBusType, aCtrType);
    endInsertRows();
    return index (mRootItem->childCount() - 1, 0, root());
}

void StorageModel::delController (const QUuid &uCtrId)
{
    if (AbstractItem *item = mRootItem->childItemById (uCtrId))
    {
        int itemPosition = mRootItem->posOfChild (item);
        beginRemoveRows (root(), itemPosition, itemPosition);
        delete item;
        endRemoveRows();
    }
}

QModelIndex StorageModel::addAttachment (const QUuid &uCtrId, KDeviceType aDeviceType, const QUuid &uMediumId)
{
    if (AbstractItem *parent = mRootItem->childItemById (uCtrId))
    {
        int parentPosition = mRootItem->posOfChild (parent);
        QModelIndex parentIndex = index (parentPosition, 0, root());
        beginInsertRows (parentIndex, parent->childCount(), parent->childCount());
        AttachmentItem *pItem = new AttachmentItem (parent, aDeviceType);
        pItem->setAttIsHotPluggable(m_configurationAccessLevel != ConfigurationAccessLevel_Full);
        pItem->setAttMediumId(uMediumId);
        endInsertRows();
        return index (parent->childCount() - 1, 0, parentIndex);
    }
    return QModelIndex();
}

void StorageModel::delAttachment (const QUuid &uCtrId, const QUuid &uAttId)
{
    if (AbstractItem *parent = mRootItem->childItemById (uCtrId))
    {
        int parentPosition = mRootItem->posOfChild (parent);
        if (AbstractItem *item = parent->childItemById (uAttId))
        {
            int itemPosition = parent->posOfChild (item);
            beginRemoveRows (index (parentPosition, 0, root()), itemPosition, itemPosition);
            delete item;
            endRemoveRows();
        }
    }
}

void StorageModel::moveAttachment(const QUuid &uAttId, const QUuid &uCtrOldId, const QUuid &uCtrNewId)
{
    /* No known info about attachment device type and medium ID: */
    KDeviceType enmDeviceType = KDeviceType_Null;
    QUuid uMediumId;

    /* First of all we are looking for old controller item: */
    AbstractItem *pOldItem = mRootItem->childItemById(uCtrOldId);
    if (pOldItem)
    {
        /* And acquire controller position: */
        const int iOldCtrPosition = mRootItem->posOfChild(pOldItem);

        /* Then we are looking for an attachment item: */
        if (AbstractItem *pSubItem = pOldItem->childItemById(uAttId))
        {
            /* And make sure this is really an attachment: */
            AttachmentItem *pSubItemAttachment = qobject_cast<AttachmentItem*>(pSubItem);
            if (pSubItemAttachment)
            {
                /* This way we can acquire actual attachment device type and medium ID: */
                enmDeviceType = pSubItemAttachment->attDeviceType();
                uMediumId = pSubItemAttachment->attMediumId();

                /* And delete atachment item finally: */
                const int iAttPosition = pOldItem->posOfChild(pSubItem);
                beginRemoveRows(index(iOldCtrPosition, 0, root()), iAttPosition, iAttPosition);
                delete pSubItem;
                endRemoveRows();
            }
        }
    }

    /* As the last step we are looking for new controller item: */
    AbstractItem *pNewItem = mRootItem->childItemById(uCtrNewId);
    if (pNewItem)
    {
        /* And acquire controller position: */
        const int iNewCtrPosition = mRootItem->posOfChild(pNewItem);

        /* Then we have to make sure moved attachment is valid: */
        if (enmDeviceType != KDeviceType_Null)
        {
            /* And create new attachment item finally: */
            QModelIndex newCtrIndex = index(iNewCtrPosition, 0, root());
            beginInsertRows(newCtrIndex, pNewItem->childCount(), pNewItem->childCount());
            AttachmentItem *pItem = new AttachmentItem(pNewItem, enmDeviceType);
            pItem->setAttIsHotPluggable(m_configurationAccessLevel != ConfigurationAccessLevel_Full);
            pItem->setAttMediumId(uMediumId);
            endInsertRows();
        }
    }
}

void StorageModel::setMachineId (const QUuid &uMachineId)
{
    mRootItem->setMachineId (uMachineId);
}

void StorageModel::sort(int /* iColumn */, Qt::SortOrder order)
{
    /* Count of controller items: */
    int iItemLevel1Count = mRootItem->childCount();
    /* For each of controller items: */
    for (int iItemLevel1Pos = 0; iItemLevel1Pos < iItemLevel1Count; ++iItemLevel1Pos)
    {
        /* Get iterated controller item: */
        AbstractItem *pItemLevel1 = mRootItem->childItem(iItemLevel1Pos);
        ControllerItem *pControllerItem = qobject_cast<ControllerItem*>(pItemLevel1);
        /* Count of attachment items: */
        int iItemLevel2Count = pItemLevel1->childCount();
        /* Prepare empty list for sorted attachments: */
        QList<AbstractItem*> newAttachments;
        /* For each of attachment items: */
        for (int iItemLevel2Pos = 0; iItemLevel2Pos < iItemLevel2Count; ++iItemLevel2Pos)
        {
            /* Get iterated attachment item: */
            AbstractItem *pItemLevel2 = pItemLevel1->childItem(iItemLevel2Pos);
            AttachmentItem *pAttachmentItem = qobject_cast<AttachmentItem*>(pItemLevel2);
            /* Get iterated attachment storage slot: */
            StorageSlot attachmentSlot = pAttachmentItem->attSlot();
            int iInsertPosition = 0;
            for (; iInsertPosition < newAttachments.size(); ++iInsertPosition)
            {
                /* Get sorted attachment item: */
                AbstractItem *pNewItemLevel2 = newAttachments[iInsertPosition];
                AttachmentItem *pNewAttachmentItem = qobject_cast<AttachmentItem*>(pNewItemLevel2);
                /* Get sorted attachment storage slot: */
                StorageSlot newAttachmentSlot = pNewAttachmentItem->attSlot();
                /* Apply sorting rule: */
                if (((order == Qt::AscendingOrder) && (attachmentSlot < newAttachmentSlot)) ||
                    ((order == Qt::DescendingOrder) && (attachmentSlot > newAttachmentSlot)))
                    break;
            }
            /* Insert iterated attachment into sorted position: */
            newAttachments.insert(iInsertPosition, pItemLevel2);
        }

        /* If that controller has attachments: */
        if (iItemLevel2Count)
        {
            /* We should update corresponding model-indexes: */
            QModelIndex controllerIndex = index(iItemLevel1Pos, 0, root());
            pControllerItem->setAttachments(newAttachments);
            /* That is actually beginMoveRows() + endMoveRows() which
             * unfortunately become available only in Qt 4.6 version. */
            beginRemoveRows(controllerIndex, 0, iItemLevel2Count - 1);
            endRemoveRows();
            beginInsertRows(controllerIndex, 0, iItemLevel2Count - 1);
            endInsertRows();
        }
    }
}

QModelIndex StorageModel::attachmentBySlot(QModelIndex controllerIndex, StorageSlot attachmentStorageSlot)
{
    /* Check what parent model index is valid, set and of 'controller' type: */
    AssertMsg(controllerIndex.isValid(), ("Controller index should be valid!\n"));
    AbstractItem *pParentItem = static_cast<AbstractItem*>(controllerIndex.internalPointer());
    AssertMsg(pParentItem, ("Parent item should be set!\n"));
    AssertMsg(pParentItem->rtti() == AbstractItem::Type_ControllerItem, ("Parent item should be of 'controller' type!\n"));
    NOREF(pParentItem);

    /* Search for suitable attachment one by one: */
    for (int i = 0; i < rowCount(controllerIndex); ++i)
    {
        QModelIndex curAttachmentIndex = index(i, 0, controllerIndex);
        StorageSlot curAttachmentStorageSlot = data(curAttachmentIndex, R_AttSlot).value<StorageSlot>();
        if (curAttachmentStorageSlot ==  attachmentStorageSlot)
            return curAttachmentIndex;
    }
    return QModelIndex();
}

KChipsetType StorageModel::chipsetType() const
{
    return m_chipsetType;
}

void StorageModel::setChipsetType(KChipsetType type)
{
    m_chipsetType = type;
}

void StorageModel::setConfigurationAccessLevel(ConfigurationAccessLevel newConfigurationAccessLevel)
{
    m_configurationAccessLevel = newConfigurationAccessLevel;
}

void StorageModel::clear()
{
    while (mRootItem->childCount())
    {
        beginRemoveRows(root(), 0, 0);
        delete mRootItem->childItem(0);
        endRemoveRows();
    }
}

QMap<KStorageBus, int> StorageModel::currentControllerTypes() const
{
    QMap<KStorageBus, int> currentMap;
    for (int iStorageBusType = KStorageBus_IDE; iStorageBusType < KStorageBus_Max; ++iStorageBusType)
    {
        currentMap.insert((KStorageBus)iStorageBusType,
                          qobject_cast<RootItem*>(mRootItem)->childCount((KStorageBus)iStorageBusType));
    }
    return currentMap;
}

QMap<KStorageBus, int> StorageModel::maximumControllerTypes() const
{
    QMap<KStorageBus, int> maximumMap;
    for (int iStorageBusType = KStorageBus_IDE; iStorageBusType < KStorageBus_Max; ++iStorageBusType)
    {
        maximumMap.insert((KStorageBus)iStorageBusType,
                          uiCommon().virtualBox().GetSystemProperties().GetMaxInstancesOfStorageBus(chipsetType(), (KStorageBus)iStorageBusType));
    }
    return maximumMap;
}

Qt::ItemFlags StorageModel::flags (const QModelIndex &aIndex) const
{
    return !aIndex.isValid() ? QAbstractItemModel::flags (aIndex) :
           Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}


/* Storage Delegate */
StorageDelegate::StorageDelegate (QObject *aParent)
    : QItemDelegate (aParent)
{
}

void StorageDelegate::paint (QPainter *aPainter, const QStyleOptionViewItem &aOption, const QModelIndex &aIndex) const
{
    if (!aIndex.isValid()) return;

    /* Initialize variables */
    QStyle::State state = aOption.state;
    QRect rect = aOption.rect;
    const StorageModel *model = qobject_cast<const StorageModel*>(aIndex.model());
    Assert (model);

    aPainter->save();

    /* Draw item background */
    QItemDelegate::drawBackground (aPainter, aOption, aIndex);

    /* Setup foreground settings */
    QPalette::ColorGroup cg = state & QStyle::State_Active ? QPalette::Active : QPalette::Inactive;
    bool isSelected = state & QStyle::State_Selected;
    bool isFocused = state & QStyle::State_HasFocus;
    bool isGrayOnLoosingFocus = QApplication::style()->styleHint (QStyle::SH_ItemView_ChangeHighlightOnFocus, &aOption) != 0;
    aPainter->setPen (aOption.palette.color (cg, isSelected && (isFocused || !isGrayOnLoosingFocus) ?
                                             QPalette::HighlightedText : QPalette::Text));

    aPainter->translate (rect.x(), rect.y());

    /* Draw Item Pixmap */
    aPainter->drawPixmap (model->data (aIndex, StorageModel::R_ItemPixmapRect).toRect().topLeft(),
                          model->data (aIndex, StorageModel::R_ItemPixmap).value <QPixmap>());

    /* Draw compressed item name */
    int margin = model->data (aIndex, StorageModel::R_Margin).toInt();
    int iconWidth = model->data (aIndex, StorageModel::R_IconSize).toInt();
    int spacing = model->data (aIndex, StorageModel::R_Spacing).toInt();
    QPoint textPosition = model->data (aIndex, StorageModel::R_ItemNamePoint).toPoint();
    int textWidth = rect.width() - textPosition.x();
    if (model->data (aIndex, StorageModel::R_IsController).toBool() && state & QStyle::State_Selected)
    {
        textWidth -= (2 * spacing + iconWidth + margin);
        if (model->data (aIndex, StorageModel::R_CtrBusType).value <KStorageBus>() != KStorageBus_Floppy)
            textWidth -= (spacing + iconWidth);
    }
    QString text (model->data (aIndex, StorageModel::R_ItemName).toString());
    QString shortText (text);
    QFont font = model->data (aIndex, Qt::FontRole).value <QFont>();
    QFontMetrics fm (font);
    while ((shortText.size() > 1) && (fm.width (shortText) + fm.width ("...") > textWidth))
        shortText.truncate (shortText.size() - 1);
    if (shortText != text)
        shortText += "...";
    aPainter->setFont (font);
    aPainter->drawText (textPosition, shortText);

    /* Draw Controller Additions */
    if (model->data (aIndex, StorageModel::R_IsController).toBool() && state & QStyle::State_Selected)
    {
        DeviceTypeList devicesList (model->data (aIndex, StorageModel::R_CtrDevices).value <DeviceTypeList>());
        for (int i = 0; i < devicesList.size(); ++ i)
        {
            KDeviceType deviceType = devicesList [i];

            QRect deviceRect;
            QPixmap devicePixmap;
            switch (deviceType)
            {
                case KDeviceType_HardDisk:
                {
                    deviceRect = model->data (aIndex, StorageModel::R_HDPixmapRect).value <QRect>();
                    devicePixmap = model->data (aIndex, StorageModel::R_IsMoreAttachmentsPossible).toBool() ?
                                   model->data (aIndex, StorageModel::R_HDPixmapAddEn).value <QPixmap>() :
                                   model->data (aIndex, StorageModel::R_HDPixmapAddDis).value <QPixmap>();
                    break;
                }
                case KDeviceType_DVD:
                {
                    deviceRect = model->data (aIndex, StorageModel::R_CDPixmapRect).value <QRect>();
                    devicePixmap = model->data (aIndex, StorageModel::R_IsMoreAttachmentsPossible).toBool() ?
                                   model->data (aIndex, StorageModel::R_CDPixmapAddEn).value <QPixmap>() :
                                   model->data (aIndex, StorageModel::R_CDPixmapAddDis).value <QPixmap>();
                    break;
                }
                case KDeviceType_Floppy:
                {
                    deviceRect = model->data (aIndex, StorageModel::R_FDPixmapRect).value <QRect>();
                    devicePixmap = model->data (aIndex, StorageModel::R_IsMoreAttachmentsPossible).toBool() ?
                                   model->data (aIndex, StorageModel::R_FDPixmapAddEn).value <QPixmap>() :
                                   model->data (aIndex, StorageModel::R_FDPixmapAddDis).value <QPixmap>();
                    break;
                }
                default:
                    break;
            }

            aPainter->drawPixmap (QPoint (rect.width() + deviceRect.x(), deviceRect.y()), devicePixmap);
        }
    }

    aPainter->restore();

    drawFocus (aPainter, aOption, rect);
}


/**
 * UI Medium ID Holder.
 * Used for compliance with other storage page widgets
 * which caching and holding corresponding information.
 */
class UIMediumIDHolder : public QObject
{
    Q_OBJECT;

public:

    UIMediumIDHolder(QWidget *pParent) : QObject(pParent) {}

    QUuid id() const { return m_uId; }
    void setId(const QUuid &uId) { m_uId = uId; emit sigChanged(); }

    UIMediumDeviceType type() const { return m_type; }
    void setType(UIMediumDeviceType type) { m_type = type; }

    bool isNull() const { return m_uId == UIMedium().id(); }

signals:

    void sigChanged();

private:

    QUuid m_uId;
    UIMediumDeviceType m_type;
};


/* static */
const QString UIMachineSettingsStorage::ControllerMimeType = QString("application/virtualbox;value=StorageControllerID");
const QString UIMachineSettingsStorage::AttachmentMimeType = QString("application/virtualbox;value=StorageAttachmentID");

UIMachineSettingsStorage::UIMachineSettingsStorage()
    : m_pModelStorage(0)
    , m_pActionAddController(0), m_pActionRemoveController(0)
    , m_pActionAddControllerIDE(0), m_pActionAddControllerSATA(0), m_pActionAddControllerSCSI(0), m_pActionAddControllerSAS(0), m_pActionAddControllerFloppy(0), m_pActionAddControllerUSB(0), m_pActionAddControllerNVMe(0), m_pActionAddControllerVirtioSCSI(0)
    , m_pActionAddAttachment(0), m_pActionRemoveAttachment(0)
    , m_pActionAddAttachmentHD(0), m_pActionAddAttachmentCD(0), m_pActionAddAttachmentFD(0)
    , m_pMediumIdHolder(new UIMediumIDHolder(this))
    , m_fPolished(false)
    , m_fLoadingInProgress(0)
    , m_pCache(0)
{
    /* Prepare: */
    prepare();
}

UIMachineSettingsStorage::~UIMachineSettingsStorage()
{
    /* Cleanup: */
    cleanup();
}

void UIMachineSettingsStorage::setChipsetType(KChipsetType enmType)
{
    /* Make sure chipset type has changed: */
    if (m_pModelStorage->chipsetType() == enmType)
        return;

    /* Update chipset type value: */
    m_pModelStorage->setChipsetType(enmType);
    sltUpdateActionStates();

    /* Revalidate: */
    revalidate();
}

bool UIMachineSettingsStorage::changed() const
{
    return m_pCache->wasChanged();
}

void UIMachineSettingsStorage::loadToCacheFrom(QVariant &data)
{
    /* Fetch data to machine: */
    UISettingsPageMachine::fetchData(data);

    /* Clear cache initially: */
    m_pCache->clear();

    /* Prepare old storage data: */
    UIDataSettingsMachineStorage oldStorageData;

    /* Gather old common data: */
    m_uMachineId = m_machine.GetId();
    m_strMachineSettingsFilePath = m_machine.GetSettingsFilePath();
    m_strMachineName = m_machine.GetName();
    m_strMachineGuestOSTypeId = m_machine.GetOSTypeId();

    /* For each controller: */
    const CStorageControllerVector &controllers = m_machine.GetStorageControllers();
    for (int iControllerIndex = 0; iControllerIndex < controllers.size(); ++iControllerIndex)
    {
        /* Prepare old controller data & cache key: */
        UIDataSettingsMachineStorageController oldControllerData;
        QString strControllerKey = QString::number(iControllerIndex);

        /* Check whether controller is valid: */
        const CStorageController &comController = controllers.at(iControllerIndex);
        if (!comController.isNull())
        {
            /* Gather old controller data: */
            oldControllerData.m_strControllerName = comController.GetName();
            oldControllerData.m_controllerBus = comController.GetBus();
            oldControllerData.m_controllerType = comController.GetControllerType();
            oldControllerData.m_uPortCount = comController.GetPortCount();
            oldControllerData.m_fUseHostIOCache = comController.GetUseHostIOCache();
            /* Override controller cache key: */
            strControllerKey = oldControllerData.m_strControllerName;

            /* Sort attachments before caching/fetching: */
            const CMediumAttachmentVector &attachmentVector =
                m_machine.GetMediumAttachmentsOfController(oldControllerData.m_strControllerName);
            QMap<StorageSlot, CMediumAttachment> attachmentMap;
            foreach (const CMediumAttachment &comAttachment, attachmentVector)
            {
                const StorageSlot storageSlot(oldControllerData.m_controllerBus,
                                              comAttachment.GetPort(), comAttachment.GetDevice());
                attachmentMap.insert(storageSlot, comAttachment);
            }
            const QList<CMediumAttachment> &attachments = attachmentMap.values();

            /* For each attachment: */
            for (int iAttachmentIndex = 0; iAttachmentIndex < attachments.size(); ++iAttachmentIndex)
            {
                /* Prepare old attachment data & cache key: */
                UIDataSettingsMachineStorageAttachment oldAttachmentData;
                QString strAttachmentKey = QString::number(iAttachmentIndex);

                /* Check whether attachment is valid: */
                const CMediumAttachment &comAttachment = attachments.at(iAttachmentIndex);
                if (!comAttachment.isNull())
                {
                    /* Gather old attachment data: */
                    oldAttachmentData.m_attachmentType = comAttachment.GetType();
                    oldAttachmentData.m_iAttachmentPort = comAttachment.GetPort();
                    oldAttachmentData.m_iAttachmentDevice = comAttachment.GetDevice();
                    oldAttachmentData.m_fAttachmentPassthrough = comAttachment.GetPassthrough();
                    oldAttachmentData.m_fAttachmentTempEject = comAttachment.GetTemporaryEject();
                    oldAttachmentData.m_fAttachmentNonRotational = comAttachment.GetNonRotational();
                    oldAttachmentData.m_fAttachmentHotPluggable = comAttachment.GetHotPluggable();
                    const CMedium comMedium = comAttachment.GetMedium();
                    oldAttachmentData.m_uAttachmentMediumId = comMedium.isNull() ? UIMedium::nullID() : comMedium.GetId();
                    /* Override controller cache key: */
                    strAttachmentKey = QString("%1:%2").arg(oldAttachmentData.m_iAttachmentPort).arg(oldAttachmentData.m_iAttachmentDevice);
                }

                /* Cache old attachment data: */
                m_pCache->child(strControllerKey).child(strAttachmentKey).cacheInitialData(oldAttachmentData);
            }
        }

        /* Cache old controller data: */
        m_pCache->child(strControllerKey).cacheInitialData(oldControllerData);
    }

    /* Cache old storage data: */
    m_pCache->cacheInitialData(oldStorageData);

    /* Upload machine to data: */
    UISettingsPageMachine::uploadData(data);
}

void UIMachineSettingsStorage::getFromCache()
{
    /* Clear model initially: */
    m_pModelStorage->clear();

    /* Load old common data from the cache: */
    m_pModelStorage->setMachineId(m_uMachineId);

    /* For each controller: */
    for (int iControllerIndex = 0; iControllerIndex < m_pCache->childCount(); ++iControllerIndex)
    {
        /* Get controller cache: */
        const UISettingsCacheMachineStorageController &controllerCache = m_pCache->child(iControllerIndex);
        /* Get old controller data from the cache: */
        const UIDataSettingsMachineStorageController &oldControllerData = controllerCache.base();

        /* Load old controller data from the cache: */
        const QModelIndex controllerIndex = m_pModelStorage->addController(oldControllerData.m_strControllerName,
                                                                           oldControllerData.m_controllerBus,
                                                                           oldControllerData.m_controllerType);
        const QUuid controllerId = QUuid(m_pModelStorage->data(controllerIndex, StorageModel::R_ItemId).toString());
        m_pModelStorage->setData(controllerIndex, oldControllerData.m_uPortCount, StorageModel::R_CtrPortCount);
        m_pModelStorage->setData(controllerIndex, oldControllerData.m_fUseHostIOCache, StorageModel::R_CtrIoCache);

        /* For each attachment: */
        for (int iAttachmentIndex = 0; iAttachmentIndex < controllerCache.childCount(); ++iAttachmentIndex)
        {
            /* Get attachment cache: */
            const UISettingsCacheMachineStorageAttachment &attachmentCache = controllerCache.child(iAttachmentIndex);
            /* Get old attachment data from the cache: */
            const UIDataSettingsMachineStorageAttachment &oldAttachmentData = attachmentCache.base();

            /* Load old attachment data from the cache: */
            const QModelIndex attachmentIndex = m_pModelStorage->addAttachment(controllerId,
                                                                               oldAttachmentData.m_attachmentType,
                                                                               oldAttachmentData.m_uAttachmentMediumId);
            const StorageSlot attachmentStorageSlot(oldControllerData.m_controllerBus,
                                                    oldAttachmentData.m_iAttachmentPort,
                                                    oldAttachmentData.m_iAttachmentDevice);
            m_pModelStorage->setData(attachmentIndex, QVariant::fromValue(attachmentStorageSlot), StorageModel::R_AttSlot);
            m_pModelStorage->setData(attachmentIndex, oldAttachmentData.m_fAttachmentPassthrough, StorageModel::R_AttIsPassthrough);
            m_pModelStorage->setData(attachmentIndex, oldAttachmentData.m_fAttachmentTempEject, StorageModel::R_AttIsTempEject);
            m_pModelStorage->setData(attachmentIndex, oldAttachmentData.m_fAttachmentNonRotational, StorageModel::R_AttIsNonRotational);
            m_pModelStorage->setData(attachmentIndex, oldAttachmentData.m_fAttachmentHotPluggable, StorageModel::R_AttIsHotPluggable);
        }
    }

    /* Choose first controller as current: */
    if (m_pModelStorage->rowCount(m_pModelStorage->root()) > 0)
        m_pTreeStorage->setCurrentIndex(m_pModelStorage->index(0, 0, m_pModelStorage->root()));

    /* Fetch recent information: */
    sltHandleCurrentItemChange();

    /* Polish page finally: */
    polishPage();

    /* Revalidate: */
    revalidate();
}

void UIMachineSettingsStorage::putToCache()
{
    /* Prepare new storage data: */
    UIDataSettingsMachineStorage newStorageData;

    /* For each controller: */
    const QModelIndex rootIndex = m_pModelStorage->root();
    for (int iControllerIndex = 0; iControllerIndex < m_pModelStorage->rowCount(rootIndex); ++iControllerIndex)
    {
        /* Prepare new controller data & key: */
        UIDataSettingsMachineStorageController newControllerData;

        /* Gather new controller data & cache key from model: */
        const QModelIndex controllerIndex = m_pModelStorage->index(iControllerIndex, 0, rootIndex);
        newControllerData.m_strControllerName = m_pModelStorage->data(controllerIndex, StorageModel::R_CtrName).toString();
        newControllerData.m_controllerBus = m_pModelStorage->data(controllerIndex, StorageModel::R_CtrBusType).value<KStorageBus>();
        newControllerData.m_controllerType = m_pModelStorage->data(controllerIndex, StorageModel::R_CtrType).value<KStorageControllerType>();
        newControllerData.m_uPortCount = m_pModelStorage->data(controllerIndex, StorageModel::R_CtrPortCount).toUInt();
        newControllerData.m_fUseHostIOCache = m_pModelStorage->data(controllerIndex, StorageModel::R_CtrIoCache).toBool();
        const QString strControllerKey = m_pModelStorage->data(controllerIndex, StorageModel::R_CtrOldName).toString();

        /* For each attachment: */
        for (int iAttachmentIndex = 0; iAttachmentIndex < m_pModelStorage->rowCount(controllerIndex); ++iAttachmentIndex)
        {
            /* Prepare new attachment data & key: */
            UIDataSettingsMachineStorageAttachment newAttachmentData;

            /* Gather new attachment data & cache key from model: */
            const QModelIndex attachmentIndex = m_pModelStorage->index(iAttachmentIndex, 0, controllerIndex);
            newAttachmentData.m_attachmentType = m_pModelStorage->data(attachmentIndex, StorageModel::R_AttDevice).value<KDeviceType>();
            const StorageSlot attachmentSlot = m_pModelStorage->data(attachmentIndex, StorageModel::R_AttSlot).value<StorageSlot>();
            newAttachmentData.m_iAttachmentPort = attachmentSlot.port;
            newAttachmentData.m_iAttachmentDevice = attachmentSlot.device;
            newAttachmentData.m_fAttachmentPassthrough = m_pModelStorage->data(attachmentIndex, StorageModel::R_AttIsPassthrough).toBool();
            newAttachmentData.m_fAttachmentTempEject = m_pModelStorage->data(attachmentIndex, StorageModel::R_AttIsTempEject).toBool();
            newAttachmentData.m_fAttachmentNonRotational = m_pModelStorage->data(attachmentIndex, StorageModel::R_AttIsNonRotational).toBool();
            newAttachmentData.m_fAttachmentHotPluggable = m_pModelStorage->data(attachmentIndex, StorageModel::R_AttIsHotPluggable).toBool();
            newAttachmentData.m_uAttachmentMediumId = m_pModelStorage->data(attachmentIndex, StorageModel::R_AttMediumId).toString();
            const QString strAttachmentKey = QString("%1:%2").arg(newAttachmentData.m_iAttachmentPort).arg(newAttachmentData.m_iAttachmentDevice);

            /* Cache new attachment data: */
            m_pCache->child(strControllerKey).child(strAttachmentKey).cacheCurrentData(newAttachmentData);
        }

        /* Cache new controller data: */
        m_pCache->child(strControllerKey).cacheCurrentData(newControllerData);
    }

    /* Cache new storage data: */
    m_pCache->cacheCurrentData(newStorageData);
}

void UIMachineSettingsStorage::saveFromCacheTo(QVariant &data)
{
    /* Fetch data to machine: */
    UISettingsPageMachine::fetchData(data);

    /* Update storage data and failing state: */
    setFailed(!saveStorageData());

    /* Upload machine to data: */
    UISettingsPageMachine::uploadData(data);
}

bool UIMachineSettingsStorage::validate(QList<UIValidationMessage> &messages)
{
    /* Pass by default: */
    bool fPass = true;

    /* Prepare message: */
    UIValidationMessage message;

    /* Check controllers for name emptiness & coincidence.
     * Check attachments for the hd presence / uniqueness. */
    const QModelIndex rootIndex = m_pModelStorage->root();
    QMap <QString, QString> config;
    QMap<int, QString> names;
    /* For each controller: */
    for (int i = 0; i < m_pModelStorage->rowCount(rootIndex); ++i)
    {
        const QModelIndex ctrIndex = rootIndex.child(i, 0);
        const QString ctrName = m_pModelStorage->data(ctrIndex, StorageModel::R_CtrName).toString();

        /* Check for name emptiness: */
        if (ctrName.isEmpty())
        {
            message.second << tr("No name is currently specified for the controller at position <b>%1</b>.").arg(i + 1);
            fPass = false;
        }
        /* Check for name coincidence: */
        if (names.values().contains(ctrName))
        {
            message.second << tr("The controller at position <b>%1</b> has the same name as the controller at position <b>%2</b>.")
                                 .arg(i + 1).arg(names.key(ctrName) + 1);
            fPass = false;
        }
        else
            names.insert(i, ctrName);

        /* For each attachment: */
        for (int j = 0; j < m_pModelStorage->rowCount(ctrIndex); ++j)
        {
            const QModelIndex attIndex = ctrIndex.child(j, 0);
            const StorageSlot attSlot = m_pModelStorage->data(attIndex, StorageModel::R_AttSlot).value <StorageSlot>();
            const KDeviceType enmAttDevice = m_pModelStorage->data(attIndex, StorageModel::R_AttDevice).value <KDeviceType>();
            const QString key(m_pModelStorage->data(attIndex, StorageModel::R_AttMediumId).toString());
            const QString value(QString("%1 (%2)").arg(ctrName, gpConverter->toString(attSlot)));
            /* Check for emptiness: */
            if (uiCommon().medium(key).isNull() && enmAttDevice == KDeviceType_HardDisk)
            {
                message.second << tr("No hard disk is selected for <i>%1</i>.").arg(value);
                fPass = false;
            }
            /* Check for coincidence: */
            if (!uiCommon().medium(key).isNull() && config.contains(key))
            {
                message.second << tr("<i>%1</i> is using a disk that is already attached to <i>%2</i>.")
                                     .arg(value).arg(config[key]);
                fPass = false;
            }
            else
                config.insert(key, value);
        }
    }

    /* Check for excessive controllers on Storage page controllers list: */
    QStringList excessiveList;
    const QMap<KStorageBus, int> currentType = m_pModelStorage->currentControllerTypes();
    const QMap<KStorageBus, int> maximumType = m_pModelStorage->maximumControllerTypes();
    for (int iStorageBusType = KStorageBus_IDE; iStorageBusType < KStorageBus_Max; ++iStorageBusType)
    {
        if (currentType[(KStorageBus)iStorageBusType] > maximumType[(KStorageBus)iStorageBusType])
        {
            QString strExcessiveRecord = QString("%1 (%2)");
            strExcessiveRecord = strExcessiveRecord.arg(QString("<b>%1</b>").arg(gpConverter->toString((KStorageBus)iStorageBusType)));
            strExcessiveRecord = strExcessiveRecord.arg(maximumType[(KStorageBus)iStorageBusType] == 1 ?
                                                        tr("at most one supported", "controller") :
                                                        tr("up to %1 supported", "controllers").arg(maximumType[(KStorageBus)iStorageBusType]));
            excessiveList << strExcessiveRecord;
        }
    }
    if (!excessiveList.isEmpty())
    {
        message.second << tr("The machine currently has more storage controllers assigned than a %1 chipset supports. "
                             "Please change the chipset type on the System settings page or reduce the number "
                             "of the following storage controllers on the Storage settings page: %2")
                             .arg(gpConverter->toString(m_pModelStorage->chipsetType()))
                             .arg(excessiveList.join(", "));
        fPass = false;
    }

    /* Serialize message: */
    if (!message.second.isEmpty())
        messages << message;

    /* Return result: */
    return fPass;
}

void UIMachineSettingsStorage::setConfigurationAccessLevel(ConfigurationAccessLevel enmLevel)
{
    /* Update model 'configuration access level': */
    m_pModelStorage->setConfigurationAccessLevel(enmLevel);
    /* Update 'configuration access level' of base class: */
    UISettingsPageMachine::setConfigurationAccessLevel(enmLevel);
}

void UIMachineSettingsStorage::retranslateUi()
{
    /* Translate uic generated strings: */
    Ui::UIMachineSettingsStorage::retranslateUi(this);

    /* Translate storage-view: */
    m_pTreeStorage->setWhatsThis(tr("Lists all storage controllers for this machine and "
                                    "the virtual images and host drives attached to them."));

    /* Translate tool-bar: */
    m_pActionAddController->setShortcut(QKeySequence("Ins"));
    m_pActionRemoveController->setShortcut(QKeySequence("Del"));
    m_pActionAddAttachment->setShortcut(QKeySequence("+"));
    m_pActionRemoveAttachment->setShortcut(QKeySequence("-"));

    m_pActionAddController->setText(tr("Add Controller"));
    m_pActionAddControllerIDE->setText(tr("Add IDE Controller"));
    m_pActionAddControllerSATA->setText(tr("Add SATA Controller"));
    m_pActionAddControllerSCSI->setText(tr("Add SCSI Controller"));
    m_pActionAddControllerSAS->setText(tr("Add SAS Controller"));
    m_pActionAddControllerFloppy->setText(tr("Add Floppy Controller"));
    m_pActionAddControllerUSB->setText(tr("Add USB Controller"));
    m_pActionAddControllerNVMe->setText(tr("Add NVMe Controller"));
    m_pActionAddControllerVirtioSCSI->setText(tr("Add virtio-scsi Controller"));
    m_pActionRemoveController->setText(tr("Remove Controller"));
    m_pActionAddAttachment->setText(tr("Add Attachment"));
    m_pActionAddAttachmentHD->setText(tr("Add Hard Disk"));
    m_pActionAddAttachmentCD->setText(tr("Add Optical Drive"));
    m_pActionAddAttachmentFD->setText(tr("Add Floppy Drive"));
    m_pActionRemoveAttachment->setText(tr("Remove Attachment"));

    m_pActionAddController->setWhatsThis(tr("Adds new storage controller."));
    m_pActionRemoveController->setWhatsThis(tr("Removes selected storage controller."));
    m_pActionAddAttachment->setWhatsThis(tr("Adds new storage attachment."));
    m_pActionRemoveAttachment->setWhatsThis(tr("Removes selected storage attachment."));

    m_pActionAddController->setToolTip(m_pActionAddController->whatsThis());
    m_pActionRemoveController->setToolTip(m_pActionRemoveController->whatsThis());
    m_pActionAddAttachment->setToolTip(m_pActionAddAttachment->whatsThis());
    m_pActionRemoveAttachment->setToolTip(m_pActionRemoveAttachment->whatsThis());
}

void UIMachineSettingsStorage::polishPage()
{
    /* Declare required variables: */
    const QModelIndex index = m_pTreeStorage->currentIndex();
    const KDeviceType enmDevice = m_pModelStorage->data(index, StorageModel::R_AttDevice).value<KDeviceType>();

    /* Polish left pane availability: */
    mLsLeftPane->setEnabled(isMachineInValidMode());
    m_pTreeStorage->setEnabled(isMachineInValidMode());

    /* Polish empty information pane availability: */
    mLsEmpty->setEnabled(isMachineInValidMode());
    mLbInfo->setEnabled(isMachineInValidMode());

    /* Polish controllers pane availability: */
    mLsParameters->setEnabled(isMachineInValidMode());
    mLbName->setEnabled(isMachineOffline());
    mLeName->setEnabled(isMachineOffline());
    mLbBus->setEnabled(isMachineOffline());
    mCbBus->setEnabled(isMachineOffline());
    mLbType->setEnabled(isMachineOffline());
    mCbType->setEnabled(isMachineOffline());
    mLbPortCount->setEnabled(isMachineOffline());
    mSbPortCount->setEnabled(isMachineOffline());
    mCbIoCache->setEnabled(isMachineOffline());

    /* Polish attachments pane availability: */
    mLsAttributes->setEnabled(isMachineInValidMode());
    mLbMedium->setEnabled(isMachineOffline() || (isMachineOnline() && enmDevice != KDeviceType_HardDisk));
    mCbSlot->setEnabled(isMachineOffline());
    mTbOpen->setEnabled(isMachineOffline() || (isMachineOnline() && enmDevice != KDeviceType_HardDisk));
    mCbPassthrough->setEnabled(isMachineOffline());
    mCbTempEject->setEnabled(isMachineInValidMode());
    mCbNonRotational->setEnabled(isMachineOffline());
    m_pCheckBoxHotPluggable->setEnabled(isMachineOffline());
    mLsInformation->setEnabled(isMachineInValidMode());
    mLbHDFormat->setEnabled(isMachineInValidMode());
    mLbHDFormatValue->setEnabled(isMachineInValidMode());
    mLbCDFDType->setEnabled(isMachineInValidMode());
    mLbCDFDTypeValue->setEnabled(isMachineInValidMode());
    mLbHDVirtualSize->setEnabled(isMachineInValidMode());
    mLbHDVirtualSizeValue->setEnabled(isMachineInValidMode());
    mLbHDActualSize->setEnabled(isMachineInValidMode());
    mLbHDActualSizeValue->setEnabled(isMachineInValidMode());
    mLbSize->setEnabled(isMachineInValidMode());
    mLbSizeValue->setEnabled(isMachineInValidMode());
    mLbHDDetails->setEnabled(isMachineInValidMode());
    mLbHDDetailsValue->setEnabled(isMachineInValidMode());
    mLbLocation->setEnabled(isMachineInValidMode());
    mLbLocationValue->setEnabled(isMachineInValidMode());
    mLbUsage->setEnabled(isMachineInValidMode());
    mLbUsageValue->setEnabled(isMachineInValidMode());
    m_pLabelEncryption->setEnabled(isMachineInValidMode());
    m_pLabelEncryptionValue->setEnabled(isMachineInValidMode());

    /* Update action states: */
    sltUpdateActionStates();
}

void UIMachineSettingsStorage::showEvent(QShowEvent *pEvent)
{
    if (!m_fPolished)
    {
        m_fPolished = true;

        /* First column indent: */
        mLtEmpty->setColumnMinimumWidth(0, 10);
        mLtController->setColumnMinimumWidth(0, 10);
        mLtAttachment->setColumnMinimumWidth(0, 10);
#if 0
        /* Second column indent minimum width: */
        QList <QLabel*> labelsList;
        labelsList << mLbMedium << mLbHDFormat << mLbCDFDType
                   << mLbHDVirtualSize << mLbHDActualSize << mLbSize
                   << mLbLocation << mLbUsage;
        int maxWidth = 0;
        QFontMetrics metrics(font());
        foreach (QLabel *label, labelsList)
        {
            int width = metrics.width(label->text());
            maxWidth = width > maxWidth ? width : maxWidth;
        }
        mLtAttachment->setColumnMinimumWidth(1, maxWidth);
#endif
    }

    /* Call to base-class: */
    UISettingsPageMachine::showEvent(pEvent);
}

void UIMachineSettingsStorage::sltHandleMediumEnumerated(const QUuid &uMediumId)
{
    /* Search for corresponding medium: */
    const UIMedium medium = uiCommon().medium(uMediumId);

    const QModelIndex rootIndex = m_pModelStorage->root();
    for (int i = 0; i < m_pModelStorage->rowCount(rootIndex); ++i)
    {
        const QModelIndex ctrIndex = rootIndex.child(i, 0);
        for (int j = 0; j < m_pModelStorage->rowCount(ctrIndex); ++j)
        {
            const QModelIndex attIndex = ctrIndex.child(j, 0);
            const QUuid attMediumId = m_pModelStorage->data(attIndex, StorageModel::R_AttMediumId).toString();
            if (attMediumId == medium.id())
            {
                m_pModelStorage->setData(attIndex, attMediumId, StorageModel::R_AttMediumId);

                /* Revalidate: */
                revalidate();
            }
        }
    }
}

void UIMachineSettingsStorage::sltHandleMediumDeleted(const QUuid &uMediumId)
{
    QModelIndex rootIndex = m_pModelStorage->root();
    for (int i = 0; i < m_pModelStorage->rowCount(rootIndex); ++i)
    {
        QModelIndex ctrIndex = rootIndex.child(i, 0);
        for (int j = 0; j < m_pModelStorage->rowCount(ctrIndex); ++j)
        {
            QModelIndex attIndex = ctrIndex.child(j, 0);
            QUuid attMediumId = m_pModelStorage->data(attIndex, StorageModel::R_AttMediumId).toString();
            if (attMediumId == uMediumId)
            {
                m_pModelStorage->setData(attIndex, UIMedium().id(), StorageModel::R_AttMediumId);

                /* Revalidate: */
                revalidate();
            }
        }
    }
}

void UIMachineSettingsStorage::sltAddController()
{
    QMenu menu;
    menu.addAction(m_pActionAddControllerIDE);
    menu.addAction(m_pActionAddControllerSATA);
    menu.addAction(m_pActionAddControllerSCSI);
    menu.addAction(m_pActionAddControllerSAS);
    menu.addAction(m_pActionAddControllerFloppy);
    menu.addAction(m_pActionAddControllerUSB);
    menu.addAction(m_pActionAddControllerNVMe);
    menu.addAction(m_pActionAddControllerVirtioSCSI);
    menu.exec(QCursor::pos());
}

void UIMachineSettingsStorage::sltAddControllerIDE()
{
    addControllerWrapper(generateUniqueControllerName("IDE"), KStorageBus_IDE, KStorageControllerType_PIIX4);
}

void UIMachineSettingsStorage::sltAddControllerSATA()
{
    addControllerWrapper(generateUniqueControllerName("SATA"), KStorageBus_SATA, KStorageControllerType_IntelAhci);
}

void UIMachineSettingsStorage::sltAddControllerSCSI()
{
    addControllerWrapper(generateUniqueControllerName("SCSI"), KStorageBus_SCSI, KStorageControllerType_LsiLogic);
}

void UIMachineSettingsStorage::sltAddControllerFloppy()
{
    addControllerWrapper(generateUniqueControllerName("Floppy"), KStorageBus_Floppy, KStorageControllerType_I82078);
}

void UIMachineSettingsStorage::sltAddControllerSAS()
{
    addControllerWrapper(generateUniqueControllerName("SAS"), KStorageBus_SAS, KStorageControllerType_LsiLogicSas);
}

void UIMachineSettingsStorage::sltAddControllerUSB()
{
    addControllerWrapper(generateUniqueControllerName("USB"), KStorageBus_USB, KStorageControllerType_USB);
}

void UIMachineSettingsStorage::sltAddControllerNVMe()
{
    addControllerWrapper(generateUniqueControllerName("NVMe"), KStorageBus_PCIe, KStorageControllerType_NVMe);
}

void UIMachineSettingsStorage::sltAddControllerVirtioSCSI()
{
    addControllerWrapper(generateUniqueControllerName("VirtIO"), KStorageBus_VirtioSCSI, KStorageControllerType_VirtioSCSI);
}

void UIMachineSettingsStorage::sltRemoveController()
{
    const QModelIndex index = m_pTreeStorage->currentIndex();
    if (!m_pModelStorage->data(index, StorageModel::R_IsController).toBool())
        return;

    m_pModelStorage->delController(QUuid(m_pModelStorage->data(index, StorageModel::R_ItemId).toString()));
    emit sigStorageChanged();

    /* Revalidate: */
    revalidate();
}

void UIMachineSettingsStorage::sltAddAttachment()
{
    const QModelIndex index = m_pTreeStorage->currentIndex();
    Assert(m_pModelStorage->data(index, StorageModel::R_IsController).toBool());

    const DeviceTypeList deviceTypeList(m_pModelStorage->data(index, StorageModel::R_CtrDevices).value <DeviceTypeList>());
    const bool fJustTrigger = deviceTypeList.size() == 1;
    const bool fShowMenu = deviceTypeList.size() > 1;
    QMenu menu;
    foreach (const KDeviceType &deviceType, deviceTypeList)
    {
        switch (deviceType)
        {
            case KDeviceType_HardDisk:
                if (fJustTrigger)
                    m_pActionAddAttachmentHD->trigger();
                if (fShowMenu)
                    menu.addAction(m_pActionAddAttachmentHD);
                break;
            case KDeviceType_DVD:
                if (fJustTrigger)
                    m_pActionAddAttachmentCD->trigger();
                if (fShowMenu)
                    menu.addAction(m_pActionAddAttachmentCD);
                break;
            case KDeviceType_Floppy:
                if (fJustTrigger)
                    m_pActionAddAttachmentFD->trigger();
                if (fShowMenu)
                    menu.addAction(m_pActionAddAttachmentFD);
                break;
            default:
                break;
        }
    }
    if (fShowMenu)
        menu.exec(QCursor::pos());
}

void UIMachineSettingsStorage::sltAddAttachmentHD()
{
    addAttachmentWrapper(KDeviceType_HardDisk);
}

void UIMachineSettingsStorage::sltAddAttachmentCD()
{
    addAttachmentWrapper(KDeviceType_DVD);
}

void UIMachineSettingsStorage::sltAddAttachmentFD()
{
    addAttachmentWrapper(KDeviceType_Floppy);
}

void UIMachineSettingsStorage::sltRemoveAttachment()
{
    const QModelIndex index = m_pTreeStorage->currentIndex();

    const KDeviceType enmDevice = m_pModelStorage->data(index, StorageModel::R_AttDevice).value <KDeviceType>();
    /* Check if this would be the last DVD. If so let the user confirm this again. */
    if (   enmDevice == KDeviceType_DVD
        && deviceCount(KDeviceType_DVD) == 1)
    {
        if (!msgCenter().confirmRemovingOfLastDVDDevice(this))
            return;
    }

    const QModelIndex parent = index.parent();
    if (!index.isValid() || !parent.isValid() ||
        !m_pModelStorage->data(index, StorageModel::R_IsAttachment).toBool() ||
        !m_pModelStorage->data(parent, StorageModel::R_IsController).toBool())
        return;

    m_pModelStorage->delAttachment(QUuid(m_pModelStorage->data(parent, StorageModel::R_ItemId).toString()),
                                   QUuid(m_pModelStorage->data(index, StorageModel::R_ItemId).toString()));
    emit sigStorageChanged();

    /* Revalidate: */
    revalidate();
}

void UIMachineSettingsStorage::sltGetInformation()
{
    m_fLoadingInProgress = true;

    const QModelIndex index = m_pTreeStorage->currentIndex();
    if (!index.isValid() || index == m_pModelStorage->root())
    {
        /* Showing Initial Page: */
        mSwRightPane->setCurrentIndex(0);
    }
    else
    {
        switch (m_pModelStorage->data(index, StorageModel::R_ItemType).value <AbstractItem::ItemType>())
        {
            case AbstractItem::Type_ControllerItem:
            {
                /* Getting Controller Name: */
                const QString strCtrName = m_pModelStorage->data(index, StorageModel::R_CtrName).toString();
                if (mLeName->text() != strCtrName)
                    mLeName->setText(strCtrName);

                /* Getting Controller Bus type: */
                mCbBus->clear();
                const ControllerBusList controllerBusList(m_pModelStorage->data(index, StorageModel::R_CtrBusTypes).value<ControllerBusList>());
                for (int i = 0; i < controllerBusList.size(); ++i)
                    mCbBus->insertItem(mCbBus->count(), gpConverter->toString(controllerBusList[i]));
                const KStorageBus enmBus = m_pModelStorage->data(index, StorageModel::R_CtrBusType).value<KStorageBus>();
                const int iBusPos = mCbBus->findText(gpConverter->toString(enmBus));
                mCbBus->setCurrentIndex(iBusPos == -1 ? 0 : iBusPos);

                /* Getting Controller Sub type: */
                mCbType->clear();
                const ControllerTypeList controllerTypeList(m_pModelStorage->data(index, StorageModel::R_CtrTypes).value <ControllerTypeList>());
                for (int i = 0; i < controllerTypeList.size(); ++i)
                    mCbType->insertItem(mCbType->count(), gpConverter->toString(controllerTypeList[i]));
                const KStorageControllerType enmType = m_pModelStorage->data(index, StorageModel::R_CtrType).value <KStorageControllerType>();
                const int iCtrPos = mCbType->findText(gpConverter->toString(enmType));
                mCbType->setCurrentIndex(iCtrPos == -1 ? 0 : iCtrPos);

                mLbPortCount->setVisible(enmBus == KStorageBus_SATA || enmBus == KStorageBus_SAS);
                mSbPortCount->setVisible(enmBus == KStorageBus_SATA || enmBus == KStorageBus_SAS);
                const uint uPortCount = m_pModelStorage->data(index, StorageModel::R_CtrPortCount).toUInt();
                const uint uMaxPortCount = m_pModelStorage->data(index, StorageModel::R_CtrMaxPortCount).toUInt();
                mSbPortCount->setMaximum(uMaxPortCount);
                mSbPortCount->setValue(uPortCount);

                const bool fUseIoCache = m_pModelStorage->data(index, StorageModel::R_CtrIoCache).toBool();
                mCbIoCache->setChecked(fUseIoCache);

                /* Showing Controller Page: */
                mSwRightPane->setCurrentIndex(1);
                break;
            }
            case AbstractItem::Type_AttachmentItem:
            {
                /* Getting Attachment Slot: */
                mCbSlot->clear();
                const SlotsList slotsList(m_pModelStorage->data(index, StorageModel::R_AttSlots).value <SlotsList>());
                for (int i = 0; i < slotsList.size(); ++i)
                    mCbSlot->insertItem(mCbSlot->count(), gpConverter->toString(slotsList[i]));
                const StorageSlot slt = m_pModelStorage->data(index, StorageModel::R_AttSlot).value <StorageSlot>();
                const int iAttSlotPos = mCbSlot->findText(gpConverter->toString(slt));
                mCbSlot->setCurrentIndex(iAttSlotPos == -1 ? 0 : iAttSlotPos);
                mCbSlot->setToolTip(mCbSlot->itemText(mCbSlot->currentIndex()));

                /* Getting Attachment Medium: */
                const KDeviceType enmDevice = m_pModelStorage->data(index, StorageModel::R_AttDevice).value <KDeviceType>();
                switch (enmDevice)
                {
                    case KDeviceType_HardDisk:
                        mLbMedium->setText(tr("Hard &Disk:"));
                        mTbOpen->setIcon(iconPool()->icon(HDAttachmentNormal));
                        mTbOpen->setWhatsThis(tr("Choose or create a virtual hard disk file. The virtual machine will see "
                                                 "the data in the file as the contents of the virtual hard disk."));
                        break;
                    case KDeviceType_DVD:
                        mLbMedium->setText(tr("Optical &Drive:"));
                        mTbOpen->setIcon(iconPool()->icon(CDAttachmentNormal));
                        mTbOpen->setWhatsThis(tr("Choose a virtual optical disk or a physical drive to use with the virtual drive. "
                                                 "The virtual machine will see a disk inserted into the drive with the data "
                                                 "in the file or on the disk in the physical drive as its contents."));
                        break;
                    case KDeviceType_Floppy:
                        mLbMedium->setText(tr("Floppy &Drive:"));
                        mTbOpen->setIcon(iconPool()->icon(FDAttachmentNormal));
                        mTbOpen->setWhatsThis(tr("Choose a virtual floppy disk or a physical drive to use with the virtual drive. "
                                                 "The virtual machine will see a disk inserted into the drive with the data "
                                                 "in the file or on the disk in the physical drive as its contents."));
                        break;
                    default:
                        break;
                }

                /* Get hot-pluggable state: */
                const bool fIsHotPluggable = m_pModelStorage->data(index, StorageModel::R_AttIsHotPluggable).toBool();

                /* Fetch device-type, medium-id: */
                m_pMediumIdHolder->setType(mediumTypeToLocal(enmDevice));
                m_pMediumIdHolder->setId(m_pModelStorage->data(index, StorageModel::R_AttMediumId).toString());

                /* Get/fetch editable state: */
                const bool fIsEditable =    (isMachineOffline())
                                         || (isMachineOnline() && enmDevice != KDeviceType_HardDisk)
                                         || (isMachineOnline() && enmDevice == KDeviceType_HardDisk && fIsHotPluggable);
                mLbMedium->setEnabled(fIsEditable);
                mTbOpen->setEnabled(fIsEditable);

                /* Getting Passthrough state */
                const bool isHostDrive = m_pModelStorage->data(index, StorageModel::R_AttIsHostDrive).toBool();
                mCbPassthrough->setVisible(enmDevice == KDeviceType_DVD && isHostDrive);
                mCbPassthrough->setChecked(isHostDrive && m_pModelStorage->data(index, StorageModel::R_AttIsPassthrough).toBool());

                /* Getting TempEject state */
                mCbTempEject->setVisible(enmDevice == KDeviceType_DVD && !isHostDrive);
                mCbTempEject->setChecked(!isHostDrive && m_pModelStorage->data(index, StorageModel::R_AttIsTempEject).toBool());

                /* Getting NonRotational state */
                mCbNonRotational->setVisible(enmDevice == KDeviceType_HardDisk);
                mCbNonRotational->setChecked(m_pModelStorage->data(index, StorageModel::R_AttIsNonRotational).toBool());

                /* Fetch hot-pluggable state: */
                m_pCheckBoxHotPluggable->setVisible((slt.bus == KStorageBus_SATA) || (slt.bus == KStorageBus_USB));
                m_pCheckBoxHotPluggable->setChecked(fIsHotPluggable);

                /* Update optional widgets visibility */
                updateAdditionalDetails(enmDevice);

                /* Getting Other Information */
                mLbHDFormatValue->setText(compressText(m_pModelStorage->data(index, StorageModel::R_AttFormat).toString()));
                mLbCDFDTypeValue->setText(compressText(m_pModelStorage->data(index, StorageModel::R_AttFormat).toString()));
                mLbHDVirtualSizeValue->setText(compressText(m_pModelStorage->data(index, StorageModel::R_AttLogicalSize).toString()));
                mLbHDActualSizeValue->setText(compressText(m_pModelStorage->data(index, StorageModel::R_AttSize).toString()));
                mLbSizeValue->setText(compressText(m_pModelStorage->data(index, StorageModel::R_AttSize).toString()));
                mLbHDDetailsValue->setText(compressText(m_pModelStorage->data(index, StorageModel::R_AttDetails).toString()));
                mLbLocationValue->setText(compressText(m_pModelStorage->data(index, StorageModel::R_AttLocation).toString()));
                mLbUsageValue->setText(compressText(m_pModelStorage->data(index, StorageModel::R_AttUsage).toString()));
                m_pLabelEncryptionValue->setText(compressText(m_pModelStorage->data(index, StorageModel::R_AttEncryptionPasswordID).toString()));

                /* Showing Attachment Page */
                mSwRightPane->setCurrentIndex(2);
                break;
            }
            default:
                break;
        }
    }

    /* Revalidate: */
    revalidate();

    m_fLoadingInProgress = false;
}

void UIMachineSettingsStorage::sltSetInformation()
{
    const QModelIndex index = m_pTreeStorage->currentIndex();
    if (m_fLoadingInProgress || !index.isValid() || index == m_pModelStorage->root())
        return;

    QObject *pSdr = sender();
    switch (m_pModelStorage->data(index, StorageModel::R_ItemType).value <AbstractItem::ItemType>())
    {
        case AbstractItem::Type_ControllerItem:
        {
            /* Setting Controller Name: */
            if (pSdr == mLeName)
                m_pModelStorage->setData(index, mLeName->text(), StorageModel::R_CtrName);
            /* Setting Controller Bus-Type: */
            else if (pSdr == mCbBus)
                m_pModelStorage->setData(index, QVariant::fromValue(gpConverter->fromString<KStorageBus>(mCbBus->currentText())),
                                         StorageModel::R_CtrBusType);
            /* Setting Controller Sub-Type: */
            else if (pSdr == mCbType)
                m_pModelStorage->setData(index, QVariant::fromValue(gpConverter->fromString<KStorageControllerType>(mCbType->currentText())),
                                        StorageModel::R_CtrType);
            else if (pSdr == mSbPortCount)
                m_pModelStorage->setData(index, mSbPortCount->value(), StorageModel::R_CtrPortCount);
            else if (pSdr == mCbIoCache)
                m_pModelStorage->setData(index, mCbIoCache->isChecked(), StorageModel::R_CtrIoCache);
            break;
        }
        case AbstractItem::Type_AttachmentItem:
        {
            /* Setting Attachment Slot: */
            if (pSdr == mCbSlot)
            {
                QModelIndex controllerIndex = m_pModelStorage->parent(index);
                StorageSlot attachmentStorageSlot = gpConverter->fromString<StorageSlot>(mCbSlot->currentText());
                m_pModelStorage->setData(index, QVariant::fromValue(attachmentStorageSlot), StorageModel::R_AttSlot);
                QModelIndex theSameIndexAtNewPosition = m_pModelStorage->attachmentBySlot(controllerIndex, attachmentStorageSlot);
                AssertMsg(theSameIndexAtNewPosition.isValid(), ("Current attachment disappears!\n"));
                m_pTreeStorage->setCurrentIndex(theSameIndexAtNewPosition);
            }
            /* Setting Attachment Medium: */
            else if (pSdr == m_pMediumIdHolder)
                m_pModelStorage->setData(index, m_pMediumIdHolder->id(), StorageModel::R_AttMediumId);
            else if (pSdr == mCbPassthrough)
            {
                if (m_pModelStorage->data(index, StorageModel::R_AttIsHostDrive).toBool())
                    m_pModelStorage->setData(index, mCbPassthrough->isChecked(), StorageModel::R_AttIsPassthrough);
            }
            else if (pSdr == mCbTempEject)
            {
                if (!m_pModelStorage->data(index, StorageModel::R_AttIsHostDrive).toBool())
                    m_pModelStorage->setData(index, mCbTempEject->isChecked(), StorageModel::R_AttIsTempEject);
            }
            else if (pSdr == mCbNonRotational)
            {
                m_pModelStorage->setData(index, mCbNonRotational->isChecked(), StorageModel::R_AttIsNonRotational);
            }
            else if (pSdr == m_pCheckBoxHotPluggable)
            {
                m_pModelStorage->setData(index, m_pCheckBoxHotPluggable->isChecked(), StorageModel::R_AttIsHotPluggable);
            }
            break;
        }
        default:
            break;
    }

    emit sigStorageChanged();
    sltGetInformation();
}

void UIMachineSettingsStorage::sltPrepareOpenMediumMenu()
{
    /* This slot should be called only by open-medium menu: */
    QMenu *pOpenMediumMenu = qobject_cast<QMenu*>(sender());
    AssertMsg(pOpenMediumMenu, ("Can't access open-medium menu!\n"));
    if (pOpenMediumMenu)
    {
        /* Erase menu initially: */
        pOpenMediumMenu->clear();
        /* Depending on current medium type: */
        switch (m_pMediumIdHolder->type())
        {
            case UIMediumDeviceType_HardDisk:
            {
                /* Add "Create a new virtual hard disk" action: */
                QAction *pCreateNewHardDisk = pOpenMediumMenu->addAction(tr("Create New Hard Disk..."));
                pCreateNewHardDisk->setIcon(iconPool()->icon(HDNewEn, HDNewDis));
                connect(pCreateNewHardDisk, &QAction::triggered, this, &UIMachineSettingsStorage::sltCreateNewHardDisk);
                /* Add "Choose a virtual hard disk" action: */
                addChooseExistingMediumAction(pOpenMediumMenu, tr("Choose Virtual Hard Disk..."));
                /* Add recent media list: */
                addRecentMediumActions(pOpenMediumMenu, m_pMediumIdHolder->type());
                break;
            }
            case UIMediumDeviceType_DVD:
            {
                /* Add "Choose a virtual optical disk" action: */
                addChooseExistingMediumAction(pOpenMediumMenu, tr("Choose Virtual Optical Disk..."));
                /* Add "Choose a physical drive" actions: */
                addChooseHostDriveActions(pOpenMediumMenu);
                /* Add recent media list: */
                addRecentMediumActions(pOpenMediumMenu, m_pMediumIdHolder->type());
                /* Add "Eject current medium" action: */
                pOpenMediumMenu->addSeparator();
                QAction *pEjectCurrentMedium = pOpenMediumMenu->addAction(tr("Remove Disk from Virtual Drive"));
                pEjectCurrentMedium->setEnabled(!m_pMediumIdHolder->isNull());
                pEjectCurrentMedium->setIcon(iconPool()->icon(CDUnmountEnabled, CDUnmountDisabled));
                connect(pEjectCurrentMedium, &QAction::triggered, this, &UIMachineSettingsStorage::sltUnmountDevice);
                break;
            }
            case UIMediumDeviceType_Floppy:
            {
                /* Add "Choose a virtual floppy disk" action: */
                addChooseExistingMediumAction(pOpenMediumMenu, tr("Choose Virtual Floppy Disk..."));
                /* Add "Choose a physical drive" actions: */
                addChooseHostDriveActions(pOpenMediumMenu);
                /* Add recent media list: */
                addRecentMediumActions(pOpenMediumMenu, m_pMediumIdHolder->type());
                /* Add "Eject current medium" action: */
                pOpenMediumMenu->addSeparator();
                QAction *pEjectCurrentMedium = pOpenMediumMenu->addAction(tr("Remove Disk from Virtual Drive"));
                pEjectCurrentMedium->setEnabled(!m_pMediumIdHolder->isNull());
                pEjectCurrentMedium->setIcon(iconPool()->icon(FDUnmountEnabled, FDUnmountDisabled));
                connect(pEjectCurrentMedium, &QAction::triggered, this, &UIMachineSettingsStorage::sltUnmountDevice);
                break;
            }
            default:
                break;
        }
    }
}

void UIMachineSettingsStorage::sltCreateNewHardDisk()
{
    const QUuid uMediumId = uiCommon().openMediumCreatorDialog(this, UIMediumDeviceType_HardDisk, m_strMachineSettingsFilePath,
                                                                 m_strMachineName, m_strMachineGuestOSTypeId);

    if (!uMediumId.isNull())
        m_pMediumIdHolder->setId(uMediumId);
}

void UIMachineSettingsStorage::sltUnmountDevice()
{
    m_pMediumIdHolder->setId(UIMedium().id());
}

void UIMachineSettingsStorage::sltChooseExistingMedium()
{
    const QString strMachineFolder(QFileInfo(m_strMachineSettingsFilePath).absolutePath());

    QUuid uMediumId;
    int iResult = uiCommon().openMediumSelectorDialog(this, m_pMediumIdHolder->type(), uMediumId,
                                                        strMachineFolder, m_strMachineName,
                                                        m_strMachineGuestOSTypeId,
                                                        true /* enable create action: */);

    if (iResult == UIMediumSelector::ReturnCode_Rejected ||
        (iResult == UIMediumSelector::ReturnCode_Accepted && uMediumId.isNull()))
        return;
    if (iResult == static_cast<int>(UIMediumSelector::ReturnCode_LeftEmpty) &&
        (m_pMediumIdHolder->type() != UIMediumDeviceType_DVD && m_pMediumIdHolder->type() != UIMediumDeviceType_Floppy))
        return;

    m_pMediumIdHolder->setId(uMediumId);
}

void UIMachineSettingsStorage::sltChooseHostDrive()
{
    /* This slot should be called ONLY by choose-host-drive action: */
    QAction *pChooseHostDriveAction = qobject_cast<QAction*>(sender());
    AssertMsg(pChooseHostDriveAction, ("Can't access choose-host-drive action!\n"));
    if (pChooseHostDriveAction)
        m_pMediumIdHolder->setId(pChooseHostDriveAction->data().toString());
}

void UIMachineSettingsStorage::sltChooseRecentMedium()
{
    /* This slot should be called ONLY by choose-recent-medium action: */
    QAction *pChooseRecentMediumAction = qobject_cast<QAction*>(sender());
    AssertMsg(pChooseRecentMediumAction, ("Can't access choose-recent-medium action!\n"));
    if (pChooseRecentMediumAction)
    {
        /* Get recent medium type & name: */
        const QStringList mediumInfoList = pChooseRecentMediumAction->data().toString().split(',');
        const UIMediumDeviceType enmMediumType = (UIMediumDeviceType)mediumInfoList[0].toUInt();
        const QString strMediumLocation = mediumInfoList[1];
        const QUuid uMediumId = uiCommon().openMedium(enmMediumType, strMediumLocation, this);
        if (!uMediumId.isNull())
            m_pMediumIdHolder->setId(uMediumId);
    }
}

void UIMachineSettingsStorage::sltUpdateActionStates()
{
    const QModelIndex index = m_pTreeStorage->currentIndex();

    const bool fIDEPossible = m_pModelStorage->data(index, StorageModel::R_IsMoreIDEControllersPossible).toBool();
    const bool fSATAPossible = m_pModelStorage->data(index, StorageModel::R_IsMoreSATAControllersPossible).toBool();
    const bool fSCSIPossible = m_pModelStorage->data(index, StorageModel::R_IsMoreSCSIControllersPossible).toBool();
    const bool fFloppyPossible = m_pModelStorage->data(index, StorageModel::R_IsMoreFloppyControllersPossible).toBool();
    const bool fSASPossible = m_pModelStorage->data(index, StorageModel::R_IsMoreSASControllersPossible).toBool();
    const bool fUSBPossible = m_pModelStorage->data(index, StorageModel::R_IsMoreUSBControllersPossible).toBool();
    const bool fNVMePossible = m_pModelStorage->data(index, StorageModel::R_IsMoreNVMeControllersPossible).toBool();
    const bool fVirtioSCSIPossible = m_pModelStorage->data(index, StorageModel::R_IsMoreVirtioSCSIControllersPossible).toBool();

    const bool fController = m_pModelStorage->data(index, StorageModel::R_IsController).toBool();
    const bool fAttachment = m_pModelStorage->data(index, StorageModel::R_IsAttachment).toBool();
    const bool fAttachmentsPossible = m_pModelStorage->data(index, StorageModel::R_IsMoreAttachmentsPossible).toBool();
    const bool fIsAttachmentHotPluggable = m_pModelStorage->data(index, StorageModel::R_AttIsHotPluggable).toBool();

    /* Configure "add controller" actions: */
    m_pActionAddController->setEnabled(fIDEPossible || fSATAPossible || fSCSIPossible || fFloppyPossible || fSASPossible || fUSBPossible || fNVMePossible || fVirtioSCSIPossible);
    m_pActionAddControllerIDE->setEnabled(fIDEPossible);
    m_pActionAddControllerSATA->setEnabled(fSATAPossible);
    m_pActionAddControllerSCSI->setEnabled(fSCSIPossible);
    m_pActionAddControllerFloppy->setEnabled(fFloppyPossible);
    m_pActionAddControllerSAS->setEnabled(fSASPossible);
    m_pActionAddControllerUSB->setEnabled(fUSBPossible);
    m_pActionAddControllerNVMe->setEnabled(fNVMePossible);
    m_pActionAddControllerVirtioSCSI->setEnabled(fVirtioSCSIPossible);

    /* Configure "add attachment" actions: */
    m_pActionAddAttachment->setEnabled(fController && fAttachmentsPossible);
    m_pActionAddAttachmentHD->setEnabled(fController && fAttachmentsPossible);
    m_pActionAddAttachmentCD->setEnabled(fController && fAttachmentsPossible);
    m_pActionAddAttachmentFD->setEnabled(fController && fAttachmentsPossible);

    /* Configure "delete controller" action: */
    const bool fControllerInSuitableState = isMachineOffline();
    m_pActionRemoveController->setEnabled(fController && fControllerInSuitableState);

    /* Configure "delete attachment" action: */
    const bool fAttachmentInSuitableState = isMachineOffline() ||
                                            (isMachineOnline() && fIsAttachmentHotPluggable);
    m_pActionRemoveAttachment->setEnabled(fAttachment && fAttachmentInSuitableState);
}

void UIMachineSettingsStorage::sltHandleRowInsertion(const QModelIndex &parent, int iPosition)
{
    const QModelIndex index = m_pModelStorage->index(iPosition, 0, parent);

    switch (m_pModelStorage->data(index, StorageModel::R_ItemType).value<AbstractItem::ItemType>())
    {
        case AbstractItem::Type_ControllerItem:
        {
            /* Select the newly created Controller Item: */
            m_pTreeStorage->setCurrentIndex(index);
            break;
        }
        case AbstractItem::Type_AttachmentItem:
        {
            /* Expand parent if it is not expanded yet: */
            if (!m_pTreeStorage->isExpanded(parent))
                m_pTreeStorage->setExpanded(parent, true);
            break;
        }
        default:
            break;
    }

    sltUpdateActionStates();
    sltGetInformation();
}

void UIMachineSettingsStorage::sltHandleRowRemoval()
{
    if (m_pModelStorage->rowCount (m_pModelStorage->root()) == 0)
        m_pTreeStorage->setCurrentIndex (m_pModelStorage->root());

    sltUpdateActionStates();
    sltGetInformation();
}

void UIMachineSettingsStorage::sltHandleCurrentItemChange()
{
    sltUpdateActionStates();
    sltGetInformation();
}

void UIMachineSettingsStorage::sltHandleContextMenuRequest(const QPoint &position)
{
    const QModelIndex index = m_pTreeStorage->indexAt(position);
    if (!index.isValid())
        return sltAddController();

    QMenu menu;
    switch (m_pModelStorage->data(index, StorageModel::R_ItemType).value<AbstractItem::ItemType>())
    {
        case AbstractItem::Type_ControllerItem:
        {
            const DeviceTypeList deviceTypeList(m_pModelStorage->data(index, StorageModel::R_CtrDevices).value<DeviceTypeList>());
            foreach (KDeviceType deviceType, deviceTypeList)
            {
                switch (deviceType)
                {
                    case KDeviceType_HardDisk:
                        menu.addAction(m_pActionAddAttachmentHD);
                        break;
                    case KDeviceType_DVD:
                        menu.addAction(m_pActionAddAttachmentCD);
                        break;
                    case KDeviceType_Floppy:
                        menu.addAction(m_pActionAddAttachmentFD);
                        break;
                    default:
                        break;
                }
            }
            menu.addAction(m_pActionRemoveController);
            break;
        }
        case AbstractItem::Type_AttachmentItem:
        {
            menu.addAction(m_pActionRemoveAttachment);
            break;
        }
        default:
            break;
    }
    if (!menu.isEmpty())
        menu.exec(m_pTreeStorage->viewport()->mapToGlobal(position));
}

void UIMachineSettingsStorage::sltHandleDrawItemBranches(QPainter *pPainter, const QRect &rect, const QModelIndex &index)
{
    if (!index.parent().isValid() || !index.parent().parent().isValid())
        return;

    pPainter->save();
    QStyleOption options;
    options.initFrom(m_pTreeStorage);
    options.rect = rect;
    options.state |= QStyle::State_Item;
    if (index.row() < m_pModelStorage->rowCount(index.parent()) - 1)
        options.state |= QStyle::State_Sibling;
    /* This pen is commonly used by different
     * look and feel styles to paint tree-view branches. */
    const QPen pen(QBrush(options.palette.dark().color(), Qt::Dense4Pattern), 0);
    pPainter->setPen(pen);
    /* If we want tree-view branches to be always painted we have to use QCommonStyle::drawPrimitive()
     * because QCommonStyle performs branch painting as opposed to particular inherited sub-classing styles. */
    qobject_cast<QCommonStyle*>(style())->QCommonStyle::drawPrimitive(QStyle::PE_IndicatorBranch, &options, pPainter);
    pPainter->restore();
}

void UIMachineSettingsStorage::sltHandleMouseMove(QMouseEvent *pEvent)
{
    /* Make sure event is valid: */
    AssertPtrReturnVoid(pEvent);

    const QModelIndex index = m_pTreeStorage->indexAt(pEvent->pos());
    const QRect indexRect = m_pTreeStorage->visualRect(index);

    /* Expander tool-tip: */
    if (m_pModelStorage->data(index, StorageModel::R_IsController).toBool())
    {
        QRect expanderRect = m_pModelStorage->data(index, StorageModel::R_ItemPixmapRect).toRect();
        expanderRect.translate(indexRect.x(), indexRect.y());
        if (expanderRect.contains(pEvent->pos()))
        {
            pEvent->setAccepted(true);
            if (m_pModelStorage->data(index, StorageModel::R_ToolTipType).value<StorageModel::ToolTipType>() != StorageModel::ExpanderToolTip)
                m_pModelStorage->setData(index, QVariant::fromValue(StorageModel::ExpanderToolTip), StorageModel::R_ToolTipType);
            return;
        }
    }

    /* Adder tool-tip: */
    if (m_pModelStorage->data(index, StorageModel::R_IsController).toBool() &&
        m_pTreeStorage->currentIndex() == index)
    {
        const DeviceTypeList devicesList(m_pModelStorage->data(index, StorageModel::R_CtrDevices).value<DeviceTypeList>());
        for (int i = 0; i < devicesList.size(); ++ i)
        {
            const KDeviceType enmDeviceType = devicesList[i];

            QRect deviceRect;
            switch (enmDeviceType)
            {
                case KDeviceType_HardDisk:
                {
                    deviceRect = m_pModelStorage->data(index, StorageModel::R_HDPixmapRect).toRect();
                    break;
                }
                case KDeviceType_DVD:
                {
                    deviceRect = m_pModelStorage->data (index, StorageModel::R_CDPixmapRect).toRect();
                    break;
                }
                case KDeviceType_Floppy:
                {
                    deviceRect = m_pModelStorage->data (index, StorageModel::R_FDPixmapRect).toRect();
                    break;
                }
                default:
                    break;
            }
            deviceRect.translate(indexRect.x() + indexRect.width(), indexRect.y());

            if (deviceRect.contains(pEvent->pos()))
            {
                pEvent->setAccepted(true);
                switch (enmDeviceType)
                {
                    case KDeviceType_HardDisk:
                    {
                        if (m_pModelStorage->data(index, StorageModel::R_ToolTipType).value<StorageModel::ToolTipType>() != StorageModel::HDAdderToolTip)
                            m_pModelStorage->setData(index, QVariant::fromValue(StorageModel::HDAdderToolTip), StorageModel::R_ToolTipType);
                        break;
                    }
                    case KDeviceType_DVD:
                    {
                        if (m_pModelStorage->data(index, StorageModel::R_ToolTipType).value<StorageModel::ToolTipType>() != StorageModel::CDAdderToolTip)
                            m_pModelStorage->setData(index, QVariant::fromValue(StorageModel::CDAdderToolTip), StorageModel::R_ToolTipType);
                        break;
                    }
                    case KDeviceType_Floppy:
                    {
                        if (m_pModelStorage->data(index, StorageModel::R_ToolTipType).value<StorageModel::ToolTipType>() != StorageModel::FDAdderToolTip)
                            m_pModelStorage->setData(index, QVariant::fromValue(StorageModel::FDAdderToolTip), StorageModel::R_ToolTipType);
                        break;
                    }
                    default:
                        break;
                }
                return;
            }
        }
    }

    /* Default tool-tip: */
    if (m_pModelStorage->data(index, StorageModel::R_ToolTipType).value<StorageModel::ToolTipType>() != StorageModel::DefaultToolTip)
        m_pModelStorage->setData(index, StorageModel::DefaultToolTip, StorageModel::R_ToolTipType);

    /* Check whether we should initiate dragging: */
    if (   !m_mousePressPosition.isNull()
        && QLineF(pEvent->screenPos(), m_mousePressPosition).length() >= QApplication::startDragDistance())
    {
        /* Forget last mouse press position: */
        m_mousePressPosition = QPoint();

        /* Check what item we are hovering currently: */
        QModelIndex index = m_pTreeStorage->indexAt(pEvent->pos());
        AbstractItem *pItem = static_cast<AbstractItem*>(index.internalPointer());
        /* And make sure this is attachment item, we are supporting dragging for this kind only: */
        AttachmentItem *pItemAttachment = qobject_cast<AttachmentItem*>(pItem);
        if (pItemAttachment)
        {
            /* Initialize dragging: */
            QDrag *pDrag = new QDrag(this);
            if (pDrag)
            {
                /* Assign pixmap: */
                pDrag->setPixmap(pItem->pixmap());
                /* Prepare mime: */
                QMimeData *pMimeData = new QMimeData;
                if (pMimeData)
                {
                    pMimeData->setData(ControllerMimeType, pItemAttachment->parent()->id().toString().toLatin1());
                    pMimeData->setData(AttachmentMimeType, pItemAttachment->id().toString().toLatin1());
                    pDrag->setMimeData(pMimeData);
                }
                /* Start dragging: */
                pDrag->exec();
            }
        }
    }
}

void UIMachineSettingsStorage::sltHandleMouseClick(QMouseEvent *pEvent)
{
    /* Make sure event is valid: */
    AssertPtrReturnVoid(pEvent);

    /* Remember last mouse press position: */
    m_mousePressPosition = pEvent->globalPos();

    const QModelIndex index = m_pTreeStorage->indexAt(pEvent->pos());
    const QRect indexRect = m_pTreeStorage->visualRect(index);

    /* Expander icon: */
    if (m_pModelStorage->data(index, StorageModel::R_IsController).toBool())
    {
        QRect expanderRect = m_pModelStorage->data(index, StorageModel::R_ItemPixmapRect).toRect();
        expanderRect.translate(indexRect.x(), indexRect.y());
        if (expanderRect.contains(pEvent->pos()))
        {
            pEvent->setAccepted(true);
            m_pTreeStorage->setExpanded(index, !m_pTreeStorage->isExpanded(index));
            return;
        }
    }

    /* Adder icons: */
    if (m_pModelStorage->data(index, StorageModel::R_IsController).toBool() &&
        m_pTreeStorage->currentIndex() == index)
    {
        const DeviceTypeList devicesList(m_pModelStorage->data(index, StorageModel::R_CtrDevices).value <DeviceTypeList>());
        for (int i = 0; i < devicesList.size(); ++ i)
        {
            const KDeviceType enmDeviceType = devicesList[i];

            QRect deviceRect;
            switch (enmDeviceType)
            {
                case KDeviceType_HardDisk:
                {
                    deviceRect = m_pModelStorage->data(index, StorageModel::R_HDPixmapRect).toRect();
                    break;
                }
                case KDeviceType_DVD:
                {
                    deviceRect = m_pModelStorage->data (index, StorageModel::R_CDPixmapRect).toRect();
                    break;
                }
                case KDeviceType_Floppy:
                {
                    deviceRect = m_pModelStorage->data (index, StorageModel::R_FDPixmapRect).toRect();
                    break;
                }
                default:
                    break;
            }
            deviceRect.translate(indexRect.x() + indexRect.width(), indexRect.y());

            if (deviceRect.contains(pEvent->pos()))
            {
                pEvent->setAccepted(true);
                if (m_pActionAddAttachment->isEnabled())
                    addAttachmentWrapper(enmDeviceType);
                return;
            }
        }
    }
}

void UIMachineSettingsStorage::sltHandleMouseRelease(QMouseEvent *)
{
    /* Forget last mouse press position: */
    m_mousePressPosition = QPoint();
}

void UIMachineSettingsStorage::sltHandleDragEnter(QDragEnterEvent *pEvent)
{
    /* Make sure event is valid: */
    AssertPtrReturnVoid(pEvent);

    /* Accept event but not the proposed action: */
    pEvent->accept();
}

void UIMachineSettingsStorage::sltHandleDragMove(QDragMoveEvent *pEvent)
{
    /* Make sure event is valid: */
    AssertPtrReturnVoid(pEvent);
    /* And mime-data is set: */
    const QMimeData *pMimeData = pEvent->mimeData();
    AssertPtrReturnVoid(pMimeData);

    /* Make sure mime-data format is valid: */
    if (   !pMimeData->hasFormat(UIMachineSettingsStorage::ControllerMimeType)
        || !pMimeData->hasFormat(UIMachineSettingsStorage::AttachmentMimeType))
        return;

    /* Get controller id: */
    const QString strControllerId = pMimeData->data(UIMachineSettingsStorage::ControllerMimeType);

    /* Check what item we are hovering currently: */
    QModelIndex index = m_pTreeStorage->indexAt(pEvent->pos());
    AbstractItem *pItem = static_cast<AbstractItem*>(index.internalPointer());
    /* And make sure this is controller item, we are supporting dropping for this kind only: */
    ControllerItem *pItemController = qobject_cast<ControllerItem*>(pItem);
    if (!pItemController || pItemController->id().toString() == strControllerId)
        return;
    /* Also make sure there is enough place for new attachment: */
    const bool fIsMoreAttachmentsPossible = m_pModelStorage->data(index, StorageModel::R_IsMoreAttachmentsPossible).toBool();
    if (!fIsMoreAttachmentsPossible)
        return;

    /* Accept drag-enter event: */
    pEvent->acceptProposedAction();
}

void UIMachineSettingsStorage::sltHandleDragDrop(QDropEvent *pEvent)
{
    /* Make sure event is valid: */
    AssertPtrReturnVoid(pEvent);
    /* And mime-data is set: */
    const QMimeData *pMimeData = pEvent->mimeData();
    AssertPtrReturnVoid(pMimeData);

    /* Check what item we are hovering currently: */
    QModelIndex index = m_pTreeStorage->indexAt(pEvent->pos());
    AbstractItem *pItem = static_cast<AbstractItem*>(index.internalPointer());
    /* And make sure this is controller item, we are supporting dropping for this kind only: */
    ControllerItem *pItemController = qobject_cast<ControllerItem*>(pItem);
    if (pItemController)
    {
        /* Get controller/attachment ids: */
        const QString strControllerId = pMimeData->data(UIMachineSettingsStorage::ControllerMimeType);
        const QString strAttachmentId = pMimeData->data(UIMachineSettingsStorage::AttachmentMimeType);
        m_pModelStorage->moveAttachment(strAttachmentId, strControllerId, pItemController->id());
    }
}

void UIMachineSettingsStorage::prepare()
{
    /* Apply UI decorations: */
    Ui::UIMachineSettingsStorage::setupUi(this);

    /* Prepare cache: */
    m_pCache = new UISettingsCacheMachineStorage;
    AssertPtrReturnVoid(m_pCache);

    /* Create icon-pool: */
    UIIconPoolStorageSettings::create();

    /* Start medium-enumeration (only if necessary): */
    if (!uiCommon().isFullMediumEnumerationRequested())
        uiCommon().enumerateMedia();

    /* Layout created in the .ui file. */
    AssertPtrReturnVoid(mLtStorage);
    {
#ifdef VBOX_WS_MAC
        /* We need a little more space for the focus rect: */
        mLtStorage->setContentsMargins(3, 0, 3, 0);
        mLtStorage->setSpacing(3);
#else
        mLtStorage->setSpacing(qApp->style()->pixelMetric(QStyle::PM_LayoutVerticalSpacing) / 3);
#endif

        /* Prepare storage tree: */
        prepareStorageTree();
        /* Prepare storage toolbar: */
        prepareStorageToolbar();
        /* Prepare storage widgets: */
        prepareStorageWidgets();
        /* Prepare connections: */
        prepareConnections();
    }

    /* Apply language settings: */
    retranslateUi();

    /* Initial setup (after first retranslateUi() call): */
    setMinimumWidth(500);
    mSplitter->setSizes(QList<int>() << (int) (0.45 * minimumWidth()) << (int) (0.55 * minimumWidth()));
}

void UIMachineSettingsStorage::prepareStorageTree()
{
    /* Create storage tree-view: */
    m_pTreeStorage = new QITreeView;
    AssertPtrReturnVoid(m_pTreeStorage);
    AssertPtrReturnVoid(mLsLeftPane);
    {
        /* Configure tree-view: */
        mLsLeftPane->setBuddy(m_pTreeStorage);
        m_pTreeStorage->setMouseTracking(true);
        m_pTreeStorage->setAcceptDrops(true);
        m_pTreeStorage->setContextMenuPolicy(Qt::CustomContextMenu);

        /* Create storage model: */
        m_pModelStorage = new StorageModel(m_pTreeStorage);
        AssertPtrReturnVoid(m_pModelStorage);
        {
            /* Configure model: */
            m_pTreeStorage->setModel(m_pModelStorage);
            m_pTreeStorage->setRootIndex(m_pModelStorage->root());
            m_pTreeStorage->setCurrentIndex(m_pModelStorage->root());
        }

        /* Create storage delegate: */
        StorageDelegate *pStorageDelegate = new StorageDelegate(m_pTreeStorage);
        AssertPtrReturnVoid(pStorageDelegate);
        {
            /* Configure delegate: */
            m_pTreeStorage->setItemDelegate(pStorageDelegate);
        }

        /* Insert tree-view into layout: */
        mLtStorage->insertWidget(0, m_pTreeStorage);
    }
}

void UIMachineSettingsStorage::prepareStorageToolbar()
{
    /* Storage toolbar created in the .ui file. */
    AssertPtrReturnVoid(mTbStorageBar);
    {
        /* Configure toolbar: */
        const int iIconMetric = QApplication::style()->pixelMetric(QStyle::PM_SmallIconSize);
        mTbStorageBar->setIconSize(QSize(iIconMetric, iIconMetric));

        /* Create 'Add Controller' action: */
        m_pActionAddController = new QAction(this);
        AssertPtrReturnVoid(m_pActionAddController);
        {
            /* Configure action: */
            m_pActionAddController->setIcon(iconPool()->icon(ControllerAddEn, ControllerAddDis));

            /* Add action into toolbar: */
            mTbStorageBar->addAction(m_pActionAddController);
        }

        /* Create 'Add IDE Controller' action: */
        m_pActionAddControllerIDE = new QAction(this);
        AssertPtrReturnVoid(m_pActionAddControllerIDE);
        {
            /* Configure action: */
            m_pActionAddControllerIDE->setIcon(iconPool()->icon(IDEControllerAddEn, IDEControllerAddDis));
        }

        /* Create 'Add SATA Controller' action: */
        m_pActionAddControllerSATA = new QAction(this);
        AssertPtrReturnVoid(m_pActionAddControllerSATA);
        {
            /* Configure action: */
            m_pActionAddControllerSATA->setIcon(iconPool()->icon(SATAControllerAddEn, SATAControllerAddDis));
        }

        /* Create 'Add SCSI Controller' action: */
        m_pActionAddControllerSCSI = new QAction(this);
        AssertPtrReturnVoid(m_pActionAddControllerSCSI);
        {
            /* Configure action: */
            m_pActionAddControllerSCSI->setIcon(iconPool()->icon(SCSIControllerAddEn, SCSIControllerAddDis));
        }

        /* Create 'Add Floppy Controller' action: */
        m_pActionAddControllerFloppy = new QAction(this);
        AssertPtrReturnVoid(m_pActionAddControllerFloppy);
        {
            /* Configure action: */
            m_pActionAddControllerFloppy->setIcon(iconPool()->icon(FloppyControllerAddEn, FloppyControllerAddDis));
        }

        /* Create 'Add SAS Controller' action: */
        m_pActionAddControllerSAS = new QAction(this);
        AssertPtrReturnVoid(m_pActionAddControllerSAS);
        {
            /* Configure action: */
            m_pActionAddControllerSAS->setIcon(iconPool()->icon(SASControllerAddEn, SASControllerAddDis));
        }

        /* Create 'Add USB Controller' action: */
        m_pActionAddControllerUSB = new QAction(this);
        AssertPtrReturnVoid(m_pActionAddControllerUSB);
        {
            /* Configure action: */
            m_pActionAddControllerUSB->setIcon(iconPool()->icon(USBControllerAddEn, USBControllerAddDis));
        }

        /* Create 'Add NVMe Controller' action: */
        m_pActionAddControllerNVMe = new QAction(this);
        AssertPtrReturnVoid(m_pActionAddControllerNVMe);
        {
            /* Configure action: */
            m_pActionAddControllerNVMe->setIcon(iconPool()->icon(NVMeControllerAddEn, NVMeControllerAddDis));
        }

        /* Create 'Add virtio-scsi Controller' action: */
        m_pActionAddControllerVirtioSCSI = new QAction(this);
        AssertPtrReturnVoid(m_pActionAddControllerVirtioSCSI);
        {
            /* Configure action: */
            m_pActionAddControllerVirtioSCSI->setIcon(iconPool()->icon(VirtioSCSIControllerAddEn, VirtioSCSIControllerAddDis));
        }

        /* Create 'Remove Controller' action: */
        m_pActionRemoveController = new QAction(this);
        AssertPtrReturnVoid(m_pActionRemoveController);
        {
            /* Configure action: */
            m_pActionRemoveController->setIcon(iconPool()->icon(ControllerDelEn, ControllerDelDis));

            /* Add action into toolbar: */
            mTbStorageBar->addAction(m_pActionRemoveController);
        }

        /* Create 'Add Attachment' action: */
        m_pActionAddAttachment = new QAction(this);
        AssertPtrReturnVoid(m_pActionAddAttachment);
        {
            /* Configure action: */
            m_pActionAddAttachment->setIcon(iconPool()->icon(AttachmentAddEn, AttachmentAddDis));

            /* Add action into toolbar: */
            mTbStorageBar->addAction(m_pActionAddAttachment);
        }

        /* Create 'Add HD Attachment' action: */
        m_pActionAddAttachmentHD = new QAction(this);
        AssertPtrReturnVoid(m_pActionAddAttachmentHD);
        {
            /* Configure action: */
            m_pActionAddAttachmentHD->setIcon(iconPool()->icon(HDAttachmentAddEn, HDAttachmentAddDis));
        }

        /* Create 'Add CD Attachment' action: */
        m_pActionAddAttachmentCD = new QAction(this);
        AssertPtrReturnVoid(m_pActionAddAttachmentCD);
        {
            /* Configure action: */
            m_pActionAddAttachmentCD->setIcon(iconPool()->icon(CDAttachmentAddEn, CDAttachmentAddDis));
        }

        /* Create 'Add FD Attachment' action: */
        m_pActionAddAttachmentFD = new QAction(this);
        AssertPtrReturnVoid(m_pActionAddAttachmentFD);
        {
            /* Configure action: */
            m_pActionAddAttachmentFD->setIcon(iconPool()->icon(FDAttachmentAddEn, FDAttachmentAddDis));
        }

        /* Create 'Remove Attachment' action: */
        m_pActionRemoveAttachment = new QAction(this);
        AssertPtrReturnVoid(m_pActionRemoveAttachment);
        {
            /* Configure action: */
            m_pActionRemoveAttachment->setIcon(iconPool()->icon(AttachmentDelEn, AttachmentDelDis));

            /* Add action into toolbar: */
            mTbStorageBar->addAction(m_pActionRemoveAttachment);
        }
    }
}

void UIMachineSettingsStorage::prepareStorageWidgets()
{
    /* Open Medium tool-button created in the .ui file. */
    AssertPtrReturnVoid(mTbOpen);
    {
        /* Create Open Medium menu: */
        QMenu *pOpenMediumMenu = new QMenu(mTbOpen);
        AssertPtrReturnVoid(pOpenMediumMenu);
        {
            /* Add menu into tool-button: */
            mTbOpen->setMenu(pOpenMediumMenu);
        }
    }

    /* Other widgets created in the .ui file. */
    AssertPtrReturnVoid(mSbPortCount);
    AssertPtrReturnVoid(mLbHDFormatValue);
    AssertPtrReturnVoid(mLbCDFDTypeValue);
    AssertPtrReturnVoid(mLbHDVirtualSizeValue);
    AssertPtrReturnVoid(mLbHDActualSizeValue);
    AssertPtrReturnVoid(mLbSizeValue);
    AssertPtrReturnVoid(mLbHDDetailsValue);
    AssertPtrReturnVoid(mLbLocationValue);
    AssertPtrReturnVoid(mLbUsageValue);
    AssertPtrReturnVoid(m_pLabelEncryptionValue);
    {
        /* Configure widgets: */
        mSbPortCount->setValue(0);
        mLbHDFormatValue->setFullSizeSelection(true);
        mLbCDFDTypeValue->setFullSizeSelection(true);
        mLbHDVirtualSizeValue->setFullSizeSelection(true);
        mLbHDActualSizeValue->setFullSizeSelection(true);
        mLbSizeValue->setFullSizeSelection(true);
        mLbHDDetailsValue->setFullSizeSelection(true);
        mLbLocationValue->setFullSizeSelection(true);
        mLbUsageValue->setFullSizeSelection(true);
        m_pLabelEncryptionValue->setFullSizeSelection(true);
    }
}

void UIMachineSettingsStorage::prepareConnections()
{
    /* Configure this: */
    connect(&uiCommon(), &UICommon::sigMediumEnumerated,
            this, &UIMachineSettingsStorage::sltHandleMediumEnumerated);
    connect(&uiCommon(), &UICommon::sigMediumDeleted,
            this, &UIMachineSettingsStorage::sltHandleMediumDeleted);

    /* Configure tree-view: */
    connect(m_pTreeStorage, &QITreeView::currentItemChanged,
             this, &UIMachineSettingsStorage::sltHandleCurrentItemChange);
    connect(m_pTreeStorage, &QITreeView::customContextMenuRequested,
            this, &UIMachineSettingsStorage::sltHandleContextMenuRequest);
    connect(m_pTreeStorage, &QITreeView::drawItemBranches,
            this, &UIMachineSettingsStorage::sltHandleDrawItemBranches);
    connect(m_pTreeStorage, &QITreeView::mouseMoved,
            this, &UIMachineSettingsStorage::sltHandleMouseMove);
    connect(m_pTreeStorage, &QITreeView::mousePressed,
            this, &UIMachineSettingsStorage::sltHandleMouseClick);
    connect(m_pTreeStorage, &QITreeView::mouseReleased,
            this, &UIMachineSettingsStorage::sltHandleMouseRelease);
    connect(m_pTreeStorage, &QITreeView::mouseDoubleClicked,
            this, &UIMachineSettingsStorage::sltHandleMouseClick);
    connect(m_pTreeStorage, &QITreeView::dragEntered,
            this, &UIMachineSettingsStorage::sltHandleDragEnter);
    connect(m_pTreeStorage, &QITreeView::dragMoved,
            this, &UIMachineSettingsStorage::sltHandleDragMove);
    connect(m_pTreeStorage, &QITreeView::dragDropped,
            this, &UIMachineSettingsStorage::sltHandleDragDrop);

    /* Create model: */
    connect(m_pModelStorage, &StorageModel::rowsInserted,
            this, &UIMachineSettingsStorage::sltHandleRowInsertion);
    connect(m_pModelStorage, &StorageModel::rowsRemoved,
            this, &UIMachineSettingsStorage::sltHandleRowRemoval);

    /* Configure actions: */
    connect(m_pActionAddController, &QAction::triggered, this, &UIMachineSettingsStorage::sltAddController);
    connect(m_pActionAddControllerIDE, &QAction::triggered, this, &UIMachineSettingsStorage::sltAddControllerIDE);
    connect(m_pActionAddControllerSATA, &QAction::triggered, this, &UIMachineSettingsStorage::sltAddControllerSATA);
    connect(m_pActionAddControllerSCSI, &QAction::triggered, this, &UIMachineSettingsStorage::sltAddControllerSCSI);
    connect(m_pActionAddControllerFloppy, &QAction::triggered, this, &UIMachineSettingsStorage::sltAddControllerFloppy);
    connect(m_pActionAddControllerSAS, &QAction::triggered, this, &UIMachineSettingsStorage::sltAddControllerSAS);
    connect(m_pActionAddControllerUSB, &QAction::triggered, this, &UIMachineSettingsStorage::sltAddControllerUSB);
    connect(m_pActionAddControllerNVMe, &QAction::triggered, this, &UIMachineSettingsStorage::sltAddControllerNVMe);
    connect(m_pActionAddControllerVirtioSCSI, &QAction::triggered, this, &UIMachineSettingsStorage::sltAddControllerVirtioSCSI);
    connect(m_pActionRemoveController, &QAction::triggered, this, &UIMachineSettingsStorage::sltRemoveController);
    connect(m_pActionAddAttachment, &QAction::triggered, this, &UIMachineSettingsStorage::sltAddAttachment);
    connect(m_pActionAddAttachmentHD, &QAction::triggered, this, &UIMachineSettingsStorage::sltAddAttachmentHD);
    connect(m_pActionAddAttachmentCD, &QAction::triggered, this, &UIMachineSettingsStorage::sltAddAttachmentCD);
    connect(m_pActionAddAttachmentFD, &QAction::triggered, this, &UIMachineSettingsStorage::sltAddAttachmentFD);
    connect(m_pActionRemoveAttachment, &QAction::triggered, this, &UIMachineSettingsStorage::sltRemoveAttachment);

    /* Configure tool-button: */
    connect(mTbOpen, &QIToolButton::clicked, mTbOpen, &QIToolButton::showMenu);
    /* Configure menu: */
    connect(mTbOpen->menu(), &QMenu::aboutToShow, this, &UIMachineSettingsStorage::sltPrepareOpenMediumMenu);

    /* Configure widgets: */
    connect(m_pMediumIdHolder, &UIMediumIDHolder::sigChanged,
            this, &UIMachineSettingsStorage::sltSetInformation);
    connect(mSbPortCount, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &UIMachineSettingsStorage::sltSetInformation);
    connect(mLeName, &QLineEdit::textEdited,
            this, &UIMachineSettingsStorage::sltSetInformation);
    connect(mCbBus, static_cast<void(QComboBox::*)(int)>(&QComboBox::activated),
            this, &UIMachineSettingsStorage::sltSetInformation);
    connect(mCbType, static_cast<void(QComboBox::*)(int)>(&QComboBox::activated),
            this, &UIMachineSettingsStorage::sltSetInformation);
    connect(mCbSlot, static_cast<void(QComboBox::*)(int)>(&QComboBox::activated),
            this, &UIMachineSettingsStorage::sltSetInformation);
    connect(mCbIoCache, &QCheckBox::stateChanged,
            this, &UIMachineSettingsStorage::sltSetInformation);
    connect(mCbPassthrough, &QCheckBox::stateChanged,
            this, &UIMachineSettingsStorage::sltSetInformation);
    connect(mCbTempEject, &QCheckBox::stateChanged,
            this, &UIMachineSettingsStorage::sltSetInformation);
    connect(mCbNonRotational, &QCheckBox::stateChanged,
            this, &UIMachineSettingsStorage::sltSetInformation);
    connect(m_pCheckBoxHotPluggable, &QCheckBox::stateChanged,
            this, &UIMachineSettingsStorage::sltSetInformation);
}

void UIMachineSettingsStorage::cleanup()
{
    /* Destroy icon-pool: */
    UIIconPoolStorageSettings::destroy();

    /* Cleanup cache: */
    delete m_pCache;
    m_pCache = 0;
}

void UIMachineSettingsStorage::addControllerWrapper(const QString &strName, KStorageBus enmBus, KStorageControllerType enmType)
{
#ifdef RT_STRICT
    const QModelIndex index = m_pTreeStorage->currentIndex();
    switch (enmBus)
    {
        case KStorageBus_IDE:
            Assert(m_pModelStorage->data(index, StorageModel::R_IsMoreIDEControllersPossible).toBool());
            break;
        case KStorageBus_SATA:
            Assert(m_pModelStorage->data(index, StorageModel::R_IsMoreSATAControllersPossible).toBool());
            break;
        case KStorageBus_SCSI:
            Assert(m_pModelStorage->data(index, StorageModel::R_IsMoreSCSIControllersPossible).toBool());
            break;
        case KStorageBus_SAS:
            Assert(m_pModelStorage->data(index, StorageModel::R_IsMoreSASControllersPossible).toBool());
            break;
        case KStorageBus_Floppy:
            Assert(m_pModelStorage->data(index, StorageModel::R_IsMoreFloppyControllersPossible).toBool());
            break;
        case KStorageBus_USB:
            Assert(m_pModelStorage->data(index, StorageModel::R_IsMoreUSBControllersPossible).toBool());
            break;
        case KStorageBus_PCIe:
            Assert(m_pModelStorage->data(index, StorageModel::R_IsMoreNVMeControllersPossible).toBool());
            break;
        case KStorageBus_VirtioSCSI:
            Assert(m_pModelStorage->data(index, StorageModel::R_IsMoreVirtioSCSIControllersPossible).toBool());
            break;
        default:
            break;
    }
#endif

    m_pModelStorage->addController(strName, enmBus, enmType);
    emit sigStorageChanged();
}

void UIMachineSettingsStorage::addAttachmentWrapper(KDeviceType enmDeviceType)
{
    const QModelIndex index = m_pTreeStorage->currentIndex();
    Assert(m_pModelStorage->data(index, StorageModel::R_IsController).toBool());
    Assert(m_pModelStorage->data(index, StorageModel::R_IsMoreAttachmentsPossible).toBool());
    // const QString strControllerName(m_pModelStorage->data(index, StorageModel::R_CtrName).toString());
    const QString strMachineFolder(QFileInfo(m_strMachineSettingsFilePath).absolutePath());

    QUuid uMediumId;
    int iResult = uiCommon().openMediumSelectorDialog(this, UIMediumDefs::mediumTypeToLocal(enmDeviceType), uMediumId,
                                                        strMachineFolder, m_strMachineName,
                                                        m_strMachineGuestOSTypeId,
                                                        true /* enable cr1eate action: */);

    /* Continue only if iResult is either UIMediumSelector::ReturnCode_Accepted or UIMediumSelector::ReturnCode_LeftEmpty: */
    /* If iResult is UIMediumSelector::ReturnCode_Accepted then we have to have a valid uMediumId: */
    if (iResult == UIMediumSelector::ReturnCode_Rejected ||
        (iResult == UIMediumSelector::ReturnCode_Accepted && uMediumId.isNull()))
        return;

    /* Only DVDs and floppy can be created empty: */
    if (iResult == static_cast<int>(UIMediumSelector::ReturnCode_LeftEmpty) &&
        (enmDeviceType != KDeviceType_DVD && enmDeviceType != KDeviceType_Floppy))
        return;

    m_pModelStorage->addAttachment(QUuid(m_pModelStorage->data(index, StorageModel::R_ItemId).toString()), enmDeviceType, uMediumId);
    m_pModelStorage->sort();
    emit sigStorageChanged();

    /* Revalidate: */
    revalidate();
}

void UIMachineSettingsStorage::updateAdditionalDetails(KDeviceType enmType)
{
    mLbHDFormat->setVisible(enmType == KDeviceType_HardDisk);
    mLbHDFormatValue->setVisible(enmType == KDeviceType_HardDisk);

    mLbCDFDType->setVisible(enmType != KDeviceType_HardDisk);
    mLbCDFDTypeValue->setVisible(enmType != KDeviceType_HardDisk);

    mLbHDVirtualSize->setVisible(enmType == KDeviceType_HardDisk);
    mLbHDVirtualSizeValue->setVisible(enmType == KDeviceType_HardDisk);

    mLbHDActualSize->setVisible(enmType == KDeviceType_HardDisk);
    mLbHDActualSizeValue->setVisible(enmType == KDeviceType_HardDisk);

    mLbSize->setVisible(enmType != KDeviceType_HardDisk);
    mLbSizeValue->setVisible(enmType != KDeviceType_HardDisk);

    mLbHDDetails->setVisible(enmType == KDeviceType_HardDisk);
    mLbHDDetailsValue->setVisible(enmType == KDeviceType_HardDisk);

    m_pLabelEncryption->setVisible(enmType == KDeviceType_HardDisk);
    m_pLabelEncryptionValue->setVisible(enmType == KDeviceType_HardDisk);
}

QString UIMachineSettingsStorage::generateUniqueControllerName(const QString &strTemplate) const
{
    int iMaxNumber = 0;
    const QModelIndex rootIndex = m_pModelStorage->root();
    for (int i = 0; i < m_pModelStorage->rowCount(rootIndex); ++i)
    {
        const QModelIndex ctrIndex = rootIndex.child(i, 0);
        const QString ctrName = m_pModelStorage->data(ctrIndex, StorageModel::R_CtrName).toString();
        if (ctrName.startsWith(strTemplate))
        {
            const QString stringNumber(ctrName.right(ctrName.size() - strTemplate.size()));
            bool fConverted = false;
            const int iNumber = stringNumber.toInt(&fConverted);
            iMaxNumber = fConverted && (iNumber > iMaxNumber) ? iNumber : 1;
        }
    }
    return iMaxNumber ? QString("%1 %2").arg(strTemplate).arg(++iMaxNumber) : strTemplate;
}

uint32_t UIMachineSettingsStorage::deviceCount(KDeviceType enmType) const
{
    uint32_t cDevices = 0;
    const QModelIndex rootIndex = m_pModelStorage->root();
    for (int i = 0; i < m_pModelStorage->rowCount(rootIndex); ++i)
    {
        const QModelIndex ctrIndex = rootIndex.child(i, 0);
        for (int j = 0; j < m_pModelStorage->rowCount(ctrIndex); ++j)
        {
            const QModelIndex attIndex = ctrIndex.child(j, 0);
            const KDeviceType enmAttDevice = m_pModelStorage->data(attIndex, StorageModel::R_AttDevice).value<KDeviceType>();
            if (enmAttDevice == enmType)
                ++cDevices;
        }
    }

    return cDevices;
}

void UIMachineSettingsStorage::addChooseExistingMediumAction(QMenu *pOpenMediumMenu, const QString &strActionName)
{
    QAction *pChooseExistingMedium = pOpenMediumMenu->addAction(strActionName);
    pChooseExistingMedium->setIcon(iconPool()->icon(ChooseExistingEn, ChooseExistingDis));
    connect(pChooseExistingMedium, &QAction::triggered, this, &UIMachineSettingsStorage::sltChooseExistingMedium);
}

void UIMachineSettingsStorage::addChooseHostDriveActions(QMenu *pOpenMediumMenu)
{
    foreach (const QUuid &uMediumId, uiCommon().mediumIDs())
    {
        const UIMedium medium = uiCommon().medium(uMediumId);
        if (medium.isHostDrive() && m_pMediumIdHolder->type() == medium.type())
        {
            QAction *pHostDriveAction = pOpenMediumMenu->addAction(medium.name());
            pHostDriveAction->setData(medium.id());
            connect(pHostDriveAction, &QAction::triggered, this, &UIMachineSettingsStorage::sltChooseHostDrive);
        }
    }
}

void UIMachineSettingsStorage::addRecentMediumActions(QMenu *pOpenMediumMenu, UIMediumDeviceType enmRecentMediumType)
{
    /* Get recent-medium list: */
    QStringList recentMediumList;
    switch (enmRecentMediumType)
    {
        case UIMediumDeviceType_HardDisk: recentMediumList = gEDataManager->recentListOfHardDrives(); break;
        case UIMediumDeviceType_DVD:      recentMediumList = gEDataManager->recentListOfOpticalDisks(); break;
        case UIMediumDeviceType_Floppy:   recentMediumList = gEDataManager->recentListOfFloppyDisks(); break;
        default: break;
    }
    /* For every list-item: */
    for (int iIndex = 0; iIndex < recentMediumList.size(); ++iIndex)
    {
        /* Prepare corresponding action: */
        const QString &strRecentMediumLocation = recentMediumList.at(iIndex);
        if (QFile::exists(strRecentMediumLocation))
        {
            QAction *pChooseRecentMediumAction = pOpenMediumMenu->addAction(QFileInfo(strRecentMediumLocation).fileName(),
                                                                            this, SLOT(sltChooseRecentMedium()));
            pChooseRecentMediumAction->setData(QString("%1,%2").arg(enmRecentMediumType).arg(strRecentMediumLocation));
        }
    }
}

bool UIMachineSettingsStorage::saveStorageData()
{
    /* Prepare result: */
    bool fSuccess = true;
    /* Save storage settings from the cache: */
    if (fSuccess && isMachineInValidMode() && m_pCache->wasChanged())
    {
        /* For each controller ('removing' step): */
        // We need to separately remove controllers first because
        // there could be limited amount of controllers available.
        for (int iControllerIndex = 0; fSuccess && iControllerIndex < m_pCache->childCount(); ++iControllerIndex)
        {
            /* Get controller cache: */
            const UISettingsCacheMachineStorageController &controllerCache = m_pCache->child(iControllerIndex);

            /* Remove controller marked for 'remove' or 'update' (if it can't be updated): */
            if (controllerCache.wasRemoved() || (controllerCache.wasUpdated() && !isControllerCouldBeUpdated(controllerCache)))
                fSuccess = removeStorageController(controllerCache);
        }

        /* For each controller ('updating' step): */
        // We need to separately update controllers first because
        // attachments should be removed/updated/created same separate way.
        for (int iControllerIndex = 0; fSuccess && iControllerIndex < m_pCache->childCount(); ++iControllerIndex)
        {
            /* Get controller cache: */
            const UISettingsCacheMachineStorageController &controllerCache = m_pCache->child(iControllerIndex);

            /* Update controller marked for 'update' (if it can be updated): */
            if (controllerCache.wasUpdated() && isControllerCouldBeUpdated(controllerCache))
                fSuccess = updateStorageController(controllerCache, true);
        }
        for (int iControllerIndex = 0; fSuccess && iControllerIndex < m_pCache->childCount(); ++iControllerIndex)
        {
            /* Get controller cache: */
            const UISettingsCacheMachineStorageController &controllerCache = m_pCache->child(iControllerIndex);

            /* Update controller marked for 'update' (if it can be updated): */
            if (controllerCache.wasUpdated() && isControllerCouldBeUpdated(controllerCache))
                fSuccess = updateStorageController(controllerCache, false);
        }

        /* For each controller ('creating' step): */
        // Finally we are creating new controllers,
        // with attachments which were released for sure.
        for (int iControllerIndex = 0; fSuccess && iControllerIndex < m_pCache->childCount(); ++iControllerIndex)
        {
            /* Get controller cache: */
            const UISettingsCacheMachineStorageController &controllerCache = m_pCache->child(iControllerIndex);

            /* Create controller marked for 'create' or 'update' (if it can't be updated): */
            if (controllerCache.wasCreated() || (controllerCache.wasUpdated() && !isControllerCouldBeUpdated(controllerCache)))
                fSuccess = createStorageController(controllerCache);
        }
    }
    /* Return result: */
    return fSuccess;
}

bool UIMachineSettingsStorage::removeStorageController(const UISettingsCacheMachineStorageController &controllerCache)
{
    /* Prepare result: */
    bool fSuccess = true;
    /* Remove controller: */
    if (fSuccess && isMachineOffline())
    {
        /* Get old controller data from the cache: */
        const UIDataSettingsMachineStorageController &oldControllerData = controllerCache.base();

        /* Search for a controller with the same name: */
        const CStorageController &comController = m_machine.GetStorageControllerByName(oldControllerData.m_strControllerName);
        fSuccess = m_machine.isOk() && comController.isNotNull();

        /* Make sure controller really exists: */
        if (fSuccess)
        {
            /* Remove controller with all the attachments at one shot: */
            m_machine.RemoveStorageController(oldControllerData.m_strControllerName);
            fSuccess = m_machine.isOk();
        }

        /* Show error message if necessary: */
        if (!fSuccess)
            notifyOperationProgressError(UIErrorString::formatErrorInfo(m_machine));
    }
    /* Return result: */
    return fSuccess;
}

bool UIMachineSettingsStorage::createStorageController(const UISettingsCacheMachineStorageController &controllerCache)
{
    /* Prepare result: */
    bool fSuccess = true;
    /* Create controller: */
    if (fSuccess && isMachineOffline())
    {
        /* Get new controller data from the cache: */
        const UIDataSettingsMachineStorageController &newControllerData = controllerCache.data();

        /* Search for a controller with the same name: */
        const CMachine comMachine(m_machine);
        CStorageController comController = comMachine.GetStorageControllerByName(newControllerData.m_strControllerName);
        fSuccess = !comMachine.isOk() && comController.isNull();
        AssertReturn(fSuccess, false);

        /* Make sure controller doesn't exist: */
        if (fSuccess)
        {
            /* Create controller: */
            comController = m_machine.AddStorageController(newControllerData.m_strControllerName, newControllerData.m_controllerBus);
            fSuccess = m_machine.isOk() && comController.isNotNull();
        }

        /* Show error message if necessary: */
        if (!fSuccess)
            notifyOperationProgressError(UIErrorString::formatErrorInfo(m_machine));
        else
        {
            /* Save controller type: */
            if (fSuccess)
            {
                comController.SetControllerType(newControllerData.m_controllerType);
                fSuccess = comController.isOk();
            }
            /* Save whether controller uses host IO cache: */
            if (fSuccess)
            {
                comController.SetUseHostIOCache(newControllerData.m_fUseHostIOCache);
                fSuccess = comController.isOk();
            }
            /* Save controller port number: */
            if (   fSuccess
                && (   newControllerData.m_controllerBus == KStorageBus_SATA
                    || newControllerData.m_controllerBus == KStorageBus_SAS
                    || newControllerData.m_controllerBus == KStorageBus_PCIe
                    || newControllerData.m_controllerBus == KStorageBus_VirtioSCSI))
            {
                ULONG uNewPortCount = newControllerData.m_uPortCount;
                if (fSuccess)
                {
                    uNewPortCount = qMax(uNewPortCount, comController.GetMinPortCount());
                    fSuccess = comController.isOk();
                }
                if (fSuccess)
                {
                    uNewPortCount = qMin(uNewPortCount, comController.GetMaxPortCount());
                    fSuccess = comController.isOk();
                }
                if (fSuccess)
                {
                    comController.SetPortCount(uNewPortCount);
                    fSuccess = comController.isOk();
                }
            }

            /* Show error message if necessary: */
            if (!fSuccess)
                notifyOperationProgressError(UIErrorString::formatErrorInfo(comController));

            /* For each attachment: */
            for (int iAttachmentIndex = 0; fSuccess && iAttachmentIndex < controllerCache.childCount(); ++iAttachmentIndex)
            {
                /* Get attachment cache: */
                const UISettingsCacheMachineStorageAttachment &attachmentCache = controllerCache.child(iAttachmentIndex);

                /* Create attachment if it was not 'removed': */
                if (!attachmentCache.wasRemoved())
                    fSuccess = createStorageAttachment(controllerCache, attachmentCache);
            }
        }
    }
    /* Return result: */
    return fSuccess;
}

bool UIMachineSettingsStorage::updateStorageController(const UISettingsCacheMachineStorageController &controllerCache,
                                                       bool fRemovingStep)
{
    /* Prepare result: */
    bool fSuccess = true;
    /* Update controller: */
    if (fSuccess)
    {
        /* Get old controller data from the cache: */
        const UIDataSettingsMachineStorageController &oldControllerData = controllerCache.base();
        /* Get new controller data from the cache: */
        const UIDataSettingsMachineStorageController &newControllerData = controllerCache.data();

        /* Search for a controller with the same name: */
        CStorageController comController = m_machine.GetStorageControllerByName(oldControllerData.m_strControllerName);
        fSuccess = m_machine.isOk() && comController.isNotNull();

        /* Show error message if necessary: */
        if (!fSuccess)
            notifyOperationProgressError(UIErrorString::formatErrorInfo(m_machine));
        else
        {
            /* Save controller type: */
            if (fSuccess && newControllerData.m_controllerType != oldControllerData.m_controllerType)
            {
                comController.SetControllerType(newControllerData.m_controllerType);
                fSuccess = comController.isOk();
            }
            /* Save whether controller uses IO cache: */
            if (fSuccess && newControllerData.m_fUseHostIOCache != oldControllerData.m_fUseHostIOCache)
            {
                comController.SetUseHostIOCache(newControllerData.m_fUseHostIOCache);
                fSuccess = comController.isOk();
            }
            /* Save controller port number: */
            if (   fSuccess
                && newControllerData.m_uPortCount != oldControllerData.m_uPortCount
                && (   newControllerData.m_controllerBus == KStorageBus_SATA
                    || newControllerData.m_controllerBus == KStorageBus_SAS
                    || newControllerData.m_controllerBus == KStorageBus_PCIe
                    || newControllerData.m_controllerBus == KStorageBus_VirtioSCSI))
            {
                ULONG uNewPortCount = newControllerData.m_uPortCount;
                if (fSuccess)
                {
                    uNewPortCount = qMax(uNewPortCount, comController.GetMinPortCount());
                    fSuccess = comController.isOk();
                }
                if (fSuccess)
                {
                    uNewPortCount = qMin(uNewPortCount, comController.GetMaxPortCount());
                    fSuccess = comController.isOk();
                }
                if (fSuccess)
                {
                    comController.SetPortCount(uNewPortCount);
                    fSuccess = comController.isOk();
                }
            }

            /* Show error message if necessary: */
            if (!fSuccess)
                notifyOperationProgressError(UIErrorString::formatErrorInfo(comController));

            // We need to separately remove attachments first because
            // there could be limited amount of attachments or media available.
            if (fRemovingStep)
            {
                /* For each attachment ('removing' step): */
                for (int iAttachmentIndex = 0; fSuccess && iAttachmentIndex < controllerCache.childCount(); ++iAttachmentIndex)
                {
                    /* Get attachment cache: */
                    const UISettingsCacheMachineStorageAttachment &attachmentCache = controllerCache.child(iAttachmentIndex);

                    /* Remove attachment marked for 'remove' or 'update' (if it can't be updated): */
                    if (attachmentCache.wasRemoved() || (attachmentCache.wasUpdated() && !isAttachmentCouldBeUpdated(attachmentCache)))
                        fSuccess = removeStorageAttachment(controllerCache, attachmentCache);
                }
            }
            else
            {
                /* For each attachment ('creating' step): */
                for (int iAttachmentIndex = 0; fSuccess && iAttachmentIndex < controllerCache.childCount(); ++iAttachmentIndex)
                {
                    /* Get attachment cache: */
                    const UISettingsCacheMachineStorageAttachment &attachmentCache = controllerCache.child(iAttachmentIndex);

                    /* Create attachment marked for 'create' or 'update' (if it can't be updated): */
                    if (attachmentCache.wasCreated() || (attachmentCache.wasUpdated() && !isAttachmentCouldBeUpdated(attachmentCache)))
                        fSuccess = createStorageAttachment(controllerCache, attachmentCache);

                    else

                    /* Update attachment marked for 'update' (if it can be updated): */
                    if (attachmentCache.wasUpdated() && isAttachmentCouldBeUpdated(attachmentCache))
                        fSuccess = updateStorageAttachment(controllerCache, attachmentCache);
                }
            }
        }
    }
    /* Return result: */
    return fSuccess;
}

bool UIMachineSettingsStorage::removeStorageAttachment(const UISettingsCacheMachineStorageController &controllerCache,
                                                       const UISettingsCacheMachineStorageAttachment &attachmentCache)
{
    /* Prepare result: */
    bool fSuccess = true;
    /* Remove attachment: */
    if (fSuccess)
    {
        /* Get old controller data from the cache: */
        const UIDataSettingsMachineStorageController &oldControllerData = controllerCache.base();
        /* Get old attachment data from the cache: */
        const UIDataSettingsMachineStorageAttachment &oldAttachmentData = attachmentCache.base();

        /* Search for an attachment with the same parameters: */
        const CMediumAttachment &comAttachment = m_machine.GetMediumAttachment(oldControllerData.m_strControllerName,
                                                                               oldAttachmentData.m_iAttachmentPort,
                                                                               oldAttachmentData.m_iAttachmentDevice);
        fSuccess = m_machine.isOk() && comAttachment.isNotNull();

        /* Make sure attachment really exists: */
        if (fSuccess)
        {
            /* Remove attachment: */
            m_machine.DetachDevice(oldControllerData.m_strControllerName,
                                   oldAttachmentData.m_iAttachmentPort,
                                   oldAttachmentData.m_iAttachmentDevice);
            fSuccess = m_machine.isOk();
        }

        /* Show error message if necessary: */
        if (!fSuccess)
            notifyOperationProgressError(UIErrorString::formatErrorInfo(m_machine));
    }
    /* Return result: */
    return fSuccess;
}

bool UIMachineSettingsStorage::createStorageAttachment(const UISettingsCacheMachineStorageController &controllerCache,
                                                       const UISettingsCacheMachineStorageAttachment &attachmentCache)
{
    /* Prepare result: */
    bool fSuccess = true;
    /* Create attachment: */
    if (fSuccess)
    {
        /* Get new controller data from the cache: */
        const UIDataSettingsMachineStorageController &newControllerData = controllerCache.data();
        /* Get new attachment data from the cache: */
        const UIDataSettingsMachineStorageAttachment &newAttachmentData = attachmentCache.data();

        /* Search for an attachment with the same parameters: */
        const CMachine comMachine(m_machine);
        const CMediumAttachment &comAttachment = comMachine.GetMediumAttachment(newControllerData.m_strControllerName,
                                                                                newAttachmentData.m_iAttachmentPort,
                                                                                newAttachmentData.m_iAttachmentDevice);
        fSuccess = !comMachine.isOk() && comAttachment.isNull();
        AssertReturn(fSuccess, false);

        /* Make sure attachment doesn't exist: */
        if (fSuccess)
        {
            /* Create attachment: */
            const UIMedium vboxMedium = uiCommon().medium(newAttachmentData.m_uAttachmentMediumId);
            const CMedium comMedium = vboxMedium.medium();
            m_machine.AttachDevice(newControllerData.m_strControllerName,
                                   newAttachmentData.m_iAttachmentPort,
                                   newAttachmentData.m_iAttachmentDevice,
                                   newAttachmentData.m_attachmentType,
                                   comMedium);
            fSuccess = m_machine.isOk();
        }

        if (newAttachmentData.m_attachmentType == KDeviceType_DVD)
        {
            /* Save whether this is a passthrough device: */
            if (fSuccess && isMachineOffline())
            {
                m_machine.PassthroughDevice(newControllerData.m_strControllerName,
                                            newAttachmentData.m_iAttachmentPort,
                                            newAttachmentData.m_iAttachmentDevice,
                                            newAttachmentData.m_fAttachmentPassthrough);
                fSuccess = m_machine.isOk();
            }
            /* Save whether this is a live cd device: */
            if (fSuccess)
            {
                m_machine.TemporaryEjectDevice(newControllerData.m_strControllerName,
                                               newAttachmentData.m_iAttachmentPort,
                                               newAttachmentData.m_iAttachmentDevice,
                                               newAttachmentData.m_fAttachmentTempEject);
                fSuccess = m_machine.isOk();
            }
        }
        else if (newAttachmentData.m_attachmentType == KDeviceType_HardDisk)
        {
            /* Save whether this is a ssd device: */
            if (fSuccess && isMachineOffline())
            {
                m_machine.NonRotationalDevice(newControllerData.m_strControllerName,
                                              newAttachmentData.m_iAttachmentPort,
                                              newAttachmentData.m_iAttachmentDevice,
                                              newAttachmentData.m_fAttachmentNonRotational);
                fSuccess = m_machine.isOk();
            }
        }

        if (newControllerData.m_controllerBus == KStorageBus_SATA || newControllerData.m_controllerBus == KStorageBus_USB)
        {
            /* Save whether this device is hot-pluggable: */
            if (fSuccess && isMachineOffline())
            {
                m_machine.SetHotPluggableForDevice(newControllerData.m_strControllerName,
                                                   newAttachmentData.m_iAttachmentPort,
                                                   newAttachmentData.m_iAttachmentDevice,
                                                   newAttachmentData.m_fAttachmentHotPluggable);
                fSuccess = m_machine.isOk();
            }
        }

        /* Show error message if necessary: */
        if (!fSuccess)
            notifyOperationProgressError(UIErrorString::formatErrorInfo(m_machine));
    }
    /* Return result: */
    return fSuccess;
}

bool UIMachineSettingsStorage::updateStorageAttachment(const UISettingsCacheMachineStorageController &controllerCache,
                                                       const UISettingsCacheMachineStorageAttachment &attachmentCache)
{
    /* Prepare result: */
    bool fSuccess = true;
    /* Update attachment: */
    if (fSuccess)
    {
        /* Get new controller data from the cache: */
        const UIDataSettingsMachineStorageController &newControllerData = controllerCache.data();
        /* Get new attachment data from the cache: */
        const UIDataSettingsMachineStorageAttachment &newAttachmentData = attachmentCache.data();

        /* Search for an attachment with the same parameters: */
        const CMediumAttachment &comAttachment = m_machine.GetMediumAttachment(newControllerData.m_strControllerName,
                                                                               newAttachmentData.m_iAttachmentPort,
                                                                               newAttachmentData.m_iAttachmentDevice);
        fSuccess = m_machine.isOk() && comAttachment.isNotNull();

        /* Make sure attachment doesn't exist: */
        if (fSuccess)
        {
            /* Remount attachment: */
            const UIMedium vboxMedium = uiCommon().medium(newAttachmentData.m_uAttachmentMediumId);
            const CMedium comMedium = vboxMedium.medium();
            m_machine.MountMedium(newControllerData.m_strControllerName,
                                  newAttachmentData.m_iAttachmentPort,
                                  newAttachmentData.m_iAttachmentDevice,
                                  comMedium,
                                  true /* force? */);
            fSuccess = m_machine.isOk();
        }

        if (newAttachmentData.m_attachmentType == KDeviceType_DVD)
        {
            /* Save whether this is a passthrough device: */
            if (fSuccess && isMachineOffline())
            {
                m_machine.PassthroughDevice(newControllerData.m_strControllerName,
                                            newAttachmentData.m_iAttachmentPort,
                                            newAttachmentData.m_iAttachmentDevice,
                                            newAttachmentData.m_fAttachmentPassthrough);
                fSuccess = m_machine.isOk();
            }
            /* Save whether this is a live cd device: */
            if (fSuccess)
            {
                m_machine.TemporaryEjectDevice(newControllerData.m_strControllerName,
                                               newAttachmentData.m_iAttachmentPort,
                                               newAttachmentData.m_iAttachmentDevice,
                                               newAttachmentData.m_fAttachmentTempEject);
                fSuccess = m_machine.isOk();
            }
        }
        else if (newAttachmentData.m_attachmentType == KDeviceType_HardDisk)
        {
            /* Save whether this is a ssd device: */
            if (fSuccess && isMachineOffline())
            {
                m_machine.NonRotationalDevice(newControllerData.m_strControllerName,
                                              newAttachmentData.m_iAttachmentPort,
                                              newAttachmentData.m_iAttachmentDevice,
                                              newAttachmentData.m_fAttachmentNonRotational);
                fSuccess = m_machine.isOk();
            }
        }

        if (newControllerData.m_controllerBus == KStorageBus_SATA || newControllerData.m_controllerBus == KStorageBus_USB)
        {
            /* Save whether this device is hot-pluggable: */
            if (fSuccess && isMachineOffline())
            {
                m_machine.SetHotPluggableForDevice(newControllerData.m_strControllerName,
                                                   newAttachmentData.m_iAttachmentPort,
                                                   newAttachmentData.m_iAttachmentDevice,
                                                   newAttachmentData.m_fAttachmentHotPluggable);
                fSuccess = m_machine.isOk();
            }
        }

        /* Show error message if necessary: */
        if (!fSuccess)
            notifyOperationProgressError(UIErrorString::formatErrorInfo(m_machine));
    }
    /* Return result: */
    return fSuccess;
}

bool UIMachineSettingsStorage::isControllerCouldBeUpdated(const UISettingsCacheMachineStorageController &controllerCache) const
{
    /* IController interface doesn't allow to change 'bus' attribute but allows
     * to change 'name' attribute which can conflict with another one controller.
     * Both those attributes could be changed in GUI directly or indirectly.
     * For such cases we have to recreate IController instance,
     * for other cases we will update controller attributes only. */
    const UIDataSettingsMachineStorageController &oldControllerData = controllerCache.base();
    const UIDataSettingsMachineStorageController &newControllerData = controllerCache.data();
    return true
           && (newControllerData.m_strControllerName == oldControllerData.m_strControllerName)
           && (newControllerData.m_controllerBus == oldControllerData.m_controllerBus)
           ;
}

bool UIMachineSettingsStorage::isAttachmentCouldBeUpdated(const UISettingsCacheMachineStorageAttachment &attachmentCache) const
{
    /* IMediumAttachment could be indirectly updated through IMachine
     * only if attachment type, device and port were NOT changed and is one of the next types:
     * KDeviceType_Floppy or KDeviceType_DVD.
     * For other cases we will recreate attachment fully: */
    const UIDataSettingsMachineStorageAttachment &oldAttachmentData = attachmentCache.base();
    const UIDataSettingsMachineStorageAttachment &newAttachmentData = attachmentCache.data();
    return true
           && (newAttachmentData.m_attachmentType == oldAttachmentData.m_attachmentType)
           && (newAttachmentData.m_iAttachmentPort == oldAttachmentData.m_iAttachmentPort)
           && (newAttachmentData.m_iAttachmentDevice == oldAttachmentData.m_iAttachmentDevice)
           && (newAttachmentData.m_attachmentType == KDeviceType_Floppy || newAttachmentData.m_attachmentType == KDeviceType_DVD)
           ;
}

# include "UIMachineSettingsStorage.moc"
