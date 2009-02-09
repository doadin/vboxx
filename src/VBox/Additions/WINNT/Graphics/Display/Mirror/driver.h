/******************************Module*Header*******************************\
*
* Copyright (C) 2006-2007 Sun Microsystems, Inc.
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
*
* Based in part on Microsoft DDK sample code
*
*                           *******************
*                           * GDI SAMPLE CODE *
*                           *******************
*
* Module Name: driver.h
*
* contains prototypes for the frame buffer driver.
*
* Copyright (c) 1992-1998 Microsoft Corporation
\**************************************************************************/

#define DBG 1

#include "stddef.h"

#include <stdarg.h>

#include "windef.h"
#include "wingdi.h"
#include "winddi.h"
#include "devioctl.h"
#include "ntddvdeo.h"
#include "debug.h"

typedef struct  _PDEV
{
    HANDLE  hDriver;                    // Handle to \Device\Screen
    HDEV    hdevEng;                    // Engine's handle to PDEV
    HSURF   hsurfEng;                   // Engine's handle to surface
//@todo Make work without palette
    HPALETTE hpalDefault;               // Handle to the default palette for device.
    PBYTE   pjScreen;                   // This is pointer to base screen address
    ULONG   cxScreen;                   // Visible screen width
    ULONG   cyScreen;                   // Visible screen height
    POINTL  ptlOrg;                     // Where this display is anchored in
                                        //   the virtual desktop.
    ULONG   ulMode;                     // Mode the mini-port driver is in.
    LONG    lDeltaScreen;               // Distance from one scan to the next.
    ULONG   cScreenSize;                // size of video memory, including
                                        // offscreen memory.
    PVOID   pOffscreenList;             // linked list of DCI offscreen surfaces.
    FLONG   flRed;                      // For bitfields device, Red Mask
    FLONG   flGreen;                    // For bitfields device, Green Mask
    FLONG   flBlue;                     // For bitfields device, Blue Mask
    ULONG   cPaletteShift;              // number of bits the 8-8-8 palette must
                                        // be shifted by to fit in the hardware
                                        // palette.
    ULONG   ulBitCount;                 // # of bits per pel 8,16,24,32 are only supported.
    POINTL  ptlHotSpot;                 // adjustment for pointer hot spot
    VIDEO_POINTER_CAPABILITIES PointerCapabilities; // HW pointer abilities
    PVIDEO_POINTER_ATTRIBUTES pPointerAttributes; // hardware pointer attributes
    DWORD   cjPointerAttributes;        // Size of buffer allocated
    BOOL    fHwCursorActive;            // Are we currently using the hw cursor
    PALETTEENTRY *pPal;                 // If this is pal managed, this is the pal
    BOOL    bSupportDCI;                // Does the miniport support DCI?

//@todo Make work without allocation
    PVOID   pvTmpBuffer;                // ptr to MIRRSURF bits for screen surface
} PDEV, *PPDEV;

typedef struct _MIRRSURF {
    PPDEV   *pdev;
    ULONG   cx;               
    ULONG   cy;
    ULONG   lDelta;
    ULONG   ulBitCount;
    BOOL    bIsScreen;

} MIRRSURF, *PMIRRSURF;

DWORD getAvailableModes(HANDLE, PVIDEO_MODE_INFORMATION *, DWORD *);
BOOL bInitPDEV(PPDEV, PDEVMODEW, GDIINFO *, DEVINFO *);
BOOL bInitSURF(PPDEV, BOOL);
BOOL bInitPaletteInfo(PPDEV, DEVINFO *);
BOOL bInitPointer(PPDEV, DEVINFO *);
BOOL bInit256ColorPalette(PPDEV);
VOID vDisablePalette(PPDEV);
VOID vDisableSURF(PPDEV);

#define MAX_CLUT_SIZE (sizeof(VIDEO_CLUT) + (sizeof(ULONG) * 256))

//
// Determines the size of the DriverExtra information in the DEVMODE
// structure passed to and from the display driver.
//

#define DRIVER_EXTRA_SIZE 0

#define DLL_NAME                L"vrdpdd"   // Name of the DLL in UNICODE
#define STANDARD_DEBUG_PREFIX   "vrdpdd: "  // All debug output is prefixed
#define ALLOC_TAG               'rvDD'        // Four byte tag (characters in
                                              // reverse order) used for memory
                                              // allocations

