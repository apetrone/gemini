// -------------------------------------------------------------
// Copyright (C) 2014- Adam Petrone

// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM,OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
// -------------------------------------------------------------

#pragma once

using namespace Sqrat;
template<class C>
class EntityAllocator {
	
    static SQInteger setInstance(HSQUIRRELVM vm, C* instance)
    {
        sq_setinstanceup(vm, 1, instance);
        sq_setreleasehook(vm, 1, &Delete);
        return 0;
    }
	
    template <class T, bool b>
    struct NewC
    {
        T* p;
        NewC()
        {
			p = new T();
        }
    };
	
    template <class T>
    struct NewC<T, false>
    {
        T* p;
        NewC()
        {
			p = 0;
        }
    };
	
public:
    static SQInteger New(HSQUIRRELVM vm) {
        C* instance = NewC<C, is_default_constructible<C>::value >().p;
        setInstance(vm, instance);
        return 0;
    }
	
    template <int count>
    static SQInteger iNew(HSQUIRRELVM vm) {
        return New(vm);
    }
	
	// following New functions are used only if constructors are bound via Ctor() in class
	
    template <typename A1>
    static SQInteger iNew(HSQUIRRELVM vm) {
        Var<A1> a1(vm, 2);
        if (Error::Instance().Occurred(vm)) {
            return sq_throwerror(vm, Error::Instance().Message(vm).c_str());
        }
        return setInstance(vm, new C(
									 a1.value
									 ));
    }
    template <typename A1,typename A2>
    static SQInteger iNew(HSQUIRRELVM vm) {
        Var<A1> a1(vm, 2);
        Var<A2> a2(vm, 3);
        if (Error::Instance().Occurred(vm)) {
            return sq_throwerror(vm, Error::Instance().Message(vm).c_str());
        }
        return setInstance(vm, new C(
									 a1.value,
									 a2.value
									 ));
    }
    template <typename A1,typename A2,typename A3>
    static SQInteger iNew(HSQUIRRELVM vm) {
        Var<A1> a1(vm, 2);
        Var<A2> a2(vm, 3);
        Var<A3> a3(vm, 4);
        return setInstance(vm, new C(
									 a1.value,
									 a2.value,
									 a3.value
									 ));
    }
    template <typename A1,typename A2,typename A3,typename A4>
    static SQInteger iNew(HSQUIRRELVM vm) {
        Var<A1> a1(vm, 2);
        Var<A2> a2(vm, 3);
        Var<A3> a3(vm, 4);
        Var<A4> a4(vm, 5);
        if (Error::Instance().Occurred(vm)) {
            return sq_throwerror(vm, Error::Instance().Message(vm).c_str());
        }
        return setInstance(vm, new C(
									 a1.value,
									 a2.value,
									 a3.value,
									 a4.value
									 ));
    }
    template <typename A1,typename A2,typename A3,typename A4,typename A5>
    static SQInteger iNew(HSQUIRRELVM vm) {
        Var<A1> a1(vm, 2);
        Var<A2> a2(vm, 3);
        Var<A3> a3(vm, 4);
        Var<A4> a4(vm, 5);
        Var<A5> a5(vm, 6);
        if (Error::Instance().Occurred(vm)) {
            return sq_throwerror(vm, Error::Instance().Message(vm).c_str());
        }
        return setInstance(vm, new C(
									 a1.value,
									 a2.value,
									 a3.value,
									 a4.value,
									 a5.value
									 ));
    }
    template <typename A1,typename A2,typename A3,typename A4,typename A5,typename A6>
    static SQInteger iNew(HSQUIRRELVM vm) {
        Var<A1> a1(vm, 2);
        Var<A2> a2(vm, 3);
        Var<A3> a3(vm, 4);
        Var<A4> a4(vm, 5);
        Var<A5> a5(vm, 6);
        Var<A6> a6(vm, 7);
        if (Error::Instance().Occurred(vm)) {
            return sq_throwerror(vm, Error::Instance().Message(vm).c_str());
        }
        return setInstance(vm, new C(
									 a1.value,
									 a2.value,
									 a3.value,
									 a4.value,
									 a5.value,
									 a6.value
									 ));
    }
    template <typename A1,typename A2,typename A3,typename A4,typename A5,typename A6,typename A7>
    static SQInteger iNew(HSQUIRRELVM vm) {
        Var<A1> a1(vm, 2);
        Var<A2> a2(vm, 3);
        Var<A3> a3(vm, 4);
        Var<A4> a4(vm, 5);
        Var<A5> a5(vm, 6);
        Var<A6> a6(vm, 7);
        Var<A7> a7(vm, 8);
        if (Error::Instance().Occurred(vm)) {
            return sq_throwerror(vm, Error::Instance().Message(vm).c_str());
        }
        return setInstance(vm, new C(
									 a1.value,
									 a2.value,
									 a3.value,
									 a4.value,
									 a5.value,
									 a6.value,
									 a7.value
									 ));
    }
    template <typename A1,typename A2,typename A3,typename A4,typename A5,typename A6,typename A7,typename A8>
    static SQInteger iNew(HSQUIRRELVM vm) {
        Var<A1> a1(vm, 2);
        Var<A2> a2(vm, 3);
        Var<A3> a3(vm, 4);
        Var<A4> a4(vm, 5);
        Var<A5> a5(vm, 6);
        Var<A6> a6(vm, 7);
        Var<A7> a7(vm, 8);
        Var<A8> a8(vm, 9);
        if (Error::Instance().Occurred(vm)) {
            return sq_throwerror(vm, Error::Instance().Message(vm).c_str());
        }
        return setInstance(vm, new C(
									 a1.value,
									 a2.value,
									 a3.value,
									 a4.value,
									 a5.value,
									 a6.value,
									 a7.value,
									 a8.value
									 ));
    }
    template <typename A1,typename A2,typename A3,typename A4,typename A5,typename A6,typename A7,typename A8,typename A9>
    static SQInteger iNew(HSQUIRRELVM vm) {
        Var<A1> a1(vm, 2);
        Var<A2> a2(vm, 3);
        Var<A3> a3(vm, 4);
        Var<A4> a4(vm, 5);
        Var<A5> a5(vm, 6);
        Var<A6> a6(vm, 7);
        Var<A7> a7(vm, 8);
        Var<A8> a8(vm, 9);
        Var<A9> a9(vm, 10);
        if (Error::Instance().Occurred(vm)) {
            return sq_throwerror(vm, Error::Instance().Message(vm).c_str());
        }
        return setInstance(vm, new C(
									 a1.value,
									 a2.value,
									 a3.value,
									 a4.value,
									 a5.value,
									 a6.value,
									 a7.value,
									 a8.value,
									 a9.value
									 ));
    }
	
public:
	
    static SQInteger Copy(HSQUIRRELVM vm, SQInteger idx, const void* value) {
        C* instance = new C(*static_cast<const C*>(value));
        sq_setinstanceup(vm, idx, instance);
        sq_setreleasehook(vm, idx, &Delete);
        return 0;
    }
	
    static SQInteger Delete(SQUserPointer ptr, SQInteger size) {
		C * e = reinterpret_cast<C*>( ptr );
		e->flags |= Entity::EF_DELETE_INSTANCE;
		
		// this is quite a hack to get the system working well during a shutdown
		if ( !kernel::instance()->is_active() )
		{
			//        C* instance = reinterpret_cast<C*>(ptr);
			//        delete instance;
			delete e;
		}
		
        return 0;
    }
}; // EntityAllocator
