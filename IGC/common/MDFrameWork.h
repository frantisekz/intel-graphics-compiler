/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Compiler/CodeGenPublicEnums.h"

#include <string>
#include <map>
#include <vector>
#include <array>
#include <optional>
#include <climits>
#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/MapVector.h>
#include "common/LLVMWarningsPop.hpp"

namespace llvm
{
    class Module;
    class Function;
    class Value;
    class GlobalVariable;
    class StructType;
}

const unsigned int INPUT_RESOURCE_SLOT_COUNT = 128;
const unsigned int NUM_SHADER_RESOURCE_VIEW_SIZE = (INPUT_RESOURCE_SLOT_COUNT + 1) / 64;

const unsigned int g_c_maxNumberOfBufferPushed = 4;
static const int MAX_VECTOR_SIZE_TO_PRINT_IN_SHADER_DUMPS = 1000;

namespace IGC
{
    const unsigned int INVALID_CONSTANT_BUFFER_INVALID_ADDR = 0xFFFFFFFF;

    static const char* NAMED_METADATA_COARSE_PHASE = "coarse_phase";
    static const char* NAMED_METADATA_PIXEL_PHASE  = "pixel_phase";

    enum FunctionTypeMD
    {
        KernelFunction,
        CallableShader,
        UserFunction,
        NumberOfFunctionType,
    };

    enum UniqueIndirectAS
    {
        // The convention is to use a '0' index for indirect accesses if
        // you don't need to distinguish between accesses.
        DefaultIndirectIdx = 0,
    };

#include "RaytracingShaderTypes.h"

    enum ResourceTypeEnum
    {
        OtherResourceType,
        UAVResourceType,
        SRVResourceType,
        SamplerResourceType,
        BindlessUAVResourceType,
        BindlessSamplerResourceType,
        DefaultResourceType,
    };

    enum ResourceExtensionTypeEnum
    {
        NonExtensionType,

        // VME
        MediaResourceType,
        MediaResourceBlockType,
        MediaSamplerType,

        // VA
        MediaSamplerTypeConvolve,
        MediaSamplerTypeErode,
        MediaSamplerTypeDilate,
        MediaSamplerTypeMinMaxFilter,
        MediaSamplerTypeMinMax,
        MediaSamplerTypeCentroid,
        MediaSamplerTypeBoolCentroid,
        MediaSamplerTypeBoolSum,
        MediaSamplerTypeLbp,
        MediaSamplerTypeFloodFill,
        MediaSamplerTypeCorrelation,
        DefaultResourceExtensionType,
    };

    struct InlineResInfo
    {
        unsigned int textureID = 0;
        unsigned int SurfaceType = 0x7;
        unsigned int WidthOrBufferSize = 0;
        unsigned int Height = 0;
        unsigned int Depth = 0;
        unsigned int SurfaceArray = 0;
        unsigned int QWidth = 0;
        unsigned int QHeight = 0;
        unsigned int MipCount = 0;
    };

    struct ArgDependencyInfoMD
    {
        int argDependency = 0;
    };

    struct ArgAllocMD
    {
        int type = -1;
        int extensionType = -1;
        int indexType = -1;
    };

    struct InlineSamplersMD
    {
        int m_Value = 0;
        int addressMode = 0;
        int index = 0;
        int TCXAddressMode = 0;
        int TCYAddressMode = 0;
        int TCZAddressMode = 0;
        int MagFilterType = 0;
        int MinFilterType = 0;
        int MipFilterType = 0;
        int CompareFunc = 0;
        int NormalizedCoords = 0;
        float BorderColorR = 0.0f;
        float BorderColorG = 0.0f;
        float BorderColorB = 0.0f;
        float BorderColorA = 0.0f;
    };

    struct ResourceAllocMD
    {
        int uavsNumType = 0;
        int srvsNumType = 0;
        int samplersNumType = 0;
        std::vector<ArgAllocMD> argAllocMDList;
        std::vector<InlineSamplersMD> inlineSamplersMD;
    };

    struct ComputeShaderSecondCompileInputInfoMD
    {
        int runtimeVal_ResWidthHeight = 0;
        int runtimeVal_LoopCount = 0;
        int runtimeVal_ConstantBufferSize = 0;
        bool isSecondCompile = false;
        int isRowMajor = 0;
        int numChannelsUsed = 0;
    };

    struct LocalOffsetMD
    {
        int m_Offset;
        llvm::GlobalVariable* m_Var;
    };

    struct WorkGroupWalkOrderMD
    {
        int dim0 = 0;
        int dim1 = 0;
        int dim2 = 0;
    };

    struct FuncArgMD
    {
        int bufferLocationIndex = -1;
        int bufferLocationCount = -1;
        bool isEmulationArg = 0;
    };

    enum StackEntryType
    {
        ENTRY_RETURN_IP,
        ENTRY_ARGUMENT,
        ENTRY_ALLOCA,
        ENTRY_SPILL,
        ENTRY_UNKNOWN,
    };

    struct StackFrameEntry
    {
        // Name of the value if it exists.
        std::string Name;
        // This is just a string representation of an LLVM type.
        std::string TypeRepr;
        // Helpful to get a rough idea of what the value is without a name.
        StackEntryType EntryType = ENTRY_UNKNOWN;
        // Size in bytes that this entry occupies on the stack.
        uint32_t Size = 0;
        // Offset from the base of the stack frame.
        uint32_t Offset = 0;
    };

    // A raytracing shader may have an arbitrary number of TraceRay() calls
    // within it.  Live values across the trace need to be spilled so they
    // can be refilled in the corresponding continuation. The live values can
    // be different at different TraceRay() calls so the spilled memory is
    // interpreted different at each of those sites.
    struct StackFrameSpillUnion
    {
        std::string ContinuationName;
        std::vector<StackFrameEntry> Entries;
    };

    // We maintain a collection of named structs which is populated by passes
    // when generating structured accesses to the Raytracing SW Stack.
    struct RayTracingSWTypes
    {
        std::vector<llvm::StructType*> FrameStartTys;
        std::vector<llvm::StructType*> ArgumentTys;
        std::vector<llvm::StructType*> FullFrameTys;
    };

    // Info common to all shaders in the module
    struct RayTraceModuleInfo
    {
        // The size of a single sync stack entry that the UMD must allocate
        // for synchronous raytracing.
        uint32_t RayQueryAllocSizeInBytes = 0;

        // SplitAsyncPass sets the number of continuations that were generated.
        // This is heuristically used to determine whether we should inline
        // or indirectly BTD to the continuations.
        uint32_t NumContinuations = UINT_MAX;

        // Track the address spaces and SSH offsets for indirect stateful
        // accesses.
        uint32_t RTAsyncStackAddrspace = UINT_MAX;
        std::optional<uint32_t> RTAsyncStackSurfaceStateOffset;

        uint32_t SWHotZoneAddrspace = UINT_MAX;
        std::optional<uint32_t> SWHotZoneSurfaceStateOffset;

        uint32_t SWStackAddrspace = UINT_MAX;
        std::optional<uint32_t> SWStackSurfaceStateOffset;

        uint32_t RTSyncStackAddrspace = UINT_MAX;
        std::optional<uint32_t> RTSyncStackSurfaceStateOffset;
    };

    // Info specific to each raytracing shader
    struct RayTraceShaderInfo
    {
        CallableShaderTypeMD callableShaderType = NumberOfCallableShaderTypes;
        bool isContinuation = false;
        bool hasTraceRayPayload = false;
        bool hasHitAttributes = false;
        bool hasCallableData = false;
        uint32_t ShaderStackSize = 0;
        uint64_t ShaderHash = 0;
        std::string ShaderName;
        // if 'isContinuation' is true, this will contain the name of the
        // original shader.
        std::string ParentName;
        // if 'isContinuation' is true, this may contain the slot num for
        // the shader identifier it has been promoted to.
        std::optional<uint32_t> SlotNum;
        // Size in bytes of the cross-thread constant data. Each frontend
        // (e.g., DX, Vulkan) will need to populate this according to its
        // needs.  For DX, it is:
        // Align(
        //     Align(sizeof(RayDispatchGlobalData), 8) + GlobalRootSigSize, 32)
        uint32_t NOSSize = 0;
        // A given raytracing shader will have some amount of stack allocated
        // for its arguments, allocas, and spilled values.  We collect
        // information about those entries here for debugging purposes to
        // read *output.yaml for more information or for external tools to
        // consume and display.
        std::vector<StackFrameEntry> Entries;
        std::vector<StackFrameSpillUnion> SpillUnions;
        // This will be set by an early processing pass and read out by
        // StackFrameInfo to allocate enough space for whatever type the
        // shader uses.
        uint32_t CustomHitAttrSizeInBytes = 0;
        RayTracingSWTypes Types;
        // Shaders that satisfy `isPrimaryShaderIdentifier()` can also have
        // a collection of other names that they go by.
        std::vector<std::string> Aliases;
    };

    struct ConstantAddress
    {
        unsigned int bufId = 0;
        unsigned int eltId = 0;
        unsigned int size = 0;
    };
    struct ConstantAddressDescriptorTable : ConstantAddress
    {
        unsigned int tableOffset = 0;
    };
    bool operator < (const ConstantAddress &a, const ConstantAddress &b);

    //to hold metadata of every function
    struct FunctionMetaData
    {
        std::vector<LocalOffsetMD> localOffsets;
        WorkGroupWalkOrderMD workGroupWalkOrder;
        std::vector<FuncArgMD> funcArgs;
        FunctionTypeMD functionType = KernelFunction;
        RayTraceShaderInfo rtInfo;
        ResourceAllocMD resAllocMD;
        std::vector<unsigned> maxByteOffsets;
        bool IsInitializer = false;
        bool IsFinalizer = false;
        unsigned CompiledSubGroupsNumber = 0;
        bool hasInlineVmeSamplers = false;
        int localSize = 0;
        bool localIDPresent = false;
        bool groupIDPresent = false;
        int privateMemoryPerWI = 0;
        bool globalIDPresent = false;
        // This is true if the function has any sync raytracing functionality
        bool hasSyncRTCalls = false;

        // Analysis result of if there are non-kernel-argument ld/st in the kernel
        bool hasNonKernelArgLoad = false;
        bool hasNonKernelArgStore = false;
        bool hasNonKernelArgAtomic = false;

        std::vector<std::string> UserAnnotations;

        std::vector<int32_t> m_OpenCLArgAddressSpaces;
        std::vector<std::string> m_OpenCLArgAccessQualifiers;
        std::vector<std::string> m_OpenCLArgTypes;
        std::vector<std::string> m_OpenCLArgBaseTypes;
        std::vector<std::string> m_OpenCLArgTypeQualifiers;
        std::vector<std::string> m_OpenCLArgNames;
    };

    // isCloned member is added to mark whether a function is clone
    // of another one. If two kernels from a compilation unit invoke
    // the same callee, IGC ends up creating clone of the callee
    // to separate call graphs. But it doesnt create metadata nodes
    // so debug info for cloned function will be empty. Marking
    // function as clone and later in debug info iterating over
    // original function instead of clone helps emit out correct debug
    // info.

    //new structure to replace old Metatdata framework's CompilerOptions
    struct CompOptions
    {
        bool DenormsAreZero                             = false;
        bool CorrectlyRoundedDivSqrt                    = false;
        bool OptDisable                                 = false;
        bool MadEnable                                  = false;
        bool NoSignedZeros                              = false;
        bool NoNaNs                                     = false;

        // default rounding modes
        unsigned FloatRoundingMode                      = IGC::ROUND_TO_NEAREST_EVEN;
        unsigned FloatCvtIntRoundingMode                = IGC::ROUND_TO_ZERO;

        unsigned VISAPreSchedRPThreshold           = 0;
        unsigned SetLoopUnrollThreshold            = 0;
        bool UnsafeMathOptimizations                    = false;
        bool FiniteMathOnly                             = false;
        bool FastRelaxedMath                            = false;
        bool DashGSpecified                             = false;
        bool FastCompilation                            = false;
        bool UseScratchSpacePrivateMemory               = true;
        bool RelaxedBuiltins                            = false;
        bool SubgroupIndependentForwardProgressRequired = true;
        bool GreaterThan2GBBufferRequired               = true;
        bool GreaterThan4GBBufferRequired               = true;
        bool DisableA64WA                               = false;
        bool ForceEnableA64WA                           = false;
        bool PushConstantsEnable                        = true;
        bool HasPositivePointerOffset                   = false;
        bool HasBufferOffsetArg                         = false;
        bool BufferOffsetArgOptional                    = true;
        bool HasSubDWAlignedPtrArg                      = false;
        bool replaceGlobalOffsetsByZero                 = false;
        unsigned forcePixelShaderSIMDMode               = 0;
        bool pixelShaderDoNotAbortOnSpill               = false;
        bool UniformWGS                                 = false;
        bool disableVertexComponentPacking              = false;
        bool disablePartialVertexComponentPacking       = false;
        bool PreferBindlessImages                       = false;
        bool UseBindlessMode                            = false;
        bool UseLegacyBindlessMode                      = true;
        bool disableMathRefactoring                     = false;
        bool atomicBranch                               = false;
        bool ForceMinSimdSizeForFastestCS               = false;
        bool EnableFastestLinearScan                    = false;
        //if PTSS is enabled and if PrivateData is too large (>256k in XeHP_SDV+),
        //we might use stateless memory to hold privatedata instead of using PTSS.
        //this flag is for this scenario.
        bool UseStatelessforPrivateMemory               = false;
        bool EnableTakeGlobalAddress                    = false;
        bool IsLibraryCompilation                       = false;
        bool FastVISACompile                            = false;
        bool MatchSinCosPi                              = false;
        bool CaptureCompilerStats                       = false;
        // Suggest to enableZEBinary. IGC could still fall-back to legacy
        // patch-token based binary if the input contains features those
        // are not supported by ZEBinary
        bool EnableZEBinary                             = false;
        bool ExcludeIRFromZEBinary                      = false;

        //when true, compiler disables the Remat optimization for compute shaders
        bool allowDisableRematforCS                     = false;

        bool DisableIncSpillCostAllAddrTaken            = false;
        bool DisableCPSOmaskWA                          = false;
        bool WaForceHalfPromotion                       = false;
    };

    enum class ThreadIDLayout
    {
        // layout IDs along X,Y,Z
        X,
        // Tile along just the y-dimension
        TileY,
        // tile IDs in 2x2 groups as expected by derivative calculations
        QuadTile
    };

    struct ComputeShaderInfo
    {
        unsigned int maxWorkGroupSize = 0;
        unsigned int waveSize = 0; // force a wave size
        std::vector<ComputeShaderSecondCompileInputInfoMD> ComputeShaderSecondCompile;
        unsigned char forcedSIMDSize = 0;  // 0 means not forced
        unsigned int forceTotalGRFNum = 0; // 0 means not forced
        unsigned int VISAPreSchedRPThreshold = 0; // 0 means use the default
        unsigned int SetLoopUnrollThreshold = 0; // 0 means use the default
        bool forcedVISAPreRAScheduler = false;
        // disables dispatch along y and tiled order optimizations
        bool disableLocalIdOrderOptimizations = false;
        // force disables dispatch along y optimization
        bool disableDispatchAlongY = false;
        // If nullopt, then there is no requirement
        std::optional<ThreadIDLayout> neededThreadIdLayout;
        // force enable tile y optimization
        bool forceTileYWalk = false;
        // enable atomic branch optimization
        bool atomicBranch = false;
        // resource index for hf packing (resourceRangeID, indexIntoRange)
        std::vector<std::vector<unsigned int>> ResForHfPacking;

    };


    struct PixelShaderInfo
    {
        unsigned char BlendStateDisabledMask = 0;
        bool SkipSrc0Alpha                   = false;
        bool DualSourceBlendingDisabled      = false;
        bool ForceEnableSimd32               = false; // forces compilation of simd32; bypass heuristics
        bool outputDepth                     = false;
        bool outputStencil                   = false;
        bool outputMask                      = false;
        bool blendToFillEnabled              = false;
        bool forceEarlyZ                     = false;   // force earlyz test
        bool hasVersionedLoop                = false;   // if versioned by customloopversioning
        bool forceSingleSourceRTWAfterDualSourceRTW = false;
        // Number of samples for this pixel shader if known.
        // Valid values 0, 1, 2, 4, 8 and 16.
        // 0 means unknown or not set.
        unsigned char NumSamples             = 0;
        std::vector<int> blendOptimizationMode;
        std::vector<int> colorOutputMask;
    };

    struct MeshShaderInfo
    {
        unsigned int PrimitiveTopology            = 3; // IGC::GFX3DMESH_OUTPUT_TOPOLOGY::NUM_MAX
        unsigned int MaxNumOfPrimitives           = 0;
        unsigned int MaxNumOfVertices             = 0;
        unsigned int MaxNumOfPerPrimitiveOutputs  = 0;
        unsigned int MaxNumOfPerVertexOutputs     = 0;
        unsigned int WorkGroupSize                = 0;
        unsigned int WorkGroupMemorySizeInBytes   = 0;
        unsigned int IndexFormat                  = 6; //  IGC::GFX3DMESH_INDEX_FORMAT::NUM_MAX
        unsigned int SubgroupSize                 = 0; // force a wave size
    };

    struct TaskShaderInfo
    {
        unsigned int MaxNumOfOutputs = 0;
        unsigned int WorkGroupSize = 0;
        unsigned int WorkGroupMemorySizeInBytes = 0;
        unsigned int SubgroupSize = 0; // force a wave size
    };

    struct SInputDesc
    {
        unsigned int index = 0;
        int argIndex = 0;
        int interpolationMode = 0;
    };

    // SimplePushInfo holds information about the promoted constant buffer
    // region (see member descriptions in SSimplePushInfo). It also holds
    // mappings between the byte offsets in the promoted region and
    // corresponding argument index.
    struct SimplePushInfo
    {
        unsigned int cbIdx = 0;
        int pushableAddressGrfOffset = -1;
        int pushableOffsetGrfOffset = -1;
        unsigned int offset = 0;
        unsigned int size = 0;
        bool isStateless = false;
        bool isBindless = false;
        // std::map<offset, argumentIndex>
        std::map<unsigned int, int> simplePushLoads;
    };

    struct StatelessPushInfo
    {
        unsigned int addressOffset = 0;
        bool isStatic = false;
    };

    struct DynamicBufferInfo
    {
        // If numOffsets > 0, dynamic buffer offsets occupy a contiguous region
        // of runtime values with indices in [firstIndex, firstIndex + numOffsets).
        unsigned int firstIndex = 0;
        unsigned int numOffsets = 0;
    };

    // simplePushInfoArr needs to be initialized to a vector of size g_c_maxNumberOfBufferPushed, which we are doing in module MD initialization done in code gen context
    // All the pushinfo below is mapping to an argument number (int) so that we can retrieve relevant Argument as a value pointer from Function
    struct PushInfo
    {
        std::vector<StatelessPushInfo> pushableAddresses;

        // Indices of RuntimeValues that can be used to compute surface state
        // offsets for the bindless push along with the Descriptor Table Offset.
        std::vector<unsigned int> bindlessPushInfo;

        // Dynamic buffer offsets info.
        // Used only on with clients that support dynamic buffers.
        DynamicBufferInfo dynamicBufferInfo;
        unsigned int MaxNumberOfPushedBuffers = 0; ///> specifies the maximum number of buffers available for the simple push mechanism for current shader.

        unsigned int inlineConstantBufferSlot = INVALID_CONSTANT_BUFFER_INVALID_ADDR; // slot of the inlined constant buffer
        unsigned int inlineConstantBufferOffset = INVALID_CONSTANT_BUFFER_INVALID_ADDR;    // offset of the inlined constant buffer
        unsigned int inlineConstantBufferGRFOffset = INVALID_CONSTANT_BUFFER_INVALID_ADDR;

        std::map<ConstantAddress, int> constants;
        std::map<unsigned int, SInputDesc> inputs;
        std::map<unsigned int, int> constantReg;
        std::array<SimplePushInfo, g_c_maxNumberOfBufferPushed> simplePushInfoArr;
        unsigned int simplePushBufferUsed = 0;

        std::vector<ArgDependencyInfoMD> pushAnalysisWIInfos;
        //For non RayTracing shader using RayQuery opcodes
        //it is RTGlobals PTR is passed as push constant
        unsigned int inlineRTGlobalPtrOffset = 0;
        unsigned int rtSyncSurfPtrOffset = 0;
    };

    struct InlineProgramScopeBuffer
    {
        int alignment;
        unsigned allocSize;
        std::vector<unsigned char> Buffer;
    };

    struct ImmConstantInfo
    {
        std::vector<char> data;
    };

    struct PointerProgramBinaryInfo
    {
        int PointerBufferIndex;
        int PointerOffset;
        int PointeeAddressSpace;
        int PointeeBufferIndex;
    };

    struct PointerAddressRelocInfo
    {
        unsigned BufferOffset;
        unsigned PointerSize;
        std::string Symbol;
    };

    struct ShaderData
    {
        unsigned int numReplicas = 0;
    };

    struct SrvMapData
    {
        unsigned int resourceRangeID;
        unsigned int indexIntoRange;
        bool hfCandidate = false;
        unsigned int runtimeValue;
        unsigned int ptrAddressSpace;
    };

    struct URBLayoutInfo
    {
        bool has64BVertexHeaderInput = false;
        bool has64BVertexHeaderOutput = false;
        bool hasVertexHeader = true;
    };

    struct SPIRVCapabilities
    {

        bool globalVariableDecorationsINTEL = false;
    };

    //metadata for the entire module
    struct ModuleMetaData
    {
        bool isPrecise = false;
        CompOptions compOpt;
        llvm::MapVector<llvm::Function*, IGC::FunctionMetaData> FuncMD;
        PushInfo pushInfo;
        PixelShaderInfo psInfo;
        ComputeShaderInfo csInfo;
        MeshShaderInfo msInfo;
        TaskShaderInfo taskInfo;
        uint32_t NBarrierCnt = 0;
        RayTraceModuleInfo rtInfo;
        uint32_t CurUniqueIndirectIdx = DefaultIndirectIdx;
        std::map<uint32_t, std::array<uint32_t, 4>> inlineDynTextures;
        std::vector<InlineResInfo> inlineResInfoData;
        ImmConstantInfo immConstant;
        std::vector<InlineProgramScopeBuffer> inlineConstantBuffers;
        std::vector<InlineProgramScopeBuffer> inlineGlobalBuffers;
        std::vector<PointerProgramBinaryInfo> GlobalPointerProgramBinaryInfos;
        std::vector<PointerProgramBinaryInfo> ConstantPointerProgramBinaryInfos;
        std::vector<PointerAddressRelocInfo> GlobalBufferAddressRelocInfo;
        std::vector<PointerAddressRelocInfo> ConstantBufferAddressRelocInfo;
        std::map<uint32_t, uint32_t> forceLscCacheList;
        std::vector<SrvMapData> SrvMap;
        std::vector<uint32_t> RasterizerOrderedByteAddressBuffer;
        unsigned int MinNOSPushConstantSize = 0;
        llvm::MapVector<llvm::GlobalVariable*, int> inlineProgramScopeOffsets;
        ShaderData shaderData;
        URBLayoutInfo URBInfo;
        bool UseBindlessImage = false;
        bool enableRangeReduce = false;

        //when true, compiler enables MatchMad optimization for VS
        bool allowMatchMadOptimizationforVS = false;

        bool disableMemOptforNegativeOffsetLoads = false;

        // When true compiler can assume that resources bound to two different
        // bindings do not alias.
        bool statefulResourcesNotAliased = false;
        bool disableMixMode = false;

        unsigned int privateMemoryPerWI = 0;

        std::map<llvm::Function*, unsigned int> PrivateMemoryPerFG;

        SPIRVCapabilities capabilities;

        std::array<uint64_t, NUM_SHADER_RESOURCE_VIEW_SIZE> m_ShaderResourceViewMcsMask{};
        unsigned int computedDepthMode = 0; //Defaults to 0 meaning depth mode is off
        bool isHDCFastClearShader = false;
    };
    void serialize(const IGC::ModuleMetaData &moduleMD, llvm::Module* module);
    void deserialize(IGC::ModuleMetaData &deserializedMD, const llvm::Module* module);

    // Raytracing query functions
    bool isBindless(const IGC::FunctionMetaData &funcMD);
    bool isContinuation(const IGC::FunctionMetaData& funcMD);
    bool isCallStackHandler(const IGC::FunctionMetaData &funcMD);

    // User annotations query functions
    unsigned extractAnnotatedNumThreads(const IGC::FunctionMetaData& funcMD);
}
