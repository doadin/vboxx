/* $Id$ */
/** @file
 * Audio testcase - Mixing buffer.
 */

/*
 * Copyright (C) 2014-2015 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */


/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include <iprt/err.h>
#include <iprt/initterm.h>
#include <iprt/mem.h>
#include <iprt/rand.h>
#include <iprt/stream.h>
#include <iprt/string.h>
#include <iprt/test.h>


#include "../AudioMixBuffer.h"
#include "../DrvAudio.h"


/*******************************************************************************
*   Structures and Typedefs                                                    *
*******************************************************************************/

static int tstSingle(RTTEST hTest)
{
    RTTestSubF(hTest, "Single buffer");

    PDMAUDIOSTREAMCFG config =
    {
        44100,                   /* Hz */
        2                        /* Channels */,
        AUD_FMT_S16              /* Format */,
        PDMAUDIOENDIANNESS_LITTLE /* ENDIANNESS */
    };
    PDMPCMPROPS props;

    int rc = drvAudioStreamCfgToProps(&config, &props);
    AssertRC(rc);

    uint32_t cBufSize = _1K;

    /*
     * General stuff.
     */
    PDMAUDIOMIXBUF mb;
    RTTESTI_CHECK_RC_OK(audioMixBufInit(&mb, "Single", &props, cBufSize));
    RTTESTI_CHECK(audioMixBufSize(&mb) == cBufSize);
    RTTESTI_CHECK(AUDIOMIXBUF_B2S(&mb, audioMixBufSizeBytes(&mb)) == cBufSize);
    RTTESTI_CHECK(AUDIOMIXBUF_S2B(&mb, audioMixBufSize(&mb)) == audioMixBufSizeBytes(&mb));
    RTTESTI_CHECK(audioMixBufFree(&mb) == cBufSize);
    RTTESTI_CHECK(AUDIOMIXBUF_S2B(&mb, audioMixBufFree(&mb)) == audioMixBufFreeBytes(&mb));

    /*
     * Absolute writes.
     */
    uint32_t read  = 0, written = 0, written_abs = 0;
    int8_t  samples8 [2] = { 0x12, 0x34 };
    int16_t samples16[2] = { 0xAA, 0xBB };
    int32_t samples32[2] = { 0xCC, 0xDD };
    int64_t samples64[2] = { 0xEE, 0xFF };

    RTTESTI_CHECK_RC_OK(audioMixBufWriteAt(&mb, 0, &samples8, sizeof(samples8), &written));
    RTTESTI_CHECK(written == 0 /* Samples */);

    RTTESTI_CHECK_RC_OK(audioMixBufWriteAt(&mb, 0, &samples16, sizeof(samples16), &written));
    RTTESTI_CHECK(written == 1 /* Samples */);

    RTTESTI_CHECK_RC_OK(audioMixBufWriteAt(&mb, 2, &samples32, sizeof(samples32), &written));
    RTTESTI_CHECK(written == 2 /* Samples */);
    written_abs = 0;

    /* Beyond buffer. */
    RTTESTI_CHECK_RC(audioMixBufWriteAt(&mb, audioMixBufSize(&mb) + 1, &samples16, sizeof(samples16),
                                        &written), VERR_BUFFER_OVERFLOW);

    /*
     * Circular writes.
     */
    uint32_t cToWrite = audioMixBufSize(&mb) - written_abs - 1; /* -1 as padding plus -2 samples for above. */
    for (uint32_t i = 0; i < cToWrite; i++)
    {
        RTTESTI_CHECK_RC_OK(audioMixBufWriteCirc(&mb, &samples16, sizeof(samples16), &written));
        RTTESTI_CHECK(written == 1);
    }
    RTTESTI_CHECK(!audioMixBufIsEmpty(&mb));
    RTTESTI_CHECK(audioMixBufFree(&mb) == 1);
    RTTESTI_CHECK(audioMixBufFreeBytes(&mb) == AUDIOMIXBUF_S2B(&mb, 1U));
    RTTESTI_CHECK(audioMixBufProcessed(&mb) == cToWrite + written_abs /* + last absolute write */);

    RTTESTI_CHECK_RC_OK(audioMixBufWriteCirc(&mb, &samples16, sizeof(samples16), &written));
    RTTESTI_CHECK(written == 1);
    RTTESTI_CHECK(audioMixBufFree(&mb) == 0);
    RTTESTI_CHECK(audioMixBufFreeBytes(&mb) == AUDIOMIXBUF_S2B(&mb, 0));
    RTTESTI_CHECK(audioMixBufProcessed(&mb) == cBufSize);

    /* Circular reads. */
    uint32_t cToRead = audioMixBufSize(&mb) - written_abs - 1;
    for (uint32_t i = 0; i < cToWrite; i++)
    {
        RTTESTI_CHECK_RC_OK(audioMixBufReadCirc(&mb, &samples16, sizeof(samples16), &read));
        RTTESTI_CHECK(read == 1);
        audioMixBufFinish(&mb, read);
    }
    RTTESTI_CHECK(!audioMixBufIsEmpty(&mb));
    RTTESTI_CHECK(audioMixBufFree(&mb) == audioMixBufSize(&mb) - written_abs - 1);
    RTTESTI_CHECK(audioMixBufFreeBytes(&mb) == AUDIOMIXBUF_S2B(&mb, cBufSize - written_abs - 1));
    RTTESTI_CHECK(audioMixBufProcessed(&mb) == cBufSize - cToRead + written_abs);

    RTTESTI_CHECK_RC_OK(audioMixBufReadCirc(&mb, &samples16, sizeof(samples16), &read));
    RTTESTI_CHECK(read == 1);
    audioMixBufFinish(&mb, read);
    RTTESTI_CHECK(audioMixBufFree(&mb) == cBufSize - written_abs);
    RTTESTI_CHECK(audioMixBufFreeBytes(&mb) == AUDIOMIXBUF_S2B(&mb, cBufSize - written_abs));
    RTTESTI_CHECK(audioMixBufProcessed(&mb) == written_abs);

    audioMixBufDestroy(&mb);

    return RTTestSubErrorCount(hTest) ? VERR_GENERAL_FAILURE : VINF_SUCCESS;
}

static int tstParentChild(RTTEST hTest)
{
    RTTestSubF(hTest, "2 Children -> Parent");

    uint32_t cBufSize = _1K;

    PDMAUDIOSTREAMCFG cfg_p =
    {
        44100,                   /* Hz */
        2                        /* Channels */,
        AUD_FMT_S16              /* Format */,
        PDMAUDIOENDIANNESS_LITTLE /* ENDIANNESS */
    };
    PDMPCMPROPS props;

    int rc = drvAudioStreamCfgToProps(&cfg_p, &props);
    AssertRC(rc);

    PDMAUDIOMIXBUF parent;
    RTTESTI_CHECK_RC_OK(audioMixBufInit(&parent, "Parent", &props, cBufSize));

    PDMAUDIOSTREAMCFG cfg_c1 = /* Upmixing to parent */
    {
        22100,                   /* Hz */
        2                        /* Channels */,
        AUD_FMT_S16              /* Format */,
        PDMAUDIOENDIANNESS_LITTLE /* ENDIANNESS */
    };

    rc = drvAudioStreamCfgToProps(&cfg_c1, &props);
    AssertRC(rc);

    PDMAUDIOMIXBUF child1;
    RTTESTI_CHECK_RC_OK(audioMixBufInit(&child1, "Child1", &props, cBufSize));
    RTTESTI_CHECK_RC_OK(audioMixBufLinkTo(&child1, &parent));

    PDMAUDIOSTREAMCFG cfg_c2 = /* Downmixing to parent */
    {
        48000,                   /* Hz */
        2                        /* Channels */,
        AUD_FMT_S16              /* Format */,
        PDMAUDIOENDIANNESS_LITTLE /* ENDIANNESS */
    };

    rc = drvAudioStreamCfgToProps(&cfg_c2, &props);
    AssertRC(rc);

    PDMAUDIOMIXBUF child2;
    RTTESTI_CHECK_RC_OK(audioMixBufInit(&child2, "Child2", &props, cBufSize));
    RTTESTI_CHECK_RC_OK(audioMixBufLinkTo(&child2, &parent));

    /*
     * Writing + mixing from child/children -> parent, sequential.
     */
    uint32_t cbBuf = _1K;
    char pvBuf[_1K];
    int16_t samples[32] = { 0xAA, 0xBB };
    uint32_t read , written, mixed, temp;

    uint32_t cChild1Free     = cBufSize;
    uint32_t cChild1Mixed    = 0;
    uint32_t cSamplesParent1 = 16;
    uint32_t cSamplesChild1  = 16;

    uint32_t cChild2Free     = cBufSize;
    uint32_t cChild2Mixed    = 0;
    uint32_t cSamplesParent2 = 16;
    uint32_t cSamplesChild2  = 16;

    uint32_t t = RTRandU32() % 64;

    for (uint32_t i = 0; i < t; i++)
    {
        RTTestPrintf(hTest, RTTESTLVL_DEBUG, "i=%RU32\n", i);
        RTTESTI_CHECK_RC_OK_BREAK(audioMixBufWriteAt(&child1, 0, &samples, sizeof(samples), &written));
        RTTESTI_CHECK_MSG_BREAK(written == cSamplesChild1, ("Child1: Expected %RU32 written samples, got %RU32\n", cSamplesChild1, written));
        RTTESTI_CHECK_RC_OK_BREAK(audioMixBufMixToParent(&child1, written, &mixed));
        temp = audioMixBufProcessed(&parent) - audioMixBufMixed(&child2);
        RTTESTI_CHECK_MSG_BREAK(audioMixBufMixed(&child1) == temp, ("Child1: Expected %RU32 mixed samples, got %RU32\n", audioMixBufMixed(&child1), temp));

        RTTESTI_CHECK_RC_OK_BREAK(audioMixBufWriteAt(&child2, 0, &samples, sizeof(samples), &written));
        RTTESTI_CHECK_MSG_BREAK(written == cSamplesChild2, ("Child2: Expected %RU32 written samples, got %RU32\n", cSamplesChild2, written));
        RTTESTI_CHECK_RC_OK_BREAK(audioMixBufMixToParent(&child2, written, &mixed));
        temp = audioMixBufProcessed(&parent) - audioMixBufMixed(&child1);
        RTTESTI_CHECK_MSG_BREAK(audioMixBufMixed(&child2) == temp, ("Child2: Expected %RU32 mixed samples, got %RU32\n", audioMixBufMixed(&child2), temp));
    }

    RTTESTI_CHECK(audioMixBufProcessed(&parent) == audioMixBufMixed(&child1) + audioMixBufMixed(&child2));

    for (;;)
    {
        RTTESTI_CHECK_RC_OK_BREAK(audioMixBufReadCirc(&parent, pvBuf, cbBuf, &read));
        if (!read)
            break;
        audioMixBufFinish(&parent, read);
    }

    RTTESTI_CHECK(audioMixBufProcessed(&parent) == 0);
    RTTESTI_CHECK(audioMixBufMixed(&child1) == 0);
    RTTESTI_CHECK(audioMixBufMixed(&child2) == 0);

    audioMixBufDestroy(&parent);
    audioMixBufDestroy(&child1);
    audioMixBufDestroy(&child2);

    return RTTestSubErrorCount(hTest) ? VERR_GENERAL_FAILURE : VINF_SUCCESS;
}

static int tstConversion(RTTEST hTest)
{
    unsigned        i;
    uint32_t        cBufSize = 256;
    PDMPCMPROPS     props;


    RTTestSubF(hTest, "Sample conversion");

    PDMAUDIOSTREAMCFG cfg_p =
    {
        44100,                   /* Hz */
        1                        /* Channels */,
        AUD_FMT_S16              /* Format */,
        PDMAUDIOENDIANNESS_LITTLE /* ENDIANNESS */
    };

    int rc = drvAudioStreamCfgToProps(&cfg_p, &props);
    AssertRC(rc);

    PDMAUDIOMIXBUF parent;
    RTTESTI_CHECK_RC_OK(audioMixBufInit(&parent, "Parent", &props, cBufSize));

    /* Child uses half the sample rate; that ensures the mixing engine can't
     * take shortcuts and performs conversion. Because conversion to double
     * the sample rate effectively inserts one additional sample between every
     * two source samples, N source samples will be converted to N * 2 - 1
     * samples. However, the last source sample will be saved for later
     * interpolation and not immediately output.
     */
    PDMAUDIOSTREAMCFG cfg_c =   /* Upmixing to parent */
    {
        22050,                   /* Hz */
        1                        /* Channels */,
        AUD_FMT_S16              /* Format */,
        PDMAUDIOENDIANNESS_LITTLE /* ENDIANNESS */
    };

    rc = drvAudioStreamCfgToProps(&cfg_c, &props);
    AssertRC(rc);

    PDMAUDIOMIXBUF child;
    RTTESTI_CHECK_RC_OK(audioMixBufInit(&child, "Child", &props, cBufSize));
    RTTESTI_CHECK_RC_OK(audioMixBufLinkTo(&child, &parent));

    /*
     * Writing + mixing from child -> parent, sequential.
     */
    uint32_t cbBuf = 256;
    char pvBuf[256];
    int16_t samples[16] = { 0xAA, 0xBB, INT16_MIN, INT16_MIN + 1, INT16_MIN / 2, -3, -2, -1,
                            0, 1, 2, 3, INT16_MAX / 2, INT16_MAX - 1, INT16_MAX, 0 };
    uint32_t read, written, mixed, temp;

    uint32_t cChildFree     = cBufSize;
    uint32_t cChildMixed    = 0;
    uint32_t cSamplesChild  = 16;
    uint32_t cSamplesParent = cSamplesChild * 2 - 2;
    uint32_t cSamplesRead   = 0;

    RTTestPrintf(hTest, RTTESTLVL_DEBUG, "Conversion test %uHz %uch\n", cfg_c.uHz, cfg_c.cChannels);
    RTTESTI_CHECK_RC_OK(audioMixBufWriteAt(&child, 0, &samples, sizeof(samples), &written));
    RTTESTI_CHECK_MSG(written == cSamplesChild, ("Child: Expected %RU32 written samples, got %RU32\n", cSamplesChild, written));
    RTTESTI_CHECK_RC_OK(audioMixBufMixToParent(&child, written, &mixed));
    temp = audioMixBufProcessed(&parent);
    RTTESTI_CHECK_MSG(audioMixBufMixed(&child) == temp, ("Child: Expected %RU32 mixed samples, got %RU32\n", audioMixBufMixed(&child), temp));

    RTTESTI_CHECK(audioMixBufProcessed(&parent) == audioMixBufMixed(&child));

    for (;;)
    {
        RTTESTI_CHECK_RC_OK_BREAK(audioMixBufReadCirc(&parent, pvBuf, cbBuf, &read));
        if (!read)
            break;
        cSamplesRead += read;
        audioMixBufFinish(&parent, read);
    }
    RTTESTI_CHECK_MSG(cSamplesRead == cSamplesParent, ("Parent: Expected %RU32 mixed samples, got %RU32\n", cSamplesParent, cSamplesRead));

    /* Check that the samples came out unharmed. Every other sample is interpolated and we ignore it. */
    int16_t *pSrc16 = &samples[0];
    int16_t *pDst16 = (int16_t *)pvBuf;

    for (i = 0; i < cSamplesChild - 1; ++i)
    {
        RTTESTI_CHECK_MSG(*pSrc16 == *pDst16, ("index %u: Dst=%d, Src=%d\n", i, *pDst16, *pSrc16));
        pSrc16 += 1;
        pDst16 += 2;
    }

    RTTESTI_CHECK(audioMixBufProcessed(&parent) == 0);
    RTTESTI_CHECK(audioMixBufMixed(&child) == 0);
    
    audioMixBufDestroy(&parent);
    audioMixBufDestroy(&child);

    return RTTestSubErrorCount(hTest) ? VERR_GENERAL_FAILURE : VINF_SUCCESS;
}

int main(int argc, char **argv)
{
    RTR3InitExe(argc, &argv, 0);

    /*
     * Initialize IPRT and create the test.
     */
    RTTEST hTest;
    int rc = RTTestInitAndCreate("tstAudioMixBuffer", &hTest);
    if (rc)
        return rc;
    RTTestBanner(hTest);

    rc = tstSingle(hTest);
    if (RT_SUCCESS(rc))
        rc = tstParentChild(hTest);
    if (RT_SUCCESS(rc))
        rc = tstConversion(hTest);

    /*
     * Summary
     */
    return RTTestSummaryAndDestroy(hTest);
}
