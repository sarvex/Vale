package dev.vale.simplifying

import dev.vale.{PackageCoordinate, StrI, vassert, vcurious, vfail, vimpl}
import dev.vale.finalast.{IdH, FunctionH, InterfaceDefinitionH, InterfaceHT, KindHT, PrototypeH, RuntimeSizedArrayDefinitionHT, RuntimeSizedArrayHT, StaticSizedArrayDefinitionHT, StaticSizedArrayHT, StructDefinitionH, StructHT}
import dev.vale.typing.ast.PrototypeT
import dev.vale.typing.types._
import dev.vale.finalast._
import dev.vale.typing.types.InterfaceTT
import dev.vale.von.IVonData


case class HamutsBox(var inner: Hamuts) {
  override def equals(obj: Any): Boolean = vcurious(); override def hashCode(): Int = vfail() // Shouldnt hash, is mutable

  def packageCoordToExportNameToFunction: Map[PackageCoordinate, Map[StrI, PrototypeH]] = inner.packageCoordToExportNameToFunction
  def packageCoordToExportNameToKind: Map[PackageCoordinate, Map[StrI, KindHT]] = inner.packageCoordToExportNameToKind
  def packageCoordToExternNameToFunction: Map[PackageCoordinate, Map[StrI, PrototypeH]] = inner.packageCoordToExternNameToFunction
  def packageCoordToExternNameToKind: Map[PackageCoordinate, Map[StrI, KindHT]] = inner.packageCoordToExternNameToKind
  def structTToStructH: Map[StructTT, StructHT] = inner.structTToStructH
  def structTToStructDefH: Map[StructTT, StructDefinitionH] = inner.structTToStructDefH
  def structDefs: Vector[StructDefinitionH] = inner.structDefs
  def interfaceTToInterfaceH: Map[InterfaceTT, InterfaceHT] = inner.interfaceTToInterfaceH
  def interfaceTToInterfaceDefH: Map[InterfaceTT, InterfaceDefinitionH] = inner.interfaceTToInterfaceDefH
  def functionRefs: Map[PrototypeT, FunctionRefH] = inner.functionRefs
  def functionDefs: Map[PrototypeT, FunctionH] = inner.functionDefs
  def staticSizedArrays: Map[StaticSizedArrayTT, StaticSizedArrayDefinitionHT] = inner.staticSizedArrays
  def runtimeSizedArrays: Map[RuntimeSizedArrayTT, RuntimeSizedArrayDefinitionHT] = inner.runtimeSizedArrays

  def forwardDeclareStruct(structTT: StructTT, structRefH: StructHT): Unit = {
    inner = inner.forwardDeclareStruct(structTT, structRefH)
  }

  def addStructOriginatingFromTypingPass(structTT: StructTT, structDefH: StructDefinitionH): Unit = {
    inner = inner.addStructOriginatingFromTypingPass(structTT, structDefH)
  }

  def addStructOriginatingFromHammer(structDefH: StructDefinitionH): Unit = {
    inner = inner.addStructOriginatingFromHammer(structDefH)
  }

  def forwardDeclareInterface(interfaceTT: InterfaceTT, interfaceRefH: InterfaceHT): Unit = {
    inner = inner.forwardDeclareInterface(interfaceTT, interfaceRefH)
  }

  def addInterface(interfaceTT: InterfaceTT, interfaceDefH: InterfaceDefinitionH): Unit = {
    inner = inner.addInterface(interfaceTT, interfaceDefH)
  }

  def addStaticSizedArray(ssaTT: StaticSizedArrayTT, staticSizedArrayDefinitionTH: StaticSizedArrayDefinitionHT): Unit = {
    inner = inner.addStaticSizedArray(ssaTT, staticSizedArrayDefinitionTH)
  }

  def addRuntimeSizedArray(rsaTT: RuntimeSizedArrayTT, runtimeSizedArrayDefinitionTH: RuntimeSizedArrayDefinitionHT): Unit = {
    inner = inner.addRuntimeSizedArray(rsaTT, runtimeSizedArrayDefinitionTH)
  }

  def forwardDeclareFunction(functionRef2: PrototypeT, functionRefH: FunctionRefH): Unit = {
    inner = inner.forwardDeclareFunction(functionRef2, functionRefH)
  }

  def addFunction(functionRef2: PrototypeT, functionDefH: FunctionH): Unit = {
    inner = inner.addFunction(functionRef2, functionDefH)
  }

  def addKindExport(kind: KindHT, packageCoordinate: PackageCoordinate, exportedName: StrI): Unit = {
    inner = inner.addKindExport(kind, packageCoordinate, exportedName)
  }

  def addKindExtern(kind: KindHT, packageCoordinate: PackageCoordinate, exportedName: StrI): Unit = {
    inner = inner.addKindExtern(kind, packageCoordinate, exportedName)
  }

  def addFunctionExport(prototype: PrototypeH, packageCoordinate: PackageCoordinate, exportedName: StrI): Unit = {
    inner = inner.addFunctionExport(prototype, packageCoordinate, exportedName)
  }

  def addFunctionExtern(prototype: PrototypeH, packageCoordinate: PackageCoordinate, exportedName: StrI): Unit = {
    inner = inner.addFunctionExtern(prototype, packageCoordinate, exportedName)
  }

//  def getNameId(readableName: String, packageCoordinate: PackageCoordinate, parts: Vector[IVonData]): Int = {
//    val (newInner, id) = inner.getNameId(readableName, packageCoordinate, parts)
//    inner = newInner
//    id
//  }

  def getStaticSizedArray(staticSizedArrayTH: StaticSizedArrayHT): StaticSizedArrayDefinitionHT = {
    inner.getStaticSizedArray(staticSizedArrayTH)
  }
  def getRuntimeSizedArray(runtimeSizedArrayTH: RuntimeSizedArrayHT): RuntimeSizedArrayDefinitionHT = {
    inner.getRuntimeSizedArray(runtimeSizedArrayTH)
  }
}

case class Hamuts(
    humanNameToFullNameToId: Map[String, Map[String, Int]],
    structTToStructH: Map[StructTT, StructHT],
    structTToStructDefH: Map[StructTT, StructDefinitionH],
    structDefs: Vector[StructDefinitionH],
    staticSizedArrays: Map[StaticSizedArrayTT, StaticSizedArrayDefinitionHT],
    runtimeSizedArrays: Map[RuntimeSizedArrayTT, RuntimeSizedArrayDefinitionHT],
    interfaceTToInterfaceH: Map[InterfaceTT, InterfaceHT],
    interfaceTToInterfaceDefH: Map[InterfaceTT, InterfaceDefinitionH],
    functionRefs: Map[PrototypeT, FunctionRefH],
    functionDefs: Map[PrototypeT, FunctionH],
    packageCoordToExportNameToFunction: Map[PackageCoordinate, Map[StrI, PrototypeH]],
    packageCoordToExportNameToKind: Map[PackageCoordinate, Map[StrI, KindHT]],
    packageCoordToExternNameToFunction: Map[PackageCoordinate, Map[StrI, PrototypeH]],
    packageCoordToExternNameToKind: Map[PackageCoordinate, Map[StrI, KindHT]]) {
  override def equals(obj: Any): Boolean = vcurious(); override def hashCode(): Int = vfail() // Would need a really good reason to hash something this big

  vassert(functionDefs.values.map(_.fullName).toVector.distinct.size == functionDefs.values.size)
  vassert(structDefs.map(_.fullName).distinct.size == structDefs.size)
  vassert(runtimeSizedArrays.values.map(_.name).toVector.distinct.size == runtimeSizedArrays.size)

  def forwardDeclareStruct(structTT: StructTT, structRefH: StructHT): Hamuts = {
    Hamuts(
      humanNameToFullNameToId,
      structTToStructH + (structTT -> structRefH),
      structTToStructDefH,
      structDefs,
      staticSizedArrays,
      runtimeSizedArrays,
      interfaceTToInterfaceH,
      interfaceTToInterfaceDefH,
      functionRefs,
      functionDefs,
      packageCoordToExportNameToFunction,
      packageCoordToExportNameToKind,
      packageCoordToExternNameToFunction,
      packageCoordToExternNameToKind)
  }

  def addStructOriginatingFromTypingPass(structTT: StructTT, structDefH: StructDefinitionH): Hamuts = {
    vassert(structTToStructH.contains(structTT))
    vassert(!structTToStructDefH.contains(structTT))
    Hamuts(
      humanNameToFullNameToId,
      structTToStructH,
      structTToStructDefH + (structTT -> structDefH),
      structDefs :+ structDefH,
      staticSizedArrays,
      runtimeSizedArrays,
      interfaceTToInterfaceH,
      interfaceTToInterfaceDefH,
      functionRefs,
      functionDefs,
      packageCoordToExportNameToFunction,
      packageCoordToExportNameToKind,
      packageCoordToExternNameToFunction,
      packageCoordToExternNameToKind)
  }

  def addStructOriginatingFromHammer(structDefH: StructDefinitionH): Hamuts = {
    vassert(!structDefs.exists(_.fullName == structDefH.fullName))

    Hamuts(
      humanNameToFullNameToId,
      structTToStructH,
      structTToStructDefH,
      structDefs :+ structDefH,
      staticSizedArrays,
      runtimeSizedArrays,
      interfaceTToInterfaceH,
      interfaceTToInterfaceDefH,
      functionRefs,
      functionDefs,
      packageCoordToExportNameToFunction,
      packageCoordToExportNameToKind,
      packageCoordToExternNameToFunction,
      packageCoordToExternNameToKind)
  }

  def forwardDeclareInterface(interfaceTT: InterfaceTT, interfaceRefH: InterfaceHT): Hamuts = {
    Hamuts(
      humanNameToFullNameToId,
      structTToStructH,
      structTToStructDefH,
      structDefs,
      staticSizedArrays,
      runtimeSizedArrays,
      interfaceTToInterfaceH + (interfaceTT -> interfaceRefH),
      interfaceTToInterfaceDefH,
      functionRefs,
      functionDefs,
      packageCoordToExportNameToFunction,
      packageCoordToExportNameToKind,
      packageCoordToExternNameToFunction,
      packageCoordToExternNameToKind)
  }

  def addInterface(interfaceTT: InterfaceTT, interfaceDefH: InterfaceDefinitionH): Hamuts = {
    vassert(interfaceTToInterfaceH.contains(interfaceTT))
    Hamuts(
      humanNameToFullNameToId,
      structTToStructH,
      structTToStructDefH,
      structDefs,
      staticSizedArrays,
      runtimeSizedArrays,
      interfaceTToInterfaceH,
      interfaceTToInterfaceDefH + (interfaceTT -> interfaceDefH),
      functionRefs,
      functionDefs,
      packageCoordToExportNameToFunction,
      packageCoordToExportNameToKind,
      packageCoordToExternNameToFunction,
      packageCoordToExternNameToKind)
  }

  def forwardDeclareFunction(functionRef2: PrototypeT, functionRefH: FunctionRefH): Hamuts = {
    vassert(!functionRefs.contains(functionRef2))

    Hamuts(
      humanNameToFullNameToId,
      structTToStructH,
      structTToStructDefH,
      structDefs,
      staticSizedArrays,
      runtimeSizedArrays,
      interfaceTToInterfaceH,
      interfaceTToInterfaceDefH,
      functionRefs + (functionRef2 -> functionRefH),
      functionDefs,
      packageCoordToExportNameToFunction,
      packageCoordToExportNameToKind,
      packageCoordToExternNameToFunction,
      packageCoordToExternNameToKind)
  }

  def addFunction(functionRef2: PrototypeT, functionDefH: FunctionH): Hamuts = {
    vassert(functionRefs.contains(functionRef2))
    functionDefs.find(_._2.fullName == functionDefH.fullName) match {
      case None =>
      case Some(existing) => {
        vfail("Internal error: Can't add function:\n" + functionRef2 + "\nbecause there's already a function with same hammer name:\b" + existing._1 + "\nHammer name:\n" + functionDefH.fullName)
      }
    }

    Hamuts(
      humanNameToFullNameToId,
      structTToStructH,
      structTToStructDefH,
      structDefs,
      staticSizedArrays,
      runtimeSizedArrays,
      interfaceTToInterfaceH,
      interfaceTToInterfaceDefH,
      functionRefs,
      functionDefs + (functionRef2 -> functionDefH),
      packageCoordToExportNameToFunction,
      packageCoordToExportNameToKind,
      packageCoordToExternNameToFunction,
      packageCoordToExternNameToKind)
  }

  def addKindExport(kind: KindHT, packageCoordinate: PackageCoordinate, exportedName: StrI): Hamuts = {
    val newPackageCoordToExportNameToKind =
      packageCoordToExportNameToKind.get(packageCoordinate) match {
        case None => {
          packageCoordToExportNameToKind + (packageCoordinate -> Map(exportedName -> kind))
        }
        case Some(exportNameToFullName) => {
          exportNameToFullName.get(exportedName) match {
            case None => {
              packageCoordToExportNameToKind + (packageCoordinate -> (exportNameToFullName + (exportedName -> kind)))
            }
            case Some(existingFullName) => {
              vfail("Already exported a `" + exportedName + "` from package `" + packageCoordinate + " : " + existingFullName)
            }
          }
        }
      }

    Hamuts(
      humanNameToFullNameToId,
      structTToStructH,
      structTToStructDefH,
      structDefs,
      staticSizedArrays,
      runtimeSizedArrays,
      interfaceTToInterfaceH,
      interfaceTToInterfaceDefH,
      functionRefs,
      functionDefs,
      packageCoordToExportNameToFunction,
      newPackageCoordToExportNameToKind,
      packageCoordToExternNameToFunction,
      packageCoordToExternNameToKind)
  }

  def addFunctionExport(function: PrototypeH, packageCoordinate: PackageCoordinate, exportedName: StrI): Hamuts = {
    val newPackageCoordToExportNameToFunction =
      packageCoordToExportNameToFunction.get(packageCoordinate) match {
        case None => {
          packageCoordToExportNameToFunction + (packageCoordinate -> Map(exportedName -> function))
        }
        case Some(exportNameToFullName) => {
          exportNameToFullName.get(exportedName) match {
            case None => {
              packageCoordToExportNameToFunction + (packageCoordinate -> (exportNameToFullName + (exportedName -> function)))
            }
            case Some(existingFullName) => {
              vfail("Already exported a `" + exportedName + "` from package `" + packageCoordinate + " : " + existingFullName)
            }
          }
        }
      }

    Hamuts(
      humanNameToFullNameToId,
      structTToStructH,
      structTToStructDefH,
      structDefs,
      staticSizedArrays,
      runtimeSizedArrays,
      interfaceTToInterfaceH,
      interfaceTToInterfaceDefH,
      functionRefs,
      functionDefs,
      newPackageCoordToExportNameToFunction,
      packageCoordToExportNameToKind,
      packageCoordToExternNameToFunction,
      packageCoordToExternNameToKind)
  }

  def addKindExtern(kind: KindHT, packageCoordinate: PackageCoordinate, exportedName: StrI): Hamuts = {
    val newPackageCoordToExternNameToKind =
      packageCoordToExternNameToKind.get(packageCoordinate) match {
        case None => {
          packageCoordToExternNameToKind + (packageCoordinate -> Map(exportedName -> kind))
        }
        case Some(exportNameToFullName) => {
          exportNameToFullName.get(exportedName) match {
            case None => {
              packageCoordToExternNameToKind + (packageCoordinate -> (exportNameToFullName + (exportedName -> kind)))
            }
            case Some(existingFullName) => {
              vfail("Already exported a `" + exportedName + "` from package `" + packageCoordinate + " : " + existingFullName)
            }
          }
        }
      }

    Hamuts(
      humanNameToFullNameToId,
      structTToStructH,
      structTToStructDefH,
      structDefs,
      staticSizedArrays,
      runtimeSizedArrays,
      interfaceTToInterfaceH,
      interfaceTToInterfaceDefH,
      functionRefs,
      functionDefs,
      packageCoordToExportNameToFunction,
      packageCoordToExportNameToKind,
      packageCoordToExternNameToFunction,
      newPackageCoordToExternNameToKind)
  }

  def addFunctionExtern(function: PrototypeH, packageCoordinate: PackageCoordinate, exportedName: StrI): Hamuts = {
    val newPackageCoordToExternNameToFunction =
      packageCoordToExternNameToFunction.get(packageCoordinate) match {
        case None => {
          packageCoordToExternNameToFunction + (packageCoordinate -> Map(exportedName -> function))
        }
        case Some(exportNameToFullName) => {
          exportNameToFullName.get(exportedName) match {
            case None => {
              packageCoordToExternNameToFunction + (packageCoordinate -> (exportNameToFullName + (exportedName -> function)))
            }
            case Some(existingFullName) => {
              vfail("Already exported a `" + exportedName + "` from package `" + packageCoordinate + " : " + existingFullName)
            }
          }
        }
      }

    Hamuts(
      humanNameToFullNameToId,
      structTToStructH,
      structTToStructDefH,
      structDefs,
      staticSizedArrays,
      runtimeSizedArrays,
      interfaceTToInterfaceH,
      interfaceTToInterfaceDefH,
      functionRefs,
      functionDefs,
      packageCoordToExportNameToFunction,
      packageCoordToExportNameToKind,
      newPackageCoordToExternNameToFunction,
      packageCoordToExternNameToKind)
  }

  def addStaticSizedArray(
    ssaTT: StaticSizedArrayTT,
    staticSizedArrayDefinitionHT: StaticSizedArrayDefinitionHT
  ): Hamuts = {
    Hamuts(
      humanNameToFullNameToId,
      structTToStructH,
      structTToStructDefH,
      structDefs,
      staticSizedArrays + (ssaTT -> staticSizedArrayDefinitionHT),
      runtimeSizedArrays,
      interfaceTToInterfaceH,
      interfaceTToInterfaceDefH,
      functionRefs,
      functionDefs,
      packageCoordToExportNameToFunction,
      packageCoordToExportNameToKind,
      packageCoordToExternNameToFunction,
      packageCoordToExternNameToKind)
  }

  def addRuntimeSizedArray(
    rsaTT: RuntimeSizedArrayTT,
    runtimeSizedArrayDefinitionHT: RuntimeSizedArrayDefinitionHT
  ): Hamuts = {
    Hamuts(
      humanNameToFullNameToId,
      structTToStructH,
      structTToStructDefH,
      structDefs,
      staticSizedArrays,
      runtimeSizedArrays + (rsaTT -> runtimeSizedArrayDefinitionHT),
      interfaceTToInterfaceH,
      interfaceTToInterfaceDefH,
      functionRefs,
      functionDefs,
      packageCoordToExportNameToFunction,
      packageCoordToExportNameToKind,
      packageCoordToExternNameToFunction,
      packageCoordToExternNameToKind)
  }

//  // This returns a unique ID for that specific human name.
//  // Two things with two different human names could result in the same ID here.
//  // This ID is meant to be concatenated onto the human name.
//  def getNameId(readableName: String, packageCoordinate: PackageCoordinate, parts: Vector[IVonData]): (Hamuts, Int) = {
//    val namePartsString = IdH.namePartsToString(packageCoordinate, parts)
//    val idByFullNameForHumanName =
//      humanNameToFullNameToId.get(readableName) match {
//        case None => Map[String, Int]()
//        case Some(x) => x
//      }
//    val id =
//      idByFullNameForHumanName.get(namePartsString) match {
//        case None => idByFullNameForHumanName.size
//        case Some(i) => i
//      }
//    val idByFullNameForHumanNameNew = idByFullNameForHumanName + (namePartsString -> id)
//    val idByFullNameByHumanNameNew = humanNameToFullNameToId + (readableName -> idByFullNameForHumanNameNew)
//    val newHamuts =
//      Hamuts(
//        idByFullNameByHumanNameNew,
//        structTToStructH,
//        structTToStructDefH,
//        structDefs,
//        staticSizedArrays,
//        runtimeSizedArrays,
//        interfaceTToInterfaceH,
//        interfaceTToInterfaceDefH,
//        functionRefs,
//        functionDefs,
//        packageCoordToExportNameToFunction,
//        packageCoordToExportNameToKind,
//        packageCoordToExternNameToFunction,
//        packageCoordToExternNameToKind)
//    (newHamuts, id)
//  }

  def getStaticSizedArray(staticSizedArrayHT: StaticSizedArrayHT): StaticSizedArrayDefinitionHT = {
    staticSizedArrays.values.find(_.kind == staticSizedArrayHT).get
  }
  def getRuntimeSizedArray(runtimeSizedArrayTH: RuntimeSizedArrayHT): RuntimeSizedArrayDefinitionHT = {
    runtimeSizedArrays.values.find(_.kind == runtimeSizedArrayTH).get
  }
}
