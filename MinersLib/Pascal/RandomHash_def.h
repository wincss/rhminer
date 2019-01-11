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

#include "corelib/basetypes.h"
#include "MinersLib/Pascal/PascalCommon.h"

#ifndef __CUDA_ARCH__
  #ifdef RANDOMHASH_CUDA
    #include <emmintrin.h>
  #else
    #include <immintrin.h>
  #endif

#endif

#define RH_DISABLE_RH_ASSERTS
#define RH_ENABLE_OPTIM_STRIDE_ARRAY_MURMUR3
//#define RH_ENABLE_OPTIM_EXPAND_ACCUM8

#ifdef RHMINER_DEBUG
    #undef RH_DISABLE_RH_ASSERTS
    #define RHMINER_DEBUG_STRIDE_INTEGRITY_CHECK
#endif


#ifdef RANDOMHASH_CUDA
    //Cache re-use does not work on cuda. Kernels gets out of sync and driver fails.
    #define RH_DISABLE_NONCE_REUSE_AND_CACHE

    #define RH_TOTAL_STRIDES_INSTANCES ((RH_StrideArrayCount+1))
#else
    #define RH_TOTAL_STRIDES_INSTANCES (RH_StrideArrayCount+1)
#endif

//#define RH_STRIDE_BANK_SIZE (RH_StrideSize * RH_TOTAL_STRIDES_INSTANCES)
#define RH_STRIDE_BANK_SIZE 5033984

#define RH_GET_MEMBER_POS(STRUCT, MEMBER)  (size_t)(void*)&((STRUCT*)0)->MEMBER

#define copy4(dst, src) {(dst)[0] = (src)[0]; \
                         (dst)[1] = (src)[1]; \
                         (dst)[2] = (src)[2]; \
                         (dst)[3] = (src)[3];}
                               
#define copy4_op(dst, src, op) {(dst)[0] = op((src)[0]); \
                                (dst)[1] = op((src)[1]); \
                                (dst)[2] = op((src)[2]); \
                                (dst)[3] = op((src)[3]);}

#define copy8(dst, src) {(dst)[0] = (src)[0]; \
                         (dst)[1] = (src)[1]; \
                         (dst)[2] = (src)[2]; \
                         (dst)[3] = (src)[3]; \
                         (dst)[4] = (src)[4]; \
                         (dst)[5] = (src)[5]; \
                         (dst)[6] = (src)[6]; \
                         (dst)[7] = (src)[7];}

#define copy8_op(dst, src, op) {(dst)[0] = op((src)[0]); \
                                (dst)[1] = op((src)[1]); \
                                (dst)[2] = op((src)[2]); \
                                (dst)[3] = op((src)[3]); \
                                (dst)[4] = op((src)[4]); \
                                (dst)[5] = op((src)[5]); \
                                (dst)[6] = op((src)[6]); \
                                (dst)[7] = op((src)[7]);}

#define copy6_op(dst, src, op) {(dst)[0] = op((src)[0]); \
                                (dst)[1] = op((src)[1]); \
                                (dst)[2] = op((src)[2]); \
                                (dst)[3] = op((src)[3]); \
                                (dst)[4] = op((src)[4]); \
                                (dst)[5] = op((src)[5]);} \


#ifdef RHMINER_PLATFORM_CPU

template<unsigned i>
inline U32 _mm_extract_epi32_( __m128i V)
{
    V = _mm_shuffle_epi32(V, _MM_SHUFFLE(i, i, i, i));
    return (U32)_mm_cvtsi128_si32(V);
}

#define RH_MM_LOAD128           _mm_loadu_si128
#define RH_MM_STORE128          _mm_storeu_si128
#define TH_MM_STREAM_STORE128   _mm_storeu_si128
#define RH_MM_BARRIER           void

#ifndef RotateLeft8
#define _rotr8(x,n)	    ((U8)(((x) >> (n)) | ((x) << (8 - (n)))))
#define _rotl8(x,n)	    ((U8)(((x) << (n)) | ((x) >> (8 - (n)))))
#endif

#ifndef RotateLeft32
#define _rotr(x,n)	    ((((x) >> (n)) | ((x) << (32 - (n)))))
#define _rotl(x,n)	    ((((x) << (n)) | ((x) >> (32 - (n)))))
#define _rotr64(x,n)	(((x) >> (n)) | ((x) << (64 - (n))))
#define _rotl64(x,n)	(((x) << (n)) | ((x) >> (64 - (n))))
#endif


#endif //CPU


#ifndef RHMINER_PLATFORM_GPU // CPU
    #define RH_DECL_HOST
    #define RH_DECL_DEVICE 
    #define RH_DECL_HOST_DEVICE 
    #define RH_DECL_FORCEINLINE inline    
    #define RH_DEVICE_BARRIER() 
    #define RH_CUDA_ERROR_CHECK() 
    
    #define ROTR32(x,y) _rotr((x),(y))
    #define ROTL8(x,y)  _rotl8((x),(U8)((y) % 8))
    #define ROTR8(x,y)  _rotr8((x),(U8)((y) % 8))
    #define ROTL32(x,y)	_rotl((x),(y))
    #define ROTL64(x,y)	_rotl64((x),(y))
    #define ROTR64(x,y)	_rotr64((x),(y))
    #define ROTL_epi32(m_tmp, m, count) {\
            m_tmp = m; \
            m = _mm_slli_epi32(m,count); \
            m_tmp = _mm_srli_epi32(m_tmp,(32-count)); \
            m = _mm_or_si128(m,m_tmp);}


    #define BIG_CONSTANT(x) (x)
    #define KERNEL_LOG(...) PrintOutCritical(__VA_ARGS__)
    #define KERNEL0_LOG(...) PrintOutCritical(__VA_ARGS__)

    #define KERNEL_GET_GID() 0

#elif defined(RHMINER_PLATFORM_CUDA)
#ifdef __CUDA_ARCH__
    #define KERNEL0_LOG(...) {if (KERNEL_GET_GID() == 0) {printf(__VA_ARGS__);}}
    #define RH_DEVICE_BARRIER() 
    #define RH_CUDA_ERROR_CHECK()
#else
    #define RH_CUDA_ERROR_CHECK() \
    {  \
        cudaError_t err = ::cudaGetLastError(); \
        if (err != cudaSuccess) \
            PrintOut("CUDA Last Error %d %s\n", err, cudaGetErrorString(err)); \
    }
    #define KERNEL0_LOG(...) printf(__VA_ARGS__)
    #define RH_DEVICE_BARRIER() CUDA_SAFE_CALL(cudaDeviceSynchronize())
#endif

    #define KERNEL_LOG(...) printf(__VA_ARGS__)

    #define RH_DECL_HOST
    #define RH_DECL_DEVICE __device__ 
    #define RH_DECL_HOST_DEVICE __host__ __device__ 
    #define RH_DECL_FORCEINLINE __forceinline__
    #define KERNEL_GET_GID() (blockIdx.x * blockDim.x + threadIdx.x)

    #include "cuda_helper.h"

    #define ROTL8(x,y)  cuROTL8((x),(uint8_t)((y) % 8))
    #define ROTR8(x,y)  cuROTR8((x),(uint8_t)((y) % 8))

#elif defined(RHMINER_PLATFORM_OPENCL)
    #define KERNEL_LOG(...) printf(__VA_ARGS__);
    #define KERNEL0_LOG(...) {if (KERNEL_GET_GID() == 0) {printf(__VA_ARGS__);}}

    #include "cuda_helper.h"
    #define ROTR64(x, y)  (((x) >> (y)) ^ ((x) << (64 - (y))))
    
    #define KERNEL_GET_GID() (globalIndex.x)

    inline uint32_t rotl32 ( uint32_t x, int8_t r )
    {
      return (x << r) | (x >> (32 - r));
    }

    inline uint64_t rotl64 ( uint64_t x, int8_t r )
    {
      return (x << r) | (x >> (64 - r));
    }

    #define ROTL8(x,y)  rotl8((x),(U8)((y) % 8)));
    #define	ROTL32(x,y)	rotl32((x),(y))
    #define ROTL64(x,y)	rotl64((x),(y))

    #define BIG_CONSTANT(x) (x##LLU)
#endif


//--------------------------------------------------------------
#define RH_memzero_8(ptr, size)             \
    {                                       \
        RH_ASSERT((size_t(ptr) % 8) == 0);  \
        RH_ASSERT((size) == 8);             \
        U64* buf = static_cast<U64*>((void*)(ptr));               \
        *buf = 0;                           \
    }

#if !defined(RANDOMHASH_CUDA)
    #define RH_memzero_16(ptr, size)            \
    {                                           \
        RH_ASSERT((size_t(ptr) % 32) == 0);     \
        RH_ASSERT((size) == 16);                \
        __m128i xmm = _mm_set1_epi8((char)0);   \
        RH_MM_STORE128((__m128i *)ptr, xmm);    \
    }

    #define RH_memzero_32(ptr, size)  \
    { \
        RH_ASSERT((size_t(ptr) % 32) == 0); \
        RH_ASSERT((size) == 32); \
        __m128i xmm = _mm_set1_epi8((char)0); \
        RH_MM_STORE128(((__m128i *)ptr)+0, xmm); \
        RH_MM_STORE128(((__m128i *)ptr)+1, xmm); \
    }

    #define RH_memzero_64(ptr, size)  \
    { \
        RH_ASSERT((size_t(ptr) % 32) == 0); \
        RH_ASSERT(size == 64); \
        __m128i xmm = _mm_set1_epi8((char)0); \
        RH_MM_STORE128(((__m128i *)ptr)+0, xmm); \
        RH_MM_STORE128(((__m128i *)ptr)+1, xmm); \
        RH_MM_STORE128(((__m128i *)ptr)+2, xmm); \
        RH_MM_STORE128(((__m128i *)ptr)+3, xmm); \
    }
    

    #define RH_memzero_of16(ptr, s)              \
    {                                            \
        RH_ASSERT((size_t(ptr) % 32) == 0);      \
        RH_ASSERT((s % 16) == 0);             \
        __m128i xmm = _mm_set1_epi8((char)0);    \
        __m128i * buf = (__m128i *)ptr;          \
        size_t size = s / 16;                    \
        while (size)                             \
        {                                        \
            RH_MM_STORE128(buf, xmm);            \
            buf++;                               \
            size--;                              \
        }                                        \
    }

#else
    #define RH_memzero_16(ptr, size) \
    { \
        RH_ASSERT((size_t(ptr) % 32) == 0);  \
        RH_ASSERT((size) == 16);  \
        ((U64*)ptr)[0] = 0; \
        ((U64*)ptr)[1] = 0; \
    }

    #define RH_memzero_32(ptr, size)  \
    { \
        RH_ASSERT((size_t(ptr) % 32) == 0); \
        RH_ASSERT((size) == 32); \
        ((U64*)ptr)[0] = 0; \
        ((U64*)ptr)[1] = 0; \
        ((U64*)ptr)[2] = 0; \
        ((U64*)ptr)[3] = 0; \
    }

    #define RH_memzero_64(ptr, size)  \
    { \
        RH_ASSERT((size_t(ptr) % 32) == 0); \
        RH_ASSERT(size == 64); \
        ((U64*)ptr)[0] = 0; \
        ((U64*)ptr)[1] = 0; \
        ((U64*)ptr)[2] = 0; \
        ((U64*)ptr)[3] = 0; \
        ((U64*)ptr)[4] = 0; \
        ((U64*)ptr)[5] = 0; \
        ((U64*)ptr)[6] = 0; \
        ((U64*)ptr)[7] = 0; \
    }


    #define RH_memzero_of16 RH_memzero_of8 

#endif

    #define RH_memzero_of8(ptr, s)   \
    {                                \
        RH_ASSERT((size_t(ptr) % 32) == 0); \
        RH_ASSERT((s % 8) == 0); \
        U64* buf = (U64*)ptr;        \
        size_t size = s / 8;         \
        while (size)                 \
        {                            \
            *buf = 0;                \
            buf++;                   \
            size--;                  \
        }                            \
    }


//--------------------------------------------------------------


#define RHMINER_T64(x)    ((x) & uint64_t(0xFFFFFFFFFFFFFFFF))

RH_DECL_DEVICE
RH_DECL_FORCEINLINE
void ReadUInt64AsBytesLE(const uint64_t a_in, uint8_t* a_out)
{
	a_out[0] = (uint8_t)a_in;
	a_out[0 + 1] = (uint8_t)(a_in >> 8);
	a_out[0 + 2] = (uint8_t)(a_in >> 16);
	a_out[0 + 3] = (uint8_t)(a_in >> 24);
	a_out[0 + 4] = (uint8_t)(a_in >> 32);
	a_out[0 + 5] = (uint8_t)(a_in >> 40);
	a_out[0 + 6] = (uint8_t)(a_in >> 48);
	a_out[0 + 7] = (uint8_t)(a_in >> 56);
}

RH_DECL_DEVICE
RH_DECL_FORCEINLINE
uint64_t ReverseBytesUInt64(const uint64_t value)
{
	return  (value & uint64_t(0x00000000000000FF)) << 56 |
		    (value & uint64_t(0x000000000000FF00)) << 40 |
		    (value & uint64_t(0x0000000000FF0000)) << 24 |
		    (value & uint64_t(0x00000000FF000000)) << 8 |
		    (value & uint64_t(0x000000FF00000000)) >> 8 |
		    (value & uint64_t(0x0000FF0000000000)) >> 24 |
		    (value & uint64_t(0x00FF000000000000)) >> 40 |
		    (value & uint64_t(0xFF00000000000000)) >> 56;
}

RH_DECL_DEVICE
RH_DECL_FORCEINLINE
uint32_t ReverseBytesUInt32(const uint32_t value)
{
	return (value & uint32_t(0x000000FF)) << 24 | 
		   (value & uint32_t(0x0000FF00)) << 8 | 
		   (value & uint32_t(0x00FF0000)) >> 8 |
		   (value & uint32_t(0xFF000000)) >> 24;
}

#define RandomHash_MD_BASE_MAIN_LOOP(ALGO_BLOCK_SIZE, ALGO_FUNC, BE_LE_CONVERT)      { \
    int32_t len = (int32_t)RH_STRIDE_GET_SIZE(roundInput);                             \
    uint32_t blockCount = len / ALGO_BLOCK_SIZE;                                       \
    uint32_t *dataPtr = (uint32_t *)RH_STRIDE_GET_DATA(roundInput);                    \
    uint64_t bits = len * 8;                                                           \
    while(blockCount > 0)                                                              \
    {                                                                                  \
        ALGO_FUNC(dataPtr, state);                                                     \
        len -= ALGO_BLOCK_SIZE;                                                        \
        dataPtr += ALGO_BLOCK_SIZE / 4;                                                \
        blockCount--;                                                                  \
    }                                                                                  \
    {                                                                                  \
		int32_t padindex;                                                              \
        RH_ALIGN(64) uint8_t pad[80];                                                  \
		                                                                               \
		if (len < 56)                                                                  \
			padindex = 56 - len;                                                       \
		else                                                                           \
			padindex = 120 - len;                                                      \
                                                                                       \
        RH_memzero_of16(pad, sizeof(pad));                                              \
		pad[0] = 0x80;                                                                 \
        bits = BE_LE_CONVERT(bits);                                                    \
		ReadUInt64AsBytesLE(bits, pad+padindex);                                       \
                                                                                       \
		padindex = padindex + 8;                                                       \
        memcpy(((uint8_t*)dataPtr) + len, pad, padindex);                              \
        RH_ASSERT(padindex <= 72);                                                   \
        RH_ASSERT(((padindex + len) % ALGO_BLOCK_SIZE)==0);                          \
                                                                                       \
		ALGO_FUNC(dataPtr, state);                                                     \
        padindex -= ALGO_BLOCK_SIZE;                                                   \
        if (padindex > 0)                                                              \
            ALGO_FUNC(dataPtr+(ALGO_BLOCK_SIZE/4), state);                             \
        RH_ASSERT(padindex > -ALGO_BLOCK_SIZE);                                      \
    } }

//---------------------------------------------------------------------------------

#define RH_ENABLE_ASSERT
#ifdef RH_ENABLE_ASSERT
#define RH_ASSERT RHMINER_ASSERT
#else
#define RH_ASSERT(...) 
#endif

#if defined(RH_DISABLE_RH_ASSERTS)
#undef RH_ASSERT
#define RH_ASSERT(...) 
#endif

#ifdef RHMINER_DEBUG_DEV
  #ifdef RHMINER_DEBUG
    #define RH_DISABLE_RH_ASSERTS
  #endif
  #include "MinersLib/Pascal/RandomHash_DEV_TESTING.h"
#endif
