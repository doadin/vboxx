/* $Id$ */
/** @file
 * VBox Console COM Class implementation - Guest drag'n drop target.
 */

/*
 * Copyright (C) 2014-2020 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP LOG_GROUP_GUEST_DND //LOG_GROUP_MAIN_GUESTDNDTARGET
#include "LoggingNew.h"

#include "GuestImpl.h"
#include "GuestDnDTargetImpl.h"
#include "ConsoleImpl.h"

#include "Global.h"
#include "AutoCaller.h"
#include "ThreadTask.h"

#include <algorithm>        /* For std::find(). */

#include <iprt/asm.h>
#include <iprt/file.h>
#include <iprt/dir.h>
#include <iprt/path.h>
#include <iprt/uri.h>
#include <iprt/cpp/utils.h> /* For unconst(). */

#include <VBox/com/array.h>

#include <VBox/GuestHost/DragAndDrop.h>
#include <VBox/HostServices/Service.h>


/**
 * Base class for a target task.
 */
class GuestDnDTargetTask : public ThreadTask
{
public:

    GuestDnDTargetTask(GuestDnDTarget *pTarget)
        : ThreadTask("GenericGuestDnDTargetTask")
        , mTarget(pTarget)
        , mRC(VINF_SUCCESS) { }

    virtual ~GuestDnDTargetTask(void) { }

    int getRC(void) const { return mRC; }
    bool isOk(void) const { return RT_SUCCESS(mRC); }
    const ComObjPtr<GuestDnDTarget> &getTarget(void) const { return mTarget; }

protected:

    const ComObjPtr<GuestDnDTarget>     mTarget;
    int                                 mRC;
};

/**
 * Task structure for sending data to a target using
 * a worker thread.
 */
class GuestDnDSendDataTask : public GuestDnDTargetTask
{
public:

    GuestDnDSendDataTask(GuestDnDTarget *pTarget, GuestDnDSendCtx *pCtx)
        : GuestDnDTargetTask(pTarget),
          mpCtx(pCtx)
    {
        m_strTaskName = "dndTgtSndData";
    }

    void handler()
    {
        GuestDnDTarget::i_sendDataThreadTask(this);
    }

    virtual ~GuestDnDSendDataTask(void)
    {
        if (mpCtx)
        {
            delete mpCtx;
            mpCtx = NULL;
        }
    }


    GuestDnDSendCtx *getCtx(void) { return mpCtx; }

protected:

    /** Pointer to send data context. */
    GuestDnDSendCtx *mpCtx;
};

// constructor / destructor
/////////////////////////////////////////////////////////////////////////////

DEFINE_EMPTY_CTOR_DTOR(GuestDnDTarget)

HRESULT GuestDnDTarget::FinalConstruct(void)
{
    /* Set the maximum block size our guests can handle to 64K. This always has
     * been hardcoded until now. */
    /* Note: Never ever rely on information from the guest; the host dictates what and
     *       how to do something, so try to negogiate a sensible value here later. */
    mData.mcbBlockSize = _64K; /** @todo Make this configurable. */

    LogFlowThisFunc(("\n"));
    return BaseFinalConstruct();
}

void GuestDnDTarget::FinalRelease(void)
{
    LogFlowThisFuncEnter();
    uninit();
    BaseFinalRelease();
    LogFlowThisFuncLeave();
}

// public initializer/uninitializer for internal purposes only
/////////////////////////////////////////////////////////////////////////////

int GuestDnDTarget::init(const ComObjPtr<Guest>& pGuest)
{
    LogFlowThisFuncEnter();

    /* Enclose the state transition NotReady->InInit->Ready. */
    AutoInitSpan autoInitSpan(this);
    AssertReturn(autoInitSpan.isOk(), E_FAIL);

    unconst(m_pGuest) = pGuest;

    /* Confirm a successful initialization when it's the case. */
    autoInitSpan.setSucceeded();

    return VINF_SUCCESS;
}

/**
 * Uninitializes the instance.
 * Called from FinalRelease().
 */
void GuestDnDTarget::uninit(void)
{
    LogFlowThisFunc(("\n"));

    /* Enclose the state transition Ready->InUninit->NotReady. */
    AutoUninitSpan autoUninitSpan(this);
    if (autoUninitSpan.uninitDone())
        return;
}

// implementation of wrapped IDnDBase methods.
/////////////////////////////////////////////////////////////////////////////

HRESULT GuestDnDTarget::isFormatSupported(const com::Utf8Str &aFormat, BOOL *aSupported)
{
#if !defined(VBOX_WITH_DRAG_AND_DROP)
    ReturnComNotImplemented();
#else /* VBOX_WITH_DRAG_AND_DROP */

    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();

    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    return GuestDnDBase::i_isFormatSupported(aFormat, aSupported);
#endif /* VBOX_WITH_DRAG_AND_DROP */
}

HRESULT GuestDnDTarget::getFormats(GuestDnDMIMEList &aFormats)
{
#if !defined(VBOX_WITH_DRAG_AND_DROP)
    ReturnComNotImplemented();
#else /* VBOX_WITH_DRAG_AND_DROP */

    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();

    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    return GuestDnDBase::i_getFormats(aFormats);
#endif /* VBOX_WITH_DRAG_AND_DROP */
}

HRESULT GuestDnDTarget::addFormats(const GuestDnDMIMEList &aFormats)
{
#if !defined(VBOX_WITH_DRAG_AND_DROP)
    ReturnComNotImplemented();
#else /* VBOX_WITH_DRAG_AND_DROP */

    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    return GuestDnDBase::i_addFormats(aFormats);
#endif /* VBOX_WITH_DRAG_AND_DROP */
}

HRESULT GuestDnDTarget::removeFormats(const GuestDnDMIMEList &aFormats)
{
#if !defined(VBOX_WITH_DRAG_AND_DROP)
    ReturnComNotImplemented();
#else /* VBOX_WITH_DRAG_AND_DROP */

    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    return GuestDnDBase::i_removeFormats(aFormats);
#endif /* VBOX_WITH_DRAG_AND_DROP */
}

HRESULT GuestDnDTarget::getProtocolVersion(ULONG *aProtocolVersion)
{
#if !defined(VBOX_WITH_DRAG_AND_DROP)
    ReturnComNotImplemented();
#else /* VBOX_WITH_DRAG_AND_DROP */

    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();

    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    return GuestDnDBase::i_getProtocolVersion(aProtocolVersion);
#endif /* VBOX_WITH_DRAG_AND_DROP */
}

// implementation of wrapped IDnDTarget methods.
/////////////////////////////////////////////////////////////////////////////

HRESULT GuestDnDTarget::enter(ULONG aScreenId, ULONG aX, ULONG aY,
                              DnDAction_T                      aDefaultAction,
                              const std::vector<DnDAction_T>  &aAllowedActions,
                              const GuestDnDMIMEList          &aFormats,
                              DnDAction_T                     *aResultAction)
{
#if !defined(VBOX_WITH_DRAG_AND_DROP)
    ReturnComNotImplemented();
#else /* VBOX_WITH_DRAG_AND_DROP */

    /* Input validation. */
    if (aDefaultAction == DnDAction_Ignore)
        return setError(E_INVALIDARG, tr("No default action specified"));
    if (!aAllowedActions.size())
        return setError(E_INVALIDARG, tr("Number of allowed actions is empty"));
    if (!aFormats.size())
        return setError(E_INVALIDARG, tr("Number of supported formats is empty"));

    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();

    /* Determine guest DnD protocol to use. */
    GuestDnDBase::getProtocolVersion(&mDataBase.m_uProtocolVersion);

    /* Default action is ignoring. */
    DnDAction_T resAction = DnDAction_Ignore;

    /* Check & convert the drag & drop actions. */
    VBOXDNDACTION     dndActionDefault     = 0;
    VBOXDNDACTIONLIST dndActionListAllowed = 0;
    GuestDnD::toHGCMActions(aDefaultAction, &dndActionDefault,
                            aAllowedActions, &dndActionListAllowed);

    /* If there is no usable action, ignore this request. */
    if (isDnDIgnoreAction(dndActionDefault))
        return S_OK;

    /*
     * Make a flat data string out of the supported format list.
     * In the GuestDnDTarget case the source formats are from the host,
     * as GuestDnDTarget acts as a source for the guest.
     */
    Utf8Str strFormats       = GuestDnD::toFormatString(GuestDnD::toFilteredFormatList(m_lstFmtSupported, aFormats));
    if (strFormats.isEmpty())
        return setError(E_INVALIDARG, tr("No or not supported format(s) specified"));
    const uint32_t cbFormats = (uint32_t)strFormats.length() + 1; /* Include terminating zero. */

    LogRel2(("DnD: Offered formats to guest:\n"));
    RTCList<RTCString> lstFormats = strFormats.split("\r\n");
    for (size_t i = 0; i < lstFormats.size(); i++)
        LogRel2(("DnD: \t%s\n", lstFormats[i].c_str()));

    /* Save the formats offered to the guest. This is needed to later
     * decide what to do with the data when sending stuff to the guest. */
    m_lstFmtOffered = aFormats;
    Assert(m_lstFmtOffered.size());

    HRESULT hr = S_OK;

    /* Adjust the coordinates in a multi-monitor setup. */
    int rc = GUESTDNDINST()->adjustScreenCoordinates(aScreenId, &aX, &aY);
    if (RT_SUCCESS(rc))
    {
        GuestDnDMsg Msg;
        Msg.setType(HOST_DND_HG_EVT_ENTER);
        if (mDataBase.m_uProtocolVersion >= 3)
            Msg.setNextUInt32(0); /** @todo ContextID not used yet. */
        Msg.setNextUInt32(aScreenId);
        Msg.setNextUInt32(aX);
        Msg.setNextUInt32(aY);
        Msg.setNextUInt32(dndActionDefault);
        Msg.setNextUInt32(dndActionListAllowed);
        Msg.setNextPointer((void *)strFormats.c_str(), cbFormats);
        Msg.setNextUInt32(cbFormats);

        rc = GUESTDNDINST()->hostCall(Msg.getType(), Msg.getCount(), Msg.getParms());
        if (RT_SUCCESS(rc))
        {
            GuestDnDResponse *pResp = GUESTDNDINST()->response();
            if (pResp && RT_SUCCESS(pResp->waitForGuestResponse()))
                resAction = GuestDnD::toMainAction(pResp->getActionDefault());
        }
    }

    if (RT_FAILURE(rc))
        hr = VBOX_E_IPRT_ERROR;

    if (SUCCEEDED(hr))
    {
        if (aResultAction)
            *aResultAction = resAction;
    }

    LogFlowFunc(("hr=%Rhrc, resAction=%ld\n", hr, resAction));
    return hr;
#endif /* VBOX_WITH_DRAG_AND_DROP */
}

HRESULT GuestDnDTarget::move(ULONG aScreenId, ULONG aX, ULONG aY,
                             DnDAction_T                      aDefaultAction,
                             const std::vector<DnDAction_T>  &aAllowedActions,
                             const GuestDnDMIMEList          &aFormats,
                             DnDAction_T                     *aResultAction)
{
#if !defined(VBOX_WITH_DRAG_AND_DROP)
    ReturnComNotImplemented();
#else /* VBOX_WITH_DRAG_AND_DROP */

    /* Input validation. */

    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();

    /* Default action is ignoring. */
    DnDAction_T resAction = DnDAction_Ignore;

    /* Check & convert the drag & drop actions. */
    VBOXDNDACTION     dndActionDefault     = 0;
    VBOXDNDACTIONLIST dndActionListAllowed = 0;
    GuestDnD::toHGCMActions(aDefaultAction, &dndActionDefault,
                            aAllowedActions, &dndActionListAllowed);

    /* If there is no usable action, ignore this request. */
    if (isDnDIgnoreAction(dndActionDefault))
        return S_OK;

    /*
     * Make a flat data string out of the supported format list.
     * In the GuestDnDTarget case the source formats are from the host,
     * as GuestDnDTarget acts as a source for the guest.
     */
    Utf8Str strFormats       = GuestDnD::toFormatString(GuestDnD::toFilteredFormatList(m_lstFmtSupported, aFormats));
    if (strFormats.isEmpty())
        return setError(E_INVALIDARG, tr("No or not supported format(s) specified"));
    const uint32_t cbFormats = (uint32_t)strFormats.length() + 1; /* Include terminating zero. */

    HRESULT hr = S_OK;

    int rc = GUESTDNDINST()->adjustScreenCoordinates(aScreenId, &aX, &aY);
    if (RT_SUCCESS(rc))
    {
        GuestDnDMsg Msg;
        Msg.setType(HOST_DND_HG_EVT_MOVE);
        if (mDataBase.m_uProtocolVersion >= 3)
            Msg.setNextUInt32(0); /** @todo ContextID not used yet. */
        Msg.setNextUInt32(aScreenId);
        Msg.setNextUInt32(aX);
        Msg.setNextUInt32(aY);
        Msg.setNextUInt32(dndActionDefault);
        Msg.setNextUInt32(dndActionListAllowed);
        Msg.setNextPointer((void *)strFormats.c_str(), cbFormats);
        Msg.setNextUInt32(cbFormats);

        rc = GUESTDNDINST()->hostCall(Msg.getType(), Msg.getCount(), Msg.getParms());
        if (RT_SUCCESS(rc))
        {
            GuestDnDResponse *pResp = GUESTDNDINST()->response();
            if (pResp && RT_SUCCESS(pResp->waitForGuestResponse()))
                resAction = GuestDnD::toMainAction(pResp->getActionDefault());
        }
    }

    if (RT_FAILURE(rc))
        hr = VBOX_E_IPRT_ERROR;

    if (SUCCEEDED(hr))
    {
        if (aResultAction)
            *aResultAction = resAction;
    }

    LogFlowFunc(("hr=%Rhrc, *pResultAction=%ld\n", hr, resAction));
    return hr;
#endif /* VBOX_WITH_DRAG_AND_DROP */
}

HRESULT GuestDnDTarget::leave(ULONG uScreenId)
{
    RT_NOREF(uScreenId);
#if !defined(VBOX_WITH_DRAG_AND_DROP)
    ReturnComNotImplemented();
#else /* VBOX_WITH_DRAG_AND_DROP */

    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();

    HRESULT hr = S_OK;

    GuestDnDMsg Msg;
    Msg.setType(HOST_DND_HG_EVT_LEAVE);
    if (mDataBase.m_uProtocolVersion >= 3)
        Msg.setNextUInt32(0); /** @todo ContextID not used yet. */

    int rc = GUESTDNDINST()->hostCall(Msg.getType(), Msg.getCount(), Msg.getParms());
    if (RT_SUCCESS(rc))
    {
        GuestDnDResponse *pResp = GUESTDNDINST()->response();
        if (pResp)
            pResp->waitForGuestResponse();
    }

    if (RT_FAILURE(rc))
        hr = VBOX_E_IPRT_ERROR;

    LogFlowFunc(("hr=%Rhrc\n", hr));
    return hr;
#endif /* VBOX_WITH_DRAG_AND_DROP */
}

HRESULT GuestDnDTarget::drop(ULONG aScreenId, ULONG aX, ULONG aY,
                             DnDAction_T                      aDefaultAction,
                             const std::vector<DnDAction_T>  &aAllowedActions,
                             const GuestDnDMIMEList          &aFormats,
                             com::Utf8Str                    &aFormat,
                             DnDAction_T                     *aResultAction)
{
#if !defined(VBOX_WITH_DRAG_AND_DROP)
    ReturnComNotImplemented();
#else /* VBOX_WITH_DRAG_AND_DROP */

    if (aDefaultAction == DnDAction_Ignore)
        return setError(E_INVALIDARG, tr("Invalid default action specified"));
    if (!aAllowedActions.size())
        return setError(E_INVALIDARG, tr("Invalid allowed actions specified"));
    if (!aFormats.size())
        return setError(E_INVALIDARG, tr("No drop format(s) specified"));
    /* aResultAction is optional. */

    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();

    /* Default action is ignoring. */
    DnDAction_T resAction    = DnDAction_Ignore;

    /* Check & convert the drag & drop actions to HGCM codes. */
    VBOXDNDACTION     dndActionDefault     = VBOX_DND_ACTION_IGNORE;
    VBOXDNDACTIONLIST dndActionListAllowed = 0;
    GuestDnD::toHGCMActions(aDefaultAction,  &dndActionDefault,
                            aAllowedActions, &dndActionListAllowed);

    /* If there is no usable action, ignore this request. */
    if (isDnDIgnoreAction(dndActionDefault))
    {
        aFormat = "";
        if (aResultAction)
            *aResultAction = DnDAction_Ignore;
        return S_OK;
    }

    /*
     * Make a flat data string out of the supported format list.
     * In the GuestDnDTarget case the source formats are from the host,
     * as GuestDnDTarget acts as a source for the guest.
     */
    Utf8Str strFormats       = GuestDnD::toFormatString(GuestDnD::toFilteredFormatList(m_lstFmtSupported, aFormats));
    if (strFormats.isEmpty())
        return setError(E_INVALIDARG, tr("No or not supported format(s) specified"));
    const uint32_t cbFormats = (uint32_t)strFormats.length() + 1; /* Include terminating zero. */

    /* Adjust the coordinates in a multi-monitor setup. */
    HRESULT hr = GUESTDNDINST()->adjustScreenCoordinates(aScreenId, &aX, &aY);
    if (SUCCEEDED(hr))
    {
        GuestDnDMsg Msg;
        Msg.setType(HOST_DND_HG_EVT_DROPPED);
        if (mDataBase.m_uProtocolVersion >= 3)
            Msg.setNextUInt32(0); /** @todo ContextID not used yet. */
        Msg.setNextUInt32(aScreenId);
        Msg.setNextUInt32(aX);
        Msg.setNextUInt32(aY);
        Msg.setNextUInt32(dndActionDefault);
        Msg.setNextUInt32(dndActionListAllowed);
        Msg.setNextPointer((void*)strFormats.c_str(), cbFormats);
        Msg.setNextUInt32(cbFormats);

        int vrc = GUESTDNDINST()->hostCall(Msg.getType(), Msg.getCount(), Msg.getParms());
        if (RT_SUCCESS(vrc))
        {
            GuestDnDResponse *pResp = GUESTDNDINST()->response();
            AssertPtr(pResp);

            vrc = pResp->waitForGuestResponse();
            if (RT_SUCCESS(vrc))
            {
                resAction = GuestDnD::toMainAction(pResp->getActionDefault());

                GuestDnDMIMEList lstFormats = pResp->formats();
                if (lstFormats.size() == 1) /* Exactly one format to use specified? */
                {
                    aFormat = lstFormats.at(0);
                    LogFlowFunc(("resFormat=%s, resAction=%RU32\n", aFormat.c_str(), pResp->getActionDefault()));
                }
                else
                    /** @todo r=bird: This isn't an IPRT error, is it?   */
                    hr = setError(VBOX_E_IPRT_ERROR, tr("Guest returned invalid drop formats (%zu formats)"), lstFormats.size());
            }
            else
                hr = setErrorBoth(VBOX_E_IPRT_ERROR, vrc, tr("Waiting for response of dropped event failed (%Rrc)"), vrc);
        }
        else
            hr = setErrorBoth(VBOX_E_IPRT_ERROR, vrc, tr("Sending dropped event to guest failed (%Rrc)"), vrc);
    }
    else
        hr = setError(hr, tr("Retrieving drop coordinates failed"));

    if (SUCCEEDED(hr))
    {
        if (aResultAction)
            *aResultAction = resAction;
    }

    LogFlowFunc(("Returning hr=%Rhrc\n", hr));
    return hr;
#endif /* VBOX_WITH_DRAG_AND_DROP */
}

/**
 * Thread handler function for sending data to the guest.
 *
 * @param   pTask               Thread task this handler is associated with.
 */
/* static */
void GuestDnDTarget::i_sendDataThreadTask(GuestDnDSendDataTask *pTask)
{
    LogFlowFunc(("pTask=%p\n", pTask));
    AssertPtrReturnVoid(pTask);

    const ComObjPtr<GuestDnDTarget> pThis(pTask->getTarget());
    Assert(!pThis.isNull());

    AutoCaller autoCaller(pThis);
    if (FAILED(autoCaller.rc()))
        return;

    int vrc = pThis->i_sendData(pTask->getCtx(), RT_INDEFINITE_WAIT /* msTimeout */);
    if (RT_FAILURE(vrc)) /* In case we missed some error handling within i_sendData(). */
    {
        AssertFailed();
        LogRel(("DnD: Sending data to guest failed with %Rrc\n", vrc));
    }

    AutoWriteLock alock(pThis COMMA_LOCKVAL_SRC_POS);

    Assert(pThis->mDataBase.m_cTransfersPending);
    if (pThis->mDataBase.m_cTransfersPending)
        pThis->mDataBase.m_cTransfersPending--;

    LogFlowFunc(("pTarget=%p, vrc=%Rrc (ignored)\n", (GuestDnDTarget *)pThis, vrc));
}

/**
 * Initiates a data transfer from the host to the guest.
 *
 * The source is the host, whereas the target is the guest.
 *
 * @return  HRESULT
 * @param   aScreenId           Screen ID where this data transfer was initiated from.
 * @param   aFormat             Format of data to send. MIME-style.
 * @param   aData               Actual data to send.
 * @param   aProgress           Where to return the progress object on success.
 */
HRESULT GuestDnDTarget::sendData(ULONG aScreenId, const com::Utf8Str &aFormat, const std::vector<BYTE> &aData,
                                 ComPtr<IProgress> &aProgress)
{
#if !defined(VBOX_WITH_DRAG_AND_DROP)
    ReturnComNotImplemented();
#else /* VBOX_WITH_DRAG_AND_DROP */

    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();

    /* Input validation. */
    if (RT_UNLIKELY((aFormat.c_str()) == NULL || *(aFormat.c_str()) == '\0'))
        return setError(E_INVALIDARG, tr("No data format specified"));
    if (RT_UNLIKELY(!aData.size()))
        return setError(E_INVALIDARG, tr("No data to send specified"));

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    /* At the moment we only support one transfer at a time. */
    if (mDataBase.m_cTransfersPending)
        return setError(E_INVALIDARG, tr("Another drop operation already is in progress"));

    /* Ditto. */
    GuestDnDResponse *pResp = GUESTDNDINST()->response();
    AssertPtr(pResp);

    HRESULT hr = pResp->resetProgress(m_pGuest);
    if (FAILED(hr))
        return hr;

    GuestDnDSendDataTask *pTask    = NULL;
    GuestDnDSendCtx      *pSendCtx = NULL;

    try
    {
        /* pSendCtx is passed into SendDataTask where one is deleted in destructor. */
        pSendCtx = new GuestDnDSendCtx();
        pSendCtx->mpTarget  = this;
        pSendCtx->mpResp    = pResp;
        pSendCtx->mScreenID = aScreenId;
        pSendCtx->mFmtReq   = aFormat;

        pSendCtx->Meta.add(aData);

        /* pTask is responsible for deletion of pSendCtx after creating */
        pTask = new GuestDnDSendDataTask(this, pSendCtx);
        if (!pTask->isOk())
        {
            delete pTask;
            LogRel(("DnD: Could not create SendDataTask object\n"));
            throw hr = E_FAIL;
        }

        /* This function delete pTask in case of exceptions,
         * so there is no need in the call of delete operator. */
        hr = pTask->createThreadWithType(RTTHREADTYPE_MAIN_WORKER);
        pTask = NULL; /* Note: pTask is now owned by the worker thread. */
    }
    catch (std::bad_alloc &)
    {
        hr = setError(E_OUTOFMEMORY);
    }
    catch (...)
    {
        LogRel(("DnD: Could not create thread for data sending task\n"));
        hr = E_FAIL;
    }

    if (SUCCEEDED(hr))
    {
        mDataBase.m_cTransfersPending++;

        hr = pResp->queryProgressTo(aProgress.asOutParam());
        ComAssertComRC(hr);
    }
    else
        hr = setError(hr, tr("Starting thread for GuestDnDTarget::i_sendDataThread (%Rhrc)"), hr);

    LogFlowFunc(("Returning hr=%Rhrc\n", hr));
    return hr;
#endif /* VBOX_WITH_DRAG_AND_DROP */
}

/* static */
Utf8Str GuestDnDTarget::i_guestErrorToString(int guestRc)
{
    Utf8Str strError;

    switch (guestRc)
    {
        case VERR_ACCESS_DENIED:
            strError += Utf8StrFmt(tr("For one or more guest files or directories selected for transferring to the host your guest "
                                      "user does not have the appropriate access rights for. Please make sure that all selected "
                                      "elements can be accessed and that your guest user has the appropriate rights"));
            break;

        case VERR_NOT_FOUND:
            /* Should not happen due to file locking on the guest, but anyway ... */
            strError += Utf8StrFmt(tr("One or more guest files or directories selected for transferring to the host were not"
                                      "found on the guest anymore. This can be the case if the guest files were moved and/or"
                                      "altered while the drag and drop operation was in progress"));
            break;

        case VERR_SHARING_VIOLATION:
            strError += Utf8StrFmt(tr("One or more guest files or directories selected for transferring to the host were locked. "
                                      "Please make sure that all selected elements can be accessed and that your guest user has "
                                      "the appropriate rights"));
            break;

        case VERR_TIMEOUT:
            strError += Utf8StrFmt(tr("The guest was not able to process the drag and drop data within time"));
            break;

        default:
            strError += Utf8StrFmt(tr("Drag and drop error from guest (%Rrc)"), guestRc);
            break;
    }

    return strError;
}

/* static */
Utf8Str GuestDnDTarget::i_hostErrorToString(int hostRc)
{
    Utf8Str strError;

    switch (hostRc)
    {
        case VERR_ACCESS_DENIED:
            strError += Utf8StrFmt(tr("For one or more host files or directories selected for transferring to the guest your host "
                                      "user does not have the appropriate access rights for. Please make sure that all selected "
                                      "elements can be accessed and that your host user has the appropriate rights."));
            break;

        case VERR_NOT_FOUND:
            /* Should not happen due to file locking on the host, but anyway ... */
            strError += Utf8StrFmt(tr("One or more host files or directories selected for transferring to the host were not"
                                      "found on the host anymore. This can be the case if the host files were moved and/or"
                                      "altered while the drag and drop operation was in progress."));
            break;

        case VERR_SHARING_VIOLATION:
            strError += Utf8StrFmt(tr("One or more host files or directories selected for transferring to the guest were locked. "
                                      "Please make sure that all selected elements can be accessed and that your host user has "
                                      "the appropriate rights."));
            break;

        default:
            strError += Utf8StrFmt(tr("Drag and drop error from host (%Rrc)"), hostRc);
            break;
    }

    return strError;
}

/**
 * Main function for sending DnD host data to the guest.
 *
 * @returns VBox status code.
 * @param   pCtx                Send context to use.
 * @param   msTimeout           Timeout (in ms) to wait for getting the data sent.
 */
int GuestDnDTarget::i_sendData(GuestDnDSendCtx *pCtx, RTMSINTERVAL msTimeout)
{
    AssertPtrReturn(pCtx, VERR_INVALID_POINTER);

    /* Is this context already in sending state? */
    if (ASMAtomicReadBool(&pCtx->mIsActive))
        return VERR_WRONG_ORDER;
    ASMAtomicWriteBool(&pCtx->mIsActive, true);

    /* Clear all remaining outgoing messages. */
    mDataBase.m_lstMsgOut.clear();

    /**
     * Do we need to build up a file tree?
     * Note: The decision whether we need to build up a file tree and sending
     *       actual file data only depends on the actual formats offered by this target.
     *       If the guest does not want a transfer list ("text/uri-list") but text ("TEXT" and
     *       friends) instead, still send the data over to the guest -- the file as such still
     *       is needed on the guest in this case, as the guest then just wants a simple path
     *       instead of a transfer list (pointing to a file on the guest itself).
     *
     ** @todo Support more than one format; add a format<->function handler concept. Later. */
    int rc;
    bool fHasURIList = std::find(m_lstFmtOffered.begin(),
                                 m_lstFmtOffered.end(), "text/uri-list") != m_lstFmtOffered.end();
    if (fHasURIList)
    {
        rc = i_sendTransferData(pCtx, msTimeout);
    }
    else
    {
        rc = i_sendRawData(pCtx, msTimeout);
    }

    ASMAtomicWriteBool(&pCtx->mIsActive, false);

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/**
 * Sends the common meta data body to the guest.
 *
 * @returns VBox status code.
 * @param   pCtx                Send context to use.
 */
int GuestDnDTarget::i_sendMetaDataBody(GuestDnDSendCtx *pCtx)
{
    AssertPtrReturn(pCtx, VERR_INVALID_POINTER);

    /** @todo Add support for multiple HOST_DND_HG_SND_DATA messages in case of more than 64K data! */
    if (pCtx->Meta.cbData > _64K)
        return VERR_NOT_IMPLEMENTED;

    const uint32_t cbFmt = (uint32_t)pCtx->Meta.strFmt.length() + 1; /* Include terminator. */

    LogFlowFunc(("cbFmt=%RU32, cbMeta=%RU32\n", cbFmt, pCtx->Meta.cbData));

    GuestDnDMsg Msg;
    Msg.setType(HOST_DND_HG_SND_DATA);
    if (mDataBase.m_uProtocolVersion < 3)
    {
        Msg.setNextUInt32(pCtx->mScreenID);                                 /* uScreenId */
        Msg.setNextPointer(pCtx->Meta.strFmt.mutableRaw(), cbFmt);          /* pvFormat */
        Msg.setNextUInt32(cbFmt);                                           /* cbFormat */
        Msg.setNextPointer(pCtx->Meta.pvData, (uint32_t)pCtx->Meta.cbData); /* pvData */
        /* Fill in the current data block size to send.
         * Note: Only supports uint32_t. */
        Msg.setNextUInt32((uint32_t)pCtx->Meta.cbData);                     /* cbData */
    }
    else
    {
        Msg.setNextUInt32(0); /** @todo ContextID not used yet. */
        Msg.setNextPointer(pCtx->Meta.pvData, (uint32_t)pCtx->Meta.cbData); /* pvData */
        Msg.setNextUInt32((uint32_t)pCtx->Meta.cbData);                     /* cbData */
        Msg.setNextPointer(NULL, 0);                                        /** @todo pvChecksum; not used yet. */
        Msg.setNextUInt32(0);                                               /** @todo cbChecksum; not used yet. */
    }

    int rc = GUESTDNDINST()->hostCall(Msg.getType(), Msg.getCount(), Msg.getParms());
    if (RT_SUCCESS(rc))
    {
        rc = updateProgress(pCtx, pCtx->mpResp, pCtx->Meta.cbData);
        AssertRC(rc);
    }

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/**
 * Sends the common meta data header to the guest.
 *
 * @returns VBox status code.
 * @param   pCtx                Send context to use.
 */
int GuestDnDTarget::i_sendMetaDataHeader(GuestDnDSendCtx *pCtx)
{
    AssertPtrReturn(pCtx, VERR_INVALID_POINTER);

    if (mDataBase.m_uProtocolVersion < 3) /* Protocol < v3 did not support this, skip. */
        return VINF_SUCCESS;

    GuestDnDMsg Msg;

    Msg.setType(HOST_DND_HG_SND_DATA_HDR);

    Msg.setNextUInt32(0);                                                /** @todo uContext; not used yet. */
    Msg.setNextUInt32(0);                                                /** @todo uFlags; not used yet. */
    Msg.setNextUInt32(pCtx->mScreenID);                                  /* uScreen */
    Msg.setNextUInt64(pCtx->getTotal());                                 /* cbTotal */
    Msg.setNextUInt32(pCtx->Meta.cbData);                                /* cbMeta*/
    Msg.setNextPointer(unconst(pCtx->Meta.strFmt.c_str()), (uint32_t)pCtx->Meta.strFmt.length() + 1); /* pvMetaFmt */
    Msg.setNextUInt32((uint32_t)pCtx->Meta.strFmt.length() + 1);                                      /* cbMetaFmt */
    Msg.setNextUInt64(pCtx->mTransfer.cObjToProcess );                   /* cObjects */
    Msg.setNextUInt32(0);                                                /** @todo enmCompression; not used yet. */
    Msg.setNextUInt32(0);                                                /** @todo enmChecksumType; not used yet. */
    Msg.setNextPointer(NULL, 0);                                         /** @todo pvChecksum; not used yet. */
    Msg.setNextUInt32(0);                                                /** @todo cbChecksum; not used yet. */

    int rc = GUESTDNDINST()->hostCall(Msg.getType(), Msg.getCount(), Msg.getParms());

    LogFlowFuncLeaveRC(rc);
    return rc;
}

int GuestDnDTarget::i_sendDirectory(GuestDnDSendCtx *pCtx, PDNDTRANSFEROBJECT pObj, GuestDnDMsg *pMsg)
{
    AssertPtrReturn(pCtx, VERR_INVALID_POINTER);
    AssertPtrReturn(pMsg, VERR_INVALID_POINTER);

    const char *pcszDstPath = DnDTransferObjectGetDestPath(pObj);
    AssertPtrReturn(pcszDstPath, VERR_INVALID_POINTER);
    const size_t cchPath = RTStrNLen(pcszDstPath, RTPATH_MAX); /* Note: Maximum is RTPATH_MAX on guest side. */
    AssertReturn(cchPath, VERR_INVALID_PARAMETER);

    LogRel2(("DnD: Transferring host directory '%s' to guest\n", DnDTransferObjectGetSourcePath(pObj)));

    pMsg->setType(HOST_DND_HG_SND_DIR);
    if (mDataBase.m_uProtocolVersion >= 3)
        pMsg->setNextUInt32(0); /** @todo ContextID not used yet. */
    pMsg->setNextString(pcszDstPath);                    /* path */
    pMsg->setNextUInt32((uint32_t)(cchPath + 1));        /* path length, including terminator. */
    pMsg->setNextUInt32(DnDTransferObjectGetMode(pObj)); /* mode */

    return VINF_SUCCESS;
}

/**
 * Sends a transfer file to the guest.
 *
 * @returns VBox status code.
 * @param   pCtx
 * @param   pObj
 * @param   pMsg
 */
int GuestDnDTarget::i_sendFile(GuestDnDSendCtx *pCtx,
                               PDNDTRANSFEROBJECT pObj, GuestDnDMsg *pMsg)
{
    AssertPtrReturn(pCtx, VERR_INVALID_POINTER);
    AssertPtrReturn(pObj, VERR_INVALID_POINTER);
    AssertPtrReturn(pMsg, VERR_INVALID_POINTER);

    const char *pcszSrcPath = DnDTransferObjectGetSourcePath(pObj);
    AssertPtrReturn(pcszSrcPath, VERR_INVALID_POINTER);
    const char *pcszDstPath = DnDTransferObjectGetDestPath(pObj);
    AssertPtrReturn(pcszDstPath, VERR_INVALID_POINTER);

    int rc = VINF_SUCCESS;

    if (!DnDTransferObjectIsOpen(pObj))
    {
        LogRel2(("DnD: Opening host file '%s' for transferring to guest\n", pcszSrcPath));

        rc = DnDTransferObjectOpen(pObj, RTFILE_O_OPEN | RTFILE_O_READ | RTFILE_O_DENY_WRITE, 0 /* fMode */,
                                   DNDTRANSFEROBJECT_FLAGS_NONE);
        if (RT_FAILURE(rc))
            LogRel(("DnD: Opening host file '%s' failed, rc=%Rrc\n", pcszSrcPath, rc));
    }

    if (RT_FAILURE(rc))
        return rc;

    bool fSendData = false;
    if (RT_SUCCESS(rc))
    {
        if (mDataBase.m_uProtocolVersion >= 2)
        {
            if (!(pCtx->mTransfer.mfObjState & DND_OBJ_STATE_HAS_HDR))
            {
                const size_t  cchDstPath = RTStrNLen(pcszDstPath, RTPATH_MAX);
                const size_t  cbSize     = DnDTransferObjectGetSize(pObj);
                const RTFMODE fMode      = DnDTransferObjectGetMode(pObj);

                /*
                 * Since protocol v2 the file header and the actual file contents are
                 * separate messages, so send the file header first.
                 * The just registered callback will be called by the guest afterwards.
                 */
                pMsg->setType(HOST_DND_HG_SND_FILE_HDR);
                pMsg->setNextUInt32(0); /** @todo ContextID not used yet. */
                pMsg->setNextString(pcszDstPath);                    /* pvName */
                pMsg->setNextUInt32((uint32_t)(cchDstPath + 1));     /* cbName */
                pMsg->setNextUInt32(0);                              /* uFlags */
                pMsg->setNextUInt32(fMode);                          /* fMode */
                pMsg->setNextUInt64(cbSize);                         /* uSize */

                LogRel2(("DnD: Transferring host file '%s' to guest (%zu bytes, mode %#x)\n",
                         pcszSrcPath, cbSize, fMode));

                /** @todo Set progress object title to current file being transferred? */

                /* Update object state to reflect that we have sent the file header. */
                pCtx->mTransfer.mfObjState |= DND_OBJ_STATE_HAS_HDR;
            }
            else
            {
                /* File header was sent, so only send the actual file data. */
                fSendData = true;
            }
        }
        else /* Protocol v1. */
        {
            /* Always send the file data, every time. */
            fSendData = true;
        }
    }

    if (   RT_SUCCESS(rc)
        && fSendData)
    {
        rc = i_sendFileData(pCtx, pObj, pMsg);
    }

    if (RT_FAILURE(rc))
        LogRel(("DnD: Sending host file '%s' to guest failed, rc=%Rrc\n", pcszSrcPath, rc));

    LogFlowFuncLeaveRC(rc);
    return rc;
}

int GuestDnDTarget::i_sendFileData(GuestDnDSendCtx *pCtx,
                                   PDNDTRANSFEROBJECT pObj, GuestDnDMsg *pMsg)
{
    AssertPtrReturn(pCtx, VERR_INVALID_POINTER);
    AssertPtrReturn(pObj, VERR_INVALID_POINTER);
    AssertPtrReturn(pMsg, VERR_INVALID_POINTER);

    AssertPtrReturn(pCtx->mpResp, VERR_WRONG_ORDER);

    /** @todo Don't allow concurrent reads per context! */

    /* Set the message type. */
    pMsg->setType(HOST_DND_HG_SND_FILE_DATA);

    const char *pcszSrcPath = DnDTransferObjectGetSourcePath(pObj);
    const char *pcszDstPath = DnDTransferObjectGetDestPath(pObj);

    /* Protocol version 1 sends the file path *every* time with a new file chunk.
     * In protocol version 2 we only do this once with HOST_DND_HG_SND_FILE_HDR. */
    if (mDataBase.m_uProtocolVersion <= 1)
    {
        const size_t cchDstPath = RTStrNLen(pcszDstPath, RTPATH_MAX);

        pMsg->setNextString(pcszDstPath);              /* pvName */
        pMsg->setNextUInt32((uint32_t)cchDstPath + 1); /* cbName */
    }
    else if (mDataBase.m_uProtocolVersion >= 2)
    {
        pMsg->setNextUInt32(0);                        /** @todo ContextID not used yet. */
    }

    void *pvBuf  = pCtx->mTransfer.pvScratchBuf;
    AssertPtr(pvBuf);
    size_t cbBuf = pCtx->mTransfer.cbScratchBuf;
    Assert(cbBuf);

    uint32_t cbRead;

    int rc = DnDTransferObjectRead(pObj, pvBuf, cbBuf, &cbRead);
    if (RT_SUCCESS(rc))
    {
        pCtx->addProcessed(cbRead);

        LogFlowFunc(("cbBufe=%zu, cbRead=%RU32\n", cbBuf, cbRead));

        if (mDataBase.m_uProtocolVersion <= 1)
        {
            pMsg->setNextPointer(pvBuf, cbRead);                            /* pvData */
            pMsg->setNextUInt32(cbRead);                                    /* cbData */
            pMsg->setNextUInt32(DnDTransferObjectGetMode(pObj));            /* fMode */
        }
        else /* Protocol v2 and up. */
        {
            pMsg->setNextPointer(pvBuf, cbRead);                            /* pvData */
            pMsg->setNextUInt32(cbRead);                                    /* cbData */

            if (mDataBase.m_uProtocolVersion >= 3)
            {
                /** @todo Calculate checksum. */
                pMsg->setNextPointer(NULL, 0);                              /* pvChecksum */
                pMsg->setNextUInt32(0);                                     /* cbChecksum */
            }
        }

        if (DnDTransferObjectIsComplete(pObj)) /* Done reading? */
        {
            LogRel2(("DnD: Transferring host file '%s' to guest complete\n", pcszSrcPath));

            /* DnDTransferObjectRead() returns VINF_EOF when finished reading the entire file,
             * but we don't want this here -- so just set VINF_SUCCESS. */
            rc = VINF_SUCCESS;
        }
    }
    else
        LogRel(("DnD: Reading from host file '%s' failed, rc=%Rrc\n", pcszSrcPath, rc));

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/* static */
DECLCALLBACK(int) GuestDnDTarget::i_sendURIDataCallback(uint32_t uMsg, void *pvParms, size_t cbParms, void *pvUser)
{
    GuestDnDSendCtx *pCtx = (GuestDnDSendCtx *)pvUser;
    AssertPtrReturn(pCtx, VERR_INVALID_POINTER);

    GuestDnDTarget *pThis = pCtx->mpTarget;
    AssertPtrReturn(pThis, VERR_INVALID_POINTER);

    LogFlowFunc(("pThis=%p, uMsg=%RU32\n", pThis, uMsg));

    int  rc      = VINF_SUCCESS;
    int  rcGuest = VINF_SUCCESS; /* Contains error code from guest in case of VERR_GSTDND_GUEST_ERROR. */
    bool fNotify = false;

    switch (uMsg)
    {
        case GUEST_DND_CONNECT:
            /* Nothing to do here (yet). */
            break;

        case GUEST_DND_DISCONNECT:
            rc = VERR_CANCELLED;
            break;

        case GUEST_DND_GET_NEXT_HOST_MSG:
        {
            PVBOXDNDCBHGGETNEXTHOSTMSG pCBData = reinterpret_cast<PVBOXDNDCBHGGETNEXTHOSTMSG>(pvParms);
            AssertPtr(pCBData);
            AssertReturn(sizeof(VBOXDNDCBHGGETNEXTHOSTMSG) == cbParms, VERR_INVALID_PARAMETER);
            AssertReturn(CB_MAGIC_DND_HG_GET_NEXT_HOST_MSG == pCBData->hdr.uMagic, VERR_INVALID_PARAMETER);

            try
            {
                GuestDnDMsg *pMsg = new GuestDnDMsg();

                rc = pThis->i_sendTransferDataLoop(pCtx, pMsg);
                if (rc == VINF_EOF) /* Transfer complete? */
                {
                    LogFlowFunc(("Last URI item processed, bailing out\n"));
                }
                else if (RT_SUCCESS(rc))
                {
                    rc = pThis->msgQueueAdd(pMsg);
                    if (RT_SUCCESS(rc)) /* Return message type & required parameter count to the guest. */
                    {
                        LogFlowFunc(("GUEST_DND_GET_NEXT_HOST_MSG -> %RU32 (%RU32 params)\n", pMsg->getType(), pMsg->getCount()));
                        pCBData->uMsg   = pMsg->getType();
                        pCBData->cParms = pMsg->getCount();
                    }
                }

                if (   RT_FAILURE(rc)
                    || rc == VINF_EOF) /* Transfer complete? */
                {
                    delete pMsg;
                    pMsg = NULL;
                }
            }
            catch(std::bad_alloc & /*e*/)
            {
                rc = VERR_NO_MEMORY;
            }
            break;
        }
        case GUEST_DND_GH_EVT_ERROR:
        {
            PVBOXDNDCBEVTERRORDATA pCBData = reinterpret_cast<PVBOXDNDCBEVTERRORDATA>(pvParms);
            AssertPtr(pCBData);
            AssertReturn(sizeof(VBOXDNDCBEVTERRORDATA) == cbParms, VERR_INVALID_PARAMETER);
            AssertReturn(CB_MAGIC_DND_GH_EVT_ERROR == pCBData->hdr.uMagic, VERR_INVALID_PARAMETER);

            pCtx->mpResp->reset();

            if (RT_SUCCESS(pCBData->rc))
            {
                AssertMsgFailed(("Guest has sent an error event but did not specify an actual error code\n"));
                pCBData->rc = VERR_GENERAL_FAILURE; /* Make sure some error is set. */
            }

            rc = pCtx->mpResp->setProgress(100, DND_PROGRESS_ERROR, pCBData->rc,
                                           GuestDnDTarget::i_guestErrorToString(pCBData->rc));
            if (RT_SUCCESS(rc))
            {
                rc      = VERR_GSTDND_GUEST_ERROR;
                rcGuest = pCBData->rc;
            }
            break;
        }
        case HOST_DND_HG_SND_DIR:
        case HOST_DND_HG_SND_FILE_HDR:
        case HOST_DND_HG_SND_FILE_DATA:
        {
            PVBOXDNDCBHGGETNEXTHOSTMSGDATA pCBData
                = reinterpret_cast<PVBOXDNDCBHGGETNEXTHOSTMSGDATA>(pvParms);
            AssertPtr(pCBData);
            AssertReturn(sizeof(VBOXDNDCBHGGETNEXTHOSTMSGDATA) == cbParms, VERR_INVALID_PARAMETER);

            LogFlowFunc(("pCBData->uMsg=%RU32, paParms=%p, cParms=%RU32\n", pCBData->uMsg, pCBData->paParms, pCBData->cParms));

            GuestDnDMsg *pMsg = pThis->msgQueueGetNext();
            if (pMsg)
            {
                /*
                 * Sanity checks.
                 */
                if (   pCBData->uMsg    != uMsg
                    || pCBData->paParms == NULL
                    || pCBData->cParms  != pMsg->getCount())
                {
                    LogFlowFunc(("Current message does not match:\n"));
                    LogFlowFunc(("\tCallback: uMsg=%RU32, cParms=%RU32, paParms=%p\n",
                                 pCBData->uMsg, pCBData->cParms, pCBData->paParms));
                    LogFlowFunc(("\t    Next: uMsg=%RU32, cParms=%RU32\n", pMsg->getType(), pMsg->getCount()));

                    /* Start over. */
                    pThis->msgQueueClear();

                    rc = VERR_INVALID_PARAMETER;
                }

                if (RT_SUCCESS(rc))
                {
                    LogFlowFunc(("Returning uMsg=%RU32\n", uMsg));
                    rc = HGCM::Message::CopyParms(pCBData->paParms, pCBData->cParms, pMsg->getParms(), pMsg->getCount(),
                                                  false /* fDeepCopy */);
                    if (RT_SUCCESS(rc))
                    {
                        pCBData->cParms = pMsg->getCount();
                        pThis->msgQueueRemoveNext();
                    }
                    else
                        LogFlowFunc(("Copying parameters failed with rc=%Rrc\n", rc));
                }
            }
            else
                rc = VERR_NO_DATA;

            LogFlowFunc(("Processing next message ended with rc=%Rrc\n", rc));
            break;
        }
        default:
            rc = VERR_NOT_SUPPORTED;
            break;
    }

    int rcToGuest = VINF_SUCCESS; /* Status which will be sent back to the guest. */

    /*
     * Resolve errors.
     */
    switch (rc)
    {
        case VINF_SUCCESS:
            break;

        case VINF_EOF:
        {
            LogRel2(("DnD: Transfer to guest complete\n"));

            /* Complete operation on host side. */
            fNotify = true;

            /* The guest expects VERR_NO_DATA if the transfer is complete. */
            rcToGuest = VERR_NO_DATA;
            break;
        }

        case VERR_GSTDND_GUEST_ERROR:
        {
            LogRel(("DnD: Guest reported error %Rrc, aborting transfer to guest\n", rcGuest));
            break;
        }

        case VERR_CANCELLED:
        {
            LogRel2(("DnD: Transfer to guest canceled\n"));
            rcToGuest = VERR_CANCELLED; /* Also cancel on guest side. */
            break;
        }

        default:
        {
            LogRel(("DnD: Host error %Rrc occurred, aborting transfer to guest\n", rc));
            rcToGuest = VERR_CANCELLED; /* Also cancel on guest side. */
            break;
        }
    }

    if (RT_FAILURE(rc))
    {
        /* Unregister this callback. */
        AssertPtr(pCtx->mpResp);
        int rc2 = pCtx->mpResp->setCallback(uMsg, NULL /* PFNGUESTDNDCALLBACK */);
        AssertRC(rc2);

        /* Let the waiter(s) know. */
        fNotify = true;
    }

    LogFlowFunc(("fNotify=%RTbool, rc=%Rrc, rcToGuest=%Rrc\n", fNotify, rc, rcToGuest));

    if (fNotify)
    {
        int rc2 = pCtx->mCBEvent.Notify(rc); /** @todo Also pass guest error back? */
        AssertRC(rc2);
    }

    LogFlowFuncLeaveRC(rc);
    return rcToGuest; /* Tell the guest. */
}

/**
 * Main function for sending the actual transfer data (i.e. files + directories) to the guest.
 *
 * @returns VBox status code.
 * @param   pCtx                Send context to use.
 * @param   msTimeout           Timeout (in ms) to use for getting the data sent.
 */
int GuestDnDTarget::i_sendTransferData(GuestDnDSendCtx *pCtx, RTMSINTERVAL msTimeout)
{
    AssertPtrReturn(pCtx, VERR_INVALID_POINTER);
    AssertPtr(pCtx->mpResp);

#define REGISTER_CALLBACK(x)                                            \
    do {                                                                \
        rc = pCtx->mpResp->setCallback(x, i_sendURIDataCallback, pCtx); \
        if (RT_FAILURE(rc))                                             \
            return rc;                                                  \
    } while (0)

#define UNREGISTER_CALLBACK(x)                        \
    do {                                              \
        int rc2 = pCtx->mpResp->setCallback(x, NULL); \
        AssertRC(rc2);                                \
    } while (0)

    int rc = pCtx->mTransfer.init(mData.mcbBlockSize);
    if (RT_FAILURE(rc))
        return rc;

    rc = pCtx->mCBEvent.Reset();
    if (RT_FAILURE(rc))
        return rc;

    /*
     * Register callbacks.
     */
    /* Guest callbacks. */
    REGISTER_CALLBACK(GUEST_DND_CONNECT);
    REGISTER_CALLBACK(GUEST_DND_DISCONNECT);
    REGISTER_CALLBACK(GUEST_DND_GET_NEXT_HOST_MSG);
    REGISTER_CALLBACK(GUEST_DND_GH_EVT_ERROR);
    /* Host callbacks. */
    REGISTER_CALLBACK(HOST_DND_HG_SND_DIR);
    if (mDataBase.m_uProtocolVersion >= 2)
        REGISTER_CALLBACK(HOST_DND_HG_SND_FILE_HDR);
    REGISTER_CALLBACK(HOST_DND_HG_SND_FILE_DATA);

    do
    {
        /*
         * Extract transfer list from current meta data.
         */
        rc = DnDTransferListAppendPathsFromBuffer(&pCtx->mTransfer.mList, DNDTRANSFERLISTFMT_NATIVE,
                                                  (const char *)pCtx->Meta.pvData, pCtx->Meta.cbData, "\n", DNDTRANSFERLIST_FLAGS_NONE);
        if (RT_FAILURE(rc))
            break;

        /*
         * Set the extra data size
         */
        pCtx->cbExtra = DnDTransferListObjTotalBytes(&pCtx->mTransfer.mList);

        /*
         * The first message always is the data header. The meta data itself then follows
         * and *only* contains the root elements of a transfer list.
         *
         * After the meta data we generate the messages required to send the
         * file/directory data itself.
         *
         * Note: Protocol < v3 use the first data message to tell what's being sent.
         */
        GuestDnDMsg Msg;

        /*
         * Send the data header first.
         */
        if (mDataBase.m_uProtocolVersion >= 3)
            rc = i_sendMetaDataHeader(pCtx);

        /*
         * Send the (meta) data body.
         */
        if (RT_SUCCESS(rc))
            rc = i_sendMetaDataBody(pCtx);

        if (RT_SUCCESS(rc))
        {
            rc = waitForEvent(&pCtx->mCBEvent, pCtx->mpResp, msTimeout);
            if (RT_SUCCESS(rc))
                pCtx->mpResp->setProgress(100, DND_PROGRESS_COMPLETE, VINF_SUCCESS);
        }

    } while (0);

    /*
     * Unregister callbacks.
     */
    /* Guest callbacks. */
    UNREGISTER_CALLBACK(GUEST_DND_CONNECT);
    UNREGISTER_CALLBACK(GUEST_DND_DISCONNECT);
    UNREGISTER_CALLBACK(GUEST_DND_GET_NEXT_HOST_MSG);
    UNREGISTER_CALLBACK(GUEST_DND_GH_EVT_ERROR);
    /* Host callbacks. */
    UNREGISTER_CALLBACK(HOST_DND_HG_SND_DIR);
    if (mDataBase.m_uProtocolVersion >= 2)
        UNREGISTER_CALLBACK(HOST_DND_HG_SND_FILE_HDR);
    UNREGISTER_CALLBACK(HOST_DND_HG_SND_FILE_DATA);

#undef REGISTER_CALLBACK
#undef UNREGISTER_CALLBACK

    if (RT_FAILURE(rc))
    {
        if (rc == VERR_CANCELLED) /* Transfer was cancelled by the host. */
        {
            /*
             * Now that we've cleaned up tell the guest side to cancel.
             * This does not imply we're waiting for the guest to react, as the
             * host side never must depend on anything from the guest.
             */
            int rc2 = sendCancel();
            AssertRC(rc2);

            LogRel2(("DnD: Sending transfer data to guest cancelled by user\n"));

            rc2 = pCtx->mpResp->setProgress(100, DND_PROGRESS_CANCELLED, VINF_SUCCESS);
            AssertRC(rc2);
        }
        else if (rc != VERR_GSTDND_GUEST_ERROR) /* Guest-side error are already handled in the callback. */
        {
            LogRel(("DnD: Sending transfer data to guest failed with rc=%Rrc\n", rc));
            int rc2 = pCtx->mpResp->setProgress(100, DND_PROGRESS_ERROR, rc,
                                                GuestDnDTarget::i_hostErrorToString(rc));
            AssertRC(rc2);
        }

        rc = VINF_SUCCESS; /* The error was handled by the setProgress() calls above. */
    }

    LogFlowFuncLeaveRC(rc);
    return rc;
}

int GuestDnDTarget::i_sendTransferDataLoop(GuestDnDSendCtx *pCtx, GuestDnDMsg *pMsg)
{
    AssertPtrReturn(pCtx, VERR_INVALID_POINTER);
    AssertPtrReturn(pMsg, VERR_INVALID_POINTER);

    int rc = updateProgress(pCtx, pCtx->mpResp);
    AssertRC(rc);

    if (   pCtx->isComplete()
        && pCtx->mTransfer.isComplete())
    {
        return VINF_EOF;
    }

    PDNDTRANSFEROBJECT pObj = DnDTransferListObjGetFirst(&pCtx->mTransfer.mList);
    if (!pObj)
        return VERR_WRONG_ORDER;

    switch (DnDTransferObjectGetType(pObj))
    {
        case DNDTRANSFEROBJTYPE_DIRECTORY:
            rc = i_sendDirectory(pCtx, pObj, pMsg);
            break;

        case DNDTRANSFEROBJTYPE_FILE:
            rc = i_sendFile(pCtx, pObj, pMsg);
            break;

        default:
            AssertFailedStmt(rc = VERR_NOT_SUPPORTED);
            break;
    }

    if (   DnDTransferObjectIsComplete(pObj)
        || RT_FAILURE(rc))
        DnDTransferListObjRemoveFirst(&pCtx->mTransfer.mList);

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/**
 * Main function for sending raw data (e.g. text, RTF, ...) to the guest.
 *
 * @returns VBox status code.
 * @param   pCtx                Send context to use.
 * @param   msTimeout           Timeout (in ms) to use for getting the data sent.
 */
int GuestDnDTarget::i_sendRawData(GuestDnDSendCtx *pCtx, RTMSINTERVAL msTimeout)
{
    AssertPtrReturn(pCtx, VERR_INVALID_POINTER);
    NOREF(msTimeout);

    /** @todo At the moment we only allow sending up to 64K raw data.
     *        For protocol v1+v2: Fix this by using HOST_DND_HG_SND_MORE_DATA.
     *        For protocol v3   : Send another HOST_DND_HG_SND_DATA message. */
    if (!pCtx->Meta.cbData)
        return VINF_SUCCESS;

    int rc = i_sendMetaDataHeader(pCtx);
    if (RT_SUCCESS(rc))
        rc = i_sendMetaDataBody(pCtx);

    int rc2;
    if (RT_FAILURE(rc))
    {
        LogRel(("DnD: Sending raw data to guest failed with rc=%Rrc\n", rc));
        rc2 = pCtx->mpResp->setProgress(100 /* Percent */, DND_PROGRESS_ERROR, rc,
                                        GuestDnDTarget::i_hostErrorToString(rc));
    }
    else
        rc2 = pCtx->mpResp->setProgress(100 /* Percent */, DND_PROGRESS_COMPLETE, rc);
    AssertRC(rc2);

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/**
 * Cancels sending DnD data.
 *
 * @returns VBox status code.
 * @param   aVeto               Whether cancelling was vetoed or not.
 *                              Not implemented yet.
 */
HRESULT GuestDnDTarget::cancel(BOOL *aVeto)
{
#if !defined(VBOX_WITH_DRAG_AND_DROP)
    ReturnComNotImplemented();
#else /* VBOX_WITH_DRAG_AND_DROP */

    LogRel2(("DnD: Sending cancelling request to the guest ...\n"));

    int rc = GuestDnDBase::sendCancel();

    if (aVeto)
        *aVeto = FALSE; /** @todo Impplement vetoing. */

    HRESULT hr = RT_SUCCESS(rc) ? S_OK : VBOX_E_IPRT_ERROR;

    LogFlowFunc(("hr=%Rhrc\n", hr));
    return hr;
#endif /* VBOX_WITH_DRAG_AND_DROP */
}

