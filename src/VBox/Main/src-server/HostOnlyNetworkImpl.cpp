/* $Id$ */
/** @file
 * IHostOnlyNetwork  COM class implementations.
 */

/*
 * Copyright (C) 2021 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */


#define LOG_GROUP LOG_GROUP_MAIN_HOSTONLYNETWORK
#include <VBox/settings.h>
#include <iprt/cpp/utils.h>

#include "VirtualBoxImpl.h"
#include "HostOnlyNetworkImpl.h"
#include "AutoCaller.h"
#include "LoggingNew.h"


struct HostOnlyNetwork::Data
{
    Data() : pVirtualBox(NULL) {}
    virtual ~Data() {}

    /** weak VirtualBox parent */
    VirtualBox * const pVirtualBox;

    /** HostOnlyNetwork settings */
    settings::HostOnlyNetwork s;
};

////////////////////////////////////////////////////////////////////////////////
//
// HostOnlyNetwork constructor / destructor
//
// ////////////////////////////////////////////////////////////////////////////////
HostOnlyNetwork::HostOnlyNetwork() : m(NULL)
{
}

HostOnlyNetwork::~HostOnlyNetwork()
{
}


HRESULT HostOnlyNetwork::FinalConstruct()
{
    return BaseFinalConstruct();
}

void HostOnlyNetwork::FinalRelease()
{
    uninit();

    BaseFinalRelease();
}

HRESULT HostOnlyNetwork::init(VirtualBox *aVirtualBox, Utf8Str aName)
{
    // Enclose the state transition NotReady->InInit->Ready.
    AutoInitSpan autoInitSpan(this);
    AssertReturn(autoInitSpan.isOk(), E_FAIL);

    m = new Data();
    /* share VirtualBox weakly */
    unconst(m->pVirtualBox) = aVirtualBox;

    m->s.strNetworkName = aName;
    m->s.fEnabled = true;
    m->s.uuid.create();

    autoInitSpan.setSucceeded();
    return S_OK;
}

void HostOnlyNetwork::uninit()
{
    // Enclose the state transition Ready->InUninit->NotReady.
    AutoUninitSpan autoUninitSpan(this);
    if (autoUninitSpan.uninitDone())
        return;
}

HRESULT HostOnlyNetwork::i_loadSettings(const settings::HostOnlyNetwork &data)
{
    AutoCaller autoCaller(this);
    AssertComRCReturnRC(autoCaller.rc());

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);
    m->s = data;

    return S_OK;
}

HRESULT HostOnlyNetwork::i_saveSettings(settings::HostOnlyNetwork &data)
{
    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();

    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);
    AssertReturn(!m->s.strNetworkName.isEmpty(), E_FAIL);
    data = m->s;

    return S_OK;
}

#if 0
Utf8Str HostOnlyNetwork::i_getNetworkId()
{
    return m->s.strNetworkId;
}

Utf8Str HostOnlyNetwork::i_getNetworkName()
{
    return m->s.strNetworkName;
}
#endif


HRESULT HostOnlyNetwork::getNetworkName(com::Utf8Str &aNetworkName)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);
    AssertReturn(!m->s.strNetworkName.isEmpty(), E_FAIL);
    aNetworkName = m->s.strNetworkName;
    return S_OK;
}

HRESULT HostOnlyNetwork::setNetworkName(const com::Utf8Str &aNetworkName)
{
    if (aNetworkName.isEmpty())
        return setError(E_INVALIDARG,
                        tr("Network name cannot be empty"));
    {
        AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);
        if (aNetworkName == m->s.strNetworkName)
            return S_OK;

        m->s.strNetworkName = aNetworkName;
    }

    AutoWriteLock vboxLock(m->pVirtualBox COMMA_LOCKVAL_SRC_POS);
    HRESULT rc = m->pVirtualBox->i_saveSettings();
    ComAssertComRCRetRC(rc);
    return S_OK;
}

HRESULT HostOnlyNetwork::getNetworkMask(com::Utf8Str &aNetworkMask)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);
    AssertReturn(!m->s.strNetworkMask.isEmpty(), E_FAIL);
    aNetworkMask = m->s.strNetworkMask;
    return S_OK;
}

HRESULT HostOnlyNetwork::setNetworkMask(const com::Utf8Str &aNetworkMask)
{
    if (aNetworkMask.isEmpty())
        return setError(E_INVALIDARG,
                        tr("Network mask cannot be empty"));
    {
        AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);
        if (aNetworkMask == m->s.strNetworkMask)
            return S_OK;

        m->s.strNetworkMask = aNetworkMask;
    }

    AutoWriteLock vboxLock(m->pVirtualBox COMMA_LOCKVAL_SRC_POS);
    HRESULT rc = m->pVirtualBox->i_saveSettings();
    ComAssertComRCRetRC(rc);
    return S_OK;
}

HRESULT HostOnlyNetwork::getEnabled(BOOL *aEnabled)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);
    *aEnabled = m->s.fEnabled;
    return S_OK;
}

HRESULT HostOnlyNetwork::setEnabled(BOOL aEnabled)
{
    {
        AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);
        if (RT_BOOL(aEnabled) == m->s.fEnabled)
            return S_OK;
        m->s.fEnabled = RT_BOOL(aEnabled);
    }

    AutoWriteLock vboxLock(m->pVirtualBox COMMA_LOCKVAL_SRC_POS);
    HRESULT rc = m->pVirtualBox->i_saveSettings();
    ComAssertComRCRetRC(rc);
    return S_OK;
}

HRESULT HostOnlyNetwork::getHostIP(com::Utf8Str &aHostIP)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);
    aHostIP = m->s.strIPLower;
    return S_OK;
}

HRESULT HostOnlyNetwork::getLowerIP(com::Utf8Str &aLowerIP)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);
    aLowerIP = m->s.strIPLower;
    return S_OK;
}

HRESULT HostOnlyNetwork::setLowerIP(const com::Utf8Str &aLowerIP)
{
    /// @TODO: Validation?
    {
        AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);
        if (aLowerIP == m->s.strIPLower)
            return S_OK;
        m->s.strIPLower = aLowerIP;
    }

    AutoWriteLock vboxLock(m->pVirtualBox COMMA_LOCKVAL_SRC_POS);
    HRESULT rc = m->pVirtualBox->i_saveSettings();
    ComAssertComRCRetRC(rc);
    return S_OK;
}

HRESULT HostOnlyNetwork::getUpperIP(com::Utf8Str &aUpperIP)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);
    aUpperIP = m->s.strIPUpper;
    return S_OK;
}

HRESULT HostOnlyNetwork::setUpperIP(const com::Utf8Str &aUpperIP)
{
    /// @TODO: Validation?
    {
        AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);
        if (aUpperIP == m->s.strIPUpper)
            return S_OK;
        m->s.strIPUpper = aUpperIP;
    }

    AutoWriteLock vboxLock(m->pVirtualBox COMMA_LOCKVAL_SRC_POS);
    HRESULT rc = m->pVirtualBox->i_saveSettings();
    ComAssertComRCRetRC(rc);
    return S_OK;
}

HRESULT HostOnlyNetwork::getId(com::Guid &aId)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);
    aId = m->s.uuid;
    return S_OK;
}

HRESULT HostOnlyNetwork::setId(const com::Guid &aId)
{
    {
        AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);
        if (aId == m->s.uuid)
            return S_OK;
        m->s.uuid = aId;
    }

    AutoWriteLock vboxLock(m->pVirtualBox COMMA_LOCKVAL_SRC_POS);
    HRESULT rc = m->pVirtualBox->i_saveSettings();
    ComAssertComRCRetRC(rc);
    return S_OK;
}

