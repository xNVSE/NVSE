#pragma once

#include <map>
#include "common/IDataStream.h"
#include "common/IErrors.h"

// this screws with edit-and-continue and we don't use it
#define ENABLE_IDYNAMICCREATE 0

#if ENABLE_IDYNAMICCREATE

//! Get a pointer to the IDynamicType for a class.
//! @note This is not a function; the parameter must be constant.
#define GetDynType(name)	(&(##name##::__DYN_DynamicType))

//! Declare the members used for dynamic class creation
#define DYNAMIC_DECLARE(name)															\
public:																					\
	class __DYN_##name##_DynamicType : public IDynamicType								\
	{																					\
		public:																			\
			__DYN_##name##_DynamicType() { }											\
			~__DYN_##name##_DynamicType() { }											\
																						\
			virtual IDynamic *	Create(void)	{ return new name; }					\
			virtual char *		GetName(void)	{ return #name; }						\
			virtual IDynamic *	Instantiate(IDataStream * stream);						\
	};																					\
																						\
	static __DYN_##name##_DynamicType	__DYN_DynamicType;								\
	virtual IDynamicType * __DYN_GetDynamicType(void) { return &__DYN_DynamicType; }	\
																						\
	friend __DYN_##name##_DynamicType;

//! Define the members used for dynamic class creation
#define DYNAMIC_DEFINE(name) name##::__DYN_##name##_DynamicType name##::__DYN_DynamicType;

//! Define a dynamic instantiation handler
#define DYNAMIC_INSTANTIATE_HANDLER(name)		IDynamic * name##::__DYN_##name##_DynamicType::Instantiate(IDataStream * stream) { name * object = new name;
#define END_DYNAMIC_INSTANTIATE_HANDLER			return object; }

//! Specifies that a dynamic class should not be instantiated automatically
#define NO_DYNAMIC_INSTANTIATE_HANDLER(name)	DYNAMIC_INSTANTIATE_HANDLER(name) { HALT("attempted to instantiate " #name); } END_DYNAMIC_INSTANTIATE_HANDLER

//! Casts 
#define CAST(ptr, type)	_DynamicCast <type>(ptr);

class IDynamicType;

/**
 *	Pure virtual base class allowing dynamic creation of objects
 *	
 *	To allow dynamic creation of a class, publicly inherit IDynamic, add the
 *	macro DYNAMIC_DECLARE(classname) first in the class declaration, and add
 *	the macro DYNAMIC_DEFINE(classname) somewhere in the class definition file.
 */
class IDynamic
{
	public:
				IDynamic()	{ }
		virtual ~IDynamic()	{ }

		virtual IDynamicType *	__DYN_GetDynamicType(void) = 0;
};

/**
 *	Pure virtual base class allowing class instantiation and information retrieval
 */
class IDynamicType
{
	public:
				IDynamicType()	{ }
		virtual ~IDynamicType() { }

		virtual IDynamic *	Create(void) = 0;
		virtual char *		GetName(void) = 0;

		virtual IDynamic *	Instantiate(IDataStream * stream) = 0;
};

//! 
template <typename T>
T * _DynamicCast(IDynamic * ptr)
{
	if(ptr && (&T::__DYN_DynamicType == ptr->__DYN_GetDynamicType()))
		return static_cast<T *>(ptr);

	return NULL;
}

/**
 *	Registry of dynamic classes
 */
class IClassRegistry
{
	public:
				IClassRegistry();
				~IClassRegistry();

		static void	RegisterClassInfo(UInt32 id, IDynamicType * typeInfo);

		static IDynamicType *	LookupClassInfo(UInt32 id);
		static IDynamicType *	LookupClassInfo(char * name);

		static IDynamic *		Create(UInt32 id)	{ IDynamicType * info = LookupClassInfo(id); return info ? info->Create() : NULL; }
		static IDynamic *		Create(char * name)	{ IDynamicType * info = LookupClassInfo(name); return info ? info->Create() : NULL; }

		static IDynamic *		Instantiate(UInt32 id, IDataStream * stream)	{ IDynamicType * info = LookupClassInfo(id); return info ? info->Instantiate(stream) : NULL; }
		static IDynamic *		Instantiate(char * name, IDataStream * stream)	{ IDynamicType * info = LookupClassInfo(name); return info ? info->Instantiate(stream) : NULL; }

		static char *			GetName(UInt32 id)	{ IDynamicType * info = LookupClassInfo(id); return info ? info->GetName() : NULL; }

	private:
		typedef std::map <UInt32, IDynamicType *>	ClassRegistryType;
		static ClassRegistryType	theClassRegistry;
};

#endif
