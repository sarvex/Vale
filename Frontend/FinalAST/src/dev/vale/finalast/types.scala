package dev.vale.finalast

import dev.vale.{FileCoordinate, Interner, Keywords, PackageCoordinate, vassert, vfail, vimpl}

// Represents a reference type.
// A reference contains these things:
// - The kind; the thing that this reference points at.
// - The ownership; this reference's relationship to the kind. This can be:
//   - Share, which means the references all share ownership of the kind. This
//     means that the kind will only be deallocated once all references to it are
//     gone. Share references can only point at immutable kinds, and immutable
//     kinds can *only* be pointed at by share references.
//   - Owning, which means this reference owns the object, and when this reference
//     disappears (without being moved), the object should disappear (this is taken
//     care of by the typing stage). Owning refs can only point at mutable kinds.
//   - Constraint, which means this reference doesn't own the kind. The kind
//     is guaranteed not to die while this constraint ref is active (indeed if it did
//     the program would panic). Constraint refs can only point at mutable kinds.
//   - Raw, which is a weird ownership and should go away. We point at Void with this.
//     TODO: Get rid of raw.
//   - (in the future) Weak, which is a reference that will null itself out when the
//     kind is destroyed. Weak refs can only point at mutable kinds.
// - Permission, how one can modify the object through this reference.
//   - Readonly, we cannot modify the object through this reference.
//   - Readwrite, we can.
// - (in the future) Location, either inline or yonder. Inline means that this reference
//   isn't actually a pointer, it's just the value itself, like C's Car vs Car*.
// In previous stages, this is referred to as a "coord", because these four things can be
// thought of as dimensions of a coordinate.
case class CoordH[+T <: KindHT](
    ownership: OwnershipH, location: LocationH, kind: T) {
  val hash = runtime.ScalaRunTime._hashCode(this); override def hashCode(): Int = hash;

  (ownership, location) match {
    case (OwnH, YonderH) =>
    case (ShareH, _) =>
    case (BorrowH, YonderH) =>
    case (WeakH, YonderH) =>
    case _ => vfail()
  }

  kind match {
    case IntHT(_) | BoolHT() | FloatHT() | NeverHT(_) => {
      // Make sure that if we're pointing at a primitives, it's via a Share reference.
      vassert(ownership == ShareH)
      vassert(location == InlineH)
    }
    case StrHT() => {
      // Strings need to be yonder because Midas needs to do refcounting for them.
      vassert(ownership == ShareH)
      vassert(location == YonderH)
    }
    case StructHT(name) => {
      val isBox = name.fullyQualifiedName.startsWith("__Box")

      if (isBox) {
        vassert(ownership == OwnH || ownership == BorrowH)
      }
    }
    case _ =>
  }

  // Convenience function for casting this to a Reference which the compiler knows
  // points at a static sized array.
  def expectStaticSizedArrayCoord() = {
    kind match {
      case atH @ StaticSizedArrayHT(_) => CoordH[StaticSizedArrayHT](ownership, location, atH)
    }
  }
  // Convenience function for casting this to a Reference which the compiler knows
  // points at an unstatic sized array.
  def expectRuntimeSizedArrayCoord() = {
    kind match {
      case atH @ RuntimeSizedArrayHT(_) => CoordH[RuntimeSizedArrayHT](ownership, location, atH)
    }
  }
  // Convenience function for casting this to a Reference which the compiler knows
  // points at struct.
  def expectStructCoord() = {
    kind match {
      case atH @ StructHT(_) => CoordH[StructHT](ownership, location, atH)
    }
  }
  // Convenience function for casting this to a Reference which the compiler knows
  // points at interface.
  def expectInterfaceCoord() = {
    kind match {
      case atH @ InterfaceHT(_) => CoordH[InterfaceHT](ownership, location, atH)
    }
  }
}

// A value, a thing that can be pointed at. See ReferenceH for more information.
sealed trait KindHT {
  def packageCoord(interner: Interner, keywords: Keywords): PackageCoordinate
}
object IntHT {
  val i32 = IntHT(32)
}
case class IntHT(bits: Int) extends KindHT {
  val hash = runtime.ScalaRunTime._hashCode(this); override def hashCode(): Int = hash;
  override def packageCoord(interner: Interner, keywords: Keywords): PackageCoordinate = PackageCoordinate.BUILTIN(interner, keywords)
}
case class VoidHT() extends KindHT {
  val hash = runtime.ScalaRunTime._hashCode(this); override def hashCode(): Int = hash;
  override def packageCoord(interner: Interner, keywords: Keywords): PackageCoordinate = PackageCoordinate.BUILTIN(interner, keywords)
}
case class BoolHT() extends KindHT {
  val hash = runtime.ScalaRunTime._hashCode(this); override def hashCode(): Int = hash;
  override def packageCoord(interner: Interner, keywords: Keywords): PackageCoordinate = PackageCoordinate.BUILTIN(interner, keywords)
}
case class StrHT() extends KindHT {
  val hash = runtime.ScalaRunTime._hashCode(this); override def hashCode(): Int = hash;
  override def packageCoord(interner: Interner, keywords: Keywords): PackageCoordinate = PackageCoordinate.BUILTIN(interner, keywords)
}
case class FloatHT() extends KindHT {
  val hash = runtime.ScalaRunTime._hashCode(this); override def hashCode(): Int = hash;
  override def packageCoord(interner: Interner, keywords: Keywords): PackageCoordinate = PackageCoordinate.BUILTIN(interner, keywords)
}
// A primitive which can never be instantiated. If something returns this, it
// means that it will never actually return. For example, the return type of
// __panic() is a NeverH.
// TODO: This feels weird being a kind in metal. Figure out a way to not
// have this? Perhaps replace all kinds with Optional[Optional[KindH]],
// where None is never, Some(None) is Void, and Some(Some(_)) is a normal thing.
case class NeverHT(fromBreak: Boolean) extends KindHT {
  val hash = runtime.ScalaRunTime._hashCode(this); override def hashCode(): Int = hash;
  override def packageCoord(interner: Interner, keywords: Keywords): PackageCoordinate = PackageCoordinate.BUILTIN(interner, keywords)
}

case class InterfaceHT(
  // Unique identifier for the interface.
  fullName: IdH
) extends KindHT {
  val hash = runtime.ScalaRunTime._hashCode(this); override def hashCode(): Int = hash;
  override def packageCoord(interner: Interner, keywords: Keywords): PackageCoordinate = fullName.packageCoordinate
}

case class StructHT(
  // Unique identifier for the interface.
  fullName: IdH
) extends KindHT {
  val hash = runtime.ScalaRunTime._hashCode(this); override def hashCode(): Int = hash;
  override def packageCoord(interner: Interner, keywords: Keywords): PackageCoordinate = fullName.packageCoordinate
}

// An array whose size is known at compile time, and therefore doesn't need to
// carry around its size at runtime.
case class StaticSizedArrayHT(
  // This is useful for naming the Midas struct that wraps this array and its ref count.
  name: IdH,
) extends KindHT {
  val hash = runtime.ScalaRunTime._hashCode(this); override def hashCode(): Int = hash;
  override def packageCoord(interner: Interner, keywords: Keywords): PackageCoordinate = name.packageCoordinate
}

// An array whose size is known at compile time, and therefore doesn't need to
// carry around its size at runtime.
case class StaticSizedArrayDefinitionHT(
  // This is useful for naming the Midas struct that wraps this array and its ref count.
  name: IdH,
  // The size of the array.
  size: Long,
  mutability: Mutability,
  variability: Variability,
  elementType: CoordH[KindHT]
) {
  val hash = runtime.ScalaRunTime._hashCode(this); override def hashCode(): Int = hash;
  def kind = StaticSizedArrayHT(name)
}

case class RuntimeSizedArrayHT(
  // This is useful for naming the Midas struct that wraps this array and its ref count.
  name: IdH,
) extends KindHT {
  val hash = runtime.ScalaRunTime._hashCode(this); override def hashCode(): Int = hash;
  override def packageCoord(interner: Interner, keywords: Keywords): PackageCoordinate = name.packageCoordinate
}

case class RuntimeSizedArrayDefinitionHT(
  // This is useful for naming the Midas struct that wraps this array and its ref count.
  name: IdH,
  mutability: Mutability,
  elementType: CoordH[KindHT]
) {
  val hash = runtime.ScalaRunTime._hashCode(this); override def hashCode(): Int = hash;
  def kind = RuntimeSizedArrayHT(name)
}

// Place in the original source code that something came from. Useful for uniquely
// identifying templates.
case class CodeLocation(
  file: FileCoordinate,
  offset: Int) { val hash = runtime.ScalaRunTime._hashCode(this); override def hashCode(): Int = hash; }

// Ownership is the way a reference relates to the kind's lifetime, see
// ReferenceH for explanation.
sealed trait OwnershipH
case object OwnH extends OwnershipH
case object BorrowH extends OwnershipH
case object WeakH extends OwnershipH
case object ShareH extends OwnershipH

// Location says whether a reference contains the kind's location (yonder) or
// contains the kind itself (inline).
// Yes, it's weird to consider a reference containing a kind, but it makes a
// lot of things simpler for the language.
// Examples (with C++ translations):
//   This will create a variable `car` that lives on the stack ("inline"):
//     Vale: car = inl Car(4, "Honda Civic");
//     C++:  Car car(4, "Honda Civic");
//   This will create a variable `car` that lives on the heap ("yonder"):
//     Vale: car = Car(4, "Honda Civic");
//     C++:  Car* car = new Car(4, "Honda Civic");
//   This will create a struct Spaceship whose engine and reactor are allocated
//   separately somewhere else on the heap (yonder):
//     Vale: struct Car { engine Engine; reactor Reactor; }
//     C++:  class Car { Engine* engine; Reactor* reactor; }
//   This will create a struct Spaceship whose engine and reactor are embedded
//   into its own memory (inline):
//     Vale: struct Car { engine inl Engine; reactor inl Reactor; }
//     C++:  class Car { Engine engine; Reactor reactor; }
// Note that the compiler will often automatically add an `inl` onto whatever
// local variables it can, to speed up the program.
sealed trait LocationH
// Means that the kind will be in the containing stack frame or struct.
case object InlineH extends LocationH
// Means that the kind will be allocated separately, in the heap.
case object YonderH extends LocationH

// Used to say whether an object can be modified or not.
// Structs and interfaces specify whether theyre immutable or mutable, but all
// primitives are immutable (after all, you can't change 4 itself to be another
// number).
sealed trait Mutability
// Immutable structs can only contain or point at other immutable structs, in
// other words, something immutable is *deeply* immutable.
// Immutable things can only be referred to with Share references.
case object Immutable extends Mutability
// Mutable objects have a lifetime.
case object Mutable extends Mutability

// Used to say whether a variable (or member) reference can be changed to point
// at something else.
// Examples (with C++ translations):
//   This will create a varying local, which can be changed to point elsewhere:
//     Vale:
//       x = Car(4, "Honda Civic");
//       set x = someOtherCar;
//       set x = Car(4, "Toyota Camry");
//     C++:
//       Car* x = new Car(4, "Honda Civic");
//       x = someOtherCar;
//       x = new Car(4, "Toyota Camry");
//   This will create a final local, which can't be changed to point elsewhere:
//     Vale: x = Car(4, "Honda Civic");
//     C++:  Car* const x = new Car(4, "Honda Civic");
//   Note the position of the const, which says that the pointer cannot change,
//   but we can still change the members of the Car, which is also true in Vale:
//     Vale:
//       x = Car(4, "Honda Civic");
//       mut x.numWheels = 6;
//     C++:
//       Car* const x = new Car(4, "Honda Civic");
//       x->numWheels = 6;
// In other words, variability affects whether the variable (or member) can be
// changed to point at something different, but it doesn't affect whether we can
// change anything inside the kind (this reference's permission and the
// kind struct's member's variability affect that).
sealed trait Variability
case object Final extends Variability
case object Varying extends Variability
