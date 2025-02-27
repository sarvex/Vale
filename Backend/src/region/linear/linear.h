#ifndef REGION_COMMON_LINEAR_LINEAR_H_
#define REGION_COMMON_LINEAR_LINEAR_H_

#include <llvm-c/Types.h>
#include "../../globalstate.h"
#include <iostream>
#include "../common/primitives.h"
#include "../../function/expressions/shared/afl.h"
#include "../../function/function.h"
#include "../common/defaultlayout/structs.h"
#include "linearstructs.h"

class Linear : public IRegion {
public:
  Linear(GlobalState* globalState_);


  void alias(
      AreaAndFileAndLine from,
      FunctionState* functionState,
      LLVMBuilderRef builder,
      Reference* sourceRef,
      Ref ref) override;

  void dealias(
      AreaAndFileAndLine from,
      FunctionState* functionState,
      LLVMBuilderRef builder,
      Reference* sourceMT,
      Ref sourceRef) override;

  Ref lockWeak(
      FunctionState* functionState,
      LLVMBuilderRef builder,
      bool thenResultIsNever,
      bool elseResultIsNever,
      Reference* resultOptTypeM,
//      LLVMTypeRef resultOptTypeL,
      Reference* constraintRefM,
      Reference* sourceWeakRefMT,
      Ref sourceWeakRefLE,
      bool weakRefKnownLive,
      std::function<Ref(LLVMBuilderRef, Ref)> buildThen,
      std::function<Ref(LLVMBuilderRef)> buildElse) override;

  Ref asSubtype(
      FunctionState* functionState,
      LLVMBuilderRef builder,
      Reference* resultOptTypeM,
      Reference* sourceInterfaceRefMT,
      Ref sourceInterfaceRef,
      bool sourceRefKnownLive,
      Kind* targetKind,
      std::function<Ref(LLVMBuilderRef, Ref)> buildThen,
      std::function<Ref(LLVMBuilderRef)> buildElse) override;

  LLVMTypeRef translateType(Reference* referenceM) override;

  LLVMValueRef getCensusObjectId(
      AreaAndFileAndLine checkerAFL,
      FunctionState* functionState,
      LLVMBuilderRef builder,
      Reference* refM,
      Ref ref) override;

  Ref upcastWeak(
      FunctionState* functionState,
      LLVMBuilderRef builder,
      WeakFatPtrLE sourceRefLE,
      StructKind* sourceStructKindM,
      Reference* sourceStructTypeM,
      InterfaceKind* targetInterfaceKindM,
      Reference* targetInterfaceTypeM) override;

  void declareStaticSizedArray(StaticSizedArrayDefinitionT* ssaDefM) override;
  void defineStaticSizedArray(StaticSizedArrayDefinitionT* ssaDefM) override;
  void declareStaticSizedArrayExtraFunctions(StaticSizedArrayDefinitionT* ssaDef) override;
  void defineStaticSizedArrayExtraFunctions(StaticSizedArrayDefinitionT* ssaDef) override;

  void declareRuntimeSizedArray(RuntimeSizedArrayDefinitionT* rsaDefM) override;
  void declareRuntimeSizedArrayExtraFunctions(RuntimeSizedArrayDefinitionT* rsaDefM) override;
  void defineRuntimeSizedArray(RuntimeSizedArrayDefinitionT* rsaDefM) override;
  void defineRuntimeSizedArrayExtraFunctions(RuntimeSizedArrayDefinitionT* rsaDefM) override;

  void declareStruct(StructDefinition* structDefM) override;
  void declareStructExtraFunctions(StructDefinition* structDefM) override;
  void defineStruct(StructDefinition* structDefM) override;
  void defineStructExtraFunctions(StructDefinition* structDefM) override;

  void declareInterface(InterfaceDefinition* interfaceDefM) override;
  void declareInterfaceExtraFunctions(InterfaceDefinition* structDefM) override;
  void defineInterface(InterfaceDefinition* interfaceDefM) override;
  void defineInterfaceExtraFunctions(InterfaceDefinition* structDefM) override;

  void declareEdge(Edge* edge) override;
  void defineEdge(Edge* edge) override;

  Ref weakAlias(
      FunctionState* functionState, LLVMBuilderRef builder, Reference* sourceRefMT, Reference* targetRefMT, Ref sourceRef) override;

  void discardOwningRef(
      AreaAndFileAndLine from,
      FunctionState* functionState,
      BlockState* blockState,
      LLVMBuilderRef builder,
      Reference* sourceMT,
      Ref sourceRef) override;


  void noteWeakableDestroyed(
      FunctionState* functionState,
      LLVMBuilderRef builder,
      Reference* refM,
      ControlBlockPtrLE controlBlockPtrLE) override;

  Ref loadMember(
      FunctionState* functionState,
      LLVMBuilderRef builder,
      Ref regionInstanceRef,
      Reference* structRefMT,
      Ref structRef,
      bool structKnownLive,
      int memberIndex,
      Reference* expectedMemberType,
      Reference* targetMemberType,
      const std::string& memberName) override;

  void storeMember(
      FunctionState* functionState,
      LLVMBuilderRef builder,
      Ref regionInstanceRef,
      Reference* structRefMT,
      Ref structRef,
      bool structKnownLive,
      int memberIndex,
      const std::string& memberName,
      Reference* newMemberRefMT,
      Ref newMemberRef) override;

  std::tuple<LLVMValueRef, LLVMValueRef> explodeInterfaceRef(
      FunctionState* functionState,
      LLVMBuilderRef builder,
      Reference* virtualParamMT,
      Ref virtualArgRef) override;


  void aliasWeakRef(
      AreaAndFileAndLine from,
      FunctionState* functionState,
      LLVMBuilderRef builder,
      Reference* weakRefMT,
      Ref weakRef) override;

  void discardWeakRef(
      AreaAndFileAndLine from,
      FunctionState* functionState,
      LLVMBuilderRef builder,
      Reference* weakRefMT,
      Ref weakRef) override;

  Ref getIsAliveFromWeakRef(
      FunctionState* functionState,
      LLVMBuilderRef builder,
      Reference* weakRefM,
      Ref weakRef,
      bool knownLive) override;

  LLVMValueRef getStringBytesPtr(
      FunctionState* functionState,
      LLVMBuilderRef builder,
      Ref regionInstanceRef,
      Ref ref) override;

  Ref allocate(
      Ref regionInstanceRef,
      AreaAndFileAndLine from,
      FunctionState* functionState,
      LLVMBuilderRef builder,
      Reference* desiredStructMT,
      const std::vector<Ref>& memberRefs) override;

  Ref upcast(
      FunctionState* functionState,
      LLVMBuilderRef builder,

      Reference* sourceStructMT,
      StructKind* sourceStructKindM,
      Ref sourceRefLE,

      Reference* targetInterfaceTypeM,
      InterfaceKind* targetInterfaceKindM) override;

  WrapperPtrLE lockWeakRef(
      AreaAndFileAndLine from,
      FunctionState* functionState,
      LLVMBuilderRef builder,
      Reference* refM,
      Ref weakRefLE,
      bool weakRefKnownLive) override;

  // Returns a LLVMValueRef for a ref to the string object.
  // The caller should then use getStringBytesPtr to then fill the string's contents.
  Ref constructStaticSizedArray(
      Ref regionInstanceRef,
      FunctionState* functionState,
      LLVMBuilderRef builder,
      Reference* referenceM,
      StaticSizedArrayT* kindM) override;

  // should expose a dereference thing instead
//  LLVMValueRef getStaticSizedArrayElementsPtr(
//      LLVMBuilderRef builder,
//      LLVMValueRef staticSizedArrayWrapperPtrLE) override;
//  LLVMValueRef getRuntimeSizedArrayElementsPtr(
//      LLVMBuilderRef builder,
//      LLVMValueRef runtimeSizedArrayWrapperPtrLE) override;

  Ref getRuntimeSizedArrayLength(
      FunctionState* functionState,
      LLVMBuilderRef builder,
      Ref regionInstanceRef,
      Reference* rsaRefMT,
      Ref arrayRef,
      bool arrayKnownLive) override;

  Ref getRuntimeSizedArrayCapacity(
      FunctionState* functionState,
      LLVMBuilderRef builder,
      Ref regionInstanceRef,
      Reference* rsaRefMT,
      Ref arrayRef,
      bool arrayKnownLive) override;

  LLVMValueRef checkValidReference(
      AreaAndFileAndLine checkerAFL,
      FunctionState* functionState,
      LLVMBuilderRef builder,
      bool expectLive,
      Reference* refM,
      Ref ref) override;


  // TODO maybe combine with alias/acquireReference?
  // After we load from a local, member, or element, we can feed the result through this
  // function to turn it into a desired ownership.
  // Example:
  // - Can load from an owning ref member to get a constraint ref.
  // - Can load from a constraint ref member to get a weak ref.
  Ref upgradeLoadResultToRefWithTargetOwnership(
      FunctionState* functionState,
      LLVMBuilderRef builder,
      Reference* sourceType,
      Reference* targetType,
      LoadResult sourceRef) override;

  void checkInlineStructType(
      FunctionState* functionState,
      LLVMBuilderRef builder,
      Reference* refMT,
      Ref ref) override;

  LoadResult loadElementFromSSA(
      FunctionState* functionState,
      LLVMBuilderRef builder,
      Ref regionInstanceRef,
      Reference* ssaRefMT,
      StaticSizedArrayT* ssaMT,
      Ref arrayRef,
      bool arrayKnownLive,
      Ref indexRef) override;
  LoadResult loadElementFromRSA(
      FunctionState* functionState,
      LLVMBuilderRef builder,
      Ref regionInstanceRef,
      Reference* rsaRefMT,
      RuntimeSizedArrayT* rsaMT,
      Ref arrayRef,
      bool arrayKnownLive,
      Ref indexRef) override;


  Ref storeElementInRSA(
      FunctionState* functionState,
      LLVMBuilderRef builder,
      Reference* rsaRefMT,
      RuntimeSizedArrayT* rsaMT,
      Ref arrayRef,
      bool arrayKnownLive,
      Ref indexRef,
      Ref elementRef) override;


  void deallocate(
      AreaAndFileAndLine from,
      FunctionState* functionState,
      LLVMBuilderRef builder,
      Reference* refMT,
      Ref ref) override;


  Ref constructRuntimeSizedArray(
      Ref regionInstanceRef,
      FunctionState* functionState,
      LLVMBuilderRef builder,
      Reference* rsaMT,
      RuntimeSizedArrayT* runtimeSizedArrayT,
      Ref capacityRef,
      const std::string& typeName) override;

  void pushRuntimeSizedArrayNoBoundsCheck(
      FunctionState* functionState,
      LLVMBuilderRef builder,
      Ref regionInstanceRef,
      Reference* rsaRefMT,
      RuntimeSizedArrayT* rsaMT,
      Ref arrayRef,
      bool arrayRefKnownLive,
      Ref indexRef,
      Ref elementRef) override;

  Ref popRuntimeSizedArrayNoBoundsCheck(
      FunctionState* functionState,
      LLVMBuilderRef builder,
      Ref regionInstanceRef,
      Reference* rsaRefMT,
      RuntimeSizedArrayT* rsaMT,
      Ref arrayRef,
      bool arrayRefKnownLive,
      Ref indexRef) override;

  void initializeElementInSSA(
      FunctionState* functionState,
      LLVMBuilderRef builder,
      Ref regionInstanceRef,
      Reference* ssaRefMT,
      StaticSizedArrayT* ssaMT,
      Ref arrayRef,
      bool arrayRefKnownLive,
      Ref indexRef,
      Ref elementRef) override;

  Ref deinitializeElementFromSSA(
      FunctionState* functionState,
      LLVMBuilderRef builder,
      Reference* ssaRefMT,
      StaticSizedArrayT* ssaMT,
      Ref arrayRef,
      bool arrayRefKnownLive,
      Ref indexRef) override;

  Ref mallocStr(
      Ref regionInstanceRef,
      FunctionState* functionState,
      LLVMBuilderRef builder,
      LLVMValueRef lengthLE,
      LLVMValueRef sourceCharsPtrLE) override;

  Ref innerMallocStr(
      Ref regionInstanceRef,
      FunctionState* functionState,
      LLVMBuilderRef builder,
      LLVMValueRef lengthLE,
      LLVMValueRef sourceCharsPtrLE,
      Ref dryRunBoolRef);

  Ref innerConstructStaticSizedArray(
      Ref regionInstanceRef,
      FunctionState* functionState,
      LLVMBuilderRef builder,
      Reference* referenceM,
      StaticSizedArrayT* kindM,
      Ref dryRunBoolRef);

  LLVMValueRef getStringLen(
      FunctionState* functionState,
      LLVMBuilderRef builder,
      Ref regionInstanceRef,
      Ref ref) override;

  std::string getExportName(Package* currentPackage, Reference* refMT, bool includeProjectName) override;

  std::string generateStructDefsC(
    Package* currentPackage,
      StructDefinition* refMT) override;
  std::string generateInterfaceDefsC(
    Package* currentPackage,
      InterfaceDefinition* refMT) override;
  std::string generateStaticSizedArrayDefsC(
    Package* currentPackage,
      StaticSizedArrayDefinitionT* ssaDefM) override;
  std::string generateRuntimeSizedArrayDefsC(
    Package* currentPackage,
      RuntimeSizedArrayDefinitionT* rsaDefM) override;


  LLVMTypeRef getExternalType(
      Reference* refMT) override;

  std::pair<Ref, Ref> receiveUnencryptedAlienReference(
      FunctionState* functionState,
      LLVMBuilderRef builder,
      Ref sourceRegionInstanceRef,
      Ref targetRegionInstanceRef,
      Reference* sourceRefMT,
      Reference* targetRefMT,
      Ref sourceRef) override;

  Ref receiveAndDecryptFamiliarReference(
      FunctionState* functionState,
      LLVMBuilderRef builder,
      Reference* sourceRefMT,
      LLVMValueRef sourceRefLE) override;

  LLVMValueRef encryptAndSendFamiliarReference(
      FunctionState* functionState,
      LLVMBuilderRef builder,
      Reference* sourceRefMT,
      Ref sourceRef) override;

  LoadResult loadMember(
      FunctionState* functionState,
      LLVMBuilderRef builder,
      Ref regionInstanceRef,
      Reference* structRefMT,
      LLVMTypeRef structInnerLT,
      Ref structRef,
      int memberIndex,
      Reference* expectedMemberType,
      Reference* targetType,
      const std::string& memberName);

  void checkValidReference(
      AreaAndFileAndLine checkerAFL,
      FunctionState* functionState,
      LLVMBuilderRef builder,
      bool expectLive,
      KindStructs* kindStructs,
      Reference* refM,
      LLVMValueRef refLE);

  LLVMTypeRef getInterfaceMethodVirtualParamAnyType(Reference* reference) override;

  RegionId* getRegionId() override;

  void declareExtraFunctions() override;
  void defineExtraFunctions() override;

  // Temporary, gets the corresponding Linear type reference.
  Kind* linearizeKind(Kind* kindMT);
  StaticSizedArrayT* unlinearizeSSA(StaticSizedArrayT* kindMT);
  StructKind* unlinearizeStructKind(StructKind* kindMT);
  InterfaceKind* unlinearizeInterfaceKind(InterfaceKind* kindMT);
  StructKind* linearizeStructKind(StructKind* kindMT);
  InterfaceKind* linearizeInterfaceKind(InterfaceKind* kindMT);
  Reference* linearizeReference(Reference* immRcRefMT);
  Reference* unlinearizeReference(Reference* hostRefMT);

  Weakability getKindWeakability(Kind* kind) override;

  FuncPtrLE getInterfaceMethodFunctionPtr(
      FunctionState* functionState,
      LLVMBuilderRef builder,
      Reference* virtualParamMT,
      Ref virtualArgRef,
      int indexInEdge) override;

  LLVMValueRef stackify(
      FunctionState* functionState,
      LLVMBuilderRef builder,
      Local* local,
      Ref refToStore,
      bool knownLive) override;

  Ref unstackify(FunctionState* functionState, LLVMBuilderRef builder, Local* local, LLVMValueRef localAddr) override;

  Ref loadLocal(FunctionState* functionState, LLVMBuilderRef builder, Local* local, LLVMValueRef localAddr) override;

  Ref localStore(
      FunctionState* functionState, LLVMBuilderRef builder, Local* local, LLVMValueRef localAddr, Ref refToStore, bool knownLive) override;

  void mainSetup(FunctionState* functionState, LLVMBuilderRef builder) override {}
  void mainCleanup(FunctionState* functionState, LLVMBuilderRef builder) override {}

  Reference* getRegionRefType() override;

  // This is only temporarily virtual, while we're still creating fake ones on the fly.
  // Soon it'll be non-virtual, and parameters will differ by region... like the below one.
  Ref createRegionInstanceLocal(FunctionState* functionState, LLVMBuilderRef builder) override;
  // This will replace the above createRegionInstanceLocal once it's non-virtual.
  Ref createRegionInstanceLocal(
      FunctionState *functionState,
      LLVMBuilderRef builder,
      LLVMValueRef useOffsetsLE,
      LLVMValueRef bufferBeginOffsetLE);

private:
  void declareConcreteSerializeFunction(Kind* valeKindM);
  void defineConcreteSerializeFunction(Kind* valeKindM);
  void declareInterfaceSerializeFunction(InterfaceKind* valeKind);
  void defineEdgeSerializeFunction(Edge* edge);

  void initializeMember(
      FunctionState* functionState,
      LLVMBuilderRef builder,
      Ref regionInstanceRef,
      Reference* structRefMT,
      Ref structRef,
      bool structKnownLive,
      int memberIndex,
      const std::string& memberName,
      Reference* newMemberRefMT,
      Ref newMemberRef);

  Ref assembleInterfaceRef(
      LLVMBuilderRef builder,
      Reference* targetInterfaceTypeM,
      LLVMValueRef structRefLE,
      LLVMValueRef edgeNumberLE);

  Ref innerConstructRuntimeSizedArray(
      Ref regionInstanceRef,
      FunctionState* functionState,
      LLVMBuilderRef builder,
      Reference* rsaMT,
      RuntimeSizedArrayT* runtimeSizedArrayT,
      Ref sizeRef,
      Ref dryRunBoolRef);

  Prototype* getSerializePrototype(Kind* valeKind);
  Prototype* getSerializeThunkPrototype(StructKind* structKind, InterfaceKind* interfaceKind);

  LLVMValueRef predictShallowSize(
      FunctionState* functionState,
      LLVMBuilderRef builder,
      bool includeHeader,
      Kind* kind,
      // Ignored if kind isn't an array or string.
      // If it's a string, this will be the length of the string.
      // If it's an array, this will be the number of elements.
      LLVMValueRef lenIntLE);

  Ref innerAllocate(
      Ref regionInstanceRef,
      AreaAndFileAndLine from,
      FunctionState* functionState,
      LLVMBuilderRef builder,
      Reference* desiredStructMT);

  InterfaceMethod* getSerializeInterfaceMethod(Kind* valeKind);

  Ref callSerialize(
      FunctionState *functionState,
      LLVMBuilderRef builder,
      Kind* valeKind,
      Ref regionInstanceRef,
      Ref sourceRegionInstanceRef,
      Ref objectRef,
      Ref dryRunBoolRef);

  // Does the entire serialization process: measuring the length, allocating a buffer, and
  // serializing into it.
  // Returns the pointer to it and the size.
  std::pair<Ref, Ref> topLevelSerialize(
      FunctionState* functionState,
      LLVMBuilderRef builder,
      Ref regionInstanceRef,
      Ref sourceRegionInstanceRef,
      Kind* valeKind,
      Ref ref);

public:
  LLVMValueRef getRegionInstanceDestinationBufferStartPtr(
      FunctionState* functionState,
      LLVMBuilderRef builder,
      Ref regionInstanceRef);
private:
  void setRegionInstanceDestinationBufferStartPtr(
      FunctionState* functionState,
      LLVMBuilderRef builder,
      Ref regionInstanceRef,
      LLVMValueRef destinationBufferStartPtrLE);
  void setRegionInstanceDestinationOffset(
      FunctionState* functionState,
      LLVMBuilderRef builder,
      Ref regionInstanceRef,
      LLVMValueRef destinationOffsetLE);
  void setRegionInstanceUseOffsets(
      FunctionState* functionState,
      LLVMBuilderRef builder,
      Ref regionInstanceRef,
      LLVMValueRef useOffsetsLE);
  LLVMValueRef getRegionInstanceUseOffsets(
      FunctionState* functionState,
      LLVMBuilderRef builder,
      Ref regionInstanceRef);
  void setRegionInstanceBufferBeginOffset(
      FunctionState* functionState,
      LLVMBuilderRef builder,
      Ref regionInstanceRef,
      LLVMValueRef bufferBeginOffsetLE);
  LLVMValueRef getRegionInstanceBufferBeginOffset(
      FunctionState* functionState,
      LLVMBuilderRef builder,
      Ref regionInstanceRef);
  void setRegionInstanceSerializedAddressAdjuster(
      FunctionState* functionState,
      LLVMBuilderRef builder,
      Ref regionInstanceRef,
      LLVMValueRef serializedAddressAdjusterLE);
  // Returns the address space begin pointer, see PSBCBO.
  LLVMValueRef getRegionInstanceSerializedAddressAdjuster(
      FunctionState* functionState,
      LLVMBuilderRef builder,
      Ref regionInstanceRef);

  void bumpDestinationOffset(
      FunctionState* functionState,
      LLVMBuilderRef builder,
      Ref regionInstanceRef,
      LLVMValueRef sizeIntLE);

//  void reserveRootMetadataBytesIfNeeded(
//      FunctionState* functionState,
//      LLVMBuilderRef builder,
//      Ref regionInstanceRef);

  LLVMValueRef getDestinationPtr(
      FunctionState* functionState,
      LLVMBuilderRef builder,
      Ref regionInstanceRef);

  Ref getDestinationRef(
      FunctionState* functionState,
      LLVMBuilderRef builder,
      Ref regionInstanceRef,
      Reference* desiredRefMT);

  LLVMValueRef getRegionInstanceDestinationOffset(
      FunctionState* functionState,
      LLVMBuilderRef builder,
      Ref regionInstanceRef);

  // This is meant to be called just before we write to a serialized buffer, if it's
  // a pointer (or a fat pointer) it will make it relative to the buffer begin or file
  // begin or 0 or whatever, see PSBCBO.
  LLVMValueRef translateBetweenBufferAddressAndPointer(
      FunctionState* functionState,
      LLVMBuilderRef builder,
      Ref regionInstanceRef,
      Reference* hostRefMT,
      LLVMValueRef unadjustedHostRefLE,
      bool bufferAddressToPointer);

  void addMappedKind(Kind* valeKind, Kind* hostKind) {
    hostKindByValeKind.emplace(valeKind, hostKind);
    valeKindByHostKind.emplace(hostKind, valeKind);
  }

  GlobalState* globalState = nullptr;

  LinearStructs structs;

  std::unordered_map<
      Kind*,
      Kind*,
      AddressHasher<Kind*>> hostKindByValeKind;
  std::unordered_map<
      Kind*,
      Kind*,
      AddressHasher<Kind*>> valeKindByHostKind;

  std::string namePrefix = "__Linear";

  StructKind* regionKind = nullptr;
  Reference* regionRefMT = nullptr;


//  StructKind* startMetadataKind = nullptr;
//  Reference* startMetadataRefMT = nullptr;
//
//  StructKind* rootMetadataKind = nullptr;
//  Reference* rootMetadataRefMT = nullptr;

  Str* linearStr = nullptr;
  Reference* linearStrRefMT = nullptr;
};

#endif
