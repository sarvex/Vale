#ifndef FUNCTION_EXPRESSIONS_SHARED_ELEMENTS_H_
#define FUNCTION_EXPRESSIONS_SHARED_ELEMENTS_H_

#include <llvm-c/Core.h>

#include <unordered_map>
#include <functional>

#include "../../../metal/ast.h"
#include "../../../metal/instructions.h"
#include "../../../globalstate.h"
#include "../../function.h"

//Ref loadElementWithUpgrade(
//    GlobalState* globalState,
//    FunctionState* functionState,
//    BlockState* blockState,
//    LLVMBuilderRef builder,
//    Reference* arrayRefM,
//    Reference* elementRefM,
//    Ref sizeLE,
//    LLVMValueRef arrayPtrLE,
//    Mutability mutability,
//    Ref indexLE,
//    Reference* resultRefM);

void initializeElementAndIncrementSize(
    GlobalState* globalState,
    FunctionState* functionState,
    LLVMBuilderRef builder,
    Location location,
    Reference* elementRefM,
    LLVMValueRef sizePtrLE,
    LLVMValueRef elemsPtrLE,
    Ref indexRef,
    Ref sourceRef);

void initializeElementWithoutIncrementSize(
    GlobalState* globalState,
    FunctionState* functionState,
    LLVMBuilderRef builder,
    Location location,
    Reference* elementRefM,
    Ref sizeRef,
    LLVMValueRef elemsPtrLE,
    Ref indexRef,
    Ref sourceRef);

Ref swapElement(
    GlobalState* globalState,
    FunctionState* functionState,
    LLVMBuilderRef builder,
    Location location,
    Reference* elementRefM,
    Ref sizeLE,
    LLVMValueRef arrayPtrLE,
    Ref indexLE,
    Ref sourceLE);



LoadResult loadElement(
    GlobalState* globalState,
    FunctionState* functionState,
    LLVMBuilderRef builder,
    LLVMValueRef elemsPtrLE,
    Reference* elementRefM,
    Ref sizeRef,
    Ref indexRef);

void intRangeLoop(
    GlobalState* globalState,
    FunctionState* functionState,
    LLVMBuilderRef builder,
    LLVMValueRef sizeRef,
    std::function<void(LLVMValueRef, LLVMBuilderRef)> iterationBuilder);

void intRangeLoopV(
    GlobalState* globalState,
    FunctionState* functionState,
    LLVMBuilderRef builder,
    Ref sizeRef,
    std::function<void(Ref, LLVMBuilderRef)> iterationBuilder);

void intRangeLoopReverseV(
    GlobalState* globalState,
    FunctionState* functionState,
    LLVMBuilderRef builder,
    Int* innt,
    Ref sizeRef,
    std::function<void(Ref, LLVMBuilderRef)> iterationBuilder);

LLVMValueRef getStaticSizedArrayContentsPtr(
    LLVMBuilderRef builder,
    WrapperPtrLE staticSizedArrayWrapperPtrLE);

LLVMValueRef getRuntimeSizedArrayContentsPtr(
    LLVMBuilderRef builder,
    bool capacityExists,
    WrapperPtrLE arrayWrapperPtrLE);

LLVMValueRef getRuntimeSizedArrayLengthPtr(
    GlobalState* globalState,
    LLVMBuilderRef builder,
    WrapperPtrLE runtimeSizedArrayWrapperPtrLE);

LLVMValueRef getRuntimeSizedArrayCapacityPtr(
    GlobalState* globalState,
    LLVMBuilderRef builder,
    WrapperPtrLE runtimeSizedArrayWrapperPtrLE);

void decrementRSASize(
    GlobalState* globalState, FunctionState *functionState, KindStructs* kindStructs, LLVMBuilderRef builder, Reference *rsaRefMT, WrapperPtrLE rsaWrapperPtrLE);

// Returns a ptr to the address it just wrote to
void storeInnerArrayMember(
    GlobalState* globalState,
    FunctionState* functionState,
    LLVMBuilderRef builder,
    LLVMTypeRef elementLT,
    LLVMValueRef elemsPtrLE,
    LLVMValueRef indexLE,
    LLVMValueRef sourceLE);

#endif
