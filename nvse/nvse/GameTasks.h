#pragma once

#include "GameTypes.h"
#include "NiTypes.h"
//#include "NiNodes.h"

/*******************************************************
*
* BSTask
*	IOTask
*		QueuedFile
*			QueuedFileEntry
*				QueuedModel
*					QueuedDistantLOD
*					QueuedTreeModel
*				QueuedTexture
*					QueuedTreeBillboard
*				QueuedKF
*					QueuedAnimIdle
*				DistantLODLoaderTask
*				TerrainLODQuadLoadTask
*				SkyTask
*				LipTask
*				GrassLoadTask
*			QueuedReference
*				QueuedTree
*				QueuedActor
*					QueuedCharacter
*						QueuedPlayer
*					QueuedCreature
*			QueuedHead
*			QueuedHelmet
*			QueuedMagicItem
*		AttachDistant3DTask
*		ExteriorCellLoaderTask
*
* NiTArray< NiPointer<QueuedFile> >
*	QueuedChildren
*
*********************************************************/

class TESObjectREFR;
class TESModel;
class QueuedCharacter;
class TESNPC;
class BSFaceGenNiNode;
class BackgroundCloneThread;
class TESAnimGroup;
class BSFaceGenModel;
class QueuedChildren;
class QueuedReference;
class Character;
class AttachDistant3DTask;
class BSTaskManagerThread;

class ModelLoader;

class NiNode;
class NiControllerSequence;
class NiRefObject;
class RefNiObject;

class BSAnimGroupSequence;
struct BSAData;

class RefNiRefObject
{
public:
	NiRefObject*	niRefObject;
	
	//	RefNiRefObject SetNiRefObject(NiRefObject* niRefObject);
};

// 18
class BSTask
{
public:
	virtual void Destroy(bool doFree);
	virtual void Run(void) = 0;
	virtual void Unk_02(void) = 0;
	virtual void Unk_03(UInt32 arg0, UInt32 arg1);						// doesNothing
	virtual bool GetDebugDescription(char* outDesc, UInt32 * arg1) = 0;	// return 0

	// void		** vtbl
	
	BSTask	* unk004;	// uninitialized OBSE, not confirmed, NiRefObject
	UInt32	refCounter; // Counter: NiRefObject RefCounter
	UInt32	unk00C;		// Semaphore/Status
	UInt32	unk010;		// Paired : 10 and 14 as a 64 bit integer, it could be very large flag bits or an integer
	UInt32	unk014;

	static UInt32*	GetCounterSingleton();
};

// 18
class IOTask : public BSTask
{
public:
	virtual void Unk_05(void);			// doesNothing
	virtual void Unk_06(void);				
	virtual void Unk_07(UInt32 arg0);	// most (all?) implementations appear to call IOManager::00C3DF40(this, arg0) eventually. It updates the bits 23/32 of the giant bit flag possibly.

	IOTask();
	~IOTask();

};

class QueuedFile;

// 014
class QueuedChildren : public BSSimpleArray<NiPointer<QueuedFile>>
{
	UInt32	counter;
};

// 028
class QueuedFile : public IOTask
{
public:
	QueuedFile();
	~QueuedFile();

	//Unk_01:	doesNothing
	//Unk_02:	virtual void Call_Unk_0A(void);
	//Unk_03:	implemented
	//Unk_07:	recursivly calls Unk_07(arg_0) on all non null children before calling its parent.
	virtual void Unk_08(void);				// doesNothing
	virtual void Unk_09(UInt32 arg0);
	virtual void Unk_0A(void);				

	// size?
	struct FileEntry {
		UInt32		unk00;
		UInt32		unk04;
		UInt32		size;
		UInt32		offset;
	};

	BSTask			* unk018;			// 018 init to tlsData:2B4 not confirmed OBSE for QueuedModel, seen QueuedReference (ref to which model will be attached)
	QueuedReference	* queuedRef;		// 01C could be last QueuedRef
	QueuedChildren	* queuedChildren;	// 020
	UInt32			* unk024;			// 024	struct, 004 is a base, 008 is a length
};

// 40
class QueuedReference : public QueuedFile
{
public:
	QueuedReference();
	~QueuedReference();

	virtual void Unk_0B(void);			// Initialize validBip01Names (and cretae the 3D model ?)
	virtual void Unk_0C(void);
	virtual void Unk_0D(NiNode* arg0);
	virtual bool Unk_0E(void);
	virtual void Unk_0F(void);
	virtual void Unk_10(void);			// doesNothing

	TESObjectREFR	* refr;				// 028 
	RefNiRefObject	* unk02C;			// OBSE QueuedChildren	* queuedChildren;	// 02C
	NiRefObject		* unk030;			// 030 
	NiRefObject		* unk034;			// 034 
	RefNiRefObject	* unk038;			// 038 
	UInt32			unk03C;				// 03C uninitialized
};

// 40
class QueuedActor : public QueuedReference
{
public:
	QueuedActor();
	~QueuedActor();
};

// 40
class QueuedCreature : public QueuedActor
{
public:
	QueuedCreature();
	~QueuedCreature();
};

// 48
class QueuedCharacter : public QueuedActor
{
public:
	QueuedCharacter();
	~QueuedCharacter();

	typedef RefNiRefObject RefQueuedHead;

	RefQueuedHead	* refQueuedHead;	// 040
	RefNiRefObject	* unk044;	// 044

};

// 48
class QueuedPlayer : public QueuedCharacter
{
public:
	QueuedPlayer();
	~QueuedPlayer();
};

// 030
class QueuedFileEntry : public QueuedFile
{
public:
	QueuedFileEntry();
	~QueuedFileEntry();

	virtual bool Unk_0B(void) = 0;

	char	* name;		// 028
	BSAData	* bsaData;	// 02C
};

class Model // NiObject
{
	const char	* path;		// 004
	UInt32		counter;	// 008
	NiNode		* ninode;	// 00C
};

// 44
class QueuedModel : public QueuedFileEntry
{
public:
	QueuedModel();
	~QueuedModel();

	virtual void Unk_0C(UInt32 arg0);

	Model		* model;		// 030
	TESModel	* tesModel;		// 034
	UInt32		baseFormClass;	// 038	table at offset : 0x045C708. Pickable, invisible, unpickable ? 6 is VisibleWhenDistant or internal
	UInt8		flags;			// 03C	bit 0 and bit 1 init'd by parms, bit 2 set after textureSwap, bit 3 is model set, bit 4 is file found.
	UInt8		pad03D[3];		// 03D
	float		flt040;			// 040

	// There are at least 3 Create/Initiator
};

// 30
class QueuedTexture : public QueuedFileEntry
{
public:
	QueuedTexture();
	~QueuedTexture();

	void	* niTexture;	// 030
};

// 014
class KFModel
{
	const char			* path;					// 000
	BSAnimGroupSequence	* controllerSequence;	// 004
	TESAnimGroup		* animGroup;			// 008
	UInt32				unk0C;					// 00C
	UInt32				unk10;					// 010
};

// 30
class QueuedKF : public QueuedFileEntry
{
public:
	QueuedKF();
	~QueuedKF();

	KFModel		* kf;		// 030
	UInt8		unk034;		// 034
	UInt8		pad035[3];	// 035
};

// 040
class QueuedAnimIdle : public QueuedKF
{
public:
	QueuedAnimIdle();
	~QueuedAnimIdle();

	ModelLoader	* modelLoader;	// 038	Init"d by arg2
	RefNiObject	* unk03C;		// 03C	Init"d by arg1
};

// 38
class QueuedHead : public QueuedFile
{
public:
	QueuedHead();
	~QueuedHead();

	TESNPC			* npc;				// 028
	BSFaceGenNiNode * faceNiNodes[2];	// 02C OBSE presumably male and female
	UInt32			unk034;				// 034
};

/*
// 38
class QueuedHelmet : public QueuedFile
{
public:
	QueuedHelmet();
	~QueuedHelmet();

	QueuedCharacter		* queuedCharacter;		// 18
	QueuedChildren		* queuedChildren;		// 1C
	void				* unk20;				// 20
	QueuedModel			* queuedModel;			// 24
	BSFaceGenModel		* faceGenModel;			// 28
	NiNode				* niNode;				// 2C
	Character			* character;			// 30
	UInt32				unk34;					// 34
};

// 30
class BSTaskManager : public LockFreeMap< NiPointer< BSTask > >
{
public:
	virtual void Unk_0F(UInt32 arg0) = 0;
	virtual void Unk_10(UInt32 arg0) = 0;
	virtual void Unk_11(UInt32 arg0) = 0;
	virtual void Unk_12(void) = 0;
	virtual void Unk_13(UInt32 arg0) = 0;

	UInt32				unk1C;			// 1C
	UInt32				unk20;			// 20
	UInt32				numThreads;		// 24
	BSTaskManagerThread	** threads;		// 28 array of size numThreads
	UInt32				unk2C;			// 2C
};

// 3C
class IOManager : public BSTaskManager
{
public:
	virtual void Unk_14(UInt32 arg0) = 0;

	static IOManager* GetSingleton();

	UInt32									unk30;			// 30
	LockFreeQueue< NiPointer< IOTask > >	* taskQueue;	// 34
	UInt32									unk38;			// 38

	bool IsInQueue(TESObjectREFR *refr);
	void QueueForDeletion(TESObjectREFR* refr);
	void DumpQueuedTasks();
};

extern IOManager** g_ioManager;
*/

// O4 assumed
class InterfacedClass {
	virtual void Destroy(bool doFree);
	virtual void AllocateTLSValue(void) = 0;		// not implemented
};

// 40
template<typename _K, class _C>
class LockFreeMap: InterfacedClass
{
	// 0C
	struct Data004
	{
		UInt32	unk000;		// 00
		UInt32	unk004;		// 04
		UInt32	*unk008;	// 08
	};

	// 24
	struct TLSValue
	{
		LockFreeMap	*map;				// 00
		UInt32		mapData004Unk000;	// 04
		UInt32		mapData004Unk008;	// 08
		UInt32		*mapData004Unk00C;	// 0C	stores first DWord of bucket during lookup, next pointer is data, next flags bit 0 is status ok/found
		UInt32		unk010;				// 10	stores bucketOffset during lookup
		UInt32		*unk014;			// 14	stores pointer to bucket during lookup
		UInt32		*unk018;			// 18
		UInt32		unk01C;				// 1C
		UInt32		unk020;				// 20
	};

	// 10
	struct Data014
	{
		// 08
		struct Data008
		{
			UInt32		threadID;	// 00 threadID
			TLSValue	*tlsValue;	// 04 lpTlsValue obtained from AllocateTLSValue of LockFreeMap
		};

		UInt32	alloc;			// Init'd to arg0, count of array at 008
		UInt32	tlsDataIndex;	// Init'd by TlsAlloc
		Data008	**dat008;		// array of pair of DWord
		UInt32	count;		// Init'd to 0
	};	// most likely an array or a map

	virtual bool Get(_K key, _C *destination) = 0;
	virtual void Unk_03(void) = 0;
	virtual void Unk_04(void) = 0;
	virtual void Unk_05(void) = 0;
	virtual void Unk_06(void) = 0;
	virtual void Unk_07(void) = 0;
	virtual void Unk_08(void) = 0;
	virtual UInt32 Hash(_K key) = 0;
	virtual void Unk_0A(void) = 0;
	virtual void Unk_0B(void) = 0;
	virtual void Unk_0C(void) = 0;
	virtual void Unk_0D(void) = 0;
	virtual void Unk_0E(void) = 0;
	virtual void Unk_0F(void) = 0;
	virtual void Unk_10(void) = 0;
	virtual void Unk_11(void) = 0;

	Data004	**dat004;		// 04 array of arg0 12 bytes elements (uninitialized)
	UInt32	bucketCount;	// 08 Init'd to arg1, count of DWord to allocate in array at 000C
	UInt32	**buckets;		// 0C array of arg1 DWord elements
	UInt32	unk010;			// 10 Init'd to arg2
	Data014	*dat014;		// 14 Init'd to a 16 bytes structure
	UInt32	unk018;			// 18
	UInt32	unk01C;			// 1C
	UInt32	unk020[2];		// 20 Pair of DWord (tList ?)
	// ?
};

template<class _C>
class LockFreeStringMap: LockFreeMap<char const *, _C> {};

template<class _C>
class LockFreeCaseInsensitiveStringMap: LockFreeStringMap<_C> {};

// 1C
class ModelLoader
{
public:
	ModelLoader();
	~ModelLoader();

	// #TODO: generalize key for LockFreeMap, document LockFreeStringMap

	LockFreeCaseInsensitiveStringMap<Model *>			* modelMap;				// 00
	LockFreeCaseInsensitiveStringMap<KFModel *>			* kfMap;				// 04
	LockFreeMap< TESObjectREFR*, NiPointer<QueuedReference *> >		* refMap;	// 08 key is TESObjectREFR*
	//LockFreeMap< NiPointer<QueuedAnimIdle *> >		* idleMap;					// 0C key is AnimIdle* (strange same constructor as for 08)
	//LockFreeMap< NiPointer<QueuedHelmet *> >			* helmetMap;				// 10 key is TESObjectREFR*
	//LockFreeQueue< NiPointer<AttachDistant3DTask *> >	* distant3DMap;				// 14
	//BackgroundCloneThread								* bgCloneThread;			// 18

	static ModelLoader* GetSingleton();

	void QueueReference(TESObjectREFR* refr, UInt32 arg1, bool ifInMainThread);
};