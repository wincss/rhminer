/**
 * RandomHash source code implementation
 *
 * Copyright 2018 Polyminer1 <https://github.com/polyminer1>
 *
 * To the extent possible under law, the author(s) have dedicated all copyright
 * and related and neighboring rights to this software to the public domain
 * worldwide. This software is distributed without any warranty.
 *
 * You should have received a copy of the CC0 Public Domain Dedication along with
 * this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
 */
///
/// @file
/// @copyright Polyminer1

#pragma once
#include "MinersLib/Pascal/RandomHash_def.h"
#include "MinersLib/Pascal/RandomHash_MurMur3_32_def.h"
#include "MinersLib/Pascal/RandomHash_mersenne_twister.h"

#if defined(RANDOMHASH_CUDA)
    #include "cuda_helper.h"
    #include "cuda_device_runtime_api.h"
#endif


enum RandomHashAlgos
{
    RH_SHA2_256        = 0,
    RH_SHA2_384        = 1,
    RH_SHA2_512        = 2,
    RH_SHA3_256        = 3,
    RH_SHA3_384        = 4,
    RH_SHA3_512        = 5,
    RH_RIPEMD160       = 6,
    RH_RIPEMD256       = 7,
    RH_RIPEMD320       = 8,
    RH_Blake2b         = 9,
    RH_Blake2s         = 10,
    RH_Tiger2_5_192    = 11,
    RH_Snefru_8_256    = 12,
    RH_Grindahl512     = 13,
    RH_Haval_5_256     = 14,
    RH_MD5             = 15,
    RH_RadioGatun32    = 16,
    RH_Whirlpool       = 17
};

//------------------------------------------------------------------------------------
typedef U8* RH_StridePtr;
typedef U8* RH_StridePtrArray;

#ifdef RHMINER_DEBUG_STRIDE_INTEGRITY_CHECK
    #define RH_STRIDE_GET_INDEX(strideVar)              ((RH_StrideStruct*)(strideVar))->index
    #define RH_STRIDE_SET_INDEX(strideVar, val)         {((RH_StrideStruct*)(strideVar))->index = val;}
  #ifdef __CUDA_ARCH__
    #define RH_STRIDE_INIT_INTEGRITY(strideVar)         
    #define RH_STRIDE_CHECK_INTEGRITY(strideVar)        
  #else
    #define RH_STRIDE_INIT_INTEGRITY(strideVar)         {U64* ic = (U64*)((strideVar)+RH_IDEAL_ALIGNMENT+RH_STRIDE_GET_SIZE(strideVar)); *ic = (U64)0xAABBCCDDEEFF5577LLU;}
    #define RH_STRIDE_CHECK_INTEGRITY(strideVar)        {RH_ASSERT(*(U64*)((strideVar)+RH_IDEAL_ALIGNMENT+RH_STRIDE_GET_SIZE(strideVar)) == (U64)0xAABBCCDDEEFF5577LLU);}
  #endif
#else
    #define RH_STRIDE_GET_INDEX(strideVar)                  (0)
    #define RH_STRIDE_SET_INDEX(strideVar, val)             {}
    #define RH_STRIDE_INIT_INTEGRITY(strideVar)              {}
    #define RH_STRIDE_CHECK_INTEGRITY(strideVar)   {}
#endif

struct RH_StrideStruct
{
    U32 size;
    U32 index;
};

#define RH_STRIDE_GET_SIZE(strideVar)                   (*(U32*)(strideVar))
#define RH_STRIDE_SET_SIZE(strideVar, val)              {(*(U32*)(strideVar)) = (U32)(val);}

#define RH_STRIDE_RESET(strideVar)                       {RH_StrideStruct* p = reinterpret_cast<RH_StrideStruct*>(strideVar); p->size = (U32)0;}
#define RH_STRIDE_GET_DATA(strideVar)                    (((U8*)(strideVar)) + RH_IDEAL_ALIGNMENT)


#define RH_STRIDEARRAY_GET_SIZE(strideArrayVar)         (*(U32*)(strideArrayVar))
#define RH_STRIDEARRAY_SET_SIZE(strideArrayVar, val)    (*(U32*)(strideArrayVar)) = (val);
#define RH_STRIDEARRAY_GET_MAXSIZE(strideArrayVar)      reinterpret_cast<RH_StrideArrayStruct*>((void*)strideArrayVar)->maxSize
#define RH_STRIDEARRAY_GET_EXTRA(strideArrayVar, field) reinterpret_cast<RH_StrideArrayStruct*>((void*)strideArrayVar)->field
#define RH_STRIDEARRAY_RESET(strideArrayVar)            _CM(RH_StrideArrayReset)(strideArrayVar)
#define RH_STRIDEARRAY_GET(strideArrayVar, idx)         reinterpret_cast<RH_StrideArrayStruct*>((void*)strideArrayVar)->strides[idx]


//strideItrator is the current stride in the for-loop
#define RH_STRIDEARRAY_FOR_EACH_BEGIN(strideArrayVarSrc) \
    U32 cnt = RH_STRIDEARRAY_GET_SIZE(strideArrayVarSrc); \
    RH_ASSERT(RH_STRIDEARRAY_GET_SIZE(strideArrayVarSrc) <= RH_StrideArrayCount); \
    RH_ASSERT(RH_STRIDEARRAY_GET_SIZE(strideArrayVarSrc) <= RH_STRIDEARRAY_GET_MAXSIZE(strideArrayVarSrc)); \
    U8** endStridePtr = ((RH_StrideArrayStruct*)strideArrayVarSrc)->strides + cnt; \
    U8** stridePtr = ((RH_StrideArrayStruct*)strideArrayVarSrc)->strides; \
    while (stridePtr != endStridePtr)  \
    { \
        U8* strideItrator = *stridePtr; \

#define RH_STRIDEARRAY_FOR_EACH_END(strideArrayVarSrc) \
        stridePtr++; \
    } \

#ifdef RH_ENABLE_OPTIM_STRIDE_ARRAY_MURMUR3
    #define RH_STRIDEARRAY_PUSHBACK(strideArrayVar, stride)                     \
    {                                                                           \
        U32 _as = RH_STRIDEARRAY_GET_SIZE(strideArrayVar)++;                    \
        RH_ASSERT(_as < RH_STRIDEARRAY_GET_MAXSIZE(strideArrayVar));            \
        ((RH_StrideArrayStruct*)(strideArrayVar))->strides[_as] = (stride);     \
    }

#else
    #define RH_STRIDEARRAY_PUSHBACK(strideArrayVar, stride)                  \
    {                                                                            \
        U32 _as = RH_STRIDEARRAY_GET_SIZE(strideArrayVar)++;                     \
        RH_ASSERT(_as < RH_STRIDEARRAY_GET_MAXSIZE(strideArrayVar));             \
        ((RH_StrideArrayStruct*)strideArrayVar)->strides[_as] = (stride);       \
    }

#endif


#ifndef RANDOMHASH_CUDA

#else //!CPU

#define RH_INPLACE_MEMCPY_128(pDst, pSrc, byteCount)                    \
    {U8* end = pDst + byteCount;                                        \
    while(pDst < end)                                                   \
    {                                                                   \
        *(uint4 *)pDst = *(uint4 *)pSrc;                                \
        pDst += 16;                                                     \
        pSrc += 16;                                                     \
    }}


#endif //CPU


#define RH_STRIDE_COPY(dstStride, srcStride)                                                 \
{                                                                                            \
    U32 _ss = RH_STRIDE_GET_SIZE(srcStride);                                                 \
    RH_STRIDE_CHECK_INTEGRITY(srcStride);                                                    \
    _CM(RH_STRIDE_MEMCPY_ALIGNED_SIZE128)(dstStride, srcStride, _ss + RH_IDEAL_ALIGNMENT);   \
    RH_STRIDE_CHECK_INTEGRITY(dstStride);                                                    \
}
/*
#ifdef RH_ENABLE_OPTIM_STRIDE_ARRAY_MURMUR3
#define RH_STRIDE_COPY_ACCUM(dstStride, srcStride, accum)                                                       \
{                                                                                                               \
    U32 _ss = RH_STRIDE_GET_SIZE(srcStride);                                                                    \
    RH_STRIDE_SET_SIZE(dstStride, _ss);                                                                         \
    RH_STRIDE_CHECK_INTEGRITY(srcStride);                                                                       \
    _CM(RH_INPLACE_MEMCPY_128_A)(RH_STRIDE_GET_DATA(dstStride), RH_STRIDE_GET_DATA(srcStride), _ss, accum);     \
    RH_STRIDE_CHECK_INTEGRITY(dstStride);                                                                       \
}
#endif
*/

struct RH_StrideArrayStruct
{
    U32 size;
    U32 maxSize;
    U64 memoryboost;
    U64 supportsse41;
    U64 sseoptimization;
    MurmurHash3_x86_32_State accum;
    U8  dummy2[(RH_IDEAL_ALIGNMENT / 2) - sizeof(MurmurHash3_x86_32_State)];
#ifdef RHMINER_DEBUG_STRIDE_INTEGRITY_CHECK
    U8* strides[RH_StrideArrayCount + 1];
#else
    U8* strides[RH_StrideArrayCount];
#endif
};
#define RH_StrideArrayStruct_GetAccum(strideArray) (&((RH_StrideArrayStruct*)strideArray)->accum)


//common for the 3 RIPEMD
PLATFORM_CONST uint32_t RH_RIPEMD_C1 = 0x50A28BE6;
PLATFORM_CONST uint32_t RH_RIPEMD_C2 = 0x5A827999;
PLATFORM_CONST uint32_t RH_RIPEMD_C3 = 0x5C4DD124;
PLATFORM_CONST uint32_t RH_RIPEMD_C4 = 0x6ED9EBA1;
PLATFORM_CONST uint32_t RH_RIPEMD_C5 = 0x6D703EF3;
PLATFORM_CONST uint32_t RH_RIPEMD_C6 = 0x8F1BBCDC;
PLATFORM_CONST uint32_t RH_RIPEMD_C7 = 0x7A6D76E9;
PLATFORM_CONST uint32_t RH_RIPEMD_C8 = 0xA953FD4E;
