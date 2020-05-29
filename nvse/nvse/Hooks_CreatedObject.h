// hooks and structures to support saving/loading created base objects list

#pragma once
#include "GameData.h"
#include "GameForms.h"
#include "Serialization.h"

struct FakeModInfo : public ModInfo
{
	FakeModInfo(NVSESerializationInterface* _intfc);
	~FakeModInfo();

	NVSESerializationInterface* intfc;	// alternative: Fake BSFile redefining virtual funcs to use serialization intfc

	// hooked methods
	void	InitializeForm(TESForm* form);
	UInt32	GetChunkType();
	bool	GetChunkData(void* buf, UInt32 bufsize);
	bool	NextSubRecord();	// return true on success
	bool	ReadFormInfo();		// as above
	bool	ReadHeader();		// as above

	static FakeModInfo* Get();
	UInt32 SetStaticFieldsAndGetFormTypeEnum(UInt32 aChunkType);
};


void Hook_CreatedObjects_Init();

bool LoadCreatedObject(NVSESerializationInterface* nvse);
void SaveCreatedObjects(NVSESerializationInterface* nvse);

/*********************************

LoadForm routines

#Basic process:

TESForm::LoadForm(ModInfo*)
	modInfo->InitializeForm(form)
		read record TYPE (UInt32) and size (UInt16) from stream
		set static fields 106AA30 (formType enum), 106AA34 (record TYPE)
		read ModInfo::FormInfo struct (0x18 bytes) from stream
			check if formID reserved (< 0x800)
			resolve refID
		set form flags, typeID, call form->SetFormID(refID, 0)
		form->sub_4560D0(modInfo) - finish up <-- TODO: investigate this
	while (modInfo->GetSubrecord) <-- returns record TYPE
		switch (TYPE)
			read data based on type
	-DONE-

#stuff to investigate on ModInfo:
	Is dataBuf ever used?
	+268 = offset of current chunk into databuf?
	+424 = size of entire record (but is zero when +244 is size...) used when not loading a form? probably irrelevant
	+400 = ModInfo array? Test with altered mod load order. Relevant to cloned forms? Probably, if form references other forms
		-> can probably fix up refIDs without this
	
#other stuff:
	TESForm::Unk_38(modInfo*) virtual func
	TESForm::FinishInit(modinfo*)

#ModInfo routines:

ModInfo::GetRefModInfo?(UInt8 modIndex)	// *** modIndex is modIndex + 1. (Why? -> means 0xFF invalid...)
{
	if modIndex > unk3FC || modIndex == 0
		return NULL
	else if unk400 != NULL
		return unk400[modIndex*4 - 4]
	else	
		return NULL
}

// no need to override this
UInt8 ModInfo::SetStaticFieldsAndGetFormTypeEnum(UInt32 recordType)	// returns typeID
{
	if recordType == s_curChunkType	
		return s_curFormEnum
	
	//TODO: investigate this (need address of array and struct def for elements)
	look up type enum
	set s_curFormEnum, s_curChunkType
	return s_curFormEnum
}	

ModInfo::ReadFormInfoStruct()
{
	BSFile->readFile(&formInfo, 0x18)
	if numRead != 0x18
		set all formInfo fields to zero
		return false
	else if bBigEndian	
		swap byte order
	
	if (unk400 != 0)
		UInt8 modIndex = curFormID >> 0x18 + 1
		ModInfo* refInfo = GetRefModInfo?(modIndex)	// returns a ModInfo*
		if refInfo
			formInfo->formID &= 0x00FFFFFF
			formInfo->formID |= (refInfo->modIndex << 0x18)
		else
			formID &= 0x00FFFFFF; formID |= modIndex << 0x18		// use this->modIndex
	else
		formInfo->formID &= 0x00FFFFFF; formID |= (modIndex << 0x18)
	if (IsFormIDReserved(formID && 0x00FFFFFF)		// checks if below 0x800
		formID &= 0x00FFFFFF						// set modIndex to 0
		
	return true
}

ModInfo::InitBuffer()
{
	....this does not appear to be used - buffer always empty?
}

bool ModInfo::NextSubRecord()
{
	UInt32 formType = formInfo->type
	UInt32 recordLen = formInfo->unk004
	
	if (type != 'GRUP' && flags & 40000 != 0)
		recordLen = unk424
	
	dataOffset += subrecord.size + sizeof(ChunkHeader)	// sizeof(ChunkHeader) = 6
	if (dataOffset >= recordLen)
		return false
	
	if (formtype == 'GRUP' || flags & 40000 == 0)	// shouldn't need to worry about this - no GRUP records
		bsfile->SetFilePointer(fileOffset + dataOffset + sizeof(FormInfo))	// sizeof(FormInfo) = 0x18
	
	unk26C = 0
	subrecord.size = subrecord.type = 0
	this->ReadChunkHeader()
	return true;
}

bool ModInfo::ReadChunkHeader()
{
	ChunkHeader hdr
	
	if formInfo->recordType == 'GRUP' || flags & 40000 == 0)
		bsfile->Read(&hdr, 6)
	else
		if (dataBuf == 0)
			this->InitBuffer()		// sub_447A60
		
		if (dataBuf == 0)	
			return false;
			
		if (+268 >= +424)		// record offset exceeds max size?
			return false
		else
			hdr = *(ChunkHeader*)(dataBuf[subrecordOffset])
			
	subrecordType = hdr.type		// yes, really subrecord (+258)
	subrecordSize = hdr.size		// +25C	
	if (subrecordType != 'XXXX')
		return true;
	else
		......probably don't need to worry about this case
}
	
ModInfo::InitializeForm(form)
{
	if formInfo->recordType == 0
		if (ReadFormInfoStruct())
			subrecordType = subrecordSize = 0
			ReadChunkHeader()
		else	// readforminfo returned false
			// do nothing - formInfo->recordType remains zero
			
	if formInfo->recordType
		UInt8 typeID = SetStaticFieldsAndGetFormTypeEnum(formInfo->recordType)
		form->typeID = typeID
	else
		form->typeID = 0
		
	if (form->flags & ( 1 << 0x0E))	// bit 0E is set
		form->flags = formInfo->flags | 0x4000
	else
		form->flags = formInfo->flags
		
	form->SetFormID(formInfo->formID, 1)	// virtual function call
	
	form->FinishInit(modInfo)		// what's this do?
}

// using TESFaction, other forms use similar process
TESFaction::LoadForm(modInfo)
{
	modInfo->InitializeForm(this)
	
**************************/
