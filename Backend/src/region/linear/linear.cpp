#include "../../function/expressions/shared/shared.h"
#include "../../utils/counters.h"
#include "../../utils/branch.h"
#include "../common/controlblock.h"
#include "../common/heap.h"
#include "../../function/expressions/shared/string.h"
#include "../common/common.h"
#include <sstream>
#include "../../function/expressions/shared/elements.h"
#include "linear.h"
#include "../../translatetype.h"
#include "../rcimm/rcimm.h"
#include <region/common/migration.h>

Ref unsafeCast(
    GlobalState* globalState,
    FunctionState* functionState,
    LLVMBuilderRef builder,
    bool expectLive,
    Reference* sourceRefMT,
    Reference* desiredRefMT,
    Ref sourceRef) {
  auto sourcePtrLE = globalState->getRegion(sourceRefMT)->checkValidReference(FL(), functionState, builder, expectLive, sourceRefMT, sourceRef);
  auto desiredPtrLT = globalState->getRegion(sourceRefMT)->translateType(desiredRefMT);
  auto desiredPtrLE = LLVMBuildPointerCast(builder, sourcePtrLE, desiredPtrLT, "destStructPtr");
  auto desiredRef = wrap(globalState->getRegion(desiredRefMT), desiredRefMT, desiredPtrLE);
  return desiredRef;
}

Linear::Linear(GlobalState* globalState_)
  : globalState(globalState_),
    structs(globalState_),
    hostKindByValeKind(0, globalState->addressNumberer->makeHasher<Kind*>()),
    valeKindByHostKind(0, globalState->addressNumberer->makeHasher<Kind*>()) {

  regionKind =
      globalState->metalCache->getStructKind(
          globalState->metalCache->getName(
              globalState->metalCache->builtinPackageCoord, namePrefix + "_Region"));
  regionRefMT =
      globalState->metalCache->getReference(
          Ownership::BORROW, Location::YONDER, regionKind);
  globalState->regionIdByKind.emplace(regionKind, globalState->metalCache->linearRegionId);
  structs.declareStruct(regionKind);
  structs.defineStruct(regionKind, {
      // Pointer to the beginning of the destination buffer
      LLVMPointerType(LLVMInt8TypeInContext(globalState->context), 0),
      // Offset into the destination buffer to write to
      LLVMInt64TypeInContext(globalState->context),
      // Whether we're writing pointers to other parts of the buffer, or just offsets relative to the buffer start.
      // 0 if we're using pointers, 1 if we're using buffers. If we're using buffers, then the Serialized Address
      // Adjuster becomes relevant. Only supplied by the outside world, never changed. See PSBCBO.
      LLVMInt1TypeInContext(globalState->context),
      // Buffer begin offset, see PSBCBO. Only supplied by the outside world, never changed.
      LLVMInt64TypeInContext(globalState->context),
      // Serialized Address Adjuster, see PSBCBO.
      LLVMInt64TypeInContext(globalState->context),
      // Eventually we might want a hash map or something in here, if we want to avoid serialize duplicates
  });

  linearStr = globalState->metalCache->getStr(globalState->metalCache->linearRegionId);
  linearStrRefMT =
      globalState->metalCache->getReference(
          Ownership::SHARE, Location::YONDER, linearStr);

  addMappedKind(globalState->metalCache->vooid, globalState->metalCache->getVoid(getRegionId()));
  addMappedKind(globalState->metalCache->i32, globalState->metalCache->getInt(getRegionId(), 32));
  addMappedKind(globalState->metalCache->i64, globalState->metalCache->getInt(getRegionId(), 64));
  addMappedKind(globalState->metalCache->boool, globalState->metalCache->getBool(getRegionId()));
  addMappedKind(globalState->metalCache->flooat, globalState->metalCache->getFloat(getRegionId()));
  addMappedKind(globalState->metalCache->str, linearStr);
  addMappedKind(globalState->metalCache->never, globalState->metalCache->getNever(getRegionId()));
}

Reference* Linear::getRegionRefType() {
  return regionRefMT;
}

void Linear::declareExtraFunctions() {
  auto boolMT = globalState->metalCache->boolRef;
  auto valeStrMT = globalState->metalCache->strRef;
  auto sourceRegionRefMT =
      globalState->rcImm->getRegionRefType();
  auto prototype =
      globalState->metalCache->getPrototype(
          globalState->serializeName, linearStrRefMT, {regionRefMT, sourceRegionRefMT, valeStrMT, boolMT});
  auto nameL = globalState->serializeName->name + "__str";
  declareExtraFunction(globalState, prototype, nameL);
}

void Linear::defineExtraFunctions() {
  defineConcreteSerializeFunction(globalState->metalCache->str);
}

RegionId* Linear::getRegionId() {
  return globalState->metalCache->linearRegionId;
}

void Linear::alias(
    AreaAndFileAndLine from,
    FunctionState* functionState,
    LLVMBuilderRef builder,
    Reference* sourceRef,
    Ref ref) {
  assert(false); // impl
}

void Linear::dealias(
    AreaAndFileAndLine from,
    FunctionState* functionState,
    LLVMBuilderRef builder,
    Reference* sourceMT,
    Ref sourceRef) {
  auto sourceRefLE = checkValidReference(FL(), functionState, builder, true, sourceMT, sourceRef);

  auto sourceI8PtrLE =
      LLVMBuildPointerCast(
          builder,
          sourceRefLE,
          LLVMPointerType(LLVMInt8TypeInContext(globalState->context), 0),
          "extStrPtrLE");

  buildFlare(FL(), globalState, functionState, builder, "Freeing ", ptrToIntLE(globalState, builder, sourceI8PtrLE));
  globalState->externs->free.call(builder, {sourceI8PtrLE}, "");
}

Ref Linear::lockWeak(
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
    std::function<Ref(LLVMBuilderRef)> buildElse) {
  assert(false);
  exit(1);
}

Ref Linear::asSubtype(
    FunctionState* functionState,
    LLVMBuilderRef builder,
    Reference* resultOptTypeM,
    Reference* sourceInterfaceRefMT,
    Ref sourceInterfaceRef,
    bool sourceRefKnownLive,
    Kind* targetKind,
    std::function<Ref(LLVMBuilderRef, Ref)> buildThen,
    std::function<Ref(LLVMBuilderRef)> buildElse) {
  assert(false);
  exit(1);
}

LLVMTypeRef Linear::translateType(Reference* referenceM) {
  if (auto innt = dynamic_cast<Int*>(referenceM->kind)) {
    assert(referenceM->ownership == Ownership::SHARE);
    return LLVMIntTypeInContext(globalState->context, innt->bits);
  } else if (dynamic_cast<Bool*>(referenceM->kind) != nullptr) {
    assert(referenceM->ownership == Ownership::SHARE);
    return LLVMInt8TypeInContext(globalState->context);
  } else if (dynamic_cast<Float*>(referenceM->kind) != nullptr) {
    assert(referenceM->ownership == Ownership::SHARE);
    return LLVMDoubleTypeInContext(globalState->context);
  } else if (dynamic_cast<Never*>(referenceM->kind) != nullptr) {
    return LLVMArrayType(LLVMIntTypeInContext(globalState->context, NEVER_INT_BITS), 0);
  } else if (dynamic_cast<Str *>(referenceM->kind) != nullptr) {
    assert(referenceM->location != Location::INLINE);
    assert(referenceM->ownership == Ownership::SHARE);
    return LLVMPointerType(structs.getStringStruct(), 0);
  } else if (auto staticSizedArrayMT = dynamic_cast<StaticSizedArrayT *>(referenceM->kind)) {
    assert(referenceM->location != Location::INLINE);
    auto staticSizedArrayCountedStructLT = structs.getStaticSizedArrayStruct(staticSizedArrayMT);
    return LLVMPointerType(staticSizedArrayCountedStructLT, 0);
  } else if (auto runtimeSizedArrayMT =
      dynamic_cast<RuntimeSizedArrayT *>(referenceM->kind)) {
    assert(referenceM->location != Location::INLINE);
    auto runtimeSizedArrayCountedStructLT =
        structs.getRuntimeSizedArrayStruct(runtimeSizedArrayMT);
    return LLVMPointerType(runtimeSizedArrayCountedStructLT, 0);
  } else if (auto structKind =
      dynamic_cast<StructKind *>(referenceM->kind)) {
    if (referenceM->location == Location::INLINE) {
      auto innerStructL = structs.getStructStruct(structKind);
      return innerStructL;
    } else {
      auto countedStructL = structs.getStructStruct(structKind);
      return LLVMPointerType(countedStructL, 0);
    }
  } else if (auto interfaceKind =
      dynamic_cast<InterfaceKind *>(referenceM->kind)) {
    assert(referenceM->location != Location::INLINE);
    auto interfaceRefStructL =
        structs.getInterfaceRefStruct(interfaceKind);
    return interfaceRefStructL;
  } else if (dynamic_cast<Never*>(referenceM->kind)) {
    auto result = LLVMPointerType(makeNeverType(globalState), 0);
    assert(LLVMTypeOf(globalState->neverPtr) == result);
    return result;
  } else if (dynamic_cast<Void*>(referenceM->kind)) {
    return LLVMVoidTypeInContext(globalState->context);
  } else {
    std::cerr << "Unimplemented type: " << typeid(*referenceM->kind).name() << std::endl;
    assert(false);
    return nullptr;
  }
}

LLVMValueRef Linear::getCensusObjectId(
    AreaAndFileAndLine checkerAFL,
    FunctionState *functionState,
    LLVMBuilderRef builder,
    Reference *refM,
    Ref ref) {
  return constI64LE(globalState, 0);
}

Ref Linear::upcastWeak(
    FunctionState* functionState,
    LLVMBuilderRef builder,
    WeakFatPtrLE sourceRefLE,
    StructKind* sourceStructKindM,
    Reference* sourceStructTypeM,
    InterfaceKind* targetInterfaceKindM,
    Reference* targetInterfaceTypeM) {
  assert(false);
  exit(1);
}

void Linear::declareStaticSizedArray(
    StaticSizedArrayDefinitionT* ssaDefM) {

  auto hostName =
      globalState->metalCache->getName(
          globalState->metalCache->builtinPackageCoord, namePrefix + "_" + ssaDefM->name->name);
  auto hostKind = globalState->metalCache->getStaticSizedArray(hostName);
  addMappedKind(ssaDefM->kind, hostKind);
  globalState->regionIdByKind.emplace(hostKind, getRegionId());

  structs.declareStaticSizedArray(hostKind);
}

void Linear::defineStaticSizedArray(
    StaticSizedArrayDefinitionT* staticSizedArrayMT) {
  auto ssaDef = globalState->program->getStaticSizedArray(staticSizedArrayMT->kind);
  auto elementLT =
      translateType(
          linearizeReference(
              staticSizedArrayMT->elementType));
  auto hostKind = hostKindByValeKind.find(staticSizedArrayMT->kind)->second;
  auto hostSsaMT = dynamic_cast<StaticSizedArrayT*>(hostKind);
  assert(hostSsaMT);

  structs.defineStaticSizedArray(hostSsaMT, ssaDef->size, elementLT);
}

void Linear::declareStaticSizedArrayExtraFunctions(StaticSizedArrayDefinitionT* ssaDef) {
  declareConcreteSerializeFunction(ssaDef->kind);
}

void Linear::defineStaticSizedArrayExtraFunctions(StaticSizedArrayDefinitionT* ssaDef) {
  defineConcreteSerializeFunction(ssaDef->kind);
}

void Linear::declareRuntimeSizedArray(
    RuntimeSizedArrayDefinitionT* rsaDefM) {
  auto hostName =
      globalState->metalCache->getName(
          globalState->metalCache->builtinPackageCoord, namePrefix + "_" + rsaDefM->name->name);
  auto hostKind = globalState->metalCache->getRuntimeSizedArray(hostName);
  addMappedKind(rsaDefM->kind, hostKind);
  globalState->regionIdByKind.emplace(hostKind, getRegionId());

  structs.declareRuntimeSizedArray(hostKind);
}

void Linear::defineRuntimeSizedArray(
    RuntimeSizedArrayDefinitionT* runtimeSizedArrayMT) {
  auto elementLT =
      translateType(
          linearizeReference(
              runtimeSizedArrayMT->elementType));
  auto hostKind = hostKindByValeKind.find(runtimeSizedArrayMT->kind)->second;
  auto hostRsaMT = dynamic_cast<RuntimeSizedArrayT*>(hostKind);
  assert(hostRsaMT);
  structs.defineRuntimeSizedArray(hostRsaMT, elementLT);
}

void Linear::declareRuntimeSizedArrayExtraFunctions(RuntimeSizedArrayDefinitionT* rsaDefM) {
  declareConcreteSerializeFunction(rsaDefM->kind);
}

void Linear::defineRuntimeSizedArrayExtraFunctions(RuntimeSizedArrayDefinitionT* rsaDefM) {
  defineConcreteSerializeFunction(rsaDefM->kind);
}

void Linear::declareStruct(
    StructDefinition* structM) {

  auto hostName =
      globalState->metalCache->getName(
          globalState->metalCache->builtinPackageCoord, namePrefix + "_" + structM->name->name);
  auto hostKind = globalState->metalCache->getStructKind(hostName);
  addMappedKind(structM->kind, hostKind);
  globalState->regionIdByKind.emplace(hostKind, getRegionId());

  structs.declareStruct(hostKind);
}

void Linear::declareStructExtraFunctions(StructDefinition* structDefM) {
  declareConcreteSerializeFunction(structDefM->kind);
}

void Linear::defineStruct(
    StructDefinition* structM) {
  auto hostKind = hostKindByValeKind.find(structM->kind)->second;
  auto hostStructMT = dynamic_cast<StructKind*>(hostKind);
  assert(hostStructMT);

  std::vector<LLVMTypeRef> innerStructMemberTypesL;
  for (int i = 0; i < structM->members.size(); i++) {
    innerStructMemberTypesL.push_back(
        translateType(linearizeReference(structM->members[i]->type)));
  }
  structs.defineStruct(hostStructMT, innerStructMemberTypesL);
}

void Linear::defineStructExtraFunctions(StructDefinition* structDefM) {
  defineConcreteSerializeFunction(structDefM->kind);
}

void Linear::declareEdge(Edge* edge) {
  auto hostStructKind = linearizeStructKind(edge->structName);
  auto hostInterfaceKind = linearizeInterfaceKind(edge->interfaceName);

  structs.declareEdge(hostStructKind, hostInterfaceKind);

  auto interfaceMethod = getSerializeInterfaceMethod(edge->interfaceName);
  auto thunkPrototype = getSerializeThunkPrototype(edge->structName, edge->interfaceName);
  globalState->addEdgeExtraMethod(edge->interfaceName, edge->structName, interfaceMethod, thunkPrototype);
  auto nameL = globalState->serializeName->name + "__" + edge->interfaceName->fullName->name + "__" + edge->structName->fullName->name;
  declareExtraFunction(globalState, thunkPrototype, nameL);
}

void Linear::defineEdge(Edge* edge) {
//  auto interfaceM = globalState->program->getInterface(edge->interfaceName->fullName);

  auto interfaceFunctionsLT = globalState->getInterfaceFunctionPointerTypes(edge->interfaceName);
  auto edgeFunctionsL = globalState->getEdgeFunctions(edge);
  structs.defineEdge(edge, interfaceFunctionsLT, edgeFunctionsL);

  defineEdgeSerializeFunction(edge);
}

void Linear::defineEdgeSerializeFunction(Edge* edge) {
  auto boolMT = globalState->metalCache->boolRef;

  auto thunkPrototype = getSerializeThunkPrototype(edge->structName, edge->interfaceName);
  defineFunctionBodyV(
      globalState, thunkPrototype,
      [this, boolMT, thunkPrototype, edge](FunctionState *functionState, LLVMBuilderRef builder) {
        auto structPrototype = getSerializePrototype(edge->structName);

        auto valeObjectRefMT = structPrototype->params[2];

        auto regionInstanceRef =
            wrap(globalState->getRegion(regionRefMT), regionRefMT, LLVMGetParam(functionState->containingFuncL, 0));
        auto sourceRegionInstanceRef =
            wrap(globalState->getRegion(valeObjectRefMT), globalState->getRegion(valeObjectRefMT)->getRegionRefType(), LLVMGetParam(functionState->containingFuncL, 1));
        auto valeObjectRef =
            wrap(globalState->getRegion(valeObjectRefMT), valeObjectRefMT, LLVMGetParam(functionState->containingFuncL, 2));
        auto dryRunBoolRef =
            wrap(globalState->getRegion(boolMT), boolMT, LLVMGetParam(functionState->containingFuncL, 3));

        auto structRef =
            buildCallV(
                globalState, functionState, builder, structPrototype, {regionInstanceRef, sourceRegionInstanceRef, valeObjectRef, dryRunBoolRef});
        // Note that structRef might not actually be a pointer to a valid struct in memory,
        // it might instead be an integer offset, masquerading as a pointer. See PSBCBO for more.
        // The below code doesn't break because we don't need to dereference structs to upcast them.

        auto hostInterfaceKind = dynamic_cast<InterfaceKind *>(thunkPrototype->returnType->kind);
        assert(hostInterfaceKind);
        auto hostStructKind = dynamic_cast<StructKind *>(structPrototype->returnType->kind);
        assert(hostStructKind);

        auto interfaceRef =
            upcast(
                functionState, builder, structPrototype->returnType, hostStructKind,
                structRef, thunkPrototype->returnType, hostInterfaceKind);
        auto interfaceRefLE =
            checkValidReference(FL(), functionState, builder, true, thunkPrototype->returnType, interfaceRef);
        LLVMBuildRet(builder, interfaceRefLE);
      });
}

void Linear::declareInterface(InterfaceDefinition* interfaceM) {
  auto hostName =
      globalState->metalCache->getName(
          globalState->metalCache->builtinPackageCoord, namePrefix + "_" + interfaceM->name->name);
  auto hostKind = globalState->metalCache->getInterfaceKind(hostName);
  addMappedKind(interfaceM->kind, hostKind);
  globalState->regionIdByKind.emplace(hostKind, getRegionId());

  structs.declareInterface(hostKind);
}

void Linear::defineInterface(InterfaceDefinition* interfaceM) {
  auto hostKind = hostKindByValeKind.find(interfaceM->kind)->second;
  auto hostInterfaceMT = dynamic_cast<InterfaceKind*>(hostKind);
  assert(hostInterfaceMT);

  auto interfaceMethodTypesL = globalState->getInterfaceFunctionPointerTypes(interfaceM->kind);
  structs.defineInterface(hostInterfaceMT);
}

void Linear::declareInterfaceSerializeFunction(InterfaceKind* valeInterface) {
  auto interfaceMethod = getSerializeInterfaceMethod(valeInterface);
  globalState->addInterfaceExtraMethod(valeInterface, interfaceMethod);
}

void Linear::declareInterfaceExtraFunctions(InterfaceDefinition* interfaceDefM) {
  declareInterfaceSerializeFunction(interfaceDefM->kind);
}

void Linear::defineInterfaceExtraFunctions(InterfaceDefinition* interfaceDefM) {
}

Ref Linear::weakAlias(
    FunctionState* functionState, LLVMBuilderRef builder, Reference* sourceRefMT, Reference* targetRefMT, Ref sourceRef) {
  assert(false);
  exit(1);
}

void Linear::discardOwningRef(
    AreaAndFileAndLine from,
    FunctionState* functionState,
    BlockState* blockState,
    LLVMBuilderRef builder,
    Reference* sourceMT,
    Ref sourceRef) {
  assert(false);
}


void Linear::noteWeakableDestroyed(
    FunctionState* functionState,
    LLVMBuilderRef builder,
    Reference* refM,
    ControlBlockPtrLE controlBlockPtrLE) {
  // Do nothing
}

Ref Linear::loadMember(
    FunctionState* functionState,
    LLVMBuilderRef builder,
    Ref regionInstanceRef,
    Reference* structRefMT,
    Ref structRef,
    bool structKnownLive,
    int memberIndex,
    Reference* expectedMemberType,
    Reference* targetMemberType,
    const std::string& memberName) {
  auto structMT = dynamic_cast<StructKind*>(structRefMT->kind);
  assert(structMT);

  globalState->getRegion(structRefMT)
      ->checkValidReference(FL(), functionState, builder, true, structRefMT, structRef);
  auto structInnerLT = structs.getStructStruct(structMT);
  auto memberLE =
      loadMember(
          functionState,
          builder,
          regionInstanceRef,
          structRefMT,
          structInnerLT,
          structRef,
          memberIndex,
          expectedMemberType,
          targetMemberType,
          memberName);
  auto resultRef =
      upgradeLoadResultToRefWithTargetOwnership(
          functionState, builder, expectedMemberType, targetMemberType, memberLE);
  return resultRef;
}

void Linear::storeMember(
    FunctionState* functionState,
    LLVMBuilderRef builder,
    Ref regionInstanceRef,
    Reference* structRefMT,
    Ref structRef,
    bool structKnownLive,
    int memberIndex,
    const std::string& memberName,
    Reference* newMemberRefMT,
    Ref newMemberRef) {
  // storeMember is called when we want to overwrite something, but we don't allow that in linear.
  // To initialize something, use initializeMember instead.
  assert(false);
}


void Linear::initializeMember(
    FunctionState* functionState,
    LLVMBuilderRef builder,
    Ref regionInstanceRef,
    Reference* structRefMT,
    Ref structRef,
    bool structKnownLive,
    int memberIndex,
    const std::string& memberName,
    Reference* newMemberRefMT,
    Ref newMemberRef) {
  auto unadjustedInnerStructPtrLE =
      checkValidReference(FL(), functionState, builder, true, structRefMT, structRef);
  auto adjustedInnerStructPtrLE =
      translateBetweenBufferAddressAndPointer(
          functionState, builder, regionInstanceRef, structRefMT, unadjustedInnerStructPtrLE, true);

  auto memberLE =
      globalState->getRegion(newMemberRefMT)->checkValidReference(
          FL(), functionState, builder, false, newMemberRefMT, newMemberRef);

  auto innerStructMT = dynamic_cast<StructKind*>(structRefMT->kind);
  assert(innerStructMT);
  auto innerStructLT = structs.getStructStruct(innerStructMT);
  auto memberPtrLE =
      LLVMBuildStructGEP2(builder, innerStructLT, adjustedInnerStructPtrLE, memberIndex, memberName.c_str());
  LLVMBuildStore(builder, memberLE, memberPtrLE);
}


std::tuple<LLVMValueRef, LLVMValueRef> Linear::explodeInterfaceRef(
    FunctionState* functionState,
    LLVMBuilderRef builder,
    Reference* interfaceRefMT,
    Ref interfaceRef) {
  auto interfaceRefLE = checkValidReference(FL(), functionState, builder, false, interfaceRefMT, interfaceRef);
  auto substructPtrLE = LLVMBuildExtractValue(builder, interfaceRefLE, 0, "substructPtr");
  auto edgeNumLE = LLVMBuildExtractValue(builder, interfaceRefLE, 1, "edgeNum");
  return std::make_tuple(edgeNumLE, substructPtrLE);
}


void Linear::aliasWeakRef(
    AreaAndFileAndLine from,
    FunctionState* functionState,
    LLVMBuilderRef builder,
    Reference* weakRefMT,
    Ref weakRef) {
  assert(false);
}

void Linear::discardWeakRef(
    AreaAndFileAndLine from,
    FunctionState* functionState,
    LLVMBuilderRef builder,
    Reference* weakRefMT,
    Ref weakRef) {
  assert(false);
}

Ref Linear::getIsAliveFromWeakRef(
    FunctionState* functionState,
    LLVMBuilderRef builder,
    Reference* weakRefM,
    Ref weakRef,
    bool knownLive) {
  assert(false);
  exit(1);
}

LLVMValueRef Linear::getStringBytesPtr(
    FunctionState* functionState,
    LLVMBuilderRef builder,
    Ref regionInstanceRef,
    Ref ref) {
  auto unadjustedStrWrapperPtrLE =
      checkValidReference(FL(), functionState, builder, true, linearStrRefMT, ref);
  auto adjustedStrWrapperPtrLE =
      translateBetweenBufferAddressAndPointer(
          functionState, builder, regionInstanceRef, linearStrRefMT, unadjustedStrWrapperPtrLE, false);
  return structs.getStringBytesPtr(functionState, builder, adjustedStrWrapperPtrLE);
}

Ref Linear::allocate(
    Ref regionInstanceRef,
    AreaAndFileAndLine from,
    FunctionState* functionState,
    LLVMBuilderRef builder,
    Reference* structRefMT,
    const std::vector<Ref>& memberRefs) {
  // Nobody from the outside should call this.
  // Its API isn't good for us, it assumes we already have references to the members...
  // but Linear needs to allocate memory for the struct before it can serialize its members,
  // see MAPOWM.
  assert(false);
  exit(1);
}

Ref Linear::innerAllocate(
    Ref regionInstanceRef,
    AreaAndFileAndLine from,
    FunctionState* functionState,
    LLVMBuilderRef builder,
    Reference* hostStructRefMT) {

  // We reserve some space for it before we serialize its members
  LLVMValueRef substructSizeIntLE =
      predictShallowSize(functionState, builder, true, hostStructRefMT->kind, constI64LE(globalState, 0));
  auto destinationStructRef = getDestinationRef(functionState, builder, regionInstanceRef, hostStructRefMT);

  auto i32MT = globalState->metalCache->i32Ref;
  auto boolMT = globalState->metalCache->boolRef;

  auto valeStructRefMT = unlinearizeReference(hostStructRefMT);
  auto desiredValeStructMT = dynamic_cast<StructKind*>(valeStructRefMT->kind);
  assert(desiredValeStructMT);
  auto valeStructDefM = globalState->program->getStruct(desiredValeStructMT);

  auto destinationPtrLE =
      checkValidReference(FL(), functionState, builder, true, hostStructRefMT, destinationStructRef);

//  reserveRootMetadataBytesIfNeeded(functionState, builder, regionInstanceRef);

  bumpDestinationOffset(functionState, builder, regionInstanceRef, substructSizeIntLE); // moved

  return destinationStructRef;
}

Ref Linear::upcast(
    FunctionState* functionState,
    LLVMBuilderRef builder,

    Reference* sourceStructMT,
    StructKind* sourceStructKindM,
    Ref sourceRef,

    Reference* targetInterfaceTypeM,
    InterfaceKind* targetInterfaceKindM) {
  assert(valeKindByHostKind.find(sourceStructMT->kind) != valeKindByHostKind.end());
  assert(valeKindByHostKind.find(sourceStructKindM) != valeKindByHostKind.end());
  assert(valeKindByHostKind.find(targetInterfaceTypeM->kind) != valeKindByHostKind.end());
  assert(valeKindByHostKind.find(targetInterfaceKindM) != valeKindByHostKind.end());

  auto structRefLE = checkValidReference(FL(), functionState, builder, false, sourceStructMT, sourceRef);

  auto edgeNumber = structs.getEdgeNumber(targetInterfaceKindM, sourceStructKindM);
  LLVMValueRef edgeNumberLE = constI64LE(globalState, edgeNumber);

  return assembleInterfaceRef(builder, targetInterfaceTypeM, structRefLE, edgeNumberLE);
}

Ref Linear::assembleInterfaceRef(
    LLVMBuilderRef builder,
    Reference* targetInterfaceTypeM,
    LLVMValueRef structRefLE,
    LLVMValueRef edgeNumberLE) {
  auto i8PtrLT = LLVMPointerType(LLVMInt8TypeInContext(globalState->context), 0);

  auto interfaceKindM = dynamic_cast<InterfaceKind*>(targetInterfaceTypeM->kind);
  assert(interfaceKindM);
  auto interfaceRefLT = structs.getInterfaceRefStruct(interfaceKindM);

  auto structI8PtrLE = LLVMBuildPointerCast(builder, structRefLE, i8PtrLT, "objAsVoidPtr");

  auto interfaceRefLE = LLVMGetUndef(interfaceRefLT);
  interfaceRefLE = LLVMBuildInsertValue(builder, interfaceRefLE, structI8PtrLE, 0, "interfaceRefWithOnlyObj");
  interfaceRefLE = LLVMBuildInsertValue(builder, interfaceRefLE, edgeNumberLE, 1, "interfaceRef");

  return wrap(globalState->getRegion(targetInterfaceTypeM), targetInterfaceTypeM, interfaceRefLE);
}

WrapperPtrLE Linear::lockWeakRef(
    AreaAndFileAndLine from,
    FunctionState* functionState,
    LLVMBuilderRef builder,
    Reference* refM,
    Ref weakRefLE,
    bool weakRefKnownLive) {
  assert(false);
  exit(1);
}

Ref Linear::getRuntimeSizedArrayLength(
    FunctionState* functionState,
    LLVMBuilderRef builder,
    Ref regionInstanceRef,
    Reference* rsaRefMT,
    Ref arrayRef,
    bool arrayKnownLive) {
  auto int32LT = LLVMInt32TypeInContext(globalState->context);
  auto rsaHostMT = dynamic_cast<RuntimeSizedArrayT*>(rsaRefMT->kind);
  assert(rsaHostMT);
  auto unadjustedArrayRefLE = checkValidReference(FL(), functionState, builder, true, rsaRefMT, arrayRef);
  auto adjustedArrayRefLE =
      translateBetweenBufferAddressAndPointer(
          functionState, builder, regionInstanceRef, rsaRefMT, unadjustedArrayRefLE, false);
  auto resultLE =
      LLVMBuildStructGEP2(
          builder, structs.getRuntimeSizedArrayStruct(rsaHostMT), adjustedArrayRefLE, 0, "rsaLenPtr");
  auto intLE = LLVMBuildLoad2(builder, int32LT, resultLE, "rsaLen");
  return wrap(globalState->getRegion(globalState->metalCache->i32Ref), globalState->metalCache->i32Ref, intLE);
}

Ref Linear::getRuntimeSizedArrayCapacity(
    FunctionState* functionState,
    LLVMBuilderRef builder,
    Ref regionInstanceRef,
    Reference* rsaRefMT,
    Ref arrayRef,
    bool arrayKnownLive) {
  return getRuntimeSizedArrayLength(functionState, builder, regionInstanceRef, rsaRefMT, arrayRef, arrayKnownLive);
}

LLVMValueRef Linear::checkValidReference(
    AreaAndFileAndLine checkerAFL,
    FunctionState* functionState,
    LLVMBuilderRef builder,
    bool expectLive,
    Reference* refM,
    Ref ref) {
  Reference *actualRefM = nullptr;
  LLVMValueRef refLE = nullptr;
  std::tie(actualRefM, refLE) = megaGetRefInnardsForChecking(ref);
  assert(actualRefM == refM);
  assert(refLE != nullptr);
  assert(LLVMTypeOf(refLE) == this->translateType(refM));
  return refLE;
}

Ref Linear::upgradeLoadResultToRefWithTargetOwnership(
    FunctionState* functionState,
    LLVMBuilderRef builder,
    Reference* sourceType,
    Reference* targetType,
    LoadResult sourceLoad) {
  auto sourceRef = sourceLoad.extractForAliasingInternals();
  auto sourceOwnership = sourceType->ownership;
  auto sourceLocation = sourceType->location;
  auto targetOwnership = targetType->ownership;
  auto targetLocation = targetType->location;
//  assert(sourceLocation == targetLocation); // unimplemented

  if (sourceLocation == Location::INLINE) {
    return sourceRef;
  } else {
    return sourceRef;
  }
}

void Linear::checkInlineStructType(
    FunctionState* functionState,
    LLVMBuilderRef builder,
    Reference* refMT,
    Ref ref) {
  auto argLE = checkValidReference(FL(), functionState, builder, false, refMT, ref);
  auto structKind = dynamic_cast<StructKind*>(refMT->kind);
  assert(structKind);
  assert(LLVMTypeOf(argLE) == structs.getStructStruct(structKind));
}

LoadResult Linear::loadElementFromSSA(
    FunctionState* functionState,
    LLVMBuilderRef builder,
    Ref regionInstanceRef,
    Reference* hostSsaRefMT,
    StaticSizedArrayT* hostSsaMT,
    Ref arrayRef,
    bool arrayKnownLive,
    Ref indexRef) {
  auto unadjustedArrayRefLE = checkValidReference(FL(), functionState, builder, true, hostSsaRefMT, arrayRef);
  auto adjustedArrayRefLE =
      translateBetweenBufferAddressAndPointer(
          functionState, builder, regionInstanceRef, hostSsaRefMT, unadjustedArrayRefLE, false);
  // Array is the only member in the SSA struct.
  auto ssaStructLT = structs.getStaticSizedArrayStruct(hostSsaMT);
  auto elementsPtrLE = LLVMBuildStructGEP2(builder, ssaStructLT, adjustedArrayRefLE, 0, "ssaElemsPtr");

  auto valeSsaMT = unlinearizeSSA(hostSsaMT);
  auto valeSsaMD = globalState->program->getStaticSizedArray(valeSsaMT);
  auto valeMemberRefMT = valeSsaMD->elementType;
  auto hostMemberRefMT = linearizeReference(valeMemberRefMT);

  return loadElementFromSSAInner(
      globalState, functionState, builder, hostSsaRefMT, hostSsaMT, valeSsaMD->size, hostMemberRefMT, indexRef, elementsPtrLE);
}

LoadResult Linear::loadElementFromRSA(
    FunctionState* functionState,
    LLVMBuilderRef builder,
    Ref regionInstanceRef,
    Reference* hostRsaRefMT,
    RuntimeSizedArrayT* hostRsaMT,
    Ref arrayRef,
    bool arrayKnownLive,
    Ref indexRef) {
  auto int32LT = LLVMInt32TypeInContext(globalState->context);
  auto unadjustedArrayRefLE = checkValidReference(FL(), functionState, builder, true, hostRsaRefMT, arrayRef);
  auto adjustedArrayRefLE =
      translateBetweenBufferAddressAndPointer(
          functionState, builder, regionInstanceRef, hostRsaRefMT, unadjustedArrayRefLE, false);
  // Size is the first member in the RSA struct.
  auto rsaStructLT = structs.getRuntimeSizedArrayStruct(hostRsaMT);
  auto sizeLE = LLVMBuildLoad2(builder, int32LT, LLVMBuildStructGEP2(builder, rsaStructLT, adjustedArrayRefLE, 0, "rsaSizePtr"), "rsaSize");
  auto sizeRef = wrap(this, globalState->metalCache->i32Ref, sizeLE);
  // Elements is the 1th member in the RSA struct, after size.
  auto elementsPtrLE = LLVMBuildStructGEP2(builder, rsaStructLT, adjustedArrayRefLE, 1, "rsaElemsPtr");

  auto valeRsaRefMT = unlinearizeReference(hostRsaRefMT);
  auto valeRsaMT = dynamic_cast<RuntimeSizedArrayT*>(valeRsaRefMT->kind);
  assert(valeRsaMT);

  auto rsaDef = globalState->program->getRuntimeSizedArray(valeRsaMT);
  auto hostElementType = linearizeReference(rsaDef->elementType);

  buildFlare(FL(), globalState, functionState, builder);

  return loadElement(
      globalState, functionState, builder, elementsPtrLE, hostElementType, sizeRef, indexRef);
}


Ref Linear::storeElementInRSA(
    FunctionState* functionState,
    LLVMBuilderRef builder,
    Reference* rsaRefMT,
    RuntimeSizedArrayT* rsaMT,
    Ref arrayRef,
    bool arrayKnownLive,
    Ref indexRef,
    Ref elementRef) {
  assert(false);
  exit(1);
}

void Linear::deallocate(
    AreaAndFileAndLine from,
    FunctionState* functionState,
    LLVMBuilderRef builder,
    Reference* refMT,
    Ref ref) {
  auto refLE = checkValidReference(FL(), functionState, builder, true, refMT, ref);
  auto concreteAsCharPtrLE =
      LLVMBuildBitCast(
          builder,
          refLE,
          LLVMPointerType(LLVMInt8TypeInContext(globalState->context), 0),
          "concreteCharPtrForFree");
  buildFlare(FL(), globalState, functionState, builder, "Freeing ", ptrToIntLE(globalState, builder, concreteAsCharPtrLE));
  globalState->externs->free.call(builder, {concreteAsCharPtrLE}, "");
}

Ref Linear::constructRuntimeSizedArray(
    Ref regionInstanceRef,
    FunctionState* functionState,
    LLVMBuilderRef builder,
    Reference* rsaMT,
    RuntimeSizedArrayT* runtimeSizedArrayT,
    Ref capacityRef,
    const std::string& typeName) {
  return innerConstructRuntimeSizedArray(
      regionInstanceRef, functionState, builder, rsaMT, runtimeSizedArrayT, capacityRef, globalState->constI1(false));
}

Ref Linear::constructStaticSizedArray(
    Ref regionInstanceRef,
    FunctionState* functionState,
    LLVMBuilderRef builder,
    Reference* ssaRefMT,
    StaticSizedArrayT* ssaMT) {
  return innerConstructStaticSizedArray(
      regionInstanceRef, functionState, builder, ssaRefMT, ssaMT, globalState->constI1(false));
}

Ref Linear::innerConstructStaticSizedArray(
    Ref regionInstanceRef,
    FunctionState* functionState,
    LLVMBuilderRef builder,
    Reference* ssaRefMT,
    StaticSizedArrayT* hostSsaMT,
    Ref dryRunBoolRef) {
  buildFlare(FL(), globalState, functionState, builder);

  auto boolMT = globalState->metalCache->boolRef;
  auto i32RefMT = globalState->metalCache->i32Ref;

  assert(ssaRefMT->kind == hostSsaMT);
  assert(globalState->getRegion(hostSsaMT) == this);

  auto valeKindMT = valeKindByHostKind.find(hostSsaMT)->second;
  auto valeSsaMT = dynamic_cast<StaticSizedArrayT*>(valeKindMT);
  assert(valeSsaMT);

  auto valeSsaDefM = globalState->program->getStaticSizedArray(valeSsaMT);
  auto sizeLE = predictShallowSize(functionState, builder, true, hostSsaMT, constI64LE(globalState, valeSsaDefM->size));
  buildFlare(FL(), globalState, functionState, builder);

  auto ssaRef = getDestinationRef(functionState, builder, regionInstanceRef, ssaRefMT);
  auto ssaPtrLE = checkValidReference(FL(), functionState, builder, true, ssaRefMT, ssaRef);

  auto dryRunBoolLE =
      globalState->getRegion(boolMT)
          ->checkValidReference(FL(), functionState, builder, true, boolMT, dryRunBoolRef);
  buildIfV(
      globalState, functionState, builder, LLVMBuildNot(builder, dryRunBoolLE, "notDryRun"),
      [this, functionState, ssaPtrLE, hostSsaMT, ssaRefMT, regionInstanceRef](LLVMBuilderRef thenBuilder) mutable {
        buildFlare(FL(), globalState, functionState, thenBuilder);

        auto ssaLT = structs.getStaticSizedArrayStruct(hostSsaMT);
        auto ssaValLE = LLVMGetUndef(ssaLT); // There are no fields

        // When we write a pointer, we need to subtract the Serialized Address Adjuster, see PSBCBO.
        auto adjustedSsaPtrLE =
            translateBetweenBufferAddressAndPointer(
                functionState, thenBuilder, regionInstanceRef, ssaRefMT, ssaPtrLE, true);
        LLVMBuildStore(thenBuilder, ssaValLE, ssaPtrLE);

        buildFlare(FL(), globalState, functionState, thenBuilder);

        // Caller still needs to initialize the elements!
      });

//  reserveRootMetadataBytesIfNeeded(functionState, builder, regionInstanceRef);
  bumpDestinationOffset(functionState, builder, regionInstanceRef, sizeLE); // moved

  buildFlare(FL(), globalState, functionState, builder);

  return ssaRef;
}

Ref Linear::innerConstructRuntimeSizedArray(
    Ref regionInstanceRef,
    FunctionState* functionState,
    LLVMBuilderRef builder,
    Reference* rsaRefMT,
    RuntimeSizedArrayT* rsaMT,
    Ref sizeRef,
    Ref dryRunBoolRef) {
  buildFlare(FL(), globalState, functionState, builder);

  auto boolMT = globalState->metalCache->boolRef;
  auto i32RefMT = globalState->metalCache->i32Ref;

  assert(rsaRefMT->kind == rsaMT);
  assert(globalState->getRegion(rsaMT) == this);

  auto lenI32LE = globalState->getRegion(i32RefMT)->checkValidReference(FL(), functionState, builder, true, i32RefMT, sizeRef);
  auto lenI64LE = LLVMBuildZExt(builder, lenI32LE, LLVMInt64TypeInContext(globalState->context), "");

  auto sizeLE = predictShallowSize(functionState, builder, true, rsaMT, lenI64LE);

  auto rsaRef = getDestinationRef(functionState, builder, regionInstanceRef, rsaRefMT);
  auto rsaPtrLE = checkValidReference(FL(), functionState, builder, true, rsaRefMT, rsaRef);

  auto dryRunBoolLE =
      globalState->getRegion(boolMT)
          ->checkValidReference(FL(), functionState, builder, true, boolMT, dryRunBoolRef);
  buildIfV(
      globalState, functionState, builder, LLVMBuildNot(builder, dryRunBoolLE, "notDryRun"),
      [this, functionState, rsaPtrLE, lenI32LE, rsaMT, rsaRefMT, regionInstanceRef](LLVMBuilderRef thenBuilder) mutable {
        buildFlare(FL(), globalState, functionState, thenBuilder);

        auto rsaLT = structs.getRuntimeSizedArrayStruct(rsaMT);
        auto rsaWithLenVal = LLVMBuildInsertValue(thenBuilder, LLVMGetUndef(rsaLT), lenI32LE, 0, "rsaWithLen");

        // When we write a pointer, we need to subtract the Serialized Address Adjuster, see PSBCBO.
        auto adjustedRsaPtrLE =
            translateBetweenBufferAddressAndPointer(
                functionState, thenBuilder, regionInstanceRef, rsaRefMT, rsaPtrLE, true);
        assert(LLVMPointerType(LLVMTypeOf(rsaWithLenVal), 0) == LLVMTypeOf(adjustedRsaPtrLE));

        buildFlare(FL(), globalState, functionState, thenBuilder, "Writing RSA len: ", lenI32LE);
        LLVMBuildStore(thenBuilder, rsaWithLenVal, adjustedRsaPtrLE);

        buildFlare(FL(), globalState, functionState, thenBuilder);

        // Caller still needs to initialize the elements!
      });

//  reserveRootMetadataBytesIfNeeded(functionState, builder, regionInstanceRef);
  bumpDestinationOffset(functionState, builder, regionInstanceRef, sizeLE); // moved

  buildFlare(FL(), globalState, functionState, builder);

  return rsaRef;
}

Ref Linear::mallocStr(
    Ref regionInstanceRef,
    FunctionState* functionState,
    LLVMBuilderRef builder,
    LLVMValueRef lengthLE,
    LLVMValueRef sourceCharsPtrLE) {
  return innerMallocStr(regionInstanceRef, functionState, builder, lengthLE, sourceCharsPtrLE, globalState->constI1(false));
}

Ref Linear::innerMallocStr(
    Ref regionInstanceRef,
    FunctionState* functionState,
    LLVMBuilderRef builder,
    LLVMValueRef lenI32LE,
    LLVMValueRef sourceCharsPtrLE,
    Ref dryRunBoolRef) {
  auto boolMT = globalState->metalCache->boolRef;
  auto int8LT = LLVMInt8TypeInContext(globalState->context);
  auto int8PtrLT = LLVMPointerType(int8LT, 0);

  auto lenI64LE = LLVMBuildZExt(builder, lenI32LE, LLVMInt64TypeInContext(globalState->context), "");

  auto sizeLE = predictShallowSize(functionState, builder, true, linearStr, lenI32LE);

  auto strRef = getDestinationRef(functionState, builder, regionInstanceRef, linearStrRefMT);
  auto strPtrLE = checkValidReference(FL(), functionState, builder, true, linearStrRefMT, strRef);

  auto dryRunBoolLE =
      globalState->getRegion(boolMT)
          ->checkValidReference(FL(), functionState, builder, true, boolMT, dryRunBoolRef);

  buildFlare(FL(), globalState, functionState, builder);

  buildIfV(
      globalState, functionState, builder, LLVMBuildNot(builder, dryRunBoolLE, "notDryRun"),
      [this, functionState, regionInstanceRef, strPtrLE, int8LT, lenI32LE, lenI64LE, strRef, sourceCharsPtrLE](
          LLVMBuilderRef thenBuilder) mutable {
        auto strLT = structs.getStringStruct();
        auto strWithLenValLE =
            LLVMBuildInsertValue(thenBuilder, LLVMGetUndef(strLT), lenI32LE, 0, "strWithLen");
        assert(LLVMPointerType(LLVMTypeOf(strWithLenValLE), 0) == LLVMTypeOf(strPtrLE));

        // When we write a pointer, we need to subtract the Serialized Address Adjuster, see PSBCBO.
        auto adjustedStrPtrLE =
            translateBetweenBufferAddressAndPointer(
                functionState, thenBuilder, regionInstanceRef, linearStrRefMT, strPtrLE, true);
        assert(LLVMPointerType(LLVMTypeOf(strWithLenValLE), 0) == LLVMTypeOf(adjustedStrPtrLE));

        LLVMBuildStore(thenBuilder, strWithLenValLE, adjustedStrPtrLE);


        auto charsBeginPtr =
            structs.getStringBytesPtr(functionState, thenBuilder, adjustedStrPtrLE);

        std::vector<LLVMValueRef> argsLE = {charsBeginPtr, sourceCharsPtrLE, lenI64LE};
        globalState->externs->strncpy.call(thenBuilder, argsLE, "");

        auto charsEndPtr = LLVMBuildGEP2(thenBuilder, int8LT, charsBeginPtr, &lenI64LE, 1, "charsEndPtr_");

        // When we write a pointer, we need to subtract the Serialized Address Adjuster, see PSBCBO,
        // but here we're not writing any pointers (just chars) so no need to subtract anything here.
        LLVMBuildStore(thenBuilder, constI8LE(globalState, 0), charsEndPtr);

        return strRef;
      });

  buildFlare(FL(), globalState, functionState, builder, "innerMallocStr, from ", getRegionInstanceDestinationOffset(functionState, builder, regionInstanceRef), " adding ", sizeLE);
  bumpDestinationOffset(functionState, builder, regionInstanceRef, sizeLE); // moved
  buildFlare(FL(), globalState, functionState, builder, "...to ", getRegionInstanceDestinationOffset(functionState, builder, regionInstanceRef));

  buildFlare(FL(), globalState, functionState, builder);

  return strRef;
}

LLVMValueRef Linear::getStringLen(
    FunctionState* functionState,
    LLVMBuilderRef builder,
    Ref regionInstanceRef,
    Ref ref) {
  auto int32LT = LLVMInt32TypeInContext(globalState->context);
  auto unadjustedStrPtrLE = checkValidReference(FL(), functionState, builder, true, linearStrRefMT, ref);
  auto adjustedStrPtrLE =
      translateBetweenBufferAddressAndPointer(
          functionState, builder, regionInstanceRef, linearStrRefMT, unadjustedStrPtrLE, false);
  auto lenPtrLE =
      LLVMBuildStructGEP2(builder, structs.getStringStruct(), adjustedStrPtrLE, 0, "lenPtrC");
  return LLVMBuildLoad2(builder, int32LT, lenPtrLE, "lenZ");
}

LoadResult Linear::loadMember(
    FunctionState* functionState,
    LLVMBuilderRef builder,
    Ref regionInstanceRef,
    Reference* structRefMT,
    LLVMTypeRef structInnerLT,
    Ref structRef,
    int memberIndex,
    Reference* expectedMemberType,
    Reference* targetType,
    const std::string& memberName) {
  auto structRefLE = checkValidReference(FL(), functionState, builder, true, structRefMT, structRef);
  if (structRefMT->location == Location::INLINE) {
    auto memberLE = LLVMBuildExtractValue(builder, structRefLE, memberIndex, memberName.c_str());
    assert(false); // impl. should inline structs have their pointers adjusted already?
    return LoadResult{wrap(globalState->getRegion(expectedMemberType), expectedMemberType, memberLE)};
  } else {
    auto unadjustedStructPtrLE = structRefLE;
    // When we read a pointer, we need to add the Serialized Address Adjuster, see PSBCBO.
    auto adjustedStructPtrLE =
        translateBetweenBufferAddressAndPointer(
            functionState, builder, regionInstanceRef, structRefMT, unadjustedStructPtrLE, false);
    return loadInnerInnerStructMember(
          globalState,
          functionState,
          builder,
          structInnerLT,
          adjustedStructPtrLE,
          memberIndex,
          expectedMemberType,
          memberName);
  }
}

void Linear::checkValidReference(
    AreaAndFileAndLine checkerAFL,
    FunctionState* functionState,
    LLVMBuilderRef builder,
    bool expectLive,
    KindStructs* kindStructs,
    Reference* refM,
    LLVMValueRef refLE) {
  regularCheckValidReference(checkerAFL, globalState, functionState, builder, kindStructs, refM, refLE);
}

//std::string Linear::getExportName(
//    Package* package,
//    Reference* reference) {
//  return package->getKindExportName(reference->kind) + (reference->location == Location::YONDER ? "Ref" : "");
//}

std::string Linear::getExportName(Package* currentPackage, Reference* hostRefMT, bool includeProjectName) {
  assert(valeKindByHostKind.find(hostRefMT->kind) != valeKindByHostKind.end());

  auto hostMT = hostRefMT->kind;
  if (auto innt = dynamic_cast<Int *>(hostMT)) {
    return std::string() + "int" + std::to_string(innt->bits) + "_t";
  } else if (dynamic_cast<Never *>(hostMT)) {
    return "void";
  } else if (dynamic_cast<Void *>(hostMT)) {
    return "void";
  } else if (dynamic_cast<Bool *>(hostMT)) {
    return "int8_t";
  } else if (dynamic_cast<Float *>(hostMT)) {
    return "double";
  } else if (dynamic_cast<Str *>(hostMT)) {
    return "ValeStr*";
  } else if (auto hostInterfaceMT = dynamic_cast<InterfaceKind *>(hostMT)) {
    auto valeMT = valeKindByHostKind.find(hostMT)->second;
    auto valeInterfaceMT = dynamic_cast<InterfaceKind*>(valeMT);
    assert(valeInterfaceMT);
    auto baseName = currentPackage->getKindExportName(valeInterfaceMT, includeProjectName);
    assert(hostRefMT->ownership == Ownership::SHARE);
//    if (hostRefMT->location == Location::INLINE) {
      return baseName;
//    } else {
//      return baseName + "*";
//    };
  } else if (auto hostStructMT = dynamic_cast<StructKind *>(hostMT)) {
    auto valeMT = valeKindByHostKind.find(hostMT)->second;
    auto valeStructMT = dynamic_cast<StructKind*>(valeMT);
    assert(valeStructMT);
    auto baseName = currentPackage->getKindExportName(valeStructMT, includeProjectName);
    assert(hostRefMT->ownership == Ownership::SHARE);
    if (hostRefMT->location == Location::INLINE) {
      return baseName;
    } else {
      return baseName + "*";
    }
  } else if (dynamic_cast<StaticSizedArrayT *>(hostMT)) {
    auto valeMT = valeKindByHostKind.find(hostMT)->second;
    auto valeSsaMT = dynamic_cast<StaticSizedArrayT*>(valeMT);
    assert(valeSsaMT);
    auto baseName = currentPackage->getKindExportName(valeSsaMT, includeProjectName);
    assert(hostRefMT->ownership == Ownership::SHARE);
    if (hostRefMT->location == Location::INLINE) {
      return baseName;
    } else {
      return baseName + "*";
    }
  } else if (auto hostRsaMT = dynamic_cast<RuntimeSizedArrayT *>(hostMT)) {
    auto valeMT = valeKindByHostKind.find(hostMT)->second;
    auto valeRsaMT = dynamic_cast<RuntimeSizedArrayT*>(valeMT);
    assert(valeRsaMT);
    auto baseName = currentPackage->getKindExportName(valeRsaMT, includeProjectName);
    assert(hostRefMT->ownership == Ownership::SHARE);
    if (hostRefMT->location == Location::INLINE) {
      return baseName;
    } else {
      return baseName + "*";
    }
  } else {
    std::cerr << "Unimplemented type in immutables' getExportName: "
              << typeid(*hostRefMT->kind).name() << std::endl;
    assert(false);
  }
}

std::string Linear::generateStructDefsC(
    Package* currentPackage,
    StructDefinition* structDefM) {
  auto name = currentPackage->getKindExportName(structDefM->kind, true);
  std::stringstream s;
  s << "typedef struct " << name << " { " << std::endl;
  for (int i = 0; i < structDefM->members.size(); i++) {
    auto member = structDefM->members[i];
    auto hostMT = hostKindByValeKind.find(member->type->kind)->second;
    auto hostRefMT = globalState->metalCache->getReference(member->type->ownership, member->type->location, hostMT);
    s << "  " << getExportName(currentPackage, hostRefMT, true) << " " << member->name << ";" << std::endl;
  }
  s << "} " << name << ";" << std::endl;
  return s.str();
}

std::string Linear::generateInterfaceDefsC(
    Package* currentPackage,
    InterfaceDefinition* interfaceDefM) {
    std::stringstream s;

  auto interfaceName = currentPackage->getKindExportName(interfaceDefM->kind, true);

  auto hostKind = hostKindByValeKind.find(interfaceDefM->kind)->second;
  auto hostInterfaceKind = dynamic_cast<InterfaceKind*>(hostKind);
  assert(hostInterfaceKind);

  // Cant use an enum for this because platforms CANT AGREE ON A SIZE FOR
  // AN ENUM, I HATE C
  // Even adding a 0x7FFFFFFFFFFFFFFF entry didn't work for windows!

  int i = 0;
  for (auto hostStructKind : structs.getOrderedSubstructs(hostInterfaceKind)) {
    auto valeKind = valeKindByHostKind.find(hostStructKind)->second;
    auto valeStructKind = dynamic_cast<StructKind*>(valeKind);
    assert(valeStructKind);
    s << "#define " << interfaceName << "_Type_" << currentPackage->getKindExportName(valeStructKind, false) << " " << i << std::endl;
    i++;
  }

  s << "typedef struct " << interfaceName << " {" << std::endl;
  s << "void* obj; uint64_t type;" << std::endl;
  s << "} " << interfaceName << ";" << std::endl;

  return s.str();
}

std::string Linear::generateRuntimeSizedArrayDefsC(
    Package* currentPackage,
    RuntimeSizedArrayDefinitionT* rsaDefM) {
  auto rsaName = currentPackage->getKindExportName(rsaDefM->kind, true);

  auto valeMemberRefMT = rsaDefM->elementType;
  auto hostMemberRefMT = linearizeReference(valeMemberRefMT);

  std::stringstream s;
  s << "typedef struct " << rsaName << " {" << std::endl;
  s << "  uint32_t length;" << std::endl;
  s << "  " << getExportName(currentPackage, hostMemberRefMT, true) << " elements[0];" << std::endl;
  s << "} " << rsaName << ";" << std::endl;
  return s.str();
}

std::string Linear::generateStaticSizedArrayDefsC(
    Package* currentPackage,
    StaticSizedArrayDefinitionT* ssaDefM) {

  auto rsaName = currentPackage->getKindExportName(ssaDefM->kind, true);

  auto valeMemberRefMT = ssaDefM->elementType;
  auto hostMemberRefMT = linearizeReference(valeMemberRefMT);

  std::stringstream s;
  s << "#define " << rsaName << "_SIZE " << ssaDefM->size << std::endl;
  s << "typedef struct " << rsaName << " {" << std::endl;
  s << "  " << getExportName(currentPackage, hostMemberRefMT, true) << " elements[" << ssaDefM->size << "];" << std::endl;
  s << "} " << rsaName << ";" << std::endl;
  return s.str();
}

LLVMTypeRef Linear::getExternalType(Reference* refMT) {
  assert(false);
  exit(1);
//  return refMT;
}

std::pair<Ref, Ref> Linear::topLevelSerialize(
    FunctionState* functionState,
    LLVMBuilderRef builder,
    Ref regionInstanceRef,
    Ref sourceRegionInstanceRef,
    Kind* valeKind,
    Ref ref) {
  auto int64LT = LLVMInt64TypeInContext(globalState->context);

  auto valeRefMT =
      globalState->metalCache->getReference(
          Ownership::SHARE, Location::YONDER, valeKind);
  auto hostRefMT = linearizeReference(valeRefMT);

  auto regionLT = structs.getStructStruct(regionKind);

  auto useOffsetsLE =
      getRegionInstanceUseOffsets(functionState, builder, regionInstanceRef);
  auto bufferBeginOffsetLE =
      getRegionInstanceBufferBeginOffset(functionState, builder, regionInstanceRef);

  // Populate the region instance to zero. Should be unnecessary since we initialized it to
  // zero, but do it anyway.
  setRegionInstanceDestinationBufferStartPtr(
      functionState, builder, regionInstanceRef,
      LLVMConstNull(LLVMPointerType(LLVMInt8TypeInContext(globalState->context), 0)));
  setRegionInstanceDestinationOffset(
      functionState, builder, regionInstanceRef, constI64LE(globalState, 0));
  setRegionInstanceSerializedAddressAdjuster(
      functionState, builder, regionInstanceRef, constI64LE(globalState, 0));
  auto regionInstancePtrLE =
      checkValidReference(FL(), functionState, builder, true, regionRefMT, regionInstanceRef);

  callSerialize(functionState, builder, valeKind, regionInstanceRef, sourceRegionInstanceRef, ref, globalState->constI1(true));

//  // Reserve some space for the beginning metadata block
//  bumpDestinationOffset(functionState, builder, regionInstanceRef, constI64LE(globalState, startMetadataSize));
//  buildFlare(FL(), globalState, functionState, builder);

  auto dryRunFinalOffsetLE = getRegionInstanceDestinationOffset(functionState, builder, regionInstanceRef);
  auto sizeIntLE = dryRunFinalOffsetLE;//LLVMBuildSub(builder, dryRunCounterBeginLE, dryRunFinalOffsetLE, "size");

  LLVMValueRef bufferBeginPtrLE = callMalloc(globalState, builder, sizeIntLE);
//  buildFlare(FL(), globalState, functionState, builder, "malloced ", sizeIntLE, " got ptr ", ptrToIntLE(globalState, builder, bufferBeginPtrLE));

  auto serializedAddressAdjusterLE =
      buildIfElse(
          globalState, functionState, builder, int64LT, useOffsetsLE,
          [this, bufferBeginPtrLE, bufferBeginOffsetLE, int64LT](LLVMBuilderRef builder){
            auto bufferBeginPtrAsI64LE =
                LLVMBuildPtrToInt(builder, bufferBeginPtrLE, int64LT, "bufferBeginPtrAsI64");
            return LLVMBuildSub(
                builder, bufferBeginPtrAsI64LE, bufferBeginOffsetLE, "serializedAddressAdjuster");
          },
          [this](LLVMBuilderRef builder){
            return constI64LE(globalState, 0);
          });
  // Reset the region instance, now that we're doing the real thing.
  setRegionInstanceDestinationBufferStartPtr(
      functionState, builder, regionInstanceRef, bufferBeginPtrLE);
  setRegionInstanceDestinationOffset(
      functionState, builder, regionInstanceRef, constI64LE(globalState, 0));
  setRegionInstanceSerializedAddressAdjuster(
      functionState, builder, regionInstanceRef, serializedAddressAdjusterLE);

  auto resultRef =
      callSerialize(
          functionState, builder, valeKind, regionInstanceRef, sourceRegionInstanceRef, ref, globalState->constI1(false));

  auto rootObjectPtrLE =
      (dynamic_cast<InterfaceKind*>(valeKind) != nullptr ?
          std::get<1>(explodeInterfaceRef(functionState, builder, hostRefMT, resultRef)) :
          checkValidReference(FL(), functionState, builder, true, hostRefMT, resultRef));

  auto destinationIntLE = getRegionInstanceDestinationOffset(functionState, builder, regionInstanceRef);
  auto condLE = LLVMBuildICmp(builder, LLVMIntEQ, destinationIntLE, sizeIntLE, "cond");
  buildAssertV(globalState, functionState, builder, condLE, "Serialization start mismatch!");

  auto sizeRef =
      wrap(
          globalState->getRegion(globalState->metalCache->i32Ref),
          globalState->metalCache->i32Ref,
          LLVMBuildTrunc(builder, sizeIntLE, LLVMInt32TypeInContext(globalState->context), "truncd"));

  return std::make_pair(resultRef, sizeRef);
}

std::pair<Ref, Ref> Linear::receiveUnencryptedAlienReference(
    FunctionState* functionState,
    LLVMBuilderRef builder,
    Ref sourceRegionInstanceRef,
    Ref targetRegionInstanceRef,
    Reference* sourceRefMT,
    Reference* targetRefMT,
    Ref sourceRef) {
  buildFlare(FL(), globalState, functionState, builder);

  assert(sourceRefMT->ownership == Ownership::SHARE);

  auto sourceRegion = globalState->getRegion(sourceRefMT);

  auto sourceRefLE =
      globalState->getRegion(sourceRefMT)
          ->checkValidReference(FL(), functionState, builder, true, sourceRefMT, sourceRef);

  if (dynamic_cast<Int*>(sourceRefMT->kind)) {
    auto resultRef = wrap(globalState->getRegion(targetRefMT), targetRefMT, sourceRefLE);
    auto sizeRef = globalState->constI32(LLVMABISizeOfType(globalState->dataLayout, translateType(targetRefMT)));
    return std::make_pair(resultRef, sizeRef);
  } else if (dynamic_cast<Bool*>(sourceRefMT->kind)) {
    auto resultLE = LLVMBuildZExt(builder, sourceRefLE, LLVMInt8TypeInContext(globalState->context), "boolAsI8");
    auto resultRef = wrap(globalState->getRegion(targetRefMT), targetRefMT, resultLE);
    auto sizeRef = globalState->constI32(LLVMABISizeOfType(globalState->dataLayout, translateType(targetRefMT)));
    return std::make_pair(resultRef, sizeRef);
  } else if (dynamic_cast<Float*>(sourceRefMT->kind)) {
    auto resultRef = wrap(globalState->getRegion(targetRefMT), targetRefMT, sourceRefLE);
    auto sizeRef = globalState->constI32(LLVMABISizeOfType(globalState->dataLayout, translateType(targetRefMT)));
    return std::make_pair(resultRef, sizeRef);
  } else if (dynamic_cast<Str*>(sourceRefMT->kind) ||
      dynamic_cast<StructKind*>(sourceRefMT->kind) ||
      dynamic_cast<InterfaceKind*>(sourceRefMT->kind) ||
      dynamic_cast<StaticSizedArrayT*>(sourceRefMT->kind) ||
      dynamic_cast<RuntimeSizedArrayT*>(sourceRefMT->kind)) {
    if (sourceRefMT->location == Location::INLINE) {
      if (sourceRefMT == globalState->metalCache->voidRef) {
        auto emptyTupleRefMT = linearizeReference(globalState->metalCache->voidRef);
        auto resultRef = wrap(this, emptyTupleRefMT, LLVMGetUndef(translateType(emptyTupleRefMT)));
        auto sizeRef = globalState->constI32(LLVMABISizeOfType(globalState->dataLayout, translateType(targetRefMT)));
        return std::make_pair(resultRef, sizeRef);
      } else {
        assert(false);
      }
    } else {
      return topLevelSerialize(functionState, builder, targetRegionInstanceRef, sourceRegionInstanceRef, sourceRefMT->kind, sourceRef);
    }
  } else assert(false);

  assert(false);
}

LLVMTypeRef Linear::getInterfaceMethodVirtualParamAnyType(Reference* reference) {
  return LLVMPointerType(LLVMInt8TypeInContext(globalState->context), 0);
}

LLVMValueRef Linear::predictShallowSize(FunctionState* functionState, LLVMBuilderRef builder, bool includeHeader, Kind* kind, LLVMValueRef lenI32LE) {
  assert(includeHeader); // impl

  assert(globalState->getRegion(kind) == this);
  auto lenI64LE = LLVMBuildZExt(builder, lenI32LE, LLVMInt64TypeInContext(globalState->context), "lenAsI64");
  if (auto structKind = dynamic_cast<StructKind*>(kind)) {
    int size = LLVMABISizeOfType(globalState->dataLayout, structs.getStructStruct(structKind));
    return constI64LE(globalState, size);
  } else if (kind == linearStr) {
    auto headerBytesLE =
        constI64LE(globalState, LLVMABISizeOfType(globalState->dataLayout, structs.getStringStruct()));
    auto lenAndNullTermLE = LLVMBuildAdd(builder, lenI64LE, constI64LE(globalState, 1), "lenAndNullTerm");
    return LLVMBuildAdd(builder, headerBytesLE, lenAndNullTermLE, "sum");
  } else if (auto hostRsaMT = dynamic_cast<RuntimeSizedArrayT*>(kind)) {
    auto headerBytes =
        LLVMABISizeOfType(globalState->dataLayout, structs.getRuntimeSizedArrayStruct(hostRsaMT));
    auto headerBytesLE = constI64LE(globalState, headerBytes);

    auto valeKindMT = valeKindByHostKind.find(hostRsaMT)->second;
    auto valeRsaMT = dynamic_cast<RuntimeSizedArrayT*>(valeKindMT);
    assert(valeRsaMT);
    auto valeElementRefMT = globalState->program->getRuntimeSizedArray(valeRsaMT)->elementType;
    auto hostElementRefMT = linearizeReference(valeElementRefMT);
    auto hostElementRefLT = translateType(hostElementRefMT);

    auto sizePerElement = LLVMABISizeOfType(globalState->dataLayout, LLVMArrayType(hostElementRefLT, 1));
    // The above line tries to include padding... if the below fails, we know there are some serious shenanigans
    // going on in LLVM.
    assert(sizePerElement * 2 == LLVMABISizeOfType(globalState->dataLayout, LLVMArrayType(hostElementRefLT, 2)));
    auto elementsSizeLE = LLVMBuildMul(builder, constI64LE(globalState, sizePerElement), lenI64LE, "elementsSize");

    return LLVMBuildAdd(builder, headerBytesLE, elementsSizeLE, "sum");
  } else if (auto hostSsaMT = dynamic_cast<StaticSizedArrayT*>(kind)) {
    auto headerBytesLE =
        constI64LE(globalState, LLVMABISizeOfType(globalState->dataLayout, structs.getStaticSizedArrayStruct(hostSsaMT)));

    auto valeKindMT = valeKindByHostKind.find(hostSsaMT)->second;
    auto valeSsaMT = dynamic_cast<StaticSizedArrayT*>(valeKindMT);
    assert(valeSsaMT);
    auto valeElementRefMT = globalState->program->getStaticSizedArray(valeSsaMT)->elementType;
    auto hostElementRefMT = linearizeReference(valeElementRefMT);
    auto hostElementRefLT = translateType(hostElementRefMT);

    auto sizePerElement = LLVMABISizeOfType(globalState->dataLayout, LLVMArrayType(hostElementRefLT, 1));
    // The above line tries to include padding... if the below fails, we know there are some serious shenanigans
    // going on in LLVM.
    assert(sizePerElement * 2 == LLVMABISizeOfType(globalState->dataLayout, LLVMArrayType(hostElementRefLT, 2)));
    auto elementsSizeLE = LLVMBuildMul(builder, constI64LE(globalState, sizePerElement), lenI64LE, "elementsSize");

    return LLVMBuildAdd(builder, headerBytesLE, elementsSizeLE, "sum");
  } else assert(false);
}

Ref Linear::receiveAndDecryptFamiliarReference(
    FunctionState* functionState,
    LLVMBuilderRef builder,
    Reference* sourceRefMT,
    LLVMValueRef sourceRefLE) {
  assert(false);
  exit(1);
}

LLVMValueRef Linear::encryptAndSendFamiliarReference(
    FunctionState* functionState,
    LLVMBuilderRef builder,
    Reference* sourceRefMT,
    Ref sourceRef) {
  assert(false);
  exit(1);
}

InterfaceMethod* Linear::getSerializeInterfaceMethod(Kind* valeKind) {
  return globalState->metalCache->getInterfaceMethod(
      getSerializePrototype(valeKind), 2);
}

Ref Linear::callSerialize(
    FunctionState *functionState,
    LLVMBuilderRef builder,
    Kind* valeKind,
    Ref regionInstanceRef,
    Ref sourceRegionInstanceRef,
    Ref objectRef,
    Ref dryRunBoolRef) {
  auto prototype = getSerializePrototype(valeKind);
  if (auto interfaceKind = dynamic_cast<InterfaceKind*>(valeKind)) {
    auto virtualArgRefMT = prototype->params[2];
    int indexInEdge = globalState->getInterfaceMethodIndex(interfaceKind, prototype);
    auto methodFunctionPtrLE =
        globalState->getRegion(virtualArgRefMT)
            ->getInterfaceMethodFunctionPtr(functionState, builder, virtualArgRefMT, objectRef, indexInEdge);
    return buildInterfaceCall(
        globalState, functionState, builder, prototype, methodFunctionPtrLE, {regionInstanceRef, sourceRegionInstanceRef, objectRef, dryRunBoolRef}, 2);
  } else {
    return buildCallV(globalState, functionState, builder, prototype, {regionInstanceRef, sourceRegionInstanceRef, objectRef, dryRunBoolRef});
  }
}

//void Linear::reserveRootMetadataBytesIfNeeded(
//    FunctionState* functionState,
//    LLVMBuilderRef builder,
//    Ref regionInstanceRef) {
//  auto regionInstancePtrLE =
//      checkValidReference(FL(), functionState, builder, regionRefMT, regionInstanceRef);
//  auto rootMetadataBytesNeededPtrLE =
//      unmigratedLLVMBuildStructGEP(builder, regionInstancePtrLE, 2, "rootMetadataBytesNeeded");
//  auto rootMetadataBytesNeededLE = unmigratedLLVMBuildLoad(builder, rootMetadataBytesNeededPtrLE, "rootMetadataBytesNeeded");
//  bumpDestinationOffset(functionState, builder, regionInstanceRef, rootMetadataBytesNeededLE);
//  buildFlare(FL(), globalState, functionState, builder);
//  // Reset it to zero, we only need it once. This will make the next calls not reserve it. See MAPOWN for more.
//  LLVMBuildStore(builder, constI64LE(globalState, 0), rootMetadataBytesNeededPtrLE);
//}

Prototype* Linear::getSerializePrototype(Kind* valeKind) {
  auto boolMT = globalState->metalCache->boolRef;
  auto sourceStructRefMT =
      globalState->metalCache->getReference(
          Ownership::SHARE, Location::YONDER, valeKind);
  auto hostRefMT = linearizeReference(sourceStructRefMT);
  auto sourceRegionRefMT =
      globalState->getRegion(sourceStructRefMT)->getRegionRefType();
  return globalState->metalCache->getPrototype(
      globalState->serializeName, hostRefMT,
      {regionRefMT, sourceRegionRefMT, sourceStructRefMT, boolMT});
}

Prototype* Linear::getSerializeThunkPrototype(StructKind* structKind, InterfaceKind* interfaceKind) {
  auto boolMT = globalState->metalCache->boolRef;
  auto valeStructRefMT =
      globalState->metalCache->getReference(
          Ownership::SHARE, Location::YONDER, structKind);
  auto valeInterfaceRefMT =
      globalState->metalCache->getReference(
          Ownership::SHARE, Location::YONDER, interfaceKind);
  auto hostRefMT = linearizeReference(valeInterfaceRefMT);
  auto sourceRegionRefMT = globalState->getRegion(interfaceKind)->getRegionRefType();
  return globalState->metalCache->getPrototype(
      globalState->serializeThunkName, hostRefMT,
      {regionRefMT, sourceRegionRefMT, valeStructRefMT, boolMT});
}

void Linear::declareConcreteSerializeFunction(Kind* valeKind) {
  auto prototype = getSerializePrototype(valeKind);
  auto nameL = globalState->serializeName->name + "__" + globalState->getKindName(valeKind)->name;
  declareExtraFunction(globalState, prototype, nameL);
}

void Linear::defineConcreteSerializeFunction(Kind* valeKind) {
  auto i32MT = globalState->metalCache->i32Ref;
  auto int64LT = LLVMInt64TypeInContext(globalState->context);
  auto boolMT = globalState->metalCache->boolRef;

  auto prototype = getSerializePrototype(valeKind);

  // If given a struct or array or interface, will serialize its contents and
  // return a pointer to the serialized contents.
  // If given a primitive, will just return the primitive.
  auto serializeMemberOrElement =
      [this](
          FunctionState* functionState,
          LLVMBuilderRef builder,
          Reference* sourceMemberRefMT,
          Ref regionInstanceRef,
          Ref sourceRegionInstanceRef,
          Ref sourceMemberRef,
          Ref dryRunBoolRef) {
        auto targetMemberRefMT = linearizeReference(sourceMemberRefMT);
        auto sourceMemberLE =
            globalState->getRegion(sourceMemberRefMT)->checkValidReference(
                FL(), functionState, builder, true, sourceMemberRefMT, sourceMemberRef);
        if (sourceMemberRefMT == globalState->metalCache->i64Ref) {
          return wrap(globalState->getRegion(targetMemberRefMT), targetMemberRefMT, sourceMemberLE);
        } else if (sourceMemberRefMT == globalState->metalCache->i32Ref) {
          return wrap(globalState->getRegion(targetMemberRefMT), targetMemberRefMT, sourceMemberLE);
        } else if (sourceMemberRefMT == globalState->metalCache->boolRef) {
          auto resultLE = LLVMBuildZExt(builder, sourceMemberLE, LLVMInt8TypeInContext(globalState->context), "boolAsI8");
          return wrap(globalState->getRegion(targetMemberRefMT), targetMemberRefMT, resultLE);
        } else if (sourceMemberRefMT == globalState->metalCache->floatRef) {
          return wrap(globalState->getRegion(targetMemberRefMT), targetMemberRefMT, sourceMemberLE);
        } else if (
            dynamic_cast<Str*>(sourceMemberRefMT->kind) ||
            dynamic_cast<StructKind*>(sourceMemberRefMT->kind) ||
            dynamic_cast<InterfaceKind*>(sourceMemberRefMT->kind) ||
            dynamic_cast<StaticSizedArrayT*>(sourceMemberRefMT->kind) ||
            dynamic_cast<RuntimeSizedArrayT*>(sourceMemberRefMT->kind)) {
          auto destinationMemberRef =
              callSerialize(
                  functionState, builder, sourceMemberRefMT->kind, regionInstanceRef, sourceRegionInstanceRef, sourceMemberRef, dryRunBoolRef);
          return destinationMemberRef;
        } else assert(false);
      };

  defineFunctionBodyV(
      globalState, prototype,
      [&](FunctionState* functionState, LLVMBuilderRef builder) -> void {
        auto valeObjectRefMT = prototype->params[2];
        auto hostObjectRefMT = prototype->returnType;

        auto hostRegionInstanceRef = wrap(globalState->getRegion(regionRefMT), regionRefMT, LLVMGetParam(functionState->containingFuncL, 0));
        auto sourceRegionInstanceRef = wrap(globalState->getRegion(valeKind),
            globalState->getRegion(valeKind)->getRegionRefType(), LLVMGetParam(functionState->containingFuncL, 1));
        auto valeObjectRef = wrap(globalState->getRegion(valeObjectRefMT), valeObjectRefMT, LLVMGetParam(functionState->containingFuncL, 2));
        auto dryRunBoolRef = wrap(globalState->getRegion(boolMT), boolMT, LLVMGetParam(functionState->containingFuncL, 3));

        if (auto valeStructKind = dynamic_cast<StructKind *>(valeObjectRefMT->kind)) {
          auto hostKind = hostKindByValeKind.find(valeStructKind)->second;
          auto hostStructKind = dynamic_cast<StructKind *>(hostKind);
          assert(hostStructKind);
          auto valeStructDefM = globalState->program->getStruct(valeStructKind);

          auto hostObjectRef = innerAllocate(hostRegionInstanceRef, FL(), functionState, builder, hostObjectRefMT);
          auto innerStructPtrLE = checkValidReference(FL(), functionState, builder, true, hostObjectRefMT, hostObjectRef);

          std::vector<Ref> hostMemberRefs;
          for (int i = 0; i < valeStructDefM->members.size(); i++) {
            auto valeMemberM = valeStructDefM->members[i];
            auto sourceMemberRefMT = valeMemberM->type;
            auto sourceMemberRef =
                globalState->getRegion(valeObjectRefMT)->loadMember(
                    functionState, builder, sourceRegionInstanceRef, valeObjectRefMT, valeObjectRef, true,
                    i, valeMemberM->type, valeMemberM->type, valeMemberM->name);
            auto serializedMemberOrElementRef =
                serializeMemberOrElement(
                    functionState, builder, sourceMemberRefMT, hostRegionInstanceRef, sourceRegionInstanceRef, sourceMemberRef, dryRunBoolRef);
            hostMemberRefs.push_back(serializedMemberOrElementRef);
          }

          auto dryRunBoolLE =
              globalState->getRegion(boolMT)
                  ->checkValidReference(FL(), functionState, builder, true, boolMT, dryRunBoolRef);
          buildIfV(
              globalState, functionState, builder, LLVMBuildNot(builder, dryRunBoolLE, "notDryRun"),
              [this, hostRegionInstanceRef, functionState, hostObjectRefMT, valeStructDefM, hostMemberRefs, hostObjectRef](
                  LLVMBuilderRef thenBuilder) {
                for (int i = 0; i < valeStructDefM->members.size(); i++) {
                  auto hostMemberRef = hostMemberRefs[i];
                  auto hostMemberType = linearizeReference(valeStructDefM->members[i]->type);
                  auto memberName = valeStructDefM->members[i]->name;

                  initializeMember(
                      functionState, thenBuilder, hostRegionInstanceRef, hostObjectRefMT, hostObjectRef, true,
                      i, memberName, hostMemberType, hostMemberRef);
                }
              });

//          // Remember, we're subtracting each size from a very large number, so its easier to round down
//          // to the next multiple of 16.
//          totalSizeIntLE = hexRoundDown(globalState, builder, totalSizeIntLE);

          auto hostObjectRefLE =
              checkValidReference(FL(), functionState, builder, true, hostObjectRefMT, hostObjectRef);
          LLVMBuildRet(builder, hostObjectRefLE);
        } else if (auto valeStrMT = dynamic_cast<Str*>(valeObjectRefMT->kind)) {
          auto hostStrMT = hostKindByValeKind.find(valeStrMT)->second;
          assert(hostStrMT);
          auto hostStrRefMT = globalState->metalCache->getReference(Ownership::SHARE, Location::YONDER, hostStrMT);

          auto lengthLE =
              globalState->getRegion(valeObjectRefMT)->
                  getStringLen(functionState, builder, sourceRegionInstanceRef, valeObjectRef);
          auto sourceBytesPtrLE =
              globalState->getRegion(valeObjectRefMT)->
                  getStringBytesPtr(functionState, builder, sourceRegionInstanceRef, valeObjectRef);

          auto hostStrRef = innerMallocStr(hostRegionInstanceRef, functionState, builder, lengthLE, sourceBytesPtrLE, dryRunBoolRef);

          buildFlare(FL(), globalState, functionState, builder, "Returning from serialize function!");

          auto hostStrPtrLE =
              checkValidReference(FL(), functionState, builder, true, hostObjectRefMT, hostStrRef);
          LLVMBuildRet(builder, hostStrPtrLE);
        } else if (auto valeRsaMT = dynamic_cast<RuntimeSizedArrayT*>(valeObjectRefMT->kind)) {

          buildFlare(FL(), globalState, functionState, builder, "In RSA serialize!");

          auto hostKindMT = hostKindByValeKind.find(valeRsaMT)->second;
          auto hostRsaMT = dynamic_cast<RuntimeSizedArrayT *>(hostKindMT);
          assert(hostRsaMT);
          auto hostRsaRefMT = globalState->metalCache->getReference(Ownership::SHARE, Location::YONDER, hostKindMT);

          auto lengthRef =
              globalState->getRegion(valeObjectRefMT)
                  ->getRuntimeSizedArrayLength(
                      functionState, builder, sourceRegionInstanceRef, valeObjectRefMT, valeObjectRef, true);

          buildFlare(FL(), globalState, functionState, builder);

          auto hostRsaRef =
              innerConstructRuntimeSizedArray(
                  hostRegionInstanceRef,
                  functionState, builder, hostRsaRefMT, hostRsaMT, lengthRef, dryRunBoolRef);
          auto valeMemberRefMT = globalState->program->getRuntimeSizedArray(valeRsaMT)->elementType;

          buildFlare(FL(), globalState, functionState, builder);

          intRangeLoopV(
              globalState, functionState, builder, lengthRef,
              [this, functionState, sourceRegionInstanceRef, hostObjectRefMT, boolMT, hostRsaRef, valeObjectRefMT, hostRsaMT, valeRsaMT, valeObjectRef, valeMemberRefMT, hostRegionInstanceRef, serializeMemberOrElement, dryRunBoolRef](
                  Ref indexRef, LLVMBuilderRef bodyBuilder) {
                buildFlare(FL(), globalState, functionState, bodyBuilder, "In serialize iteration!");

                auto sourceMemberRef =
                    globalState->getRegion(valeObjectRefMT)
                        ->loadElementFromRSA(
                            functionState, bodyBuilder, sourceRegionInstanceRef, valeObjectRefMT, valeRsaMT,
                            valeObjectRef, true, indexRef)
                        .move();
                buildFlare(FL(), globalState, functionState, bodyBuilder);
                auto hostElementRef =
                    serializeMemberOrElement(
                        functionState, bodyBuilder, valeMemberRefMT, hostRegionInstanceRef, sourceRegionInstanceRef,
                        sourceMemberRef, dryRunBoolRef);
                buildFlare(FL(), globalState, functionState, bodyBuilder);
                auto dryRunBoolLE =
                    globalState->getRegion(boolMT)
                        ->checkValidReference(FL(), functionState, bodyBuilder, true, boolMT, dryRunBoolRef);
                buildIfV(
                    globalState, functionState, bodyBuilder, LLVMBuildNot(bodyBuilder, dryRunBoolLE, "notDryRun"),
                    [this, functionState, hostRegionInstanceRef, hostObjectRefMT, hostRsaRef, indexRef, hostElementRef, hostRsaMT](
                        LLVMBuilderRef thenBuilder) mutable {
                      pushRuntimeSizedArrayNoBoundsCheck(
                          functionState, thenBuilder, hostRegionInstanceRef, hostObjectRefMT, hostRsaMT, hostRsaRef,
                          true, indexRef,
                          hostElementRef);
                      buildFlare(FL(), globalState, functionState, thenBuilder);
                    });
              });

          buildFlare(FL(), globalState, functionState, builder, "Returning from serialize function!");

          auto hostRsaPtrLE =
              checkValidReference(FL(), functionState, builder, true, hostRsaRefMT, hostRsaRef);
          LLVMBuildRet(builder, hostRsaPtrLE);
        } else if (auto valeSsaMT = dynamic_cast<StaticSizedArrayT*>(valeObjectRefMT->kind)) {

          buildFlare(FL(), globalState, functionState, builder, "In RSA serialize!");

          auto hostKindMT = hostKindByValeKind.find(valeSsaMT)->second;
          auto hostSsaMT = dynamic_cast<StaticSizedArrayT *>(hostKindMT);
          assert(hostSsaMT);
          auto hostSsaRefMT = globalState->metalCache->getReference(Ownership::SHARE, Location::YONDER, hostKindMT);

          auto valeSsaDef = globalState->program->getStaticSizedArray(valeSsaMT);
          auto lengthRef = globalState->constI32(valeSsaDef->size);

          buildFlare(FL(), globalState, functionState, builder);

          auto hostSsaRef =
              innerConstructStaticSizedArray(
                  hostRegionInstanceRef,
                  functionState, builder, hostSsaRefMT, hostSsaMT, dryRunBoolRef);
          auto valeMemberRefMT = globalState->program->getStaticSizedArray(valeSsaMT)->elementType;

          buildFlare(FL(), globalState, functionState, builder);

          intRangeLoopV(
              globalState, functionState, builder, lengthRef,
              [this, functionState, sourceRegionInstanceRef, hostObjectRefMT, boolMT, hostSsaRef, valeObjectRefMT, hostSsaMT, valeSsaMT, valeObjectRef, valeMemberRefMT, hostRegionInstanceRef, serializeMemberOrElement, dryRunBoolRef](
                  Ref indexRef, LLVMBuilderRef bodyBuilder) {
                buildFlare(FL(), globalState, functionState, bodyBuilder, "In serialize iteration!");

                auto sourceMemberRef =
                    globalState->getRegion(valeObjectRefMT)
                        ->loadElementFromSSA(
                            functionState, bodyBuilder, sourceRegionInstanceRef, valeObjectRefMT, valeSsaMT,
                            valeObjectRef, true, indexRef)
                        .move();
                buildFlare(FL(), globalState, functionState, bodyBuilder);
                auto hostElementRef =
                    serializeMemberOrElement(
                        functionState, bodyBuilder, valeMemberRefMT, hostRegionInstanceRef, sourceRegionInstanceRef,
                        sourceMemberRef, dryRunBoolRef);
                buildFlare(FL(), globalState, functionState, bodyBuilder);
                auto dryRunBoolLE =
                    globalState->getRegion(boolMT)
                        ->checkValidReference(FL(), functionState, bodyBuilder, true, boolMT, dryRunBoolRef);
                buildFlare(FL(), globalState, functionState, bodyBuilder);
                buildIfV(
                    globalState, functionState, bodyBuilder, LLVMBuildNot(bodyBuilder, dryRunBoolLE, "notDryRun"),
                    [this, functionState, hostRegionInstanceRef, hostObjectRefMT, hostSsaRef, indexRef, hostElementRef, hostSsaMT](
                        LLVMBuilderRef thenBuilder) mutable {
                      buildFlare(FL(), globalState, functionState, thenBuilder);
                      initializeElementInSSA(
                          functionState, thenBuilder, hostRegionInstanceRef, hostObjectRefMT, hostSsaMT, hostSsaRef,
                          true, indexRef,
                          hostElementRef);
                      buildFlare(FL(), globalState, functionState, thenBuilder);
                    });
                buildFlare(FL(), globalState, functionState, bodyBuilder);
              });

          buildFlare(FL(), globalState, functionState, builder, "Returning from serialize function!");

          auto hostSsaPtrLE =
              checkValidReference(FL(), functionState, builder, true, hostSsaRefMT, hostSsaRef);
          LLVMBuildRet(builder, hostSsaPtrLE);
        } else assert(false);
      });
}

// See PSBCBO
LLVMValueRef Linear::translateBetweenBufferAddressAndPointer(
    FunctionState* functionState,
    LLVMBuilderRef builder,
    Ref regionInstanceRef,
    Reference* hostRefMT,
    LLVMValueRef unadjustedHostRefLE,
    bool bufferAddressToPointer) {
  // Just to sanity check, but also to hand to explodeInterfaceRef below.
  auto unadjustedHostRef = wrap(this, hostRefMT, unadjustedHostRefLE);
  auto hostRefLT = globalState->getRegion(hostRefMT)->translateType(hostRefMT);
  assert(hostRefLT == LLVMTypeOf(unadjustedHostRefLE));

  auto adjustPtr =
      [this, functionState, builder, regionInstanceRef, bufferAddressToPointer, hostRefLT](
          LLVMValueRef unadjustedHostObjPtrLE) {
        auto int64LT = LLVMInt64TypeInContext(globalState->context);

        auto serializedAddressAdjusterLE =
            getRegionInstanceSerializedAddressAdjuster(functionState, builder, regionInstanceRef);

        auto unadjustedHostObjPtrAsI64LE =
            LLVMBuildPtrToInt(builder, unadjustedHostObjPtrLE, int64LT, "unadjustedHostObjPtr");
        auto adjustedHostObjPtrAsI64LE =
            bufferAddressToPointer ?
            LLVMBuildSub(builder, unadjustedHostObjPtrAsI64LE, serializedAddressAdjusterLE, "adjustedHostObjPtrAsI64") :
            LLVMBuildAdd(builder, unadjustedHostObjPtrAsI64LE, serializedAddressAdjusterLE, "adjustedHostObjPtrAsI64");
        auto adjustedHostObjPtrLE =
            LLVMBuildIntToPtr(builder, adjustedHostObjPtrAsI64LE, hostRefLT, "adjustedHostObjPtr");

        return adjustedHostObjPtrLE;
      };

  if (dynamic_cast<Int*>(hostRefMT->kind) ||
      dynamic_cast<Bool*>(hostRefMT->kind) ||
      dynamic_cast<Float*>(hostRefMT->kind)) {
    return checkValidReference(FL(), functionState, builder, true, hostRefMT, unadjustedHostRef);
  } else if (dynamic_cast<StructKind*>(hostRefMT->kind) ||
      dynamic_cast<RuntimeSizedArrayT*>(hostRefMT->kind) ||
      dynamic_cast<StaticSizedArrayT*>(hostRefMT->kind) ||
      dynamic_cast<Str*>(hostRefMT->kind)) {
    auto unadjustedPtrLE =
        checkValidReference(FL(), functionState, builder, true, hostRefMT, unadjustedHostRef);
    auto adjustedPtrLE = adjustPtr(unadjustedPtrLE);
    // Just doublechecking
    return checkValidReference(FL(), functionState, builder, true, hostRefMT, wrap(this, hostRefMT, adjustedPtrLE));
  } else if (dynamic_cast<InterfaceKind*>(hostRefMT->kind)) {
    auto [edgeNumLE, objPtrLE] =
        explodeInterfaceRef(functionState, builder, hostRefMT, unadjustedHostRef);
    objPtrLE = adjustPtr(objPtrLE);
    auto adjustedRef = assembleInterfaceRef(builder, hostRefMT, objPtrLE, edgeNumLE);
    return checkValidReference(FL(), functionState, builder, true, hostRefMT, adjustedRef);
  } else {
    assert(false);
  }
}

Kind* Linear::linearizeKind(Kind* kindMT) {
  assert(globalState->getRegion(kindMT) == globalState->rcImm);
  return hostKindByValeKind.find(kindMT)->second;
}

StructKind* Linear::linearizeStructKind(StructKind* kindMT) {
  assert(globalState->getRegion(kindMT) == globalState->rcImm);
  auto kind = hostKindByValeKind.find(kindMT)->second;
  auto structKind = dynamic_cast<StructKind*>(kind);
  return structKind;
}

StaticSizedArrayT* Linear::unlinearizeSSA(StaticSizedArrayT *kindMT) {
  assert(globalState->getRegion(kindMT) == globalState->linearRegion);
  auto kind = valeKindByHostKind.find(kindMT)->second;
  auto ssaMT = dynamic_cast<StaticSizedArrayT*>(kind);
  return ssaMT;
}

StructKind* Linear::unlinearizeStructKind(StructKind* kindMT) {
  assert(globalState->getRegion(kindMT) == globalState->linearRegion);
  auto kind = valeKindByHostKind.find(kindMT)->second;
  auto structKind = dynamic_cast<StructKind*>(kind);
  return structKind;
}

InterfaceKind* Linear::unlinearizeInterfaceKind(InterfaceKind* kindMT) {
  assert(globalState->getRegion(kindMT) == globalState->linearRegion);
  auto kind = valeKindByHostKind.find(kindMT)->second;
  auto interfaceKind = dynamic_cast<InterfaceKind*>(kind);
  return interfaceKind;
}

InterfaceKind* Linear::linearizeInterfaceKind(InterfaceKind* kindMT) {
  assert(globalState->getRegion(kindMT) == globalState->rcImm);
  auto kind = hostKindByValeKind.find(kindMT)->second;
  auto interfaceKind = dynamic_cast<InterfaceKind*>(kind);
  return interfaceKind;
}

Reference* Linear::linearizeReference(Reference* immRcRefMT) {
  assert(globalState->getRegion(immRcRefMT) == globalState->rcImm);
  auto hostKind = hostKindByValeKind.find(immRcRefMT->kind)->second;
  return globalState->metalCache->getReference(
      immRcRefMT->ownership, immRcRefMT->location, hostKind);
}

Reference* Linear::unlinearizeReference(Reference* hostRefMT) {
  assert(globalState->getRegion(hostRefMT) == globalState->linearRegion);
  auto valeKind = valeKindByHostKind.find(hostRefMT->kind)->second;
  return globalState->metalCache->getReference(
      hostRefMT->ownership, hostRefMT->location, valeKind);
}

void Linear::pushRuntimeSizedArrayNoBoundsCheck(
    FunctionState *functionState,
    LLVMBuilderRef builder,
    Ref regionInstanceRef,
    Reference *hostRsaRefMT,
    RuntimeSizedArrayT *hostRsaMT,
    Ref hostRsaRef,
    bool arrayRefKnownLive,
    Ref indexRef,
    Ref elementRef) {
  buildFlare(FL(), globalState, functionState, builder);
  assert(hostRsaRefMT->kind == hostRsaMT);
  assert(globalState->getRegion(hostRsaMT) == this);

  buildFlare(FL(), globalState, functionState, builder);
  auto valeKindMT = valeKindByHostKind.find(hostRsaMT)->second;
  auto valeRsaMT = dynamic_cast<RuntimeSizedArrayT*>(valeKindMT);
  assert(valeRsaMT);
  auto valeElementRefMT = globalState->program->getRuntimeSizedArray(valeRsaMT)->elementType;
  auto hostElementRefMT = linearizeReference(valeElementRefMT);
  auto hostRsaElementLT = globalState->getRegion(hostElementRefMT)->translateType(hostElementRefMT);
  auto elementRefLE = globalState->getRegion(hostElementRefMT)->checkValidReference(FL(), functionState, builder, true, hostElementRefMT, elementRef);

  buildFlare(FL(), globalState, functionState, builder);
  auto i32MT = globalState->metalCache->i32Ref;

  auto indexLE = globalState->getRegion(i32MT)->checkValidReference(FL(), functionState, builder, true, i32MT, indexRef);

  buildFlare(FL(), globalState, functionState, builder);
  auto unadjustedRsaPtrLE = checkValidReference(FL(), functionState, builder, true, hostRsaRefMT, hostRsaRef);
  // When we write a pointer, we need to subtract the Serialized Address Adjuster, see PSBCBO.
  auto adjustedRsaPtrLE =
      translateBetweenBufferAddressAndPointer(
          functionState, builder, regionInstanceRef, hostRsaRefMT, unadjustedRsaPtrLE, true);
  auto hostRsaElementsPtrLE =
      structs.getRuntimeSizedArrayElementsPtr(functionState, builder, hostRsaMT, adjustedRsaPtrLE);

  buildFlare(FL(), globalState, functionState, builder);

  auto adjustedHostRsaElementsPtrLE =
      structs.getRuntimeSizedArrayElementsPtr(
          functionState, builder, hostRsaMT, adjustedRsaPtrLE);

  buildFlare(FL(), globalState, functionState, builder);

  // When we write a pointer, we need to subtract the Serialized Address Adjuster, see PSBCBO.
  storeInnerArrayMember(
      globalState, functionState, builder, hostRsaElementLT, adjustedHostRsaElementsPtrLE, indexLE, elementRefLE);
}

Ref Linear::popRuntimeSizedArrayNoBoundsCheck(
    FunctionState* functionState,
    LLVMBuilderRef builder,
    Ref arrayRegionInstanceRef,
    Reference* rsaRefMT,
    RuntimeSizedArrayT* rsaMT,
    Ref arrayRef,
    bool arrayRefKnownLive,
    Ref indexRef) {
  assert(false);
  exit(1);
}

void Linear::initializeElementInSSA(
    FunctionState* functionState,
    LLVMBuilderRef builder,
    Ref regionInstanceRef,
    Reference* hostSsaRefMT,
    StaticSizedArrayT* hostSsaMT,
    Ref hostSsaRef,
    bool arrayRefKnownLive,
    Ref indexRef,
    Ref elementRef) {
  buildFlare(FL(), globalState, functionState, builder);

  assert(hostSsaRefMT->kind == hostSsaMT);
  assert(globalState->getRegion(hostSsaMT) == this);

  auto valeKindMT = valeKindByHostKind.find(hostSsaMT)->second;
  auto valeSsaMT = dynamic_cast<StaticSizedArrayT*>(valeKindMT);
  assert(valeSsaMT);
  auto valeElementRefMT = globalState->program->getStaticSizedArray(valeSsaMT)->elementType;
  auto hostElementRefMT = linearizeReference(valeElementRefMT);
  auto hostRsaElementLT = globalState->getRegion(hostElementRefMT)->translateType(hostElementRefMT);
  auto elementRefLE = globalState->getRegion(hostElementRefMT)->checkValidReference(FL(), functionState, builder, true, hostElementRefMT, elementRef);

  auto i32MT = globalState->metalCache->i32Ref;

  auto indexLE = globalState->getRegion(i32MT)->checkValidReference(FL(), functionState, builder, true, i32MT, indexRef);

  auto ssaPtrLE = checkValidReference(FL(), functionState, builder, true, hostSsaRefMT, hostSsaRef);

  buildFlare(FL(), globalState, functionState, builder);

  // When we write a pointer, we need to subtract the Serialized Address Adjuster, see PSBCBO.
  auto adjustedHostSsaPtrLE =
      translateBetweenBufferAddressAndPointer(
          functionState, builder, regionInstanceRef, hostSsaRefMT, ssaPtrLE, true);

  buildFlare(FL(), globalState, functionState, builder);

  auto adjustedHostSsaElementsPtrLE =
      structs.getStaticSizedArrayElementsPtr(
          functionState, builder, hostSsaMT, adjustedHostSsaPtrLE);

  buildFlare(FL(), globalState, functionState, builder);

  storeInnerArrayMember(
      globalState, functionState, builder, hostRsaElementLT, adjustedHostSsaElementsPtrLE, indexLE, elementRefLE);

  buildFlare(FL(), globalState, functionState, builder);
}

Ref Linear::deinitializeElementFromSSA(
    FunctionState* functionState,
    LLVMBuilderRef builder,
    Reference* ssaRefMT,
    StaticSizedArrayT* ssaMT,
    Ref arrayRef,
    bool arrayRefKnownLive,
    Ref indexRef) {
  assert(false);
  exit(1);
}

Weakability Linear::getKindWeakability(Kind* kind) {
  return Weakability::NON_WEAKABLE;
}

FuncPtrLE Linear::getInterfaceMethodFunctionPtr(
    FunctionState* functionState,
    LLVMBuilderRef builder,
    Reference* virtualParamMT,
    Ref virtualArgRef,
    int indexInEdge) {

  assert(indexInEdge == 0); // All the below is special cased for just the unserialize method.

  auto hostInterfaceMT = dynamic_cast<InterfaceKind*>(virtualParamMT->kind);
  assert(hostInterfaceMT);
  auto valeInterfaceMT = unlinearizeInterfaceKind(hostInterfaceMT);

  LLVMValueRef edgeNumLE = nullptr;
  LLVMValueRef newVirtualArgLE = nullptr;
  std::tie(edgeNumLE, newVirtualArgLE) =
      explodeInterfaceRef(functionState, builder, virtualParamMT, virtualArgRef);
  buildFlare(FL(), globalState, functionState, builder, "edge num: ", edgeNumLE);
  buildFlare(FL(), globalState, functionState, builder, "ptr: ", ptrToIntLE(globalState, builder, newVirtualArgLE));

  auto orderedSubstructs = structs.getOrderedSubstructs(hostInterfaceMT);
  auto isValidEdgeNumLE =
      LLVMBuildICmp(builder, LLVMIntULT, edgeNumLE, constI64LE(globalState, orderedSubstructs.size()), "isValidEdgeNum");

  buildIfV(
      globalState, functionState, builder, isZeroLE(builder, isValidEdgeNumLE),
      [this, edgeNumLE](LLVMBuilderRef thenBuilder) {
        buildPrintToStderr(globalState, thenBuilder, "Invalid edge number (");
        buildPrintToStderr(globalState, thenBuilder, edgeNumLE);
        buildPrintToStderr(globalState, thenBuilder, "), exiting!\n");
        auto exitCodeIntLE = LLVMConstInt(LLVMInt64TypeInContext(globalState->context), 1, false);
        globalState->externs->exit.call(thenBuilder, {exitCodeIntLE}, "");
      });

  auto funcLT = globalState->getInterfaceFunctionTypesNonPointer(hostInterfaceMT)[indexInEdge];
  auto funcPtrLT = LLVMPointerType(funcLT, 0);

  // This is a function family table, in that it's a table of all of an abstract function's overrides.
  // It's a function family, in Valestrom terms.
  auto fftableLT = LLVMArrayType(funcPtrLT, orderedSubstructs.size());
  auto fftablePtrLE = makeBackendLocal(functionState, builder, fftableLT, "arrays", LLVMGetUndef(fftableLT));
  for (int i = 0; i < orderedSubstructs.size(); i++) {
    auto hostStructMT = orderedSubstructs[i];
    auto valeStructMT = unlinearizeStructKind(hostStructMT);
    auto prototype = globalState->rcImm->getUnserializeThunkPrototype(valeStructMT, valeInterfaceMT);
    auto funcLE = globalState->lookupFunction(prototype);
    auto bitcastedFuncPtrLE = LLVMBuildPointerCast(builder, funcLE.ptrLE, funcPtrLT, "bitcastedFunc");
    // We're using store here because LLVMBuildInsertElement caused LLVM to go into an infinite loop and crash.
    std::vector<LLVMValueRef> indices = { constI64LE(globalState, 0), constI64LE(globalState, i) };
    auto destPtrLE = LLVMBuildGEP2(builder, fftableLT, fftablePtrLE, indices.data(), indices.size(), "storeMethodPtrPtr");
    assert(LLVMTypeOf(destPtrLE) == LLVMPointerType(funcPtrLT, 0));
    LLVMBuildStore(builder, bitcastedFuncPtrLE, destPtrLE);
  }

  std::vector<LLVMValueRef> indices = { constI64LE(globalState, 0), edgeNumLE };
  auto methodPtrPtrLE = LLVMBuildGEP2(builder, fftableLT, fftablePtrLE, indices.data(), indices.size(), "methodPtrPtr");
  assert(LLVMTypeOf(methodPtrPtrLE) == LLVMPointerType(funcPtrLT, 0));
  auto methodFuncPtr = LLVMBuildLoad2(builder, funcPtrLT, methodPtrPtrLE, "methodFuncPtr");
  assert(LLVMTypeOf(methodFuncPtr) == funcPtrLT);
  return FuncPtrLE(funcLT, methodFuncPtr);
}

LLVMValueRef Linear::stackify(
    FunctionState* functionState,
    LLVMBuilderRef builder,
    Local* local,
    Ref refToStore,
    bool knownLive) {
  auto toStoreLE = checkValidReference(FL(), functionState, builder, true, local->type, refToStore);
  auto typeLT = translateType(local->type);
  return makeBackendLocal(functionState, builder, typeLT, local->id->maybeName.c_str(), toStoreLE);
}

Ref Linear::unstackify(FunctionState* functionState, LLVMBuilderRef builder, Local* local, LLVMValueRef localAddr) {
  return loadLocal(functionState, builder, local, localAddr);
}

Ref Linear::loadLocal(FunctionState* functionState, LLVMBuilderRef builder, Local* local, LLVMValueRef localAddr) {
  return normalLocalLoad(globalState, functionState, builder, local, localAddr);
}

Ref Linear::localStore(FunctionState* functionState, LLVMBuilderRef builder, Local* local, LLVMValueRef localAddr, Ref refToStore, bool knownLive) {
  return normalLocalStore(globalState, functionState, builder, local, localAddr, refToStore);
}


Ref Linear::createRegionInstanceLocal(FunctionState* functionState, LLVMBuilderRef builder) {
  // This is the method called via IRegion when we want to make a function-bound region.
  // Linear isn't used for that though, linear's used for serializing to/from C and files.
  assert(false);
  exit(1);
}

Ref Linear::createRegionInstanceLocal(
    FunctionState *functionState,
    LLVMBuilderRef builder,
    LLVMValueRef useOffsetsLE,
    LLVMValueRef bufferBeginOffsetLE) {
  auto regionLT = structs.getStructStruct(regionKind);
  auto regionInstancePtrLE =
      makeBackendLocal(functionState, builder, regionLT, "region", LLVMGetUndef(regionLT));
  auto regionInstanceRef = wrap(this, regionRefMT, regionInstancePtrLE);

  setRegionInstanceDestinationBufferStartPtr(
      functionState, builder, regionInstanceRef,
      LLVMConstNull(LLVMPointerType(LLVMInt8TypeInContext(globalState->context), 0)));
  setRegionInstanceDestinationOffset(
      functionState, builder, regionInstanceRef, constI64LE(globalState, 0));
  setRegionInstanceUseOffsets(
      functionState, builder, regionInstanceRef, useOffsetsLE);
  setRegionInstanceBufferBeginOffset(
      functionState, builder, regionInstanceRef, bufferBeginOffsetLE);
  setRegionInstanceSerializedAddressAdjuster(
      functionState, builder, regionInstanceRef, constI64LE(globalState, 0));

  return regionInstanceRef;
}

void Linear::setRegionInstanceDestinationBufferStartPtr(
    FunctionState* functionState,
    LLVMBuilderRef builder,
    Ref regionInstanceRef,
    LLVMValueRef destinationBufferStartPtrLE) {
  auto regionStructLT = structs.getStructStruct(regionKind);
  auto regionInstancePtrLE =
      checkValidReference(FL(), functionState, builder, true, regionRefMT, regionInstanceRef);
  auto destBeginningPtrLE = LLVMBuildStructGEP2(builder, regionStructLT, regionInstancePtrLE, 0, "destBeginningPtr");
  LLVMBuildStore(builder, destinationBufferStartPtrLE, destBeginningPtrLE);
}

LLVMValueRef Linear::getRegionInstanceDestinationBufferStartPtr(
    FunctionState* functionState,
    LLVMBuilderRef builder,
    Ref regionInstanceRef) {
  auto int8LT = LLVMInt8TypeInContext(globalState->context);
  auto int8PtrLT = LLVMPointerType(int8LT, 0);
  auto regionStructLT = structs.getStructStruct(regionKind);
  auto regionInstancePtrLE =
      checkValidReference(FL(), functionState, builder, true, regionRefMT, regionInstanceRef);
  auto bufferBeginPtrPtrLE =
      LLVMBuildStructGEP2(
          builder, regionStructLT, regionInstancePtrLE, 0, "bufferBeginPtrPtr");
  return LLVMBuildLoad2(builder, int8PtrLT, bufferBeginPtrPtrLE, "bufferBeginPtr");
};

LLVMValueRef Linear::getRegionInstanceDestinationOffset(
    FunctionState* functionState,
    LLVMBuilderRef builder,
    Ref regionInstanceRef) {
  auto regionStructLT = structs.getStructStruct(regionKind);
  auto int64LT = LLVMInt64TypeInContext(globalState->context);
  auto regionInstancePtrLE =
      checkValidReference(FL(), functionState, builder, true, regionRefMT, regionInstanceRef);
  auto destinationOffsetPtrLE =
      LLVMBuildStructGEP2(builder, regionStructLT, regionInstancePtrLE, 1, "destinationOffsetPtr");
  return LLVMBuildLoad2(builder, int64LT, destinationOffsetPtrLE, "destinationOffset");
}

void Linear::setRegionInstanceDestinationOffset(
    FunctionState* functionState,
    LLVMBuilderRef builder,
    Ref regionInstanceRef,
    LLVMValueRef destinationOffsetLE) {
  auto int64LT = LLVMInt64TypeInContext(globalState->context);
  auto regionStructLT = structs.getStructStruct(regionKind);
  auto regionInstancePtrLE =
      checkValidReference(FL(), functionState, builder, true, regionRefMT, regionInstanceRef);
  auto counterPtrLE =
      LLVMBuildStructGEP2(builder, regionStructLT, regionInstancePtrLE, 1, "destOffsetPtr");
  buildFlare(FL(), globalState, functionState, builder, "*counterPtrLE before: ", LLVMBuildLoad2(builder, int64LT, counterPtrLE, ""));
  LLVMBuildStore(builder, destinationOffsetLE, counterPtrLE);
  buildFlare(FL(), globalState, functionState, builder, "*counterPtrLE after: ", LLVMBuildLoad2(builder, int64LT, counterPtrLE, ""));
}

void Linear::setRegionInstanceUseOffsets(
    FunctionState* functionState,
    LLVMBuilderRef builder,
    Ref regionInstanceRef,
    LLVMValueRef useOffsetsLE) {
  auto regionStructLT = structs.getStructStruct(regionKind);
  auto regionInstancePtrLE =
      checkValidReference(
          FL(), functionState, builder, true, regionRefMT, regionInstanceRef);
  auto useOffsetsPtrLE =
      LLVMBuildStructGEP2(builder, regionStructLT, regionInstancePtrLE, 2, "useOffsetsPtr");
  LLVMBuildStore(builder, useOffsetsLE, useOffsetsPtrLE);
}

LLVMValueRef Linear::getRegionInstanceUseOffsets(
    FunctionState* functionState,
    LLVMBuilderRef builder,
    Ref regionInstanceRef) {
  auto int1LT = LLVMInt1TypeInContext(globalState->context);
  auto regionStructLT = structs.getStructStruct(regionKind);
  auto regionInstancePtrLE =
      checkValidReference(FL(), functionState, builder, true, regionRefMT, regionInstanceRef);
  auto useOffsetsPtrLE =
      LLVMBuildStructGEP2(builder, regionStructLT, regionInstancePtrLE, 2, "useOffsetsPtr");
  return LLVMBuildLoad2(builder, int1LT, useOffsetsPtrLE, "useOffsets");
}

void Linear::setRegionInstanceBufferBeginOffset(
    FunctionState* functionState,
    LLVMBuilderRef builder,
    Ref regionInstanceRef,
    LLVMValueRef bufferBeginOffsetLE) {
  auto regionStructLT = structs.getStructStruct(regionKind);
  auto regionInstancePtrLE =
      checkValidReference(FL(), functionState, builder, true, regionRefMT, regionInstanceRef);
  auto bufferBeginOffsetPtrLE =
      LLVMBuildStructGEP2(builder, regionStructLT, regionInstancePtrLE, 3, "bufferBeginOffsetPtr");
  LLVMBuildStore(builder, bufferBeginOffsetLE, bufferBeginOffsetPtrLE);
}

LLVMValueRef Linear::getRegionInstanceBufferBeginOffset(
    FunctionState* functionState,
    LLVMBuilderRef builder,
    Ref regionInstanceRef) {
  auto int64LT = LLVMInt64TypeInContext(globalState->context);
  auto regionStructLT = structs.getStructStruct(regionKind);
  auto regionInstancePtrLE =
      checkValidReference(FL(), functionState, builder, true, regionRefMT, regionInstanceRef);
  auto bufferBeginOffsetPtrLE =
      LLVMBuildStructGEP2(builder, regionStructLT, regionInstancePtrLE, 3, "bufferBeginOffsetPtr");
  return LLVMBuildLoad2(builder, int64LT, bufferBeginOffsetPtrLE, "bufferBeginOffset");
}

void Linear::setRegionInstanceSerializedAddressAdjuster(
    FunctionState* functionState,
    LLVMBuilderRef builder,
    Ref regionInstanceRef,
    LLVMValueRef serializedAddressAdjusterLE) {
  auto regionStructLT = structs.getStructStruct(regionKind);
  auto regionInstancePtrLE =
      checkValidReference(FL(), functionState, builder, true, regionRefMT, regionInstanceRef);
  auto serializedAddressAdjusterPtrLE =
      LLVMBuildStructGEP2(
          builder, regionStructLT, regionInstancePtrLE, 4, "serializedAddressAdjusterPtr");
  LLVMBuildStore(builder, serializedAddressAdjusterLE, serializedAddressAdjusterPtrLE);
}

LLVMValueRef Linear::getRegionInstanceSerializedAddressAdjuster(
    FunctionState* functionState,
    LLVMBuilderRef builder,
    Ref regionInstanceRef) {
  auto int64LT = LLVMInt64TypeInContext(globalState->context);
  auto regionStructLT = structs.getStructStruct(regionKind);
  auto regionInstancePtrLE =
      checkValidReference(FL(), functionState, builder, true, regionRefMT, regionInstanceRef);
  auto serializedAddressAdjusterPtrLE =
      LLVMBuildStructGEP2(builder, regionStructLT, regionInstancePtrLE, 4, "serializedAddressAdjusterPtr");
  return LLVMBuildLoad2(
      builder, int64LT, serializedAddressAdjusterPtrLE, "serializedAddressAdjuster");
}

LLVMValueRef Linear::getDestinationPtr(
    FunctionState* functionState,
    LLVMBuilderRef builder,
    Ref regionInstanceRef) {
  auto int8LT = LLVMInt8TypeInContext(globalState->context);
  auto int8PtrLT = LLVMPointerType(int8LT, 0);
  auto bufferBeginPtrLE =
      getRegionInstanceDestinationBufferStartPtr(functionState, builder, regionInstanceRef);
  auto destinationOffsetLE =
      getRegionInstanceDestinationOffset(functionState, builder, regionInstanceRef);
  auto destinationI8PtrLE =
      LLVMBuildGEP2(builder, int8LT, bufferBeginPtrLE, &destinationOffsetLE, 1, "destinationI8Ptr");
  return destinationI8PtrLE;
}

Ref Linear::getDestinationRef(
    FunctionState* functionState,
    LLVMBuilderRef builder,
    Ref regionInstanceRef,
    Reference* desiredRefMT) {
  auto destinationI8PtrLE = getDestinationPtr(functionState, builder, regionInstanceRef);
  auto desiredRefLT = translateType(desiredRefMT);
  auto unadjustedDestinationPtrLE = LLVMBuildBitCast(builder, destinationI8PtrLE, desiredRefLT, "unadjustedDestinationPtr");

  auto adjustedDestinationPtr =
      translateBetweenBufferAddressAndPointer(
          functionState, builder, regionInstanceRef, desiredRefMT, unadjustedDestinationPtrLE, false);
  buildFlare(FL(), globalState, functionState, builder);
  return wrap(this, desiredRefMT, adjustedDestinationPtr);
}

void Linear::bumpDestinationOffset(
    FunctionState* functionState,
    LLVMBuilderRef builder,
    Ref regionInstanceRef,
    LLVMValueRef sizeIntLE) {
  auto destinationOffsetLE =
      getRegionInstanceDestinationOffset(functionState, builder, regionInstanceRef);
  buildFlare(FL(), globalState, functionState, builder, "destinationOffsetLE before: ", destinationOffsetLE);
  destinationOffsetLE =
      LLVMBuildAdd(
          builder, destinationOffsetLE, sizeIntLE, "bumpedDestinationOffset");
  buildFlare(FL(), globalState, functionState, builder, "destinationOffsetLE middle: ", destinationOffsetLE);
  destinationOffsetLE = hexRoundUp(globalState, builder, destinationOffsetLE);
  buildFlare(FL(), globalState, functionState, builder, "destinationOffsetLE after: ", destinationOffsetLE);
  setRegionInstanceDestinationOffset(functionState, builder, regionInstanceRef, destinationOffsetLE);
}
