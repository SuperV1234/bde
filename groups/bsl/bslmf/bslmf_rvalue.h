// bslmf_rvalue.h                                                     -*-C++-*-
#ifndef INCLUDED_BSLMF_RVALUE
#define INCLUDED_BSLMF_RVALUE

#ifndef INCLUDED_BSLS_IDENT
#include <bsls_ident.h>
#endif
BSLS_IDENT("$Id: $")

#ifndef INCLUDED_BSLSCM_VERSION
#include <bslscm_version.h>
#endif


//@PURPOSE: Provide a vocabulary type to enable move semantics.
//
//@CLASSES:
//  bslmf::Rvalue: a template indicating that an object can be moved from
//  bslmf::RvalueUtil: a namespace to hold utility functions for r-values
//
//@SEE_ALSO:
//
//@DESCRIPTION: This component provides a template, 'bslmf::Rvalue' used to
// convey the information that an object will not be used anymore and its
// representation can be transferred elsewhere and a utilities struct
// 'bslmf::RvalueUtil'. In C++11 terminology an object represented by a
// 'bslmf::Rvalue<T>' can be moved from. With a C++11 implementation
// 'bslmf::Rvalue<T>' is an alias template for 'T&&'. With a C++03
// implementation 'bslmf::Rvalue<T>' is a class template providing lvalue
// access to an object whose representation can be transferred. The objective
// of this component is to provide a name for the concept of a movable object.
// Using a common name enables use of manual move semantics when using C++03.
// With C++11 automatic move semantics is enabled when moving objects known to
// the compiler to go out of scope.
//
// Using 'bslmf::Rvalue<T>' to support movable type allows implementation of
// move semantics working with both C++03 and C++11 without conditional
// compilation of the user code. Only the implementation of the component
// bslmf_rvalue uses conditional compilation to enable the appropriate
// implementation choice.
//
// For a consistent use across different versions of the C++ standard a few
// utility functions are provided in the utility class 'bslmf::RvalueUtil'.
// This class contains functions for moving and forwarding objects. To use an
// identical notation to access an object with C++11 where 'bslmf::Rvalue<T>'
// is just an lvalue of type 'T' and with C++03 where 'bslmf::Rvalue<T>' is a
// class type referencing to an lvalue of type 'T', a function template
// 'bslmf::RvalueUtil<T>::access(r)' is provided.
//
///Usage
///-----
// There are two sides of move semantics:
//: 1 Classes or class templates that are _move-enabled_, i.e., which can
//:   transfer their internal representation to another object in some
//:   situations. To become move-enabled a class needs to implement, at
//:   least, a move constructor. It should probably also implement a move
//:   assignment.
//: 2 Users of a potentially move-enabled class may take advantage of moving
//:   objects by explicitly indicating that ownership of resources may be
//:   transferred. When using C++11 the compiler can automatically detect
//:   some situations where it is safe to move objects but this features is
//:   not available with C++03.
//
// The usage example below demonstrate both use cases using a simplified
// version of 'std::vector<T>'. The class template is simplified to concentrate
// on the aspects relevant to 'bslmf::Rvalue<T>'. Most of the operations are
// just normal implementations to create a container. The last two operations
// described are using move operations and the 'reserve()' function uses
// 'moveIfNoexcept()'.
//
// The definition of the 'vector<TYPE>' class template is rather straight
// forward. For simplicity a few trivial operations are implemented directly
// in the class definition:
//..
//  template <class TYPE>
//  class vector
//  {
//      TYPE *d_begin;
//      TYPE *d_end;
//      TYPE *d_endBuffer;
//
//      static void swap(TYPE*& a, TYPE*& b);
//          // This function swaps the specified pointers 'a' and 'b'.
//    public:
//      vector();
//          // Create an empty vector.
//      vector(bslmf::Rvalue<vector> other);
//          // Create a vector by transfering the content of the specified
//          // 'other'.
//      vector(const vector& other);
//          // Create a vector by copying the content of the specified 'other'.
//      vector& operator= (vector other);
//          // Assign a vector by copying the content of the specified 'other'.
//          // The function returns a reference to the object. Note that
//          // 'other' is passed by value to have the copy or move already be
//          // done or even elided. Within the body of the assignment operator
//          // the content of 'this' and 'other' are simply swapped.
//      ~vector();
//          // Destroy the vector's elements and release any allocated memory.
//
//      TYPE&       operator[](int index)      { return this->d_begin[index]; }
//          // Return a reference to the object at the specified 'index'.
//      const TYPE& operator[](int index) const{ return this->d_begin[index]; }
//          // Return a reference to the object at the specified 'index'.
//      TYPE       *begin()       { return this->d_begin; }
//          // Return a pointer to the first element.
//      const TYPE *begin() const { return this->d_begin; }
//          // Return a pointer to the first element.
//      int capacity() const { return int(this->d_endBuffer - this->d_begin); }
//          // Return the capacity of the vector.
//      bool empty() const { return this->d_begin == this->d_end; }
//          // Return 'true' if the vector is empty and 'false' otherwise.
//      TYPE       *end()       { return this->d_end; }
//          // Return a pointer to the end of the range.
//      const TYPE *end() const { return this->d_end; }
//          // Return a pointer to the end of the range.
//
//      void push_back(const TYPE& value);
//          // Append a copy of the specified 'value' to the vector.
//      void push_back(bslmf::Rvalue<TYPE> value);
//          // Append an object moving the specified 'value' to the new
//          // location.
//      void reserve(int newCapacity);
//          // Reserve enough capacity to fit at least as many elements as
//          // specified by 'newCapacity'.
//      int size() const { return int(this->d_end - this->d_begin); }
//          // Return the size of the object.
//      void swap(vector& other);
//          // Swap the content of the vector with the specified 'other'.
//  };
//..
// The class stores pointers to the begin and the end of the elements as well
// as a pointer to the end of the allocated buffer. If there are no elements,
// null pointers are stored. There a number of accessors similar to the
// accessors used by 'std::vector<TYPE>'.
//
// The default constructor creates an empty 'vector<TYPE>' by simply
// initializing all member pointers to be null pointers:
//..
//  template <class TYPE>
//  vector<TYPE>::vector()
//      : d_begin()
//      , d_end()
//      , d_endBuffer() {
//  }
//..
// To leverage already implemented functionality some of the member functions
// operate on a temporary 'vector<TYPE>' and move the result into place using
// the 'swap()' member function that simply does a memberwise 'swap()' (the
// function swapping pointers is implemented here to avoid any dependency on
// functions defined in another level):
//..
//  template <class TYPE>
//  void vector<TYPE>::swap(TYPE*& a, TYPE*& b) {
//      TYPE *tmp = a;
//      a = b;
//      b = tmp;
//  }
//  template <class TYPE>
//  void vector<TYPE>::swap(vector& other) {
//      this->swap(this->d_begin, other.d_begin);
//      this->swap(this->d_end, other.d_end);
//      this->swap(this->d_endBuffer, other.d_endBuffer);
//  }
//..
// The member function 'reserve()' arranges for the 'vector<TYPE>' to have
// enough capacity for the number of elements specified as argument.
// The function first creates an empty 'vector<TYPE>' called 'tmp' and sets
// 'tmp' up to have enough capacity by allocating sufficient memory and
// assigning the different members to point to the allocated buffer. The
// function then iterates over the elements of 'this' and for each element
// it constructs a new element in 'tmp'. It does so using placement new with
// a constructor argument of 'bslmf::RvalueUtil::moveIfNoexcept(*it)'. Since
// a successful execution of 'reserve()' will release the buffer held by 'this'
// all elements can be moved to their new location if there is no potential of
// failure by a later move throwing an exception. If it is possibly that moving
// the elements might throw, the elements need to copied instead. Once all
// elements are in place in 'tmp' the content of 'tmp' and 'this' is swapped:
//..
//  template <class TYPE>
//  void vector<TYPE>::reserve(int newCapacity) {
//      if (this->capacity() < newCapacity) {
//          vector tmp;
//          int size = int(sizeof(TYPE) * newCapacity);
//          tmp.d_begin = static_cast<TYPE*>(operator new(size));
//          tmp.d_end = tmp.d_begin;
//          tmp.d_endBuffer = tmp.d_begin + newCapacity;
//
//          for (TYPE* it = this->d_begin; it != this->d_end; ++it) {
//              new (tmp.d_end) TYPE(bslmf::RvalueUtil::moveIfNoexcept(*it));
//              ++tmp.d_end;
//          }
//          this->swap(tmp);
//      }
//  }
//..
// Any allocated data and constructed elements need to be release in the
// destructor. The destructor does so by calling the destructor of the elements
// in the buffer from back to front. Once the elements are destroyed the buffer
// is released:
//..
//  template <class TYPE>
//  vector<TYPE>::~vector() {
//      if (this->d_begin) {
//          while (this->d_begin != this->d_end) {
//              --this->d_end;
//              this->d_end->~TYPE();
//          }
//          operator delete(this->d_begin);
//      }
//  }
//..
// Using 'reserve()' and constructing the elements it is straight forward to
// implement the copy constructor. First the member pointers are initialed to
// null. If 'other' is empty there is nothing further to do as it is desirable
// to not allocate a buffer for an empty 'vector'. If there are elements to
// copy the buffer is set up by calling 'reserve()' to create sufficient
// capacity. Once that is done elements are copied by iterating over the
// elements of 'other' and constructing elements using placement new in the
// appropriate location.
//..
//  template <class TYPE>
//  vector<TYPE>::vector(const vector& other)
//      : d_begin()
//      , d_end()
//      , d_endBuffer() {
//      if (!other.empty()) {
//          this->reserve(4 < other.size()? other.size(): 4);
//
//          ASSERT(other.size() <= this->capacity());
//          for (TYPE* it = other.d_begin; it != other.d_end; ++it) {
//              new (this->d_end) TYPE(*it);
//              ++this->d_end;
//          }
//      }
//  }
//..
// A simple copy assignment operator can be implemented in terms of copy/move
// constructors, 'swap()', and destructor (in a real implementaiton the copy
// assignment would probably try to use already allocated objects). In this
// implementation that argument is taken by value, i.e., the argument is
// already constructed using copy or move construction (which may have been
// elided), the content of 'this' is swapped with the content of 'other'
// leaving this in the desired state, and the destructor will release the
// former representation of 'this' when 'other' is destroyed':
//..
//  template <class TYPE>
//  vector<TYPE>& vector<TYPE>::operator= (vector other) {
//      this->swap(other);
//      return *this;
//  }
//..
// To complete the normal C++03 operations of 'vector<TYPE>' the only remaining
// member function is 'push_back()'. This function calls 'reserve()' to obtain
// more capacity if the current capacity is filled and then constructs the new
// element at the location pointed to by 'd_end':
//..
//  template <class TYPE>
//  void vector<TYPE>::push_back(const TYPE& value) {
//      if (this->d_end == this->d_endBuffer) {
//          this->reserve(this->size()? int(1.5 * this->size()): 4);
//      }
//      assert(this->d_end != this->d_endBuffer);
//      new(this->d_end) TYPE(value);
//      ++this->d_end;
//  }
//..
// The first operation actually demonstrating the use of 'Rvalue<TYPE>' is the
// move constructor:
//..
//  template <class TYPE>
//  vector<TYPE>::vector(bslmf::Rvalue<vector> other)
//      : d_begin(bslmf::RvalueUtil::access(other).d_begin)
//      , d_end(bslmf::RvalueUtil::access(other).d_end)
//      , d_endBuffer(bslmf::RvalueUtil::access(other).d_endBuffer) {
//      vector& reference(other);
//      reference.d_begin = 0;
//      reference.d_end = 0;
//      reference.d_endBuffer = 0;
//  }
//..
// This constructor gets an 'Rvalue<vector<TYPE> >' passed as argument that
// indicates that the referenced objects can be modified as long as it is left
// in a state meeting the class invariants. The implementation of this
// constructor first copies the 'd_begin', 'd_end', and 'd_capacity' members of
// 'other'. Since 'other' is either an object of type 'Rvalue<vector<TYPE> >'
// (when compiling using a C++03 compiler) or an rvalue reference
// 'vector<TYPE>&&' the members are accessed using 'RvalueUtil::access(other)'
// to get a reference to a 'vector<TYPE>'. Within the body of the constructor
// an lvalue reference is obtained either via the conversion operator of
// 'Rvalue<T>' or directly as 'other' is just an lvalue whe compiiling with a
// C++11 compiler. This reference is used to set the pointer members of the
// object referenced by 'other' to '0' completing the move of the content to
// the object under construction.
//
// Finally, a move version of 'push_back()' is provided: it takes an
// 'Rvalue<TYPE>' as argument. The type of this argument indicates that the
// state can be transferred and after arranging enough capacity in the
// 'vector<TYPE>' object a new element is move constructed at the position
// 'd_end':
//..
//  template <class TYPE>
//  void vector<TYPE>::push_back(bslmf::Rvalue<TYPE> value) {
//      if (this->d_end == this->d_endBuffer) {
//          this->reserve(this->size()? int(1.5 * this->size()): 4);
//      }
//      assert(this->d_end != this->d_endBuffer);
//      new(this->d_end) TYPE(bslmf::RvalueUtil::move(value));
//      ++this->d_end;
//  }
//..
// To demonstrate the newly created 'vector<TYPE>' class in action, first a
// 'vector<int>' is created and filled with a few elements:
//..
//  vector<int> vector0;
//  for (int i = 0; i != 5; ++i) {
//      vector0.push_back(i);
//  }
//  for (int i = 0; i != 5; ++i) {
//      assert(vector0[i] == i);
//  }
//..
// To verify that copying of 'vector<TYPE>' objects works, a copy is created:
//..
//  vector<int> vector1(vector0);
//  assert(vector1.size() == 5);
//  assert(vector1.size() == vector0.size());
//  for (int i = 0; i != vector1.size(); ++i) {
//      assert(vector1[i] == i);
//      assert(vector1[i] == vector0[i]);
//  }
//..
// When using moving this 'vector0' to a new location the representation of the
// new object should use the orginal 'begin()':
//..
//  const int *first = vector0.begin();
//  vector<int> vector2(bslmf::RvalueUtil::move(vector0));
//  assert(first == vector2.begin());
//..
// When create a 'vector<vector<int> >' and using 'push_back()' on this object
// with 'vector2' a copy should be inserted:
//..
//  vector<vector<int> > vvector;
//  vvector.push_back(vector2);                          // copy
//  assert(vector2.size() == 5);
//  assert(vvector.size() == 1);
//  assert(vvector[0].size() == vector2.size());
//  assert(vvector[0].begin() != first);
//  for (int i = 0; i != 5; ++i) {
//      assert(vvector[0][i] == i);
//      assert(vector2[i] == i);
//  }
//..
// When adding another element by moving 'vector2' the 'begin()' of the newly
// inserted element will be the same as 'first', i.e., the representation is
// transferred:
//..
//  vvector.push_back(bslmf::RvalueUtil::move(vector2)); // move
//  assert(vvector.size() == 2);
//  assert(vvector[1].begin() == first);
//  assert(vvector[1].size() == 5);
//..
// Compiling this code with both C++03 and C++11 compilers shows that there is
// no need for conditional compilation in when using 'Rvalue<TYPE>' while move
// semantics is enabled in both modes.

#ifndef INCLUDED_BSLS_ASSERT
#include <bsls_assert.h>
#endif

#ifndef INCLUDED_BSLS_COMPILERFEATURES
#include <bsls_compilerfeatures.h>
#endif

#ifndef INCLUDED_BSLMF_REMOVEREFERENCE
#include <bslmf_removereference.h>
#endif

namespace BloombergLP {

namespace bslmf {

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) \
    && defined(BSLS_COMPILERFEATURES_SUPPORT_ALIAS_TEMPLATES)

template <class TYPE>
using Rvalue = TYPE&&;

struct RvalueUtil {
    // This 'struct' provides a collection of utility functions operating on
    // objects of type 'Rvalue<TYPE>'. The primary use of these utilities to
    // create a consistent notation for using the C++03 'Rvalue<TYPE>' objects
    // and the C++11 'TYPE&&' rvalue references.

    template <class TYPE>
    static TYPE& access(TYPE& rvalue);
        // Obtain a reference to the object references by the specified
        // 'rvalue' object. This reference can also be obtained by a conversion
        // of 'rvalue' to 'TYPE&' in contexts where a conversion is viable.
        // When a conversion isn't applicable, e.g., when caling a member of
        // 'TYPE' the reference can be accessed using 'access()'. Since the
        // same notation should be applicable to the C++03 'Rvalue<TYPE>'
        // objects and a C++11 rvalue reference 'TYPE&&' a member function
        // cannot be used.
    template <class TYPE>
    static typename bslmf::RemoveReference<TYPE>::Type&& move(TYPE&& lvalue);
        // Get an rvalue reference of type 'Rvalue<TYPE>' from the specified
        // 'lvalue'. For a C++03 implementation this function behaves like a
        // factory for 'Rvalue<TYPE> objects. For a C++11 implementation this
        // function behaves exactly like 'std::move(lvalue)'.
    template <class TYPE>
    static const TYPE& moveIfNoexcept(TYPE& lvalue);
        // Get an rvalue reference from the specified 'lvalue' if it can be
        // determined at compile-time that moving object of type 'TYPE' cannot
        // throw an exception otherwise get an lvalue reference to 'lvalue'.
        // For a C++03 implementation this function always returns a 'const'
        // reference to 'lvalue'.
        //
        // NOTE: the correct C++11 implementation of this function requires a
        //   number of type traits that are currently not, yet, implemented
        //   (it needs 'is_nothrow_move_constructible<TYPE>' and
        //   'is_copy_constructible<TYPE>' which in turn are build upon other
        //   type traits). Until these necessary traits are implemented this
        //   function will always return an lvalue reference.
};

// ----------------------------------------------------------------------------

template <class TYPE>
inline TYPE& RvalueUtil::access(TYPE& rvalue) {
    return rvalue;
}

template <class TYPE>
inline
typename bslmf::RemoveReference<TYPE>::Type&& RvalueUtil::move(TYPE&& lvalue)
{
    return static_cast<typename bslmf::RemoveReference<TYPE>::Type&&>(lvalue);
}

// ----------------------------------------------------------------------------

#else // support rvalue references and alias templates

struct RvalueUtil;

template <class TYPE>
class Rvalue
    // The class template 'Rvalue<TYPE>' provides a reference to an object of
    // type 'TYPE' whose state will not be counted upon for later use. Put
    // differently, a function receiving an object this class template can
    // transfer ("move") the representation to a different object and leave the
    // referenced object in an unspecified, although valid (i.e., it obeys all
    // class invariants), state. With C++11 an rvalue reference ('TYPE&&') is
    // used to represent the same semantics.
{
    friend struct RvalueUtil;
    TYPE *d_pointer;

    // PRIVATE CONSTRUCTORS
    explicit Rvalue(TYPE *pointer);
        // Create an 'Rvalue<TYPE>' object referencing the object pointed to by
        // the specified 'pointer'. The behavior is undefined if 'pointer' does
        // not point to an object. This constructor is private because a C++11
        // rvalue reference cannot be created this. For information on how to
        // create objects of type 'Rvalue<TYPE>' see 'RvalueUtil::move()'.

  public:
    // ACCESSORS
    operator TYPE&() const;
        // Return a reference to the referenced object. In contexts where a
        // reference to an object of type 'TYPE' is needed, an 'Rvalue<TYPE>'
        // behaves like such a reference. For information on how to access the
        // the reference in contexts where no conversion can be used see
        // 'RvalueUtil::access()'.
};

struct RvalueUtil {
    // This 'struct' provides a collection of utility functions operating on
    // objects of type 'Rvalue<TYPE>'. The primary use of these utilities to
    // create a consistent notation for using the C++03 'Rvalue<TYPE>' objects
    // and the C++11 'TYPE&&' rvalue references.

    template <class TYPE>
    static TYPE& access(Rvalue<TYPE> rvalue);
        // Obtain a reference to the object references by the specified
        // 'rvalue' object. This reference can also be obtained by a conversion
        // of 'rvalue' to 'TYPE&' in contexts where a conversion is viable.
        // When a conversion isn't applicable, e.g., when caling a member of
        // 'TYPE' the reference can be accessed using 'access()'. Since the
        // same notation should be applicable to the C++03 'Rvalue<TYPE>'
        // objects and a C++11 r-value reference 'TYPE&&' a member function
        // cannot be used.
    template <class TYPE>
    static Rvalue<TYPE> move(TYPE& lvalue);
        // Get an rvalue reference of type 'Rvalue<TYPE>' from the specified
        // 'lvalue'. For a C++03 implementation this function behaves like a
        // factory for 'Rvalue<TYPE> objects. For a C++11 implementation this
        // function behaves exactly like 'std::move(value)'.
    template <class TYPE>
    static Rvalue<TYPE> move(Rvalue<TYPE> rvalue);
        // Forward the specified 'rvalue' as an rvalue reference. The rvalue
        // reference stays an rvalue reference to an object of type 'TYPE' and
        // doesn't become an rvalue reference to an rvalue reference.
    template <class TYPE>
    static const TYPE& moveIfNoexcept(TYPE& lvalue);
        // Get an rvalue reference from the specified 'lvalue' if it can be
        // determined at compile-time that moving object of type 'TYPE' cannot
        // throw an exception otherwise get an lvalue reference to 'lvalue'.
        // For a C++03 implementation this function always returns a 'const'
        // reference to 'lvalue'.
        //
        // NOTE: the correct C++11 implementation of this function requires a
        //   number of type traits which are currently not, yet, implemented
        //   (it needs 'is_nothrow_move_constructible<TYPE>' and
        //   'is_copy_constructible<TYPE>' which in turn are build upon other
        //   type traits). Until these necessary traits are implemented this
        //   function will always return an lvalue reference.
};

// ============================================================================
//                          INLINE DEFINITIONS
// ============================================================================

template <class TYPE>
inline Rvalue<TYPE>::Rvalue(TYPE *pointer)
    : d_pointer(pointer) {
    BSLS_ASSERT(0 != pointer);
}

template <class TYPE>
inline Rvalue<TYPE>::operator TYPE&() const {
    return *d_pointer;
}

template <class TYPE>
inline TYPE& RvalueUtil::access(Rvalue<TYPE> rvalue) {
    return rvalue;
}

template <class TYPE>
inline Rvalue<TYPE> RvalueUtil::move(TYPE& lvalue) {
    return Rvalue<TYPE>(&lvalue);
}

template <class TYPE>
inline Rvalue<TYPE> RvalueUtil::move(Rvalue<TYPE> rvalue) {
    return rvalue;
}

// ----------------------------------------------------------------------------

#endif // support rvalue references and alias templates

// ----------------------------------------------------------------------------

template <class TYPE>
inline const TYPE& RvalueUtil::moveIfNoexcept(TYPE& lvalue) {
    return lvalue;
}

// ----------------------------------------------------------------------------

}  // close package namespace

}  // close enterprise namespace

// ----------------------------------------------------------------------------

#endif

// ----------------------------------------------------------------------------
// Copyright 2015 Bloomberg Finance L.P.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// ----------------------------- END-OF-FILE ----------------------------------
