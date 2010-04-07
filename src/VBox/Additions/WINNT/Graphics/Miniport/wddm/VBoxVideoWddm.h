/*
 * Copyright (C) 2010 Sun Microsystems, Inc.
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 *
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
 * Clara, CA 95054 USA or visit http://www.sun.com if you need
 * additional information or have any questions.
 */
#ifndef ___VBoxVideoWddm_h___
#define ___VBoxVideoWddm_h___

#include "../VBoxVideo.h"

/* one page size */
#define VBOXWDDM_C_DMA_BUFFER_SIZE         0x1000
#define VBOXWDDM_C_ALLOC_LIST_SIZE         0xc00
#define VBOXWDDM_C_PATH_LOCATION_LIST_SIZE 0xc00

#define VBOXWDDM_C_POINTER_MAX_WIDTH  64
#define VBOXWDDM_C_POINTER_MAX_HEIGHT 64

#define VBOXWDDM_C_VDMA_BUFFER_SIZE   (64*_1K)

//#define VBOXWDDM_WITH_FAKE_SEGMENT

#define VBOXWDDM_ROUNDBOUND(_v, _b) (((_v) + ((_b) - 1)) & ~((_b) - 1))

PVOID vboxWddmMemAlloc(IN SIZE_T cbSize);
PVOID vboxWddmMemAllocZero(IN SIZE_T cbSize);
VOID vboxWddmMemFree(PVOID pvMem);

typedef enum
{
    VBOXWDDM_ALLOC_TYPE_UNEFINED = 0,
    VBOXWDDM_ALLOC_TYPE_STD_SHAREDPRIMARYSURFACE,
    VBOXWDDM_ALLOC_TYPE_STD_SHADOWSURFACE,
    VBOXWDDM_ALLOC_TYPE_STD_STAGINGSURFACE,
    /* this one is win 7-specific and hence unused for now */
    VBOXWDDM_ALLOC_TYPE_STD_GDISURFACE
    /* custom allocation types requested from user-mode d3d module will go here */
} VBOXWDDM_ALLOC_TYPE;

typedef struct VBOXWDDM_SURFACE_DESC
{
    UINT width;
    UINT height;
    D3DDDIFORMAT format;
    UINT bpp;
    UINT pitch;
} VBOXWDDM_SURFACE_DESC, *PVBOXWDDM_SURFACE_DESC;

typedef struct VBOXWDDM_ALLOCINFO
{
    VBOXWDDM_ALLOC_TYPE enmType;
    union
    {
        VBOXWDDM_SURFACE_DESC SurfInfo;
    } u;
} VBOXWDDM_ALLOCINFO, *PVBOXWDDM_ALLOCINFO;

#define VBOXWDDM_ALLOCINFO_HEADSIZE() (sizeof (VBOXWDDM_ALLOCINFO))
#define VBOXWDDM_ALLOCINFO_SIZE_FROMBODYSIZE(_s) (VBOXWDDM_ALLOCINFO_HEADSIZE() + (_s))
#define VBOXWDDM_ALLOCINFO_SIZE(_tCmd) (VBOXWDDM_ALLOCINFO_SIZE_FROMBODYSIZE(sizeof(_tCmd)))
#define VBOXWDDM_ALLOCINFO_BODY(_p, _t) ( (_t*)(((uint8_t*)(_p)) + VBOXWDDM_ALLOCINFO_HEADSIZE()) )
#define VBOXWDDM_ALLOCINFO_HEAD(_pb) ((VBOXWDDM_ALLOCINFO*)((uint8_t *)(_pb) - VBOXWDDM_ALLOCATION_HEADSIZE()))


typedef struct VBOXWDDM_ALLOCINFO_SHAREDPRIMARYSURFACE
{
    D3DDDI_RATIONAL                 RefreshRate;
    D3DDDI_VIDEO_PRESENT_SOURCE_ID  VidPnSourceId;
} VBOXWDDM_ALLOCINFO_SHAREDPRIMARYSURFACE, *PVBOXWDDM_ALLOCINFO_SHAREDPRIMARYSURFACE;

/* allocation */
typedef struct VBOXWDDM_ALLOCATION
{
    VBOXWDDM_ALLOC_TYPE enmType;
    VBOXVIDEOOFFSET offVram;
    UINT SegmentId;
    union
    {
        VBOXWDDM_SURFACE_DESC SurfInfo;
    } u;
} VBOXWDDM_ALLOCATION, *PVBOXWDDM_ALLOCATION;

#define VBOXWDDM_ALLOCATION_HEADSIZE() (sizeof (VBOXWDDM_ALLOCATION))
#define VBOXWDDM_ALLOCATION_SIZE_FROMBODYSIZE(_s) (VBOXWDDM_ALLOCATION_HEADSIZE() + (_s))
#define VBOXWDDM_ALLOCATION_SIZE(_tCmd) (VBOXWDDM_ALLOCATION_SIZE_FROMBODYSIZE(sizeof(_tCmd)))
#define VBOXWDDM_ALLOCATION_BODY(_p, _t) ( (_t*)(((uint8_t*)(_p)) + VBOXWDDM_ALLOCATION_HEADSIZE()) )
#define VBOXWDDM_ALLOCATION_HEAD(_pb) ((VBOXWDDM_ALLOCATION*)((uint8_t *)(_pb) - VBOXWDDM_ALLOCATION_HEADSIZE()))





typedef struct VBOXWDDM_ALLOCATION_SHAREDPRIMARYSURFACE
{
    D3DDDI_RATIONAL RefreshRate;
    D3DDDI_VIDEO_PRESENT_SOURCE_ID VidPnSourceId;
//    VBOXVIDEOOFFSET offAddress;
    BOOLEAN bVisible;
    BOOLEAN bAssigned;
} VBOXWDDM_ALLOCATION_SHAREDPRIMARYSURFACE, *PVBOXWDDM_ALLOCATION_SHAREDPRIMARYSURFACE;

typedef enum
{
    VBOXWDDM_DEVICE_TYPE_UNDEFINED = 0,
    VBOXWDDM_DEVICE_TYPE_SYSTEM
} VBOXWDDM_DEVICE_TYPE;

typedef struct VBOXWDDM_DEVICE
{
    struct _DEVICE_EXTENSION * pAdapter; /* Adapder info */
    HANDLE hDevice; /* handle passed to CreateDevice */
    VBOXWDDM_DEVICE_TYPE enmType; /* device creation flags passed to DxgkDdiCreateDevice, not sure we need it */
} VBOXWDDM_DEVICE, *PVBOXWDDM_DEVICE;

typedef enum
{
    VBOXWDDM_CONTEXT_TYPE_UNDEFINED = 0,
    VBOXWDDM_CONTEXT_TYPE_SYSTEM
} VBOXWDDM_CONTEXT_TYPE;

typedef struct VBOXWDDM_CONTEXT
{
    struct VBOXWDDM_DEVICE * pDevice;
    HANDLE hContext;
    VBOXWDDM_CONTEXT_TYPE enmType;
    UINT  NodeOrdinal;
    UINT  EngineAffinity;
    UINT uLastCompletedCmdFenceId;
} VBOXWDDM_CONTEXT, *PVBOXWDDM_CONTEXT;

typedef struct VBOXWDDM_DMA_PRIVATE_DATA
{
    PVBOXWDDM_CONTEXT pContext;
    uint8_t Reserved[8];
}VBOXWDDM_DMA_PRIVATE_DATA, *PVBOXWDDM_DMA_PRIVATE_DATA;

typedef struct VBOXWDDM_OPENALLOCATION
{
    D3DKMT_HANDLE  hAllocation;
} VBOXWDDM_OPENALLOCATION, *PVBOXWDDM_OPENALLOCATION;

#endif /* #ifndef ___VBoxVideoWddm_h___ */
