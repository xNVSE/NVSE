#include "IDynamicCreate.h"

#if ENABLE_IDYNAMICCREATE

IClassRegistry	_gClassRegistry;

IClassRegistry::IClassRegistry()
{
	//
}

IClassRegistry::~IClassRegistry()
{
	//
}

void IClassRegistry::RegisterClassInfo(UInt32 id, IDynamicType * typeInfo)
{
	theClassRegistry[id] = typeInfo;
}

IDynamicType * IClassRegistry::LookupClassInfo(UInt32 id)
{
	ClassRegistryType::iterator iter = theClassRegistry.find(id);

	return (iter == theClassRegistry.end()) ? NULL : (*iter).second;
}

IDynamicType * IClassRegistry::LookupClassInfo(char * name)
{
	for(ClassRegistryType::iterator iter = theClassRegistry.begin(); iter != theClassRegistry.end(); iter++)
		if(!strcmp((*iter).second->GetName(), name))
			return (*iter).second;

	return NULL;
}

#endif
