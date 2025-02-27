package dev.vale.postparsing.patterns

import dev.vale.Interner
import dev.vale.parsing.ast._
import dev.vale.postparsing._
import dev.vale.postparsing.rules.{IRulexSR, TemplexScout}
import dev.vale.parsing._
import dev.vale.parsing.ast._
import dev.vale.postparsing.rules._
import dev.vale.postparsing._
import dev.vale.RangeS

import scala.collection.immutable.List
import scala.collection.mutable
import scala.collection.mutable.ArrayBuffer

class PatternScout(
    interner: Interner,
    templexScout: TemplexScout) {
  def getParameterCaptures(pattern: AtomSP): Vector[VariableDeclaration] = {
    val AtomSP(_, maybeCapture, _, _, maybeDestructure) = pattern
  Vector.empty ++
      maybeCapture.toVector.flatMap(getCaptureCaptures) ++
        maybeDestructure.toVector.flatten.flatMap(getParameterCaptures)
  }
  private def getCaptureCaptures(capture: CaptureS): Vector[VariableDeclaration] = {
    if (capture.mutate) {
      Vector()
    } else {
      Vector(VariableDeclaration(capture.name))
    }
  }

  // Returns:
  // - New rules
  // - Scouted patterns
  private[postparsing] def scoutPatterns(
      stackFrame: StackFrame,
      lidb: LocationInDenizenBuilder,
      ruleBuilder: ArrayBuffer[IRulexSR],
      runeToExplicitType: mutable.ArrayBuffer[(IRuneS, ITemplataType)],
      params: Vector[PatternPP]):
  Vector[AtomSP] = {
    params.map(
      translatePattern(
        stackFrame, lidb, ruleBuilder, runeToExplicitType, _))
  }

  // Returns:
  // - Rules, which are likely just TypedSR
  // - The translated patterns
  private[postparsing] def translatePattern(
    stackFrame: StackFrame,
    lidb: LocationInDenizenBuilder,
    ruleBuilder: ArrayBuffer[IRulexSR],
    runeToExplicitType: mutable.ArrayBuffer[(IRuneS, ITemplataType)],
    patternPP: PatternPP):
  AtomSP = {
    val PatternPP(range,_,maybeCaptureP, maybeTypeP, maybeDestructureP, maybeAbstractP) = patternPP

    val maybeAbstractS =
      maybeAbstractP match {
        case None => None
        case Some(AbstractP(range)) => {
          Some(AbstractSP(PostParser.evalRange(stackFrame.file, range), stackFrame.parentEnv.isInterfaceInternalMethod))
        }
      }

    val maybeCoordRuneS =
      maybeTypeP.map(typeP => {
        val runeS =
          templexScout.translateMaybeTypeIntoRune(
            stackFrame.parentEnv,
            lidb.child(),
            PostParser.evalRange(stackFrame.file, range),
            ruleBuilder,
            maybeTypeP)
        runeToExplicitType += ((runeS.rune, CoordTemplataType()))
        runeS
      })

    val maybePatternsS =
      maybeDestructureP match {
        case None => None
        case Some(DestructureP(_, destructureP)) => {
          Some(
            destructureP.map(
              translatePattern(
                stackFrame, lidb.child(), ruleBuilder, runeToExplicitType, _)))
        }
      }

    val captureS =
      maybeCaptureP match {
        case None => {
//          val codeLocation = Scout.evalPos(stackFrame.file, patternPP.range.begin)
          None
        }
        case Some(DestinationLocalP(IgnoredLocalNameDeclarationP(_), _)) => {
          None
        }
        case Some(DestinationLocalP(LocalNameDeclarationP(NameP(_, name)), maybeMutate)) => {
          if (name.str == "set" || name.str == "mut") {
            throw CompileErrorExceptionS(CantUseThatLocalName(PostParser.evalRange(stackFrame.file, range), name.str))
          }
          Some(CaptureS(interner.intern(CodeVarNameS(name)), maybeMutate.nonEmpty))
        }
        case Some(DestinationLocalP(ConstructingMemberNameDeclarationP(NameP(_, name)), maybeMutate)) => {
          Some(CaptureS(interner.intern(ConstructingMemberNameS(name)), maybeMutate.nonEmpty))
        }
        case Some(DestinationLocalP(IterableNameDeclarationP(range), maybeMutate)) => {
          Some(CaptureS(interner.intern(IterableNameS(PostParser.evalRange(stackFrame.file, range))), maybeMutate.nonEmpty))
        }
        case Some(DestinationLocalP(IteratorNameDeclarationP(range), maybeMutate)) => {
          Some(CaptureS(interner.intern(IteratorNameS(PostParser.evalRange(stackFrame.file, range))), maybeMutate.nonEmpty))
        }
        case Some(DestinationLocalP(IterationOptionNameDeclarationP(range), maybeMutate)) => {
          Some(CaptureS(interner.intern(IterationOptionNameS(PostParser.evalRange(stackFrame.file, range))), maybeMutate.nonEmpty))
        }
      }

    AtomSP(PostParser.evalRange(stackFrame.file, range), captureS, maybeAbstractS, maybeCoordRuneS, maybePatternsS)
  }

}
