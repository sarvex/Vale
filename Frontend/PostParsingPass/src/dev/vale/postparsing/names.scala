package dev.vale.postparsing

import dev.vale.{CodeLocationS, IInterning, Interner, PackageCoordinate, RangeS, StrI, vassert, vcheck, vimpl, vpass}

trait INameS extends IInterning
trait IImpreciseNameS extends IInterning
sealed trait IVarNameS extends INameS
trait IFunctionDeclarationNameS extends INameS {
  def packageCoordinate: PackageCoordinate
  def getImpreciseName(interner: Interner): IImpreciseNameS
}
trait IImplDeclarationNameS extends INameS {
  def packageCoordinate: PackageCoordinate
}
trait ICitizenDeclarationNameS extends INameS {
  def range: RangeS
  def packageCoordinate: PackageCoordinate
  def getImpreciseName(interner: Interner): IImpreciseNameS
}
//case class FreeDeclarationNameS(codeLocationS: CodeLocationS) extends IFunctionDeclarationNameS {
//
//  override def packageCoordinate: PackageCoordinate = codeLocationS.file.packageCoordinate
//  override def getImpreciseName(interner: Interner): IImpreciseNameS = interner.intern(FreeImpreciseNameS())
//}
//case class FreeImpreciseNameS() extends IImpreciseNameS {
//
//}
case class LambdaDeclarationNameS(
//  parentName: INameS,
  codeLocation: CodeLocationS
) extends IFunctionDeclarationNameS {

  override def packageCoordinate: PackageCoordinate = codeLocation.file.packageCoordinate
  override def getImpreciseName(interner: Interner): LambdaImpreciseNameS = interner.intern(LambdaImpreciseNameS())
}
case class LambdaImpreciseNameS() extends IImpreciseNameS {
}
case class PlaceholderImpreciseNameS(index: Int) extends IImpreciseNameS {
}
case class FunctionNameS(name: StrI, codeLocation: CodeLocationS) extends IFunctionDeclarationNameS {
  override def packageCoordinate: PackageCoordinate = codeLocation.file.packageCoordinate
  override def getImpreciseName(interner: Interner): IImpreciseNameS = interner.intern(CodeNameS(name))
}
//case class AbstractVirtualDropFunctionDeclarationNameS(interfaceName: TopLevelCitizenDeclarationNameS) extends IFunctionDeclarationNameS {
//  override def packageCoordinate: PackageCoordinate = interfaceName.packageCoord
//  override def getImpreciseName(interner: Interner): IImpreciseNameS = interner.intern(CodeNameS(Scout.VIRTUAL_DROP_FUNCTION_NAME))
//}
//case class OverrideVirtualDropFunctionDeclarationNameS(implName: IImplDeclarationNameS) extends IFunctionDeclarationNameS {
//  override def packageCoordinate: PackageCoordinate = implName.packageCoord
//  override def getImpreciseName(interner: Interner): IImpreciseNameS = interner.intern(CodeNameS(Scout.VIRTUAL_DROP_FUNCTION_NAME))
//}
case class ForwarderFunctionDeclarationNameS(inner: IFunctionDeclarationNameS, index: Int) extends IFunctionDeclarationNameS {
  override def packageCoordinate: PackageCoordinate = inner.packageCoordinate
  override def getImpreciseName(interner: Interner): IImpreciseNameS = inner.getImpreciseName(interner)
}
sealed trait TopLevelCitizenDeclarationNameS extends ICitizenDeclarationNameS {
  def name: StrI
  def range: RangeS
  override def packageCoordinate: PackageCoordinate = range.file.packageCoordinate
  override def getImpreciseName(interner: Interner): IImpreciseNameS = interner.intern(CodeNameS(name))
}
object TopLevelCitizenDeclarationNameS {
  def unapply(n: TopLevelCitizenDeclarationNameS): Option[(StrI, RangeS)] = {
    Some((n.name, n.range))
  }
}
sealed trait IStructDeclarationNameS extends ICitizenDeclarationNameS
case class TopLevelStructDeclarationNameS(name: StrI, range: RangeS) extends IStructDeclarationNameS with TopLevelCitizenDeclarationNameS {
}
sealed trait IInterfaceDeclarationNameS extends ICitizenDeclarationNameS
case class TopLevelInterfaceDeclarationNameS(name: StrI, range: RangeS) extends IInterfaceDeclarationNameS with TopLevelCitizenDeclarationNameS {
}
case class LambdaStructDeclarationNameS(lambdaName: LambdaDeclarationNameS) extends INameS {
  def getImpreciseName(interner: Interner): LambdaStructImpreciseNameS = interner.intern(LambdaStructImpreciseNameS(lambdaName.getImpreciseName(interner)))
}
case class LambdaStructImpreciseNameS(lambdaName: LambdaImpreciseNameS) extends IImpreciseNameS {  }
case class ImplDeclarationNameS(codeLocation: CodeLocationS) extends IImplDeclarationNameS {
  override def packageCoordinate: PackageCoordinate = codeLocation.file.packageCoordinate
}
case class AnonymousSubstructImplDeclarationNameS(interface: TopLevelInterfaceDeclarationNameS) extends IImplDeclarationNameS {
  override def packageCoordinate: PackageCoordinate = interface.packageCoordinate
}
case class ExportAsNameS(codeLocation: CodeLocationS) extends INameS {  }
case class LetNameS(codeLocation: CodeLocationS) extends INameS {  }
//case class UnnamedLocalNameS(codeLocation: CodeLocationS) extends IVarNameS {  override def equals(obj: Any): Boolean = vcurious(); override def hashCode(): Int = vcurious() }
case class ClosureParamNameS(codeLocation: CodeLocationS) extends IVarNameS {  }
case class ClosureParamImpreciseNameS() extends IImpreciseNameS {  }
// All prototypes can be looked up via this name.
case class PrototypeNameS() extends IImpreciseNameS {  }
case class MagicParamNameS(codeLocation: CodeLocationS) extends IVarNameS {  }
case class AnonymousSubstructTemplateNameS(interfaceName: TopLevelInterfaceDeclarationNameS) extends IStructDeclarationNameS {
  vpass()
  override def packageCoordinate: PackageCoordinate = interfaceName.packageCoordinate
  override def getImpreciseName(interner: Interner): IImpreciseNameS = interner.intern(AnonymousSubstructTemplateImpreciseNameS(interfaceName.getImpreciseName(interner)))
  override def range: RangeS = interfaceName.range
}
case class AnonymousSubstructTemplateImpreciseNameS(interfaceImpreciseName: IImpreciseNameS) extends IImpreciseNameS {

}
case class AnonymousSubstructConstructorTemplateImpreciseNameS(interfaceImpreciseName: IImpreciseNameS) extends IImpreciseNameS {

}
case class AnonymousSubstructMemberNameS(index: Int) extends IVarNameS {  }
case class CodeVarNameS(name: StrI) extends IVarNameS {
  vcheck(name.str != "set", "Can't name a variable 'set'")
  vcheck(name.str != "mut", "Can't name a variable 'mut'")
}
case class ConstructingMemberNameS(name: StrI) extends IVarNameS {  }
case class IterableNameS(range: RangeS) extends IVarNameS with IImpreciseNameS {  }
case class IteratorNameS(range: RangeS) extends IVarNameS with IImpreciseNameS {  }
case class IterationOptionNameS(range: RangeS) extends IVarNameS with IImpreciseNameS {  }
case class WhileCondResultNameS(range: RangeS) extends IVarNameS {  }
case class RuneNameS(rune: IRuneS) extends INameS with IImpreciseNameS {  }

// We differentiate rune names from regular names, we scout out what's actually
// a rune so we can inform the typingpass. The typingpass wants to know so it can know
// how to handle this thing; if it's a name, we expect it to exist in the
// environment already, but if it's a rune we can assign something into it.
// Also, we might refer to a rune that was defined in our container's container's
// container, so asking "is this thing here a rune" involves looking at all our
// containers. That's much easier for the scout, so thats a nice bonus.
// We have all these subclasses instead of a string so we don't have to have
// prefixes and names like __implicit_0, __paramRune_0, etc.
// This extends INameS so we can use it as a lookup key in Compiler's environments.
trait IRuneS
case class CodeRuneS(name: StrI) extends IRuneS {
  vpass()
}
case class ImplDropCoordRuneS() extends IRuneS
case class ImplDropVoidRuneS() extends IRuneS
case class ImplicitRuneS(lid: LocationInDenizen) extends IRuneS {
  vpass()
}
case class ReachablePrototypeRuneS(num: Int) extends IRuneS
case class FreeOverrideStructTemplateRuneS() extends IRuneS
case class FreeOverrideStructRuneS() extends IRuneS
case class FreeOverrideInterfaceRuneS() extends IRuneS
case class LetImplicitRuneS(lid: LocationInDenizen) extends IRuneS {  }
case class MagicParamRuneS(lid: LocationInDenizen) extends IRuneS {  }
case class MemberRuneS(memberIndex: Int) extends IRuneS

case class ImplicitCoercionOwnershipRuneS(range: RangeS, originalCoordRune: IRuneS) extends IRuneS {  }
case class ImplicitCoercionKindRuneS(range: RangeS, originalCoordRune: IRuneS) extends IRuneS {  }
case class ImplicitCoercionTemplateRuneS(range: RangeS, originalKindRune: IRuneS) extends IRuneS {  }

// Used to type the templex handed to the size part of the static sized array expressions
case class ArraySizeImplicitRuneS() extends IRuneS {  }
// Used to type the templex handed to the mutability part of the static sized array expressions
case class ArrayMutabilityImplicitRuneS() extends IRuneS {  }
// Used to type the templex handed to the variability part of the static sized array expressions
case class ArrayVariabilityImplicitRuneS() extends IRuneS {  }
case class ReturnRuneS() extends IRuneS
case class StructNameRuneS(structName: ICitizenDeclarationNameS) extends IRuneS
case class InterfaceNameRuneS(interfaceName: ICitizenDeclarationNameS) extends IRuneS
// Vale has no notion of Self, it's just a convenient name for a first parameter.
case class SelfRuneS() extends IRuneS {  }
case class SelfOwnershipRuneS() extends IRuneS {  }
case class SelfKindRuneS() extends IRuneS {  }
case class SelfKindTemplateRuneS() extends IRuneS {  }
case class SelfCoordRuneS() extends IRuneS {  }
case class MacroVoidKindRuneS() extends IRuneS {  }
case class MacroVoidCoordRuneS() extends IRuneS {  }
case class MacroSelfKindRuneS() extends IRuneS {  }
case class MacroSelfKindTemplateRuneS() extends IRuneS {  }
case class MacroSelfCoordRuneS() extends IRuneS {  }
case class CodeNameS(name: StrI) extends IImpreciseNameS {
  vpass()
  vassert(name.str != "_")
}
// When we're calling a function, we're addressing an overload set, not a specific function.
// If we want a specific function, we use TopLevelDeclarationNameS.
case class GlobalFunctionFamilyNameS(name: String) extends INameS

// These are only made by the typingpass
case class ArgumentRuneS(argIndex: Int) extends IRuneS {  }
case class PatternInputRuneS(codeLoc: CodeLocationS) extends IRuneS {  }
case class ExplicitTemplateArgRuneS(index: Int) extends IRuneS {  }
case class AnonymousSubstructParentInterfaceTemplateRuneS() extends IRuneS {  }
case class AnonymousSubstructParentInterfaceKindRuneS() extends IRuneS {  }
case class AnonymousSubstructParentInterfaceCoordRuneS() extends IRuneS {  }
case class AnonymousSubstructTemplateRuneS() extends IRuneS {  }
case class AnonymousSubstructKindRuneS() extends IRuneS {  }
case class AnonymousSubstructCoordRuneS() extends IRuneS {  }
case class AnonymousSubstructVoidKindRuneS() extends IRuneS {  }
case class AnonymousSubstructVoidCoordRuneS() extends IRuneS {  }
case class AnonymousSubstructMemberRuneS(interface: TopLevelInterfaceDeclarationNameS, method: IFunctionDeclarationNameS) extends IRuneS {  }
case class AnonymousSubstructMethodSelfBorrowCoordRuneS(interface: TopLevelInterfaceDeclarationNameS, method: IFunctionDeclarationNameS) extends IRuneS { }
case class AnonymousSubstructMethodSelfOwnCoordRuneS(interface: TopLevelInterfaceDeclarationNameS, method: IFunctionDeclarationNameS) extends IRuneS { }
case class AnonymousSubstructDropBoundPrototypeRuneS(interface: TopLevelInterfaceDeclarationNameS, method: IFunctionDeclarationNameS) extends IRuneS { }
case class AnonymousSubstructDropBoundParamsListRuneS(interface: TopLevelInterfaceDeclarationNameS, method: IFunctionDeclarationNameS) extends IRuneS { }
case class AnonymousSubstructFunctionBoundPrototypeRuneS(interface: TopLevelInterfaceDeclarationNameS, method: IFunctionDeclarationNameS) extends IRuneS { }
case class AnonymousSubstructFunctionBoundParamsListRuneS(interface: TopLevelInterfaceDeclarationNameS, method: IFunctionDeclarationNameS) extends IRuneS { }
//case class AnonymousSubstructFunctionInterfaceKindRune(interface: TopLevelInterfaceDeclarationNameS, method: IFunctionDeclarationNameS) extends IRuneS { }
//case class AnonymousSubstructFunctionInterfaceOwnershipRune(interface: TopLevelInterfaceDeclarationNameS, method: IFunctionDeclarationNameS) extends IRuneS { }
case class AnonymousSubstructFunctionInterfaceTemplateRune(interface: TopLevelInterfaceDeclarationNameS, method: IFunctionDeclarationNameS) extends IRuneS { }
case class AnonymousSubstructFunctionInterfaceKindRune(interface: TopLevelInterfaceDeclarationNameS, method: IFunctionDeclarationNameS) extends IRuneS { }
case class AnonymousSubstructMethodInheritedRuneS(interface: TopLevelInterfaceDeclarationNameS, method: IFunctionDeclarationNameS, inner: IRuneS) extends IRuneS {
  this match {
    case AnonymousSubstructMethodInheritedRuneS(TopLevelInterfaceDeclarationNameS(StrI("Bork"),_),FunctionNameS(StrI("bork"),_),ImplicitRuneS(LocationInDenizen(Vector(2, 1, 1, 2, 1, 1)))) => {
      vpass()
    }
    case _ =>
  }
}
case class FunctorPrototypeRuneNameS() extends IRuneS
case class FunctorParamRuneNameS(index: Int) extends IRuneS
case class FunctorReturnRuneNameS() extends IRuneS
// Vale has no notion of Self, it's just a convenient name for a first parameter.
case class SelfNameS() extends IVarNameS with IImpreciseNameS {  }
// A miscellaneous name, for when a name doesn't really make sense, like it's the only entry in the environment or something.
case class ArbitraryNameS() extends INameS with IImpreciseNameS

case class DispatcherRuneFromImplS(innerRune: IRuneS) extends IRuneS
case class CaseRuneFromImplS(innerRune: IRuneS) extends IRuneS

// Only made by typingpass, see if we can take these out
case class ConstructorNameS(tlcd: ICitizenDeclarationNameS) extends IFunctionDeclarationNameS {
  override def packageCoordinate: PackageCoordinate = tlcd.range.begin.file.packageCoordinate
  override def getImpreciseName(interner: Interner): IImpreciseNameS = tlcd.getImpreciseName(interner)
}
case class ImmConcreteDestructorNameS(packageCoordinate: PackageCoordinate) extends IFunctionDeclarationNameS {
  override def getImpreciseName(interner: Interner): IImpreciseNameS = vimpl()
}
case class ImmInterfaceDestructorNameS(packageCoordinate: PackageCoordinate) extends IFunctionDeclarationNameS {
  override def getImpreciseName(interner: Interner): IImpreciseNameS = vimpl()
}

case class ImplImpreciseNameS(subCitizenImpreciseName: IImpreciseNameS, superInterfaceImpreciseName: IImpreciseNameS) extends IImpreciseNameS { }
case class ImplSubCitizenImpreciseNameS(subCitizenImpreciseName: IImpreciseNameS) extends IImpreciseNameS { }
case class ImplSuperInterfaceImpreciseNameS(superInterfaceImpreciseName: IImpreciseNameS) extends IImpreciseNameS { }

// See NSIDN for why we need this virtual name
//case class VirtualFreeImpreciseNameS() extends IImpreciseNameS { }
//case class AbstractVirtualFreeDeclarationNameS(codeLoc: CodeLocationS) extends IFunctionDeclarationNameS {
//  override def getImpreciseName(interner: Interner): IImpreciseNameS = interner.intern(VirtualFreeImpreciseNameS())
//  override def packageCoordinate: PackageCoordinate = codeLoc.file.packageCoord
//}
//case class OverrideVirtualFreeDeclarationNameS(codeLoc: CodeLocationS) extends IFunctionDeclarationNameS {
//  override def getImpreciseName(interner: Interner): IImpreciseNameS = interner.intern(VirtualFreeImpreciseNameS())
//  override def packageCoordinate: PackageCoordinate = codeLoc.file.packageCoord
//}
