#include "NiObjects.h"

void NiAVObject::Dump(UInt32 level, const char * indent)
{
	UInt32 childCount = 0;
	char locIndent[257];
	strcpy_s(locIndent, 257, indent);
	if (strlen(indent) < 254)
		strcat_s(locIndent, 257, "  ");
	NiNode* niNode = GetAsNiNode();
	if (niNode)
		childCount = niNode->m_children.Length();
	_MESSAGE("[%0.4d]%s - name: '%s' [%0.8X] has %d children", level, locIndent, m_pcName, niNode, childCount);
	for (UInt32 i = 0; i < childCount; i++)
	{
		NiAVObject* child = niNode->m_children[i];
		child->Dump(level+1, locIndent);
	}
	if (0 == level)
		_MESSAGE("\n");
}
