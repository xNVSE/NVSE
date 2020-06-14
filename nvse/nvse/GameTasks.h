#pragma once

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

class BackgroundCloneThread;
class BSFaceGenModel;
class QueuedReference;
class AttachDistant3DTask;
class BSTaskManagerThread;

class RefNiRefObject;
class RefNiObject;
struct BSAData;

// 18
class BSTask
{
public:
	virtual void Destroy(bool doFree);
	virtual void Run(void) = 0;
	virtual void Unk_02(void) = 0;
	virtual void Unk_03(UInt32 arg0, UInt32 arg1);						// doesNothing
	virtual bool GetDebugDescription(char *outDesc, void *arg1) = 0;	// return 0
	
	BSTask		*unk04;		// 04
	UInt32		refCounter; // 08
	UInt32		unk0C;		// 0C	Semaphore/Status
	UInt32		unk10;		// 10	Paired : 10 and 14 for a 64 bit integer
	UInt32		unk14;		// 14

	static UInt32 *GetCounterSingleton();
};

// 18
class IOTask : public BSTask
{
public:
	virtual void Unk_05(void);			// doesNothing
	virtual void Unk_06(void);				
	virtual void Unk_07(UInt32 arg0);	// most (all?) implementations appear to call IOManager::1202D98(this, arg0) eventually

	IOTask();
	~IOTask();
};

// 14
class QueuedChildren : public BSSimpleArray<NiPointer<QueuedFile>>
{
	UInt32	counter;
};

// 28
class QueuedFile : public IOTask
{
public:
	QueuedFile();
	~QueuedFile();

	//Unk_01:	doesNothing
	//Unk_02:	virtual void Call_Unk_0A(void);
	//Unk_03:	implemented
	virtual void Unk_08(void);				// doesNothing
	virtual void Unk_09(UInt32 arg0);
	virtual void Unk_0A(void);				

	// size?
	struct FileEntry
	{
		UInt32		unk00;
		UInt32		unk04;
		UInt32		size;
		UInt32		offset;
	};

	BSTask			*unk18;				// 18	init to tlsData:2B4 not confirmed OBSE for QueuedModel, seen QueuedReference (ref to which model will be attached)
	QueuedReference	*queuedRef;			// 1C	could be last QueuedRef
	QueuedChildren	*queuedChildren;	// 20
	UInt32			*unk24;				// 24	struct, 04 is a base, 08 is a length
};

// 40
class QueuedReference : public QueuedFile
{
public:
	QueuedReference();
	~QueuedReference();

	virtual void Unk_0B(void);			// Initialize validBip01Names (and cretae the 3D model?)
	virtual void Unk_0C(void);
	virtual void Unk_0D(NiNode *arg0);
	virtual bool Unk_0E(void);
	virtual void Unk_0F(void);
	virtual void Unk_10(void);			// doesNothing

	TESObjectREFR	*refr;		// 28 
	RefNiRefObject	*unk2C;		// 2C	OBSE QueuedChildren	*queuedChildren;
	NiRefObject		*unk30;		// 30 
	NiRefObject		*unk34;		// 34 
	RefNiRefObject	*unk38;		// 38 
	UInt32			unk3C;		// 3C	uninitialized
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

	RefQueuedHead	*refQueuedHead;	// 40
	RefNiRefObject	*unk44;			// 44
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

struct Model
{
	const char	*path;		// 00
	UInt32		counter1;	// 04
	UInt32		counter2;	// 08
	NiNode		*niNode;	// 0C
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

template <typename T_Key, typename T_Data> class LockFreeMap
{
public:
	virtual void	Unk_00(void);
	virtual void	Unk_01(void);
	virtual bool	Lookup(T_Key key, T_Data *result);
	virtual void	Unk_03(void);
	virtual void	Unk_04(void);
	virtual bool	EraseKey(T_Key key);
	virtual void	Unk_06(void);
	virtual void	Unk_07(void);
	virtual void	Unk_08(void);
	virtual void	Unk_09(void);
	virtual void	Unk_0A(void);
	virtual void	Unk_0B(void);
	virtual void	Unk_0C(void);
	virtual void	Unk_0D(void);
	virtual void	Unk_0E(void);
	virtual void	Unk_0F(void);
	virtual void	Unk_10(void);
	virtual void	Unk_11(void);
	virtual void	Unk_12(void);

	void		*ptr04;			// 04
	UInt32		numBuckets;		// 08
	void		*ptr0C;			// 0C
	UInt32		unk10;			// 10
	void		*ptr14;			// 14
	UInt32		numItems;		// 18
	UInt32		unk1C;			// 1C
	void		*semaphore;		// 20
};

class AnimIdle;
class Animation;
class QueuedReplacementKFList;
class QueuedHelmet;
class BSFileEntry;
class LoadedFile;

// 30
class ModelLoader
{
	LockFreeMap<const char*, Model*>					*modelMap;			// 00
	LockFreeMap<const char*, KFModel*>					*kfMap;				// 04
	LockFreeMap<TESObjectREFR*, QueuedReference*>		*refMap1;			// 08
	LockFreeMap<TESObjectREFR*, QueuedReference*>		*refMap2;			// 0C
	LockFreeMap<AnimIdle*, QueuedAnimIdle*>				*idleMap;			// 10
	LockFreeMap<Animation*, QueuedReplacementKFList*>	*animMap;			// 14
	LockFreeMap<TESObjectREFR*, QueuedHelmet*>			*helmetMap;			// 18
	void												*attachQueue;		// 1C
	LockFreeMap<BSFileEntry*, QueuedTexture*>			*textureMap;		// 20
	LockFreeMap<const char*, LoadedFile*>				*fileMap;			// 24
	BackgroundCloneThread								*bgCloneThread;		// 28
	UInt8												byte2C;				// 2C
	UInt8												pad2D[3];			// 2D

	static ModelLoader *GetSingleton();
	void QueueReference(TESObjectREFR *refr, UInt32 arg2, UInt32 arg3);
	NiNode *LoadModel(const char *nifPath, UInt32 arg2, UInt8 arg3, UInt32 arg4, UInt8 arg5, UInt8 arg6);
};