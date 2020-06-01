#pragma once
#include <emmintrin.h>

struct hkVector4
{
	float	x, y, z, w;
};

union AlignedVector4
{
	hkVector4	unpacked;
	__m128i		packed;
};

template <typename T_Data> class hkArray
{
public:
	enum
	{
		kFlag_NoDealloc = 0x80000000,	// data not owned by us
		kFlag_Locked = 0x40000000,	// will never be destroyed
	};

	T_Data* data;				// 00
	UInt32		size;				// 04
	UInt32		capacityAndFlags;	// 08

	class Iterator
	{
	protected:
		friend hkArray;

		T_Data* pData;
		UInt32		count;

	public:
		bool End() const { return !count; }
		void operator++()
		{
			count--;
			pData++;
		}

		T_Data& operator*() const { return *pData; }
		T_Data& operator->() const { return *pData; }
		T_Data& Get() const { return *pData; }

		Iterator(hkArray& source) : pData(source.data), count(source.size) {}
	};
};

class hkStatisticsCollector;
class hkpSimpleConstraintContactMgr;
class bhkWorldObject;
class NiStream;
class ahkpWorld;
class hkpContinuousSimulation;
class hkpDefaultWorldMaintenanceMgr;
class hkpBroadPhase;
class hkpBroadPhaseListener;
class hkpPhantomBroadPhaseListener;
class hkpEntityEntityBroadPhaseListener;
class hkpBroadPhaseBorderListener;
class hkpCollisionDispatcher;
class bhkCollisionFilter;
class hkpDefaultConvexListFilter;
class hkpEntityListener;
class hkpContactListener;
class bhkShape;
class hkpCachingShapePhantom;

// 04
class hkBaseObject
{
public:
	virtual void	Destroy(bool doFree);
};

// 08
class hkReferencedObject : public hkBaseObject
{
public:
	hkReferencedObject();
	~hkReferencedObject();

	virtual void	CalcStatistics(hkStatisticsCollector* collector);
	virtual void	Unk_02(void);

	UInt16			sizeAndFlags;		// 04
	UInt16			refCount;			// 06
};

// 04
class hkpEntityListener
{
public:
	hkpEntityListener();
	~hkpEntityListener();

	virtual void	Unk_00(void);
	virtual void	Unk_01(void);
	virtual void	Unk_02(void);
	virtual void	Unk_03(void);
	virtual void	Unk_04(void);
	virtual void	Unk_05(void);
};

// 04
class hkpPhantomListener
{
public:
	hkpPhantomListener();
	~hkpPhantomListener();

	virtual void	Unk_00(void);
	virtual void	Unk_01(void);
	virtual void	Unk_02(void);
	virtual void	Unk_03(void);
	virtual void	Unk_04(void);
};

// D0
class hkpCharacterProxy : public hkReferencedObject
{
public:
	hkpCharacterProxy();
	~hkpCharacterProxy();

	virtual void	Unk_03(void);
	virtual void	Unk_04(void);

	hkpEntityListener		entityListener;		// 08
	hkpPhantomListener		phantomListener;	// 0C

	float					velocity[3];		// 10
	float					unk1C;				// 1C
	UInt32					unk20[4];			// 20
	hkpCachingShapePhantom* shapePhantom;		// 30
	UInt32					unk34[39];			// 34
};
STATIC_ASSERT(sizeof(hkpCharacterProxy) == 0xD0);

// D0
class ahkpCharacterProxy : public hkpCharacterProxy
{
public:
	ahkpCharacterProxy();
	~ahkpCharacterProxy();
};

// 10
struct hkCdBody
{
	hkReferencedObject* shape;			// 00
	UInt32					shapeKey;		// 04
	void* motion;		// 08 - either hkTransform or hkMotionState
	hkCdBody* parent;		// 0C

	hkpWorldObject* GetWorldObj() const { return (hkpWorldObject*)((UInt8*)this - 0x10); }
};

// 80
struct ContactData
{
	UInt16							unk00;
	UInt16							flags;
	UInt32							key;
	hkpSimpleConstraintContactMgr* contactMgr;
	UInt32							unk0C;
	hkCdBody* cdBody1;
	hkCdBody* cdBody2;
	UInt32							unk18[26];
};

// 08
struct CdBodyLinker
{
	ContactData* data;
	hkCdBody* cdBody;
};

// 10
struct CdParentObj
{
	void* unk00;
	UInt32		unk04;
	void* object;
	UInt32		unk0C;
};

// 8C
class hkpWorldObject : public hkReferencedObject
{
public:
	hkpWorldObject();
	~hkpWorldObject();

	virtual void	Unk_03(void);
	virtual void	Unk_04(void);

	hkpWorld* pWorld;		// 08
	bhkWorldObject* object;		// 0C
	hkCdBody				cdBody;			// 10
	UInt32					unk20[2];		// 20
	UInt8					collisionType;	// 28	Known: 1 - hkpRigidBody, 2 - hkpPhantom
	UInt8					unk29[3];		// 29
	UInt8					layerType;		// 2C
	UInt8					filterFlags;	// 2D
	UInt8					unk2E[2];		// 2E
	UInt32					unk30[12];		// 30
	hkArray<CdBodyLinker>	cdRgdBodies;	// 60
	UInt32					unk6C[3];		// 6C
	hkArray<CdParentObj>	parentObjs;		// 78
	UInt32					unk84[2];		// 84
};
STATIC_ASSERT(sizeof(hkpWorldObject) == 0x8C);

// 10
class hkpShape : public hkReferencedObject
{
public:
	hkpShape();
	~hkpShape();

	virtual void	Unk_03(void);
	virtual void	Unk_04(void);
	virtual void	Unk_05(void);
	virtual void	Unk_06(void);
	virtual void	Unk_07(void);
	virtual void	Unk_08(void);
	virtual void	Unk_09(void);
	virtual void	Unk_0A(void);
	virtual void	Unk_0B(void);

	bhkShape* shape;		// 08
	UInt32			unk0C;		// 0C
};

// 0C
class bhkRefObject : public NiObject
{
public:
	bhkRefObject();
	~bhkRefObject();

	virtual void	SetObject(hkReferencedObject* object);
	virtual void	UpdateRefCount(bool incRef);	// inc/dec ref, depending on arg

	hkReferencedObject* refObject;		// 08
};

// 10
class bhkSerializable : public bhkRefObject
{
public:
	bhkSerializable();
	~bhkSerializable();

	virtual void		Unk_25(UInt32 arg);
	virtual hkpWorld* GetWorld(void);
	virtual bool		Unk_27(UInt32 arg);
	virtual bool		Unk_28(void);
	virtual void		FreeData(bool del);	// free hkData
	virtual UInt32		Unk_2A(void);
	virtual void		LoadHavokData(NiStream* stream);	// called from bhkSerializable::Load after primary load is done
	virtual void		Unk_2C(void);	// create object
	virtual void* CreateHavokData(UInt8* arg);	// create Cinfo, return hkData
	virtual void		Unk_2E(void);		// destroy object
	virtual void		Unk_2F(void);
	virtual void		Unk_30(void);

	void* hkData;	// 0C - stores hkConstraintData (descriptor used to build hkObj)
};

// 14
class bhkWorldObject : public bhkSerializable
{
public:
	bhkWorldObject();
	~bhkWorldObject();

	virtual void		Unk_31(void);
	virtual void		Unk_32(void);
	virtual void		Unk_33(void);
	virtual void		Unk_34(void);

	UInt32				bodyFlags;		// 10

	void ApplyForce(hkVector4* forceVector);
};

// 18
class bhkPhantom : public bhkWorldObject
{
public:
	bhkPhantom();
	~bhkPhantom();

	UInt32				unk14;		// 14
};

// 18
class bhkShapePhantom : public bhkPhantom
{
public:
	bhkShapePhantom();
	~bhkShapePhantom();
};

// 18
class bhkSimpleShapePhantom : public bhkShapePhantom
{
public:
	bhkSimpleShapePhantom();
	~bhkSimpleShapePhantom();
};

class bhkCachingShapePhantom : public bhkShapePhantom
{
public:
	bhkCachingShapePhantom();
	~bhkCachingShapePhantom();
};

// 10
class hkpTransformShape : public hkpShape
{
public:
	hkpTransformShape();
	~hkpTransformShape();
};

// 14
class bhkShape : public bhkSerializable
{
public:
	bhkShape();
	~bhkShape();

	virtual void	Unk_31(void);
	virtual void	Unk_32(void);
	virtual void	Unk_33(void);
	virtual void	Unk_34(void);
	virtual void	Unk_35(void);
	virtual void	Unk_36(void);
	virtual void	Unk_37(void);
	virtual void	Unk_38(void);

	UInt32			materialType;	// 10
};

// 14
class bhkTransformShape : public bhkShape
{
public:
	bhkTransformShape();
	~bhkTransformShape();
};

struct hkpCdBodyPair
{
	hkCdBody* cdBody1;
	UInt32			unk04;
	hkCdBody* cdBody2;
	UInt32			unk0C;
};

// 14
class hkpCdBodyPairCollector
{
public:
	hkpCdBodyPairCollector();
	~hkpCdBodyPairCollector();

	virtual void	Destroy(bool doFree);
	virtual void	Unk_01(void);
	virtual void	Unk_02(void);

	UInt32					unk04;			// 04
	hkArray<hkpCdBodyPair>	cdPairs;		// 08
};

// 114
class hkpAllCdBodyPairCollector : public hkpCdBodyPairCollector
{
public:
	hkpAllCdBodyPairCollector();
	~hkpAllCdBodyPairCollector();

	hkpCdBodyPair			cdPairsLoc[16];	// 14
};
STATIC_ASSERT(sizeof(hkpAllCdBodyPairCollector) == 0x114);

// A4
class hkpPhantom : public hkpWorldObject
{
public:
	hkpPhantom();
	~hkpPhantom();

	virtual UInt8	PhantomType();
	virtual void	Unk_06(void);
	virtual void	Unk_07(void);
	virtual void	Unk_08(void);
	virtual void	Unk_09(void);
	virtual void	Unk_0A(void);
	virtual void	Unk_0B(void);
	virtual void	Unk_0C(void);
	virtual void	Unk_0D(void);

	UInt32			unk8C[6];		// 8C
};
STATIC_ASSERT(sizeof(hkpPhantom) == 0xA4);

// E0
class hkpAabbPhantom : public hkpPhantom
{
public:
	hkpAabbPhantom();
	~hkpAabbPhantom();

	UInt32					unkA4[11];	// A4
	hkArray<hkCdBody*>		cdBodies;	// D0
	UInt32					unkDC;		// DC
};
STATIC_ASSERT(sizeof(hkpAabbPhantom) == 0xE0);

// 160
class hkpShapePhantom : public hkpPhantom
{
public:
	hkpShapePhantom();
	~hkpShapePhantom();

	virtual void	Unk_0E(void);
	virtual void	Unk_0F(void);
	virtual void	Unk_10(hkpCdBodyPairCollector* pairCollector, UInt32 arg2);
	virtual void	GetCdBodyData(hkpCdBodyPairCollector* pairCollector, UInt32 arg2);

	UInt32				unk0A4[47];		// 0A4
};
STATIC_ASSERT(sizeof(hkpShapePhantom) == 0x160);

// 170
class hkpSimpleShapePhantom : public hkpShapePhantom
{
public:
	hkpSimpleShapePhantom();
	~hkpSimpleShapePhantom();

	hkArray<hkCdBody*>		cdBodies;	// 160
	UInt32					unk16C;		// 16C
};
STATIC_ASSERT(sizeof(hkpSimpleShapePhantom) == 0x170);

// 170
class hkpCachingShapePhantom : public hkpShapePhantom
{
public:
	hkpCachingShapePhantom();
	~hkpCachingShapePhantom();

	hkArray<CdBodyLinker>	cdBodies;	// 160
	UInt32					unk16C;		// 16C
};
STATIC_ASSERT(sizeof(hkpCachingShapePhantom) == 0x170);

class hkpConstraintOwner : public hkReferencedObject
{
public:
	hkpConstraintOwner();
	~hkpConstraintOwner();

	virtual void	Unk_03(void);
	virtual void	Unk_04(void);
	virtual void	Unk_05(void);
	virtual void	Unk_06(void);
};

// 6C
class hkpSimulationIsland : public hkpConstraintOwner
{
public:
	hkpSimulationIsland();
	~hkpSimulationIsland();

	UInt32					unk08[4];		// 08
	ahkpWorld* world;			// 18
	UInt32					unk1C[11];		// 1C
	hkArray<hkpRigidBody*>	rigidBodies;	// 48
	hkpRigidBody* rgdBody54;		// 54
	UInt32					unk58;			// 58
	hkArray<ContactData*>	contactData;	// 5C
	ContactData* data68;		// 68
};

// 1B8
class hkpWorld : public hkReferencedObject
{
public:
	hkpWorld();
	~hkpWorld();

	hkpContinuousSimulation* simulation;				// 008
	float								flt00C;						// 00C
	hkVector4							gravity;					// 010
	hkpSimulationIsland* fixedIsland;				// 020
	hkpRigidBody* fixedRigidBody;			// 024
	hkArray<hkpSimulationIsland*>		activeSimulationIslands;	// 028
	hkArray<hkpSimulationIsland*>		inactiveSimulationIslands;	// 034
	hkArray<hkpSimulationIsland*>		dirtySimulationIslands;		// 040
	hkpDefaultWorldMaintenanceMgr* maintenanceMgr;			// 04C
	UInt32								unk050;						// 050
	UInt32								unk054;						// 054
	hkpBroadPhase* broadPhase;				// 058
	UInt32								unk05C;						// 05C
	UInt32								unk060;						// 060
	hkpBroadPhaseListener** broadPhaseListeners;		// 064
	hkpPhantomBroadPhaseListener* phantomBroadPhaseListener;	// 068
	hkpEntityEntityBroadPhaseListener* entityBroadPhaseListener;	// 06C
	hkpBroadPhaseBorderListener* broadPhaseBorderListener;	// 070
	void* ptr074;					// 074
	void* ptr078;					// 078
	bhkCollisionFilter* collisionFilter;			// 07C
	hkpCollisionDispatcher* collisionDispatcher;		// 080
	hkpDefaultConvexListFilter* convexListFilter;			// 084
	void* ptr088;					// 088
	UInt32								unk08C[13];					// 08C
	void* ptr0C0;					// 0C0
	UInt32								unk0C4[19];					// 0C4
	hkArray<hkpPhantom*>				phantoms;					// 110
	UInt32								unk11C[3];					// 11C
	hkArray<hkpEntityListener*>			entityListeners1;			// 128
	hkArray<hkpEntityListener*>			entityListeners2;			// 134
	UInt32								unk140[6];					// 140
	hkArray<hkpContactListener*>		contactListeners;			// 158
	UInt32								unk164[15];					// 164
	hkArray<hkpContactListener*>		collisionListeners;			// 1A0
	UInt32								unk1AC[3];					// 1AC
};
STATIC_ASSERT(sizeof(hkpWorld) == 0x1B8);

// 354
class ahkpWorld : public hkpWorld
{
public:
	ahkpWorld();
	~ahkpWorld();

	UInt32			unk1B8[83];			// 1B8
	UInt8			byte304;			// 304
	UInt8			byte305;			// 305
	UInt8			byte306;			// 306
	UInt8			byte307;			// 307
	UInt32			unk308[19];			// 308
};
STATIC_ASSERT(sizeof(ahkpWorld) == 0x354);

// 94
class bhkWorld : public bhkSerializable
{
public:
	bhkWorld();
	~bhkWorld();

	void* unk10;		// 10 - 0x108 byte object
	NiRefObject* visDebug;	// 14
	UInt8				unk18;		// 18
	UInt8				enabled;	// 19
	UInt8				unk1A;		// 1A
	UInt8				pad1B;		// 1B
	UInt32				unk1C[3];	// 1C
	void* unk28;		// 28 - 0x2EE0 byte buffer
	UInt32				unk2C;		// 2C
	void* unk30;		// 30 - 0x320 byte buffer
	UInt32				unk34;		// 34
	void* unk38;		// 38 - 0x190 byte buffer
	UInt32				unk3C;		// 3C
	void* unk40;		// 40 - 0x320 byte buffer
	UInt32				unk44;		// 44
	void* unk48;		// 48 - 0x2EE0 byte buffer
	UInt32				unk4C;		// 4C
	NiVector4			unk50;		// 50
	NiRefObject** unk60;	// 60 - simple array
	UInt32				unk64;		// 64 - num elements in 060
	UInt32				unk68;		// 68
	void* unk6C;		// 6C
	UInt32				unk70[9];	// 70
};
STATIC_ASSERT(sizeof(bhkWorld) == 0x94);

// E0
class bhkWorldM : public bhkWorld
{
public:
	bhkWorldM();
	~bhkWorldM();

	float			unk94;			// 94
	float			unk98;			// 98
	float			unk9C;			// 9C
	NiVector4		borderSize;		// A0
	NiVector4		worldTotalSize;	// B0
	NiVector4		vectorC0;		// C0
	UInt32			unkD0[4];		// D0
};
STATIC_ASSERT(sizeof(bhkWorldM) == 0xE0);

// 0C
class NiCollisionObject : public NiObject
{
public:
	NiCollisionObject();
	~NiCollisionObject();

	virtual void	Attach(NiAVObject* obj);
	virtual void	Unk_24(UInt32 arg);
	virtual void	Unk_25(void);
	virtual void	LoadBoundingVolume(UInt32 arg);
	virtual void	Unk_27(UInt32 version, UInt32 arg1);

	NiNode* linkedNode;	// 08
};

// 14
class bhkNiCollisionObject : public NiCollisionObject
{
public:
	bhkNiCollisionObject();
	~bhkNiCollisionObject();

	virtual void	Unk_28(void);
	virtual void	Unk_29(void);
	virtual void	Unk_2A(void);
	virtual void	Unk_2B(void);
	virtual void	Unk_2C(void);
	virtual void	Unk_2D(void);
	virtual void	Unk_2E(void);
	virtual void	Unk_2F(void);
	virtual void	Unk_30(void);

	UInt32			flags;			// 0C
	bhkWorldObject* worldObj;		// 10
};

// 14
class bhkCollisionObject : public bhkNiCollisionObject
{
public:
	bhkCollisionObject();
	~bhkCollisionObject();
};

// 14
class bhkPCollisionObject : public bhkNiCollisionObject
{
public:
	bhkPCollisionObject();
	~bhkPCollisionObject();
};

// 14
class bhkSPCollisionObject : public bhkPCollisionObject
{
public:
	bhkSPCollisionObject();
	~bhkSPCollisionObject();
};

// 08
class hkpCdPointCollector
{
public:
	hkpCdPointCollector();
	~hkpCdPointCollector();

	virtual void	Unk_00(void);
	virtual void	Unk_01(void);
	virtual void	Unk_02(void);

	float			earlyOutDistance;	// 04
};

// 70
struct hkCdPoint
{
	hkVector4		point00;		// 00
	hkVector4		point10;		// 10
	hkCdBody* cdBody1;		// 20
	UInt8			body1Key;		// 24
	UInt8			pad25[2];		// 25
	UInt8			obj1Flag;		// 27
	hkVector4		point28;		// 28
	hkVector4		point38;		// 38
	hkCdBody* cdBody2;		// 48
	UInt8			body2Key;		// 4C
	UInt8			pad4D[2];		// 4D
	UInt8			obj2Flag;		// 4F
	hkVector4		point50;		// 50
	hkVector4		point60;		// 60
};

// 3A0
class hkpAllCdPointCollector : public hkpCdPointCollector
{
public:
	hkpAllCdPointCollector();
	~hkpAllCdPointCollector();

	virtual void	Unk_03(void);

	UInt32					unk008[2];		// 008
	hkArray<hkCdPoint>		cdPoints;		// 010
	UInt32					unk01C;			// 01C
	hkCdPoint				cdPointsLoc[8];	// 020
};
STATIC_ASSERT(sizeof(hkpAllCdPointCollector) == 0x3A0);

// 3C8
class bhkCharacterPointCollector : public hkpAllCdPointCollector
{
public:
	bhkCharacterPointCollector();
	~bhkCharacterPointCollector();

	UInt32						unk3A0;			// 3A0
	hkArray<hkpWorldObject*>	contactBodies;	// 3A4
	hkArray<UInt32>				arr3B0;			// 3B0
	hkArray<float>				arr3BC;			// 3BC
};
STATIC_ASSERT(sizeof(bhkCharacterPointCollector) == 0x3C8);

// 3E0
class bhkCharacterProxy : public bhkSerializable
{
public:
	bhkCharacterProxy();
	~bhkCharacterProxy();

	bhkCharacterPointCollector	pointCollecter;		// 10
	UInt32						unk3D8[2];
};
STATIC_ASSERT(sizeof(bhkCharacterProxy) == 0x3E0);

// 08
class hkpCharacterState : public hkReferencedObject
{
public:
	hkpCharacterState();
	~hkpCharacterState();

	virtual void	Unk_03(void);
	virtual void	Unk_04(void);
	virtual void	Unk_05(void);
	virtual void	Unk_06(void);
	virtual void	Unk_07(void);
	virtual void	Unk_08(void);
};

class bhkCharacterState : public hkpCharacterState
{
public:
	bhkCharacterState();
	~bhkCharacterState();
};

class bhkCharacterStateOnGround : public bhkCharacterState
{
public:
	bhkCharacterStateOnGround();
	~bhkCharacterStateOnGround();
};

class bhkCharacterStateJumping : public bhkCharacterState
{
public:
	bhkCharacterStateJumping();
	~bhkCharacterStateJumping();
};

class bhkCharacterStateInAir : public bhkCharacterState
{
public:
	bhkCharacterStateInAir();
	~bhkCharacterStateInAir();
};

class bhkCharacterStateClimbing : public bhkCharacterState
{
public:
	bhkCharacterStateClimbing();
	~bhkCharacterStateClimbing();
};

class bhkCharacterStateFlying : public bhkCharacterState
{
public:
	bhkCharacterStateFlying();
	~bhkCharacterStateFlying();
};

class bhkCharacterStateSwimming : public bhkCharacterState
{
public:
	bhkCharacterStateSwimming();
	~bhkCharacterStateSwimming();
};

class bhkCharacterStateProjectile : public bhkCharacterState
{
public:
	bhkCharacterStateProjectile();
	~bhkCharacterStateProjectile();
};

// 34
class hkpCharacterStateManager : public hkReferencedObject
{
public:
	hkpCharacterStateManager();
	~hkpCharacterStateManager();

	bhkCharacterStateOnGround* stateOnGround;		// 08
	bhkCharacterStateJumping* stateJumping;		// 0C
	bhkCharacterStateInAir* stateInAir;		// 10
	bhkCharacterStateClimbing* stateClimbing;		// 14
	bhkCharacterStateFlying* stateFlying;		// 18
	bhkCharacterStateSwimming* stateSwimming;		// 1C
	bhkCharacterStateProjectile* stateProjectile;	// 20
	UInt32							unk24[4];			// 24
};

// 30
class hkpCharacterContext : public hkReferencedObject
{
public:
	hkpCharacterContext();
	~hkpCharacterContext();

	UInt32						unk08;			// 08
	hkpCharacterStateManager* stateMngr;		// 0C
	UInt32						hkState;		// 10
	UInt32						unk14[7];		// 14
};

// 04
class hkpCharacterProxyListener
{
public:
	hkpCharacterProxyListener();
	~hkpCharacterProxyListener();

	virtual void	Unk_00(void);
	virtual void	Unk_01(void);
	virtual void	Unk_02(void);
	virtual void	Unk_03(void);
	virtual void	Unk_04(void);
	virtual void	Unk_05(void);
};

// 70
class bhkCharacterListener : public hkpCharacterProxyListener
{
public:
	bhkCharacterListener();
	~bhkCharacterListener();

	UInt32			flags;			// 04 (414)
	UInt32			unk08[6];		// 08
	float			flt20;			// 20 (430)
	UInt32			unk24;			// 24 (434)
	UInt32			unk28;			// 28 (438)
	UInt32			unk2C[5];		// 2C
	NiVector4		vector40;		// 40 (450)
	UInt32			unk50[8];		// 50
};

class hkpSphereRepShape : public hkpShape
{
public:
	hkpSphereRepShape();
	~hkpSphereRepShape();
};

class hkpConvexShape : public hkpSphereRepShape
{
public:
	hkpConvexShape();
	~hkpConvexShape();
};

// 20
class hkpSphereShape : public hkpConvexShape
{
public:
	hkpSphereShape();
	~hkpSphereShape();

	UInt32			unk10[4];		// 10
};

// 30
class hkpBoxShape : public hkpConvexShape
{
public:
	hkpBoxShape();
	~hkpBoxShape();

	virtual void	Unk_0C(void);
	virtual void	Unk_0D(void);
	virtual void	Unk_0E(void);
	virtual void	Unk_0F(void);
	virtual void	Unk_10(void);
	virtual void	Unk_11(void);

	float			flt10;		// 10
	float			flt14;		// 14
	float			flt18;		// 18
	float			flt1C;		// 1C
	float			flt20;		// 20
	float			flt24;		// 24
	float			flt28;		// 28
	float			flt2C;		// 2C
};
STATIC_ASSERT(sizeof(hkpBoxShape) == 0x30);

struct VerticesBlock
{
	NiVector4	rowX;
	NiVector4	rowY;
	NiVector4	rowZ;
};

// 70
class hkpConvexVerticesShape : public hkpConvexShape
{
public:
	hkpConvexVerticesShape();
	~hkpConvexVerticesShape();

	virtual void	Unk_0C(void);
	virtual void	Unk_0D(void);
	virtual void	Unk_0E(void);
	virtual void	Unk_0F(void);
	virtual void	Unk_10(void);
	virtual void	Unk_11(void);

	float			radius;			// 10
	float			flt14;			// 14
	UInt32			unk18;			// 18
	float			flt1C;			// 1C
	float			flt20;			// 20
	float			flt24;			// 24
	float			flt28;			// 28
	UInt32			unk2C;			// 2C
	UInt32			unk30;			// 30
	UInt32			unk34;			// 34
	UInt32			unk38;			// 38
	UInt32			unk3C;			// 3C
	VerticesBlock* verticesArray;	// 40
	UInt32			numBlocks;		// 44
	UInt32			unk48;			// 48
	UInt32			numVertices;	// 4C
	UInt32			unk50;			// 50
	NiVector4* normalsArray;	// 54
	UInt32			numNormals;		// 58
	UInt32			unk5C;			// 5C
	UInt32			unk60;			// 60
	float			flt64;			// 64
	UInt32			unk68;			// 68
	float			flt6C;			// 6C
};
STATIC_ASSERT(sizeof(hkpConvexVerticesShape) == 0x70);

// 70
class hkCharControllerShape : public hkpConvexVerticesShape
{
public:
	hkCharControllerShape();
	~hkCharControllerShape();

	virtual void	Unk_12(void);
};

// 40
class hkpCapsuleShape : public hkpConvexShape
{
public:
	hkpCapsuleShape();
	~hkpCapsuleShape();

	virtual void	Unk_0C(void);
	virtual void	Unk_0D(void);
	virtual void	Unk_0E(void);
	virtual void	Unk_0F(void);
	virtual void	Unk_10(void);
	virtual void	Unk_11(void);

	float			radius;		// 10
	UInt32			unk14;		// 14
	UInt32			unk18;		// 18
	UInt32			unk1C;		// 1C
	NiVector3		point1;		// 20
	float			radius1;	// 2C
	NiVector3		point2;		// 30
	float			radius2;	// 3C
};
STATIC_ASSERT(sizeof(hkpCapsuleShape) == 0x40);

// 14
class bhkSphereRepShape : public bhkShape
{
public:
	bhkSphereRepShape();
	~bhkSphereRepShape();
};

// 14
class bhkConvexShape : public bhkSphereRepShape
{
public:
	bhkConvexShape();
	~bhkConvexShape();
};

// 14
class bhkCapsuleShape : public bhkConvexShape
{
public:
	bhkCapsuleShape();
	~bhkCapsuleShape();
};

// 14
class bhkConvexVerticesShape : public bhkConvexShape
{
public:
	bhkConvexVerticesShape();
	~bhkConvexVerticesShape();
};

// 14
class bhkCharControllerShape : public bhkConvexVerticesShape
{
public:
	bhkCharControllerShape();
	~bhkCharControllerShape();
};

// 660
class bhkCharacterController : public bhkCharacterProxy
{
public:
	bhkCharacterController();
	~bhkCharacterController();

	virtual void			Unk_31(void);
	virtual void			Unk_32(void);
	virtual void			Unk_33(void);
	virtual void			Unk_34(void);

	hkpCharacterContext		chrContext;			// 3E0
	bhkCharacterListener	chrListener;		// 410

	UInt32					unk480[4];			// 480
	NiVector4				vector490;			// 490
	NiVector4				vector4A0;			// 4A0
	float					flt4B0;				// 4B0
	UInt32					unk4B4[5];			// 4B4
	float					unk4C8[2];			// 4C8
	NiVector4				vector4D0;			// 4D0
	float					unk4E0;				// 4E0
	float					unk4E4;				// 4E4
	float					timeDelta;			// 4E8
	float					unk4EC;				// 4EC
	NiVector4				velocity;			// 4F0
	NiVector4				throwbackVelocity;	// 500
	NiVector4				vector510;			// 510
	UInt32					unk520;				// 520
	float					throwbackTimer;		// 524
	float					flt528;				// 528
	float					flt52C;				// 52C
	float					fallTimeRemaining;	// 530
	float					flt534;				// 534
	float					flt538;				// 538
	float					flt53C;				// 53C
	float					flt540;				// 540
	float					startingHeight;		// 544
	float					fallTimeElapsed;	// 548
	float					flt54C;				// 54C
	float					flt550;				// 550
	float					flt554;				// 554
	float					flt558;				// 558
	float					flt55C;				// 55C	Scale
	float					flt560;				// 560	Always 1.6
	float					flt564;				// 564	ControllerShape total length (z)
	float					flt568;				// 568
	UInt32					unk56C;				// 56C
	NiVector4				vector570;			// 570
	float					flt580;				// 580
	float					flt584;				// 584
	float					flt588;				// 588
	float					flt58C;				// 58C
	UInt32					unk590;				// 590
	bhkCachingShapePhantom* chrPhantom;		// 594
	UInt32					unk598[3];			// 598
	bhkCapsuleShape* capsuleShape;		// 5A4
	bhkCharControllerShape* charCtrlShape;		// 5A8
	UInt32					unk5AC[17];			// 5AC
	float					flt5F0;				// 5F0
	float					flt5F4;				// 5F4
	float					flt5F8;				// 5F8
	float					flt5FC;				// 5FC
	UInt32					unk600[2];			// 600
	UInt8					byte608;			// 608
	UInt8					pad609[3];			// 609
	hkpRigidBody* bodyUnderFeet;		// 60C
	UInt32					unk610[4];			// 610
	NiVector4				vector620;			// 620
	void* ptr630;			// 630
	float					flt634;				// 634
	void* ptr638;			// 638
	hkArray<hkCdPoint>		arr63C;				// 63C
	void* ptr648;			// 648
	void* ptr64C;			// 64C
	UInt32					unk650[4];			// 650
};
STATIC_ASSERT(sizeof(bhkCharacterController) == 0x660);

class hkpEntity : public hkpWorldObject
{
public:
	hkpEntity();
	~hkpEntity();

	virtual void	Unk_05(void);
};

class hkpConstraintData : public hkReferencedObject
{
public:
	hkpConstraintData();
	~hkpConstraintData();

	virtual void	Unk_03(void);
	virtual void	Unk_04(void);
	virtual void	Unk_05(void);
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
};

// 38
class hkpConstraintInstance : public hkReferencedObject
{
public:
	hkpConstraintInstance();
	~hkpConstraintInstance();

	virtual void	Unk_03(void);
	virtual void	Unk_04(void);
	virtual void	Unk_05(void);
	virtual void	Unk_06(void);
	virtual void	Unk_07(void);

	hkpSimulationIsland* simIsle;			// 08
	hkpConstraintData* constraintData;	// 0C
	UInt32					unk10;				// 10
	hkpRigidBody* contactBody;		// 14
	hkpRigidBody* parentBody;		// 18
	UInt32					unk1C[5];			// 1C
	void* ptr30;				// 30
	UInt32					unk34;				// 34
};

// F0+
class hkpMotion : public hkReferencedObject
{
public:
	hkpMotion();
	~hkpMotion();

	virtual void	Unk_03(void);
	virtual void	SetBodyMass(float mass);
	virtual void	Unk_05(void);
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
	virtual void	SetLinearVelocity(AlignedVector4* velocity);
	virtual void	SetAngularVelocity(AlignedVector4* velocity);
	virtual void	Unk_12(void);
	virtual void	Unk_13(void);
	virtual void	Unk_14(void);
	virtual void	Unk_15(void);
	virtual void	Unk_16(void);
	virtual void	Unk_17(void);
	virtual void	Unk_18(void);
	virtual void	Unk_19(void);

	enum
	{
		kMotionType_Sphere = 2,
		kMotionType_Box,
		kMotionType_KeyframedRigid,
		kMotionType_FixedRigid,
		kMotionType_ThinBox,
		kMotionType_Character,
	};

	UInt8			type;			// 008
	UInt8			byte009;		// 009
	UInt16			word00A;		// 00A
	UInt16			word00C;		// 00C
	UInt8			pad00E[2];		// 00E
	NiVector4		vector010;		// 010
	NiVector4		vector020;		// 020
	NiVector4		vector030;		// 030
	NiVector4		vector040;		// 040
	NiVector4		vector050;		// 050
	NiVector4		vector060;		// 060
	NiVector4		quaternion070;	// 070
	NiVector4		quaternion080;	// 080
	NiVector4		vector090;		// 090
	NiVector4		vector0A0;		// 0A0
	float			flt0B0;			// 0B0
	float			linDamping;		// 0B4
	float			angDamping;		// 0B8
	UInt8			byte0BC;		// 0BC
	UInt8			byte0BD;		// 0BD
	UInt8			byte0BE;		// 0BE
	UInt8			byte0BF;		// 0BF
	float			inertiaX;		// 0C0
	float			inertiaY;		// 0C4
	float			inertiaZ;		// 0C8
	float			bodyMass;		// 0CC
	NiVector4		linVelocity;	// 0D0
	NiVector4		angVelocity;	// 0E0
	NiVector4		vector0F0;		// 0F0
	NiVector4		vector100;		// 100
	UInt32			unk110[12];		// 110

	float GetBodyMass();
};
STATIC_ASSERT(sizeof(hkpMotion) == 0x140);

class hkpSphereMotion : public hkpMotion
{
public:
	hkpSphereMotion();
	~hkpSphereMotion();

	virtual void	Unk_1A(void);
};

class hkpBoxMotion : public hkpMotion
{
public:
	hkpBoxMotion();
	~hkpBoxMotion();

	virtual void	Unk_1A(void);
};

class hkpThinBoxMotion : public hkpBoxMotion
{
public:
	hkpThinBoxMotion();
	~hkpThinBoxMotion();
};

// 30
struct ConstraintContact
{
	hkpConstraintInstance* instance;		// 00
	hkpRigidBody* parentBody;	// 04
	hkpRigidBody* contactBody;	// 08
	void* ptr0C;			// 0C
	UInt32					unk10[5];		// 10
	float					flt24[3];		// 24
};

// 220
class hkpRigidBody : public hkpEntity
{
public:
	hkpRigidBody();
	~hkpRigidBody();

	virtual void	Unk_06(void);
	virtual void	Unk_07(void);

	UInt8							byte8C;			// 8C
	UInt8							pad8D[3];		// 8D
	float							friction;		// 90
	float							restitution;	// 94
	UInt32							unk98[5];		// 98
	ConstraintContact* contactsArr;	// AC
	UInt16							contactsSize;	// B0
	UInt16							contactsCap;	// B2
	hkArray<hkpConstraintInstance*>	constraintInst;	// B4
	UInt32							unkC0[3];		// C0
	hkpSimulationIsland* simIsle;		// CC
	UInt32							unkD0[4];		// D0
	hkpMotion						motion;			// E0

	void UpdateMotion();
};

// 14
class bhkEntity : public bhkWorldObject
{
public:
	bhkEntity();
	~bhkEntity();
};

// 1C
class bhkRigidBody : public bhkEntity
{
public:
	bhkRigidBody();
	~bhkRigidBody();

	virtual void	Unk_35(void);
	virtual void	Unk_36(void);
	virtual void	Unk_37(void);
	virtual void	Unk_38(void);
	virtual void	Unk_39(void);
	virtual void	Unk_3A(void);
	virtual void	Unk_3B(void);
	virtual void	Unk_3C(void);
	virtual void	Unk_3D(void);
	virtual void	Unk_3E(void);
	virtual void	Unk_3F(void);

	UInt32			unk14[2];		// 14
};

// 50
class bhkRigidBodyT : public bhkRigidBody
{
public:
	bhkRigidBodyT();
	~bhkRigidBodyT();

	UInt32			unk1C[13];		// 1C
};

// ?
class hkaRaycastInterface
{
public:
	hkaRaycastInterface();
	~hkaRaycastInterface();

	virtual void	Destroy(bool doFree);
	virtual void	Unk_01(void);
	virtual void	Unk_02(void);
};

// 2C0
class bhkRagdollController : public hkaRaycastInterface
{
public:
	bhkRagdollController();
	~bhkRagdollController();

	UInt32	unk000[(0x021C - 0x010) >> 2];	// 0010
	UInt8	fill021C[3];					// 021C
	bool	bool021F;						// 021F	when null assume FIK status is false
	bool	fikStatus;						// 0220
	UInt8	fill0221[3];					// 0221
};
