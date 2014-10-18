// -------------------------------------------------------------
// Copyright (C) 2013- Adam Petrone

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

#include <vector>
#include <string>

#include <slim/xlog.h>

#include <gemini/mem.h>

#include <sqrat.h>

#include "script.h"

#include "kernel.h"

#include "core/assets/asset_mesh.h"
#include "scene_graph.h"
#include "physics.h"

enum EntityType
{
	Logic,
	Model,
	Sprite,
	Emitter,
};

typedef std::vector< struct Entity*, GeminiAllocator<struct Entity*> > EntityVector;


const char ENTITY_START_NAME[] = "Start";
const char ENTITY_UPDATE_NAME[] = "Update";
const char ENTITY_FIXED_UPDATE_NAME[] = "FixedUpdate";

// script stuff
struct Entity
{
	enum Flags
	{
		EF_NONE = 0,
		EF_DELETE_INSTANCE,	// remove this instance
	};
	
	HSQOBJECT instance;
	HSQOBJECT class_object;
	
	HSQOBJECT on_fixed_update;
	HSQOBJECT on_update;
	
	uint64_t id;
	std::string name;
	uint8_t type;
	uint32_t flags;
	
	Entity();
	virtual ~Entity();
	
	void fixed_update(float delta_seconds);
	void update();
	
	// bind functions for this object
	void bind_functions();
	virtual void remove();
	
	// get/set functions for script interop
	const std::string & get_name() { return this->name; }
	void set_name( const std::string & object_name ) { this->name = object_name; }
	virtual void native_fixed_update( float delta_seconds );
	virtual void native_update();
	
	glm::vec3 position;
	void set_position(glm::vec3* new_position);
	glm::vec3* get_position();
	
//	assets::Mesh* mesh;
	scenegraph::Node* node;
	physics::RigidBody* body;
		
	// functions for this script object
	void set_model(const char* path);
	

	
}; // Entity


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

template <class Type>
struct EntityList
{
// There's definitely an issue where the vector isn't deallocating
// using the custom allocator. This may be my mis-understanding of the usage
// pattern though. For now, switching this over to non GeminiAllocator.
//	typedef std::vector< Type*, GeminiAllocator<Type*> > EntityVectorType;
	typedef std::vector< Type* > EntityVectorType;
	EntityVectorType objects;
	
	void add( Type * object )
	{
		this->objects.push_back( object );
	} // add
	
	virtual void remove( Type * object )
	{
		for (typename EntityVectorType::iterator it = this->objects.begin(); it != this->objects.end(); ++it )
		{
			Type * obj = (*it);
			
			if ( obj == object )
			{
//				LOGV( "removing from entity list\n" );
				objects.erase( it );
				break;
			}
		}
	} // remove
	
	void clear()
	{
		objects.clear();
	} // clear
	
	void purge()
	{
		for (typename EntityVectorType::iterator it = this->objects.begin(); it != this->objects.end(); ++it )
		{
			Entity * obj = (*it);
			delete obj;
		}
		
		clear();
	} // purge
	
	
	
	Type * find_with_name( const std::string & name )
	{
		for (typename EntityVectorType::iterator it = this->objects.begin(); it != this->objects.end(); ++it )
		{
			Entity * obj = (*it);
			
			if ( name == obj->name )
			{
				return obj;
			}
		}
		
		return 0;
	} // find_with_name
	
	
	Type * object_at_index( size_t index )
	{
		assert(index <= this->count());
		
		return objects[ index ];
	} // object_at_index
	
	size_t count() const
	{
		return objects.size();
	} // count
}; // EntityList


typedef EntityList<Entity> EntityListType;
EntityListType& entity_list();

//template <class Type>
//EntityList<Type> & entity_list()
//{
//	static EntityList<Type> _entity_list;
//	return _entity_list;
//} // entity_list


struct GameRules
{
	HSQOBJECT instance;
	HSQOBJECT class_object;
	
	HSQOBJECT on_start;
	HSQOBJECT on_update;
	HSQOBJECT on_fixed_update;
	
	GameRules()
	{
		LOGV( "GameRules created.\n" );
		
		sq_resetobject( &instance );
		sq_resetobject( &class_object );
		
		// Assumes the OT_INSTANCE is at position 1 in the stack
		SQRESULT res = sq_getstackobj( script::get_vm(), 1, &instance );
		script::check_result(res, "getstackobj");
		
		res = sq_getclass( script::get_vm(), 1 );
		script::check_result(res, "getclass" );
		
		res = sq_getstackobj( script::get_vm(), -1, &class_object );
		script::check_result(res, "getstackobj");
		
		// pop the OT_CLASS
		sq_poptop( script::get_vm() );
		
		this->bind_functions();
	}
	
	void bind_functions()
	{
		this->on_start = script::find_member( this->class_object, ENTITY_START_NAME );
		this->on_update = script::find_member( this->class_object, ENTITY_UPDATE_NAME );
		this->on_fixed_update = script::find_member( this->class_object, ENTITY_FIXED_UPDATE_NAME );
	}
	
	void native_start() {}
	void native_update() {}
	void native_fixed_update( float delta_seconds ) {}
	
	void start()
	{
		if ( sq_isnull(this->on_start) || sq_isnull(this->instance) )
		{
			return;
		}
		
		SQRESULT res;
		sq_pushobject( script::get_vm(), this->on_start );
		sq_pushobject( script::get_vm(), this->instance );
		res = sq_call( script::get_vm(), 1, SQFalse, SQTrue );
		
		sq_pop( script::get_vm(), 1 );
		if ( SQ_FAILED(res) )
		{
			script::check_result( res, "sq_call" );
			sq_pop( script::get_vm(), 1 );
		}
	}
	
	void fixed_update(float delta_seconds)
	{
		if ( sq_isnull(this->on_fixed_update) || sq_isnull(this->instance) )
		{
			return;
		}
		
		SQRESULT res;
		sq_pushobject( script::get_vm(), this->on_fixed_update );
		sq_pushobject( script::get_vm(), this->instance );
		sq_pushfloat( script::get_vm(), delta_seconds );
		res = sq_call( script::get_vm(), 2, SQFalse, SQTrue );
		
		sq_pop( script::get_vm(), 1 );
		if ( SQ_FAILED(res) )
		{
			script::check_result( res, "sq_call" );
			sq_pop( script::get_vm(), 1 );
		}
	}
	
	void update()
	{
		if ( sq_isnull(this->on_update) || sq_isnull(this->instance) )
		{
			return;
		}
		
		SQRESULT res;
		sq_pushobject( script::get_vm(), this->on_update );
		sq_pushobject( script::get_vm(), this->instance );
		res = sq_call( script::get_vm(), 1, SQFalse, SQTrue );
		
		sq_pop( script::get_vm(), 1 );
		if ( SQ_FAILED(res) )
		{
			script::check_result( res, "sq_call" );
			sq_pop( script::get_vm(), 1 );
		}
	}
}; // GameRules



void entity_startup();
void entity_post_script_load();
void entity_shutdown();
void entity_step();
void entity_tick();
void entity_set_scene_root(scenegraph::Node* root);