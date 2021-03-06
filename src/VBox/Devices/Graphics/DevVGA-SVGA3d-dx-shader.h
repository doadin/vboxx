/* $Id$ */
/** @file
 * DevVGA - VMWare SVGA device - VGPU10+ (DX) shader utilities.
 */

/*
 * Copyright (C) 2020-2021 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef VBOX_INCLUDED_SRC_Graphics_DevVGA_SVGA3d_dx_shader_h
#define VBOX_INCLUDED_SRC_Graphics_DevVGA_SVGA3d_dx_shader_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#ifndef VMSVGA3D_DX
# error "This include file is for VMSVGA3D_DX."
#endif

#include <iprt/types.h>

#include "vmsvga_headers_begin.h"
#pragma pack(1) /* VMSVGA structures are '__packed'. */
#include <svga3d_reg.h>
#include <VGPU10ShaderTokens.h>
#pragma pack()
#include "vmsvga_headers_end.h"

typedef struct DXShaderInfo
{
    VGPU10_PROGRAM_TYPE enmProgramType;
    void *pvBytecode;
    uint32_t cbBytecode;
    uint32_t cInputSignature;
    uint32_t cOutputSignature;
    uint32_t cPatchConstantSignature;
    SVGA3dDXSignatureEntry aInputSignature[32];
    SVGA3dDXSignatureEntry aOutputSignature[32];
    SVGA3dDXSignatureEntry aPatchConstantSignature[32];
} DXShaderInfo;

int DXShaderParse(void const *pvCode, uint32_t cbCode, DXShaderInfo *pInfo);
void DXShaderFree(DXShaderInfo *pInfo);
int DXShaderCreateDXBC(DXShaderInfo const *pInfo, void **ppvDXBC, uint32_t *pcbDXBC);
char const *DXShaderGetOutputSemanticName(DXShaderInfo const *pInfo, uint32_t idxRegister);

#endif /* !VBOX_INCLUDED_SRC_Graphics_DevVGA_SVGA3d_dx_shader_h */
