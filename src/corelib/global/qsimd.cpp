// Copyright (C) 2021 The Qt Company Ltd.
// Copyright (C) 2022 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

// we need ICC to define the prototype for _rdseed64_step
#define __INTEL_COMPILER_USE_INTRINSIC_PROTOTYPES
#undef _FORTIFY_SOURCE      // otherwise, the always_inline from stdio.h fail to inline

#include "qsimd_p.h"
#include "qalgorithms.h"

#include <stdio.h>
#include <string.h>

#if defined(QT_NO_DEBUG) && !defined(NDEBUG)
#  define NDEBUG
#endif
#include <assert.h>

#ifdef Q_OS_LINUX
#  include "../testlib/3rdparty/valgrind_p.h"
#endif

#define QT_FUNCTION_TARGET_BASELINE

#if defined(Q_OS_WIN)
#  if !defined(Q_CC_GNU)
#    include <intrin.h>
#  endif
#  if defined(Q_PROCESSOR_ARM_64)
#    include <qt_windows.h>
#    include <processthreadsapi.h>
#  endif
#elif defined(Q_OS_LINUX) && defined(Q_PROCESSOR_MIPS_32)
#  include "private/qcore_unix_p.h"
#elif QT_CONFIG(getauxval) && defined(Q_PROCESSOR_ARM)
#  include <sys/auxv.h>

// the kernel header definitions for HWCAP_*
// (the ones we need/may need anyway)

// copied from <asm/hwcap.h> (ARM)
#define HWCAP_NEON      4096

// copied from <asm/hwcap.h> (ARM):
#define HWCAP2_AES   (1 << 0)
#define HWCAP2_CRC32 (1 << 4)

// copied from <asm/hwcap.h> (Aarch64)
#define HWCAP_AES               (1 << 3)
#define HWCAP_CRC32             (1 << 7)

// copied from <linux/auxvec.h>
#define AT_HWCAP  16    /* arch dependent hints at CPU capabilities */
#define AT_HWCAP2 26    /* extension of AT_HWCAP */

#elif defined(Q_CC_GHS)
#  include <INTEGRITY_types.h>
#elif defined(Q_OS_DARWIN) && defined(Q_PROCESSOR_ARM)
#  include <sys/sysctl.h>
#endif

QT_BEGIN_NAMESPACE

template <typename T, uint N> QT_FUNCTION_TARGET_BASELINE
uint arraysize(T (&)[N])
{
    // Same as std::size, but with QT_FUNCTION_TARGET_BASELIE,
    // otherwise some versions of GCC fail to compile.
    return N;
}

#if defined(Q_PROCESSOR_ARM)
/* Data:
 neon
 crc32
 aes
 */
static const char features_string[] =
        "\0"
        " neon\0"
        " crc32\0"
        " aes\0";
static const int features_indices[] = { 0, 1, 7, 14 };
#elif defined(Q_PROCESSOR_MIPS)
/* Data:
 dsp
 dspr2
*/
static const char features_string[] =
    "\0"
    " dsp\0"
    " dspr2\0";

static const int features_indices[] = {
       0, 1, 6
};
#elif defined(Q_PROCESSOR_X86)
#  include "qsimd_x86.cpp"                  // generated by util/x86simdgen
#else
static const char features_string[] = "";
static const int features_indices[] = { 0 };
#endif
// end generated

#if defined(Q_PROCESSOR_ARM)
static inline quint64 detectProcessorFeatures()
{
    quint64 features = 0;

#if QT_CONFIG(getauxval)
    unsigned long auxvHwCap = getauxval(AT_HWCAP);
    if (auxvHwCap != 0) {
#  if defined(Q_PROCESSOR_ARM_64)
        // For Aarch64:
        features |= CpuFeatureNEON; // NEON is always available
        if (auxvHwCap & HWCAP_CRC32)
            features |= CpuFeatureCRC32;
        if (auxvHwCap & HWCAP_AES)
            features |= CpuFeatureAES;
#  else
        // For ARM32:
        if (auxvHwCap & HWCAP_NEON)
            features |= CpuFeatureNEON;
        auxvHwCap = getauxval(AT_HWCAP2);
        if (auxvHwCap & HWCAP2_CRC32)
            features |= CpuFeatureCRC32;
        if (auxvHwCap & HWCAP2_AES)
            features |= CpuFeatureAES;
#  endif
        return features;
    }
    // fall back to compile-time flags if getauxval failed
#elif defined(Q_OS_DARWIN) && defined(Q_PROCESSOR_ARM)
    unsigned feature;
    size_t len = sizeof(feature);
    if (sysctlbyname("hw.optional.neon", &feature, &len, nullptr, 0) == 0)
        features |= feature ? CpuFeatureNEON : 0;
    if (sysctlbyname("hw.optional.armv8_crc32", &feature, &len, nullptr, 0) == 0)
        features |= feature ? CpuFeatureCRC32 : 0;
    // There is currently no optional value for crypto/AES.
#if defined(__ARM_FEATURE_CRYPTO)
    features |= CpuFeatureAES;
#endif
    return features;
#elif defined(Q_OS_WIN) && defined(Q_PROCESSOR_ARM_64)
    features |= CpuFeatureNEON;
    if (IsProcessorFeaturePresent(PF_ARM_V8_CRC32_INSTRUCTIONS_AVAILABLE) != 0)
        features |= CpuFeatureCRC32;
    if (IsProcessorFeaturePresent(PF_ARM_V8_CRYPTO_INSTRUCTIONS_AVAILABLE) != 0)
        features |= CpuFeatureAES;
    return features;
#endif
#if defined(__ARM_NEON__) || defined(__ARM_NEON)
    features |= CpuFeatureNEON;
#endif
#if defined(__ARM_FEATURE_CRC32)
    features |= CpuFeatureCRC32;
#endif
#if defined(__ARM_FEATURE_CRYPTO)
    features |= CpuFeatureAES;
#endif

    return features;
}

#elif defined(Q_PROCESSOR_X86)

#ifdef Q_PROCESSOR_X86_32
# define PICreg "%%ebx"
#else
# define PICreg "%%rbx"
#endif
#ifdef __SSE2_MATH__
# define X86_BASELINE   "no-sse3"
#else
# define X86_BASELINE   "no-sse"
#endif

#if defined(Q_CC_GNU)
// lower the target for functions in this file
#  undef QT_FUNCTION_TARGET_BASELINE
#  define QT_FUNCTION_TARGET_BASELINE               __attribute__((target(X86_BASELINE)))
#  define QT_FUNCTION_TARGET_STRING_BASELINE_RDRND      \
    X86_BASELINE "," QT_FUNCTION_TARGET_STRING_RDRND
#endif

static bool checkRdrndWorks() noexcept;

QT_FUNCTION_TARGET_BASELINE
static int maxBasicCpuidSupported()
{
#if defined(Q_CC_EMSCRIPTEN)
    return 6; // All features supported by Emscripten
#elif defined(Q_CC_GNU)
    qregisterint tmp1;

# if Q_PROCESSOR_X86 < 5
    // check if the CPUID instruction is supported
    long cpuid_supported;
    asm ("pushf\n"
         "pop %0\n"
         "mov %0, %1\n"
         "xor $0x00200000, %0\n"
         "push %0\n"
         "popf\n"
         "pushf\n"
         "pop %0\n"
         "xor %1, %0\n" // %eax is now 0 if CPUID is not supported
         : "=a" (cpuid_supported), "=r" (tmp1)
         );
    if (!cpuid_supported)
        return 0;
# endif

    int result;
    asm ("xchg " PICreg", %1\n"
         "cpuid\n"
         "xchg " PICreg", %1\n"
        : "=&a" (result), "=&r" (tmp1)
        : "0" (0)
        : "ecx", "edx");
    return result;
#elif defined(Q_OS_WIN)
    // Use the __cpuid function; if the CPUID instruction isn't supported, it will return 0
    int info[4];
    __cpuid(info, 0);
    return info[0];
#elif defined(Q_CC_GHS)
    unsigned int info[4];
    __CPUID(0, info);
    return info[0];
#else
    return 0;
#endif
}

QT_FUNCTION_TARGET_BASELINE
static void cpuidFeatures01(uint &ecx, uint &edx)
{
#if defined(Q_CC_GNU) && !defined(Q_CC_EMSCRIPTEN)
    qregisterint tmp1;
    asm ("xchg " PICreg", %2\n"
         "cpuid\n"
         "xchg " PICreg", %2\n"
        : "=&c" (ecx), "=&d" (edx), "=&r" (tmp1)
        : "a" (1));
#elif defined(Q_OS_WIN)
    int info[4];
    __cpuid(info, 1);
    ecx = info[2];
    edx = info[3];
#elif defined(Q_CC_GHS)
    unsigned int info[4];
    __CPUID(1, info);
    ecx = info[2];
    edx = info[3];
#else
    Q_UNUSED(ecx);
    Q_UNUSED(edx);
#endif
}

#ifdef Q_OS_WIN
inline void __cpuidex(int info[4], int, __int64) { memset(info, 0, 4*sizeof(int));}
#endif

QT_FUNCTION_TARGET_BASELINE
static void cpuidFeatures07_00(uint &ebx, uint &ecx, uint &edx)
{
#if defined(Q_CC_GNU) && !defined(Q_CC_EMSCRIPTEN)
    qregisteruint rbx; // in case it's 64-bit
    qregisteruint rcx = 0;
    qregisteruint rdx = 0;
    asm ("xchg " PICreg", %0\n"
         "cpuid\n"
         "xchg " PICreg", %0\n"
        : "=&r" (rbx), "+&c" (rcx), "+&d" (rdx)
        : "a" (7));
    ebx = rbx;
    ecx = rcx;
    edx = rdx;
#elif defined(Q_OS_WIN)
    int info[4];
    __cpuidex(info, 7, 0);
    ebx = info[1];
    ecx = info[2];
    edx = info[3];
#elif defined(Q_CC_GHS)
    unsigned int info[4];
    __CPUIDEX(7, 0, info);
    ebx = info[1];
    ecx = info[2];
    edx = info[3];
#else
    Q_UNUSED(ebx);
    Q_UNUSED(ecx);
    Q_UNUSED(edx);
#endif
}

QT_FUNCTION_TARGET_BASELINE
#if defined(Q_OS_WIN) && !(defined(Q_CC_GNU) || defined(Q_CC_GHS))
// fallback overload in case this intrinsic does not exist: unsigned __int64 _xgetbv(unsigned int);
inline quint64 _xgetbv(__int64) { return 0; }
#endif
static void xgetbv(uint in, uint &eax, uint &edx)
{
#if (defined(Q_CC_GNU) && !defined(Q_CC_EMSCRIPTEN)) || defined(Q_CC_GHS)
    asm (".byte 0x0F, 0x01, 0xD0" // xgetbv instruction
        : "=a" (eax), "=d" (edx)
        : "c" (in));
#elif defined(Q_OS_WIN)
    quint64 result = _xgetbv(in);
    eax = result;
    edx = result >> 32;
#else
    Q_UNUSED(in);
    Q_UNUSED(eax);
    Q_UNUSED(edx);
#endif
}

QT_FUNCTION_TARGET_BASELINE
static quint64 adjustedXcr0(quint64 xcr0)
{
    /*
     * Some OSes hide their capability of context-switching the AVX512 state in
     * the XCR0 register. They do that so the first time we execute an
     * instruction that may access the AVX512 state (requiring the EVEX prefix)
     * they allocate the necessary context switch space.
     *
     * This behavior is deprecated with the XFD (Extended Feature Disable)
     * register, but we can't change existing OSes.
     */
#ifdef Q_OS_DARWIN
    // from <machine/cpu_capabilities.h> in xnu
    // <https://github.com/apple/darwin-xnu/blob/xnu-4903.221.2/osfmk/i386/cpu_capabilities.h>
    constexpr quint64 kHasAVX512F = Q_UINT64_C(0x0000004000000000);
    constexpr quintptr commpage = sizeof(void *) > 4 ? Q_UINT64_C(0x00007fffffe00000) : 0xffff0000;
    constexpr quintptr cpu_capabilities64 = commpage + 0x10;
    quint64 capab = *reinterpret_cast<quint64 *>(cpu_capabilities64);
    if (capab & kHasAVX512F)
        xcr0 |= XSave_Avx512State;
#endif

    return xcr0;
}

QT_FUNCTION_TARGET_BASELINE
static quint64 detectProcessorFeatures()
{
    quint64 features = 0;
    int cpuidLevel = maxBasicCpuidSupported();
#if Q_PROCESSOR_X86 < 5
    if (cpuidLevel < 1)
        return 0;
#else
    assert(cpuidLevel >= 1);
#endif

    uint results[X86CpuidMaxLeaf] = {};
    cpuidFeatures01(results[Leaf01ECX], results[Leaf01EDX]);
    if (cpuidLevel >= 7)
        cpuidFeatures07_00(results[Leaf07_00EBX], results[Leaf07_00ECX], results[Leaf07_00EDX]);

    // populate our feature list
    for (uint i = 0; i < arraysize(x86_locators); ++i) {
        uint word = x86_locators[i] / 32;
        uint bit = 1U << (x86_locators[i] % 32);
        quint64 feature = Q_UINT64_C(1) << i;
        if (results[word] & bit)
            features |= feature;
    }

    // now check the AVX state
    quint64 xcr0 = 0;
    if (results[Leaf01ECX] & (1u << 27)) {
        // XGETBV enabled
        uint xgetbvA = 0, xgetbvD = 0;
        xgetbv(0, xgetbvA, xgetbvD);

        xcr0 = xgetbvA;
        if (sizeof(XSaveBits) > sizeof(xgetbvA))
            xcr0 |= quint64(xgetbvD) << 32;
        xcr0 = adjustedXcr0(xcr0);
    }

    for (auto req : xsave_requirements) {
        if ((xcr0 & req.xsave_state) != req.xsave_state)
            features &= ~req.cpu_features;
    }

    if (features & CpuFeatureRDRND && !checkRdrndWorks())
        features &= ~(CpuFeatureRDRND | CpuFeatureRDSEED);

    return features;
}

#elif defined(Q_PROCESSOR_MIPS_32)

#if defined(Q_OS_LINUX)
//
// Do not use QByteArray: it could use SIMD instructions itself at
// some point, thus creating a recursive dependency. Instead, use a
// QSimpleBuffer, which has the bare minimum needed to use memory
// dynamically and read lines from /proc/cpuinfo of arbitrary sizes.
//
struct QSimpleBuffer
{
    static const int chunk_size = 256;
    char *data;
    unsigned alloc;
    unsigned size;

    QSimpleBuffer() : data(nullptr), alloc(0), size(0) { }
    ~QSimpleBuffer() { ::free(data); }

    void resize(unsigned newsize)
    {
        if (newsize > alloc) {
            unsigned newalloc = chunk_size * ((newsize / chunk_size) + 1);
            if (newalloc < newsize)
                newalloc = newsize;
            if (newalloc != alloc) {
                data = static_cast<char *>(::realloc(data, newalloc));
                alloc = newalloc;
            }
        }
        size = newsize;
    }
    void append(const QSimpleBuffer &other, unsigned appendsize)
    {
        unsigned oldsize = size;
        resize(oldsize + appendsize);
        ::memcpy(data + oldsize, other.data, appendsize);
    }
    void popleft(unsigned amount)
    {
        if (amount >= size)
            return resize(0);
        size -= amount;
        ::memmove(data, data + amount, size);
    }
    char *cString()
    {
        if (!alloc)
            resize(1);
        return (data[size] = '\0', data);
    }
};

//
// Uses a scratch "buffer" (which must be used for all reads done in the
// same file descriptor) to read chunks of data from a file, to read
// one line at a time. Lines include the trailing newline character ('\n').
// On EOF, line.size is zero.
//
static void bufReadLine(int fd, QSimpleBuffer &line, QSimpleBuffer &buffer)
{
    for (;;) {
        char *newline = static_cast<char *>(::memchr(buffer.data, '\n', buffer.size));
        if (newline) {
            unsigned piece_size = newline - buffer.data + 1;
            line.append(buffer, piece_size);
            buffer.popleft(piece_size);
            line.resize(line.size - 1);
            return;
        }
        if (buffer.size + QSimpleBuffer::chunk_size > buffer.alloc) {
            int oldsize = buffer.size;
            buffer.resize(buffer.size + QSimpleBuffer::chunk_size);
            buffer.size = oldsize;
        }
        ssize_t read_bytes =
                ::qt_safe_read(fd, buffer.data + buffer.size, QSimpleBuffer::chunk_size);
        if (read_bytes > 0)
            buffer.size += read_bytes;
        else
            return;
    }
}

//
// Checks if any line with a given prefix from /proc/cpuinfo contains
// a certain string, surrounded by spaces.
//
static bool procCpuinfoContains(const char *prefix, const char *string)
{
    int cpuinfo_fd = ::qt_safe_open("/proc/cpuinfo", O_RDONLY);
    if (cpuinfo_fd == -1)
        return false;

    unsigned string_len = ::strlen(string);
    unsigned prefix_len = ::strlen(prefix);
    QSimpleBuffer line, buffer;
    bool present = false;
    do {
        line.resize(0);
        bufReadLine(cpuinfo_fd, line, buffer);
        char *colon = static_cast<char *>(::memchr(line.data, ':', line.size));
        if (colon && line.size > prefix_len + string_len) {
            if (!::strncmp(prefix, line.data, prefix_len)) {
                // prefix matches, next character must be ':' or space
                if (line.data[prefix_len] == ':' || ::isspace(line.data[prefix_len])) {
                    // Does it contain the string?
                    char *found = ::strstr(line.cString(), string);
                    if (found && ::isspace(found[-1]) &&
                            (::isspace(found[string_len]) || found[string_len] == '\0')) {
                        present = true;
                        break;
                    }
                }
            }
        }
    } while (line.size);

    ::qt_safe_close(cpuinfo_fd);
    return present;
}
#endif

static inline quint64 detectProcessorFeatures()
{
    // NOTE: MIPS 74K cores are the only ones supporting DSPr2.
    quint64 flags = 0;

#if defined __mips_dsp
    flags |= CpuFeatureDSP;
#  if defined __mips_dsp_rev && __mips_dsp_rev >= 2
    flags |= CpuFeatureDSPR2;
#  elif defined(Q_OS_LINUX)
    if (procCpuinfoContains("cpu model", "MIPS 74Kc") || procCpuinfoContains("cpu model", "MIPS 74Kf"))
        flags |= CpuFeatureDSPR2;
#  endif
#elif defined(Q_OS_LINUX)
    if (procCpuinfoContains("ASEs implemented", "dsp")) {
        flags |= CpuFeatureDSP;
        if (procCpuinfoContains("cpu model", "MIPS 74Kc") || procCpuinfoContains("cpu model", "MIPS 74Kf"))
            flags |= CpuFeatureDSPR2;
    }
#endif

    return flags;
}

#else
static inline uint detectProcessorFeatures()
{
    return 0;
}
#endif

// record what CPU features were enabled by default in this Qt build
static const quint64 minFeature = qCompilerCpuFeatures;

static constexpr auto SimdInitialized = QCpuFeatureType(1) << (sizeof(QCpuFeatureType) * 8 - 1);
Q_ATOMIC(QCpuFeatureType) QT_MANGLE_NAMESPACE(qt_cpu_features)[1] = { 0 };

QT_FUNCTION_TARGET_BASELINE
uint64_t QT_MANGLE_NAMESPACE(qDetectCpuFeatures)()
{
    auto minFeatureTest = minFeature;
#if defined(Q_PROCESSOR_X86_64) && defined(cpu_feature_shstk)
    // Controlflow Enforcement Technology (CET) is an OS-assisted
    // hardware-feature, meaning the CPUID bit may be disabled if the OS
    // doesn't support it, but that's ok.
    minFeatureTest &= ~CpuFeatureSHSTK;
#endif
    QCpuFeatureType f = detectProcessorFeatures();

    // Intentionally NOT qgetenv (this code runs too early)
    if (char *disable = getenv("QT_NO_CPU_FEATURE"); disable && *disable) {
#if _POSIX_C_SOURCE >= 200112L
        char *saveptr = nullptr;
        auto strtok = [&saveptr](char *str, const char *delim) {
            return ::strtok_r(str, delim, &saveptr);
        };
#endif
        while (char *token = strtok(disable, " ")) {
            disable = nullptr;
            for (uint i = 0; i < arraysize(features_indices); ++i) {
                if (strcmp(token, features_string + features_indices[i]) == 0)
                    f &= ~(Q_UINT64_C(1) << i);
            }
        }
    }

#ifdef RUNNING_ON_VALGRIND
    bool runningOnValgrind = RUNNING_ON_VALGRIND;
#else
    bool runningOnValgrind = false;
#endif
    if (Q_UNLIKELY(!runningOnValgrind && minFeatureTest != 0 && (f & minFeatureTest) != minFeatureTest)) {
        quint64 missing = minFeatureTest & ~quint64(f);
        fprintf(stderr, "Incompatible processor. This Qt build requires the following features:\n   ");
        for (uint i = 0; i < arraysize(features_indices); ++i) {
            if (missing & (Q_UINT64_C(1) << i))
                fprintf(stderr, "%s", features_string + features_indices[i]);
        }
        fprintf(stderr, "\n");
        fflush(stderr);
        qAbort();
    }

    assert((f & SimdInitialized) == 0);
    f |= SimdInitialized;
    std::atomic_store_explicit(QT_MANGLE_NAMESPACE(qt_cpu_features), f, std::memory_order_relaxed);
    return f;
}

QT_FUNCTION_TARGET_BASELINE
void qDumpCPUFeatures()
{
    quint64 features = detectProcessorFeatures() & ~SimdInitialized;
    printf("Processor features: ");
    for (uint i = 0; i < arraysize(features_indices); ++i) {
        if (features & (Q_UINT64_C(1) << i))
            printf("%s%s", features_string + features_indices[i],
                   minFeature & (Q_UINT64_C(1) << i) ? "[required]" : "");
    }
    if ((features = (qCompilerCpuFeatures & ~features))) {
        printf("\n!!!!!!!!!!!!!!!!!!!!\n!!! Missing required features:");
        for (uint i = 0; i < arraysize(features_indices); ++i) {
            if (features & (Q_UINT64_C(1) << i))
                printf("%s", features_string + features_indices[i]);
        }
        printf("\n!!! Applications will likely crash with \"Invalid Instruction\"\n!!!!!!!!!!!!!!!!!!!!");
    }
    puts("");
}

#if defined(Q_PROCESSOR_X86) && QT_COMPILER_SUPPORTS_HERE(RDRND)

#  ifdef Q_PROCESSOR_X86_64
#    define _rdrandXX_step _rdrand64_step
#    define _rdseedXX_step _rdseed64_step
#  else
#    define _rdrandXX_step _rdrand32_step
#    define _rdseedXX_step _rdseed32_step
#  endif

// The parameter to _rdrand64_step & _rdseed64_step is unsigned long long for
// Clang and GCC but unsigned __int64 for MSVC and ICC, which is unsigned long
// long on Windows, but unsigned long on Linux.
namespace {
template <typename F> struct ExtractParameter;
template <typename T> struct ExtractParameter<int (T *)> { using Type = T; };
using randuint = ExtractParameter<decltype(_rdrandXX_step)>::Type;
}

#  if QT_COMPILER_SUPPORTS_HERE(RDSEED)
static QT_FUNCTION_TARGET(RDSEED) unsigned *qt_random_rdseed(unsigned *ptr, unsigned *end) noexcept
{
    // Unlike for the RDRAND code below, the Intel whitepaper describing the
    // use of the RDSEED instruction indicates we should not retry in a loop.
    // If the independent bit generator used by RDSEED is out of entropy, it
    // may take time to replenish.
    // https://software.intel.com/en-us/articles/intel-digital-random-number-generator-drng-software-implementation-guide
    while (ptr + sizeof(randuint) / sizeof(*ptr) <= end) {
        if (_rdseedXX_step(reinterpret_cast<randuint *>(ptr)) == 0)
            goto out;
        ptr += sizeof(randuint) / sizeof(*ptr);
    }

    if (sizeof(*ptr) != sizeof(randuint) && ptr != end) {
        if (_rdseed32_step(ptr) == 0)
            goto out;
        ++ptr;
    }

out:
    return ptr;
}
#  else
static unsigned *qt_random_rdseed(unsigned *ptr, unsigned *)
{
    return ptr;
}
#  endif

static QT_FUNCTION_TARGET(RDRND) unsigned *qt_random_rdrnd(unsigned *ptr, unsigned *end) noexcept
{
    int retries = 10;
    while (ptr + sizeof(randuint)/sizeof(*ptr) <= end) {
        if (_rdrandXX_step(reinterpret_cast<randuint *>(ptr)))
            ptr += sizeof(randuint)/sizeof(*ptr);
        else if (--retries == 0)
            goto out;
    }

    while (sizeof(*ptr) != sizeof(randuint) && ptr != end) {
        bool ok = _rdrand32_step(ptr);
        if (!ok && --retries)
            continue;
        if (ok)
            ++ptr;
        break;
    }

out:
    return ptr;
}

QT_FUNCTION_TARGET(BASELINE_RDRND) Q_DECL_COLD_FUNCTION
static bool checkRdrndWorks() noexcept
{
    /*
     * Some AMD CPUs (e.g. AMD A4-6250J and AMD Ryzen 3000-series) have a
     * failing random generation instruction, which always returns
     * 0xffffffff, even when generation was "successful".
     *
     * This code checks if hardware random generator generates four consecutive
     * equal numbers. If it does, then we probably have a failing one and
     * should disable it completely.
     *
     * https://bugreports.qt.io/browse/QTBUG-69423
     */
    constexpr qsizetype TestBufferSize = 4;
    unsigned testBuffer[TestBufferSize] = {};

    unsigned *end = qt_random_rdrnd(testBuffer, testBuffer + TestBufferSize);
    if (end < testBuffer + 3) {
        // Random generation didn't produce enough data for us to make a
        // determination whether it's working or not. Assume it isn't, but
        // don't print a warning.
        return false;
    }

    // Check the results for equality
    if (testBuffer[0] == testBuffer[1]
        && testBuffer[0] == testBuffer[2]
        && (end < testBuffer + TestBufferSize || testBuffer[0] == testBuffer[3])) {
        fprintf(stderr, "WARNING: CPU random generator seem to be failing, "
                        "disabling hardware random number generation\n"
                        "WARNING: RDRND generated:");
        for (unsigned *ptr = testBuffer; ptr < end; ++ptr)
            fprintf(stderr, " 0x%x", *ptr);
        fprintf(stderr, "\n");
        return false;
    }

    // We're good
    return true;
}

QT_FUNCTION_TARGET(RDRND) qsizetype qRandomCpu(void *buffer, qsizetype count) noexcept
{
    unsigned *ptr = reinterpret_cast<unsigned *>(buffer);
    unsigned *end = ptr + count;

    if (qCpuHasFeature(RDSEED))
        ptr = qt_random_rdseed(ptr, end);

    // fill the buffer with RDRND if RDSEED didn't
    ptr = qt_random_rdrnd(ptr, end);
    return ptr - reinterpret_cast<unsigned *>(buffer);
}
#elif defined(Q_PROCESSOR_X86) && !defined(Q_PROCESSOR_ARM)
static bool checkRdrndWorks() noexcept { return false; }
#endif // Q_PROCESSOR_X86 && RDRND

#if QT_SUPPORTS_INIT_PRIORITY
namespace {
struct QSimdInitializer
{
    inline QSimdInitializer() { QT_MANGLE_NAMESPACE(qDetectCpuFeatures)(); }
};
}

// This is intentionally a dynamic initialization of the variable
Q_DECL_INIT_PRIORITY(01) static QSimdInitializer initializer;
#endif

QT_END_NAMESPACE
