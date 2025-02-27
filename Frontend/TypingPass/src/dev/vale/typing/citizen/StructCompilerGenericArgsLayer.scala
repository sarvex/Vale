package dev.vale.typing.citizen

import dev.vale.highertyping.FunctionA
import dev.vale.postparsing.{GenericParameterS, IFunctionDeclarationNameS, ITemplataType, SealedS}
import dev.vale.postparsing.rules.{IRulexSR, RuneUsage}
import dev.vale.typing.env.IEnvironment
import dev.vale.typing.{CompilerOutputs, InferCompiler, InitialKnown, TypingPassOptions}
import dev.vale.typing.function.FunctionCompiler
import dev.vale.typing.names.{AnonymousSubstructNameT, IdT, IInterfaceNameT, IInterfaceTemplateNameT, IStructNameT, IStructTemplateNameT, NameTranslator, PackageTopLevelNameT, RuneNameT, StructTemplateNameT}
import dev.vale.typing.templata._
import dev.vale.typing.types._
import dev.vale.{Accumulator, Err, Interner, Keywords, Ok, Profiler, RangeS, typing, vassert, vassertSome, vcurious, vfail, vimpl, vwat}
import dev.vale.highertyping._
import dev.vale.solver.{CompleteSolve, FailedSolve, IncompleteSolve}
import dev.vale.typing.types._
import dev.vale.typing.templata._
import dev.vale.typing._
import dev.vale.typing.ast._
import dev.vale.typing.env._

import scala.collection.immutable.{List, Set}

class StructCompilerGenericArgsLayer(
    opts: TypingPassOptions,
    interner: Interner,
    keywords: Keywords,
    nameTranslator: NameTranslator,
    templataCompiler: TemplataCompiler,
    inferCompiler: InferCompiler,
    delegate: IStructCompilerDelegate) {
  val core = new StructCompilerCore(opts, interner, keywords, nameTranslator, delegate)

  def resolveStruct(
    coutputs: CompilerOutputs,
    originalCallingEnv: IEnvironment, // See CSSNCE
    callRange: List[RangeS],
    structTemplata: StructDefinitionTemplata,
    templateArgs: Vector[ITemplata[ITemplataType]]):
  IResolveOutcome[StructTT] = {
    Profiler.frame(() => {
      val StructDefinitionTemplata(declaringEnv, structA) = structTemplata
      val structTemplateName = nameTranslator.translateStructName(structA.name)

      // We no longer assume this:
      //   vassert(templateArgs.size == structA.genericParameters.size)
      // because we have default generic arguments now.

      val initialKnowns =
        structA.genericParameters.zip(templateArgs).map({ case (genericParam, templateArg) =>
          InitialKnown(RuneUsage(callRange.head, genericParam.rune.rune), templateArg)
        })

      val callSiteRules =
        TemplataCompiler.assembleCallSiteRules(
          structA.headerRules.toVector, structA.genericParameters, templateArgs.size)

      // Check if its a valid use of this template
      val envs = InferEnv(originalCallingEnv, callRange, declaringEnv)
      val solver =
        inferCompiler.makeSolver(
          envs,
          coutputs,
          callSiteRules,
          structA.headerRuneToType,
          callRange,
          initialKnowns,
          Vector())
      inferCompiler.continue(envs, coutputs, solver) match {
        case Ok(()) =>
        case Err(x) => return ResolveFailure(callRange, x)
      }
      val CompleteCompilerSolve(_, inferences, runeToFunctionBound, Vector()) =
        inferCompiler.interpretResults(
          envs,
          coutputs,
          callRange,
          structA.headerRuneToType,
          callSiteRules,
          true,
          false,
          Vector(),
          solver) match {
          case ccs @ CompleteCompilerSolve(_, _, _, _) => ccs
          case x : IIncompleteOrFailedCompilerSolve => return ResolveFailure(callRange, x)
        }

      // We can't just make a StructTT with the args they gave us, because they may have been
      // missing some, in which case we had to run some default rules.
      // Let's use the inferences to make one.

      val finalGenericArgs = structA.genericParameters.map(_.rune.rune).map(inferences)
      val structName = structTemplateName.makeStructName(interner, finalGenericArgs)
      val fullName = declaringEnv.fullName.addStep(structName)

      coutputs.addInstantiationBounds(fullName, runeToFunctionBound)
      val structTT = interner.intern(StructTT(fullName))

      ResolveSuccess(structTT)
    })
  }

  // See SFWPRL for how this is different from resolveInterface.
  def predictInterface(
    coutputs: CompilerOutputs,
    originalCallingEnv: IEnvironment, // See CSSNCE
    callRange: List[RangeS],
    interfaceTemplata: InterfaceDefinitionTemplata,
    templateArgs: Vector[ITemplata[ITemplataType]]):
  (InterfaceTT) = {
    Profiler.frame(() => {
      val InterfaceDefinitionTemplata(declaringEnv, interfaceA) = interfaceTemplata
      val interfaceTemplateName = nameTranslator.translateInterfaceName(interfaceA.name)

      // We no longer assume this:
      //   vassert(templateArgs.size == structA.genericParameters.size)
      // because we have default generic arguments now.

      val initialKnowns =
        interfaceA.genericParameters.zip(templateArgs).map({ case (genericParam, templateArg) =>
          InitialKnown(RuneUsage(callRange.head, genericParam.rune.rune), templateArg)
        })

      val callSiteRules =
        TemplataCompiler.assemblePredictRules(
          interfaceA.genericParameters, templateArgs.size)
      val runesForPrediction =
        (interfaceA.genericParameters.map(_.rune.rune) ++
          callSiteRules.flatMap(_.runeUsages.map(_.rune))).toSet
      val runeToTypeForPrediction =
        runesForPrediction.toVector.map(r => r -> interfaceA.runeToType(r)).toMap

      // This *doesnt* check to make sure it's a valid use of the template. Its purpose is really
      // just to populate any generic parameter default values.
      val CompleteCompilerSolve(_, inferences, _, Vector()) =
        inferCompiler.solveExpectComplete(
          InferEnv(originalCallingEnv, callRange, declaringEnv),
          coutputs,
          callSiteRules,
          runeToTypeForPrediction,
          callRange,
          initialKnowns,
          Vector(),
          // False because we're just predicting, see STCMBDP.
          false,
          false,
          Vector())

      // We can't just make a StructTT with the args they gave us, because they may have been
      // missing some, in which case we had to run some default rules.
      // Let's use the inferences to make one.

      val finalGenericArgs = interfaceA.genericParameters.map(_.rune.rune).map(inferences)
      val interfaceName = interfaceTemplateName.makeInterfaceName(interner, finalGenericArgs)
      val fullName = declaringEnv.fullName.addStep(interfaceName)

      // Usually when we make an InterfaceTT we put the instantiation bounds into the coutputs,
      // but we unfortunately can't here because we're just predicting an interface; we'll
      // try to resolve it later and then put the bounds in. Hopefully this InterfaceTT doesn't
      // escape into the wild.
      val interfaceTT = interner.intern(InterfaceTT(fullName))
      interfaceTT
    })
  }

  // See SFWPRL for how this is different from resolveStruct.
  def predictStruct(
    coutputs: CompilerOutputs,
    originalCallingEnv: IEnvironment, // See CSSNCE
    callRange: List[RangeS],
    structTemplata: StructDefinitionTemplata,
    templateArgs: Vector[ITemplata[ITemplataType]]):
  (StructTT) = {
    Profiler.frame(() => {
      val StructDefinitionTemplata(declaringEnv, structA) = structTemplata
      val structTemplateName = nameTranslator.translateStructName(structA.name)

      // We no longer assume this:
      //   vassert(templateArgs.size == structA.genericParameters.size)
      // because we have default generic arguments now.

      val initialKnowns =
        structA.genericParameters.zip(templateArgs).map({ case (genericParam, templateArg) =>
          InitialKnown(RuneUsage(vassertSome(callRange.headOption), genericParam.rune.rune), templateArg)
        })

      val callSiteRules =
        TemplataCompiler.assemblePredictRules(
          structA.genericParameters, templateArgs.size)
      val runesForPrediction =
        (structA.genericParameters.map(_.rune.rune) ++
          callSiteRules.flatMap(_.runeUsages.map(_.rune))).toSet
      val runeToTypeForPrediction =
        runesForPrediction.toVector.map(r => r -> structA.headerRuneToType(r)).toMap

      // This *doesnt* check to make sure it's a valid use of the template. Its purpose is really
      // just to populate any generic parameter default values.
      val CompleteCompilerSolve(_, inferences, _, Vector()) =
        inferCompiler.solveExpectComplete(
          InferEnv(originalCallingEnv, callRange, declaringEnv),
          coutputs,
          callSiteRules,
          runeToTypeForPrediction,
          callRange,
          initialKnowns,
          Vector(),
          // False because we're just predicting, see STCMBDP.
          false,
          false,
          Vector())

      // We can't just make a StructTT with the args they gave us, because they may have been
      // missing some, in which case we had to run some default rules.
      // Let's use the inferences to make one.

      val finalGenericArgs = structA.genericParameters.map(_.rune.rune).map(inferences)
      val structName = structTemplateName.makeStructName(interner, finalGenericArgs)
      val fullName = declaringEnv.fullName.addStep(structName)

      // Usually when we make an InterfaceTT we put the instantiation bounds into the coutputs,
      // but we unfortunately can't here because we're just predicting an interface; we'll
      // try to resolve it later and then put the bounds in. Hopefully this InterfaceTT doesn't
      // escape into the wild.
      val structTT = interner.intern(StructTT(fullName))
      structTT
    })
  }

  def resolveInterface(
    coutputs: CompilerOutputs,
    originalCallingEnv: IEnvironment, // See CSSNCE
    callRange: List[RangeS],
    interfaceTemplata: InterfaceDefinitionTemplata,
    templateArgs: Vector[ITemplata[ITemplataType]]):
  IResolveOutcome[InterfaceTT] = {
    Profiler.frame(() => {
      val InterfaceDefinitionTemplata(declaringEnv, interfaceA) = interfaceTemplata
      val interfaceTemplateName = nameTranslator.translateInterfaceName(interfaceA.name)

      // We no longer assume this:
      //   vassert(templateArgs.size == structA.genericParameters.size)
      // because we have default generic arguments now.

      val initialKnowns =
        interfaceA.genericParameters.zip(templateArgs).map({ case (genericParam, templateArg) =>
          InitialKnown(RuneUsage(callRange.head, genericParam.rune.rune), templateArg)
        })

      val callSiteRules =
        TemplataCompiler.assembleCallSiteRules(
          interfaceA.rules.toVector, interfaceA.genericParameters, templateArgs.size)

      // This checks to make sure it's a valid use of this template.
      val envs = InferEnv(originalCallingEnv, callRange, declaringEnv)
      val solver =
        inferCompiler.makeSolver(
          envs,
          coutputs,
          callSiteRules,
          interfaceA.runeToType,
          callRange,
          initialKnowns,
          Vector())
      inferCompiler.continue(envs, coutputs, solver) match {
        case Ok(()) =>
        case Err(x) => return ResolveFailure(callRange, x)
      }
      val CompleteCompilerSolve(_, inferences, runeToFunctionBound, Vector()) =
        inferCompiler.interpretResults(
          envs,
          coutputs,
          callRange,
          interfaceA.runeToType,
          callSiteRules,
          true,
          false,
          Vector(),
          solver) match {
          case ccs @ CompleteCompilerSolve(_, _, _, _) => ccs
          case x : IIncompleteOrFailedCompilerSolve => return ResolveFailure(callRange, x)
        }

      // We can't just make a StructTT with the args they gave us, because they may have been
      // missing some, in which case we had to run some default rules.
      // Let's use the inferences to make one.

      val finalGenericArgs = interfaceA.genericParameters.map(_.rune.rune).map(inferences)
      val interfaceName = interfaceTemplateName.makeInterfaceName(interner, finalGenericArgs)
      val fullName = declaringEnv.fullName.addStep(interfaceName)

      coutputs.addInstantiationBounds(fullName, runeToFunctionBound)
      val interfaceTT = interner.intern(InterfaceTT(fullName))

      ResolveSuccess(interfaceTT)
    })
  }

  def compileStruct(
    coutputs: CompilerOutputs,
    parentRanges: List[RangeS],
    structTemplata: StructDefinitionTemplata):
  Unit = {
    Profiler.frame(() => {
      val StructDefinitionTemplata(declaringEnv, structA) = structTemplata
      val structTemplateName = nameTranslator.translateStructName(structA.name)
      val structTemplateFullName = declaringEnv.fullName.addStep(structTemplateName)

      // We declare the struct's outer environment in the precompile stage instead of here because
      // of MDATOEF.
      val outerEnv = coutputs.getOuterEnvForType(parentRanges, structTemplateFullName)

      val allRulesS = structA.headerRules ++ structA.memberRules
      val allRuneToType = structA.headerRuneToType ++ structA.membersRuneToType
      val definitionRules = allRulesS.filter(InferCompiler.includeRuleInDefinitionSolve)

      val envs = InferEnv(outerEnv, List(structA.range), outerEnv)
      val solver =
        inferCompiler.makeSolver(
          envs, coutputs, definitionRules, allRuneToType, structA.range :: parentRanges, Vector(), Vector())
      // Incrementally solve and add placeholders, see IRAGP.
      inferCompiler.incrementallySolve(
        envs, coutputs, solver,
        // Each step happens after the solver has done all it possibly can. Sometimes this can lead
        // to races, see RRBFS.
        (solver) => {
          TemplataCompiler.getFirstUnsolvedIdentifyingRune(structA.genericParameters, (rune) => solver.getConclusion(rune).nonEmpty) match {
            case None => false
            case Some((genericParam, index)) => {
              // Make a placeholder for every argument even if it has a default, see DUDEWCD.
              val templata =
                templataCompiler.createPlaceholder(
                  coutputs, outerEnv, structTemplateFullName, genericParam, index, allRuneToType, true)
              solver.manualStep(Map(genericParam.rune.rune -> templata))
              true
            }
          }
        }) match {
        case Err(f @ FailedCompilerSolve(_, _, err)) => {
          throw CompileErrorExceptionT(typing.TypingPassSolverError(structA.range :: parentRanges, f))
        }
        case Ok(true) =>
        case Ok(false) => // Incomplete, will be detected in the below expectCompleteSolve
      }
      val CompleteCompilerSolve(_, inferences, _, reachableBoundsFromParamsAndReturn) =
        inferCompiler.expectCompleteSolve(
          envs, coutputs, definitionRules, allRuneToType, structA.range :: parentRanges, true, true, Vector(), solver)


      structA.maybePredictedMutability match {
        case None => {
          val mutability =
            ITemplata.expectMutability(inferences(structA.mutabilityRune.rune))
          coutputs.declareTypeMutability(structTemplateFullName, mutability)
        }
        case Some(_) =>
      }

      val templateArgs = structA.genericParameters.map(_.rune.rune).map(inferences)

      val fullName = assembleStructName(structTemplateFullName, templateArgs)

      val innerEnv =
        CitizenEnvironment(
          outerEnv.globalEnv,
          outerEnv,
          structTemplateFullName,
          fullName,
          TemplatasStore(fullName, Map(), Map())
            .addEntries(
              interner,
              inferences.toVector
                .map({ case (rune, templata) => (interner.intern(RuneNameT(rune)), TemplataEnvEntry(templata)) })))

      coutputs.declareTypeInnerEnv(structTemplateFullName, innerEnv)

      core.compileStruct(outerEnv, innerEnv, coutputs, parentRanges, structA)
    })
  }

  def compileInterface(
    coutputs: CompilerOutputs,
    parentRanges: List[RangeS],
    interfaceTemplata: InterfaceDefinitionTemplata):
  Unit = {
    Profiler.frame(() => {
      val InterfaceDefinitionTemplata(declaringEnv, interfaceA) = interfaceTemplata
      val interfaceTemplateName = nameTranslator.translateInterfaceName(interfaceA.name)
      val interfaceTemplateFullName = declaringEnv.fullName.addStep(interfaceTemplateName)

      // We declare the interface's outer environment in the precompile stage instead of here because
      // of MDATOEF.
      val outerEnv = coutputs.getOuterEnvForType(parentRanges, interfaceTemplateFullName)

      //      val fullName = env.fullName.addStep(interfaceLastName)

      val definitionRules = interfaceA.rules.filter(InferCompiler.includeRuleInDefinitionSolve)

      val envs = InferEnv(outerEnv, List(interfaceA.range), outerEnv)
      val solver =
        inferCompiler.makeSolver(
          envs, coutputs, definitionRules, interfaceA.runeToType, interfaceA.range :: parentRanges, Vector(), Vector())
      // Incrementally solve and add placeholders, see IRAGP.
      inferCompiler.incrementallySolve(
        envs, coutputs, solver,
        // Each step happens after the solver has done all it possibly can. Sometimes this can lead
        // to races, see RRBFS.
        (solver) => {
          TemplataCompiler.getFirstUnsolvedIdentifyingRune(interfaceA.genericParameters, (rune) => solver.getConclusion(rune).nonEmpty) match {
            case None => false
            case Some((genericParam, index)) => {
              // Make a placeholder for every argument even if it has a default, see DUDEWCD.
              val templata =
                templataCompiler.createPlaceholder(
                  coutputs, outerEnv, interfaceTemplateFullName, genericParam, index, interfaceA.runeToType, true)
              solver.manualStep(Map(genericParam.rune.rune -> templata))
              true
            }
          }
        }) match {
        case Err(f @ FailedCompilerSolve(_, _, err)) => {
          throw CompileErrorExceptionT(typing.TypingPassSolverError(interfaceA.range :: parentRanges, f))
        }
        case Ok(true) =>
        case Ok(false) => // Incomplete, will be detected in the below expectCompleteSolve
      }
      val CompleteCompilerSolve(_, inferences, _, reachableBoundsFromParamsAndReturn) =
        inferCompiler.expectCompleteSolve(
          envs, coutputs, definitionRules, interfaceA.runeToType, interfaceA.range :: parentRanges, true, true, Vector(), solver)

      interfaceA.maybePredictedMutability match {
        case None => {
          val mutability = ITemplata.expectMutability(inferences(interfaceA.mutabilityRune.rune))
          coutputs.declareTypeMutability(interfaceTemplateFullName, mutability)
        }
        case Some(_) =>
      }

      val templateArgs = interfaceA.genericParameters.map(_.rune.rune).map(inferences)

      val fullName = assembleInterfaceName(interfaceTemplateFullName, templateArgs)

      val innerEnv =
        CitizenEnvironment(
          outerEnv.globalEnv,
          outerEnv,
          interfaceTemplateFullName,
          fullName,
          TemplatasStore(fullName, Map(), Map())
            .addEntries(
              interner,
              inferences.toVector
                .map({ case (rune, templata) => (interner.intern(RuneNameT(rune)), TemplataEnvEntry(templata)) })))

      coutputs.declareTypeInnerEnv(interfaceTemplateFullName, innerEnv)

      core.compileInterface(declaringEnv, outerEnv, innerEnv, coutputs, parentRanges, interfaceA)
    })
  }

  // Makes a struct to back a closure
  def makeClosureUnderstruct(
    containingFunctionEnv: NodeEnvironment,
    coutputs: CompilerOutputs,
    parentRanges: List[RangeS],
    name: IFunctionDeclarationNameS,
    functionS: FunctionA,
    members: Vector[NormalStructMemberT]):
  (StructTT, MutabilityT, FunctionTemplata) = {
    core.makeClosureUnderstruct(containingFunctionEnv, coutputs, parentRanges, name, functionS, members)
  }

  def assembleStructName(
    templateName: IdT[IStructTemplateNameT],
    templateArgs: Vector[ITemplata[ITemplataType]]):
  IdT[IStructNameT] = {
    templateName.copy(
      localName = templateName.localName.makeStructName(interner, templateArgs))
  }

  def assembleInterfaceName(
    templateName: IdT[IInterfaceTemplateNameT],
    templateArgs: Vector[ITemplata[ITemplataType]]):
  IdT[IInterfaceNameT] = {
    templateName.copy(
      localName = templateName.localName.makeInterfaceName(interner, templateArgs))
  }
}
