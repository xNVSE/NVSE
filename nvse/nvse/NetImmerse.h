#pragma once
#include "Containers.h"
#include "GameTypes.h"

class NiMultiTargetTransformController;
class NiTextKeyExtraData;
class NiDefaultAVObjectPalette;
struct hkVector4;
class NiLight;
class BSCubeMapCamera;
class NiFrustumPlanes;
class NiCullingProcess;
class NiSkinPartition;
class NiSkinInstance;
class NiSourceCubeMap;
class NiRenderedCubeMap;
class NiDepthStencilBuffer;
class NiRenderTargetGroup;
class NiGeometryData;
class NiParticles;
class NiLines;
class IDirect3DDevice9;
class NiDX9RenderState;
class NiUnsharedGeometryGroup;
class BSPortalGraph;
class BSDismemberSkinInstance;

typedef FixedTypeArray<hkpWorldObject*, 0x40> ContactObjects;

class NiMemObject
{
	NiMemObject();
	~NiMemObject();
};

// 08
class NiRefObject : public NiMemObject
{
public:
	NiRefObject();
	~NiRefObject();

	virtual void	Destructor(bool freeThis);
	virtual void	Free(void);

	UInt32		m_uiRefCount;	// 04
};

// 08
class NiObject : public NiRefObject
{
public:
	NiObject();
	~NiObject();

	virtual NiRTTI* GetType();
	virtual NiNode* GetNiNode();
	virtual BSFadeNode* GetFadeNode();
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
	virtual void	Unk_10(void);
	virtual void	Unk_11(void);
	virtual void	Unk_12(UInt32 arg);
	virtual void	Unk_13(UInt32 arg);
	virtual void	Unk_14(UInt32 arg);
	virtual void	Unk_15(UInt32 arg);
	virtual void	Unk_16(UInt32 arg);
	virtual void	Unk_17(UInt32 arg);
	virtual void	Unk_18(UInt32 arg);
	virtual void	Unk_19(UInt32 arg);
	virtual void	Unk_1A(UInt32 arg);
	virtual void	Unk_1B(UInt32 arg);
	virtual void	Unk_1C(void);
	virtual void	Unk_1D(void);
	virtual void	Unk_1E(UInt32 arg);
	virtual UInt32	Unk_1F(void);
	virtual void	Unk_20(void);
	virtual void	Unk_21(UInt32 arg);
	virtual void	Unk_22(void);
};

// 40
struct QuaternionKey
{
	float			time;			// 00
	NiQuaternion	value;			// 04
	NiVector3		TBC;			// 14
	NiQuaternion	quaternion20;	// 20
	NiQuaternion	quaternion30;	// 30
};
STATIC_ASSERT(sizeof(QuaternionKey) == 0x40);

// 2C
class NiTransformData : public NiObject
{
public:
	NiTransformData();
	~NiTransformData();

	enum
	{
		kKeyType_Linear = 1,
		kKeyType_Quadratic,
		kKeyType_TBC,
		kKeyType_XYZ,
		kKeyType_Const,
	};

	UInt16		numRotationKeys;	// 08
	UInt16		numTranslationKeys;	// 0A
	UInt16		numScaleKeys;		// 0C
	UInt16		pad0E;				// 0E
	UInt32		rotationKeyType;	// 10
	UInt32		translationKeyType;	// 14
	UInt32		scaleKeyType;		// 18
	UInt8		rotationKeySize;	// 1C
	UInt8		translationKeySize;	// 1D
	UInt8		scaleKeySize;		// 1E
	UInt8		pad1F;				// 1F
	void* rotationKeys;		// 20
	void* translationKeys;	// 24
	void* scaleKeys;			// 28
};
STATIC_ASSERT(sizeof(NiTransformData) == 0x2C);

// 08
struct FloatData
{
	UInt32		unk00;
	float		value;
};

// 18
class NiFloatData : public NiObject
{
public:
	NiFloatData();
	~NiFloatData();

	UInt32			unk08;		// 08
	FloatData* fltData;	// 0C
	UInt32			unk10;		// 10
	UInt8			byte14;		// 14
	UInt8			pad15[3];	// 15
};

// 14
struct ColorData
{
	UInt32		unk00;
	float		value[4];
};

// 18
class NiColorData : public NiObject
{
public:
	NiColorData();
	~NiColorData();

	UInt32			unk08;		// 08
	ColorData* clrData;	// 0C
	UInt32			unk10;		// 10
	UInt32			unk14;		// 14
};

// 0C
class NiInterpolator : public NiObject
{
public:
	NiInterpolator();
	~NiInterpolator();

	virtual void	Unk_23(void);
	virtual void	Unk_24(void);
	virtual void	Unk_25(void);
	virtual void	Unk_26(void);
	virtual void	Unk_27(void);
	virtual void	Unk_28(void);
	virtual void	Unk_29(void);
	virtual void	Unk_2A(void);
	virtual void	Unk_2B(void);
	virtual void	Unk_2C(void);
	virtual void	Unk_2D(void);
	virtual void	Unk_2E(void);
	virtual void	Unk_2F(void);
	virtual void	Unk_30(void);
	virtual void	Unk_31(void);
	virtual void	Unk_32(void);
	virtual void	Unk_33(void);
	virtual void	Unk_34(void);
	virtual void	Unk_35(void);
	virtual void	Unk_36(void);

	float		flt08;		// 08
};

// 0C
class NiKeyBasedInterpolator : public NiInterpolator
{
public:
	NiKeyBasedInterpolator();
	~NiKeyBasedInterpolator();

	virtual void	Unk_37(void);
	virtual void	Unk_38(void);
	virtual void	Unk_39(void);
	virtual void	Unk_3A(void);
	virtual void	Unk_3B(void);
	virtual void	Unk_3C(void);
	virtual void	Unk_3D(void);
};

// 18
class NiFloatInterpolator : public NiKeyBasedInterpolator
{
public:
	NiFloatInterpolator();
	~NiFloatInterpolator();

	float				value;		// 0C
	NiFloatData* data;		// 10
	UInt32				unk14;		// 14
};

// 24
class NiColorInterpolator : public NiKeyBasedInterpolator
{
public:
	NiColorInterpolator();
	~NiColorInterpolator();

	float				value[4];	// 0C
	NiColorData* data;		// 1C
	UInt32				unk20;		// 20
};

// 48
class NiTransformInterpolator : public NiKeyBasedInterpolator
{
public:
	NiTransformInterpolator();
	~NiTransformInterpolator();

	virtual void	Unk_3E(void);

	float			flt0C;		// 0C
	float			flt10;		// 10
	float			flt14;		// 14
	float			flt18;		// 18
	float			flt1C;		// 1C
	float			flt20;		// 20
	float			flt24;		// 24
	float			flt28;		// 28
	NiTransformData* transData;	// 2C
	UInt16			unk30;		// 30
	UInt16			unk32;		// 32
	UInt16			unk34;		// 34
	UInt8			pad36[2];	// 36
	float			flt38;		// 38
	float			flt3C;		// 3C
	float			flt40;		// 40
	UInt8			byte44;		// 44
	UInt8			pad45[3];	// 45

	static NiTransformInterpolator* Create();
};
STATIC_ASSERT(sizeof(NiTransformInterpolator) == 0x48);

// 74
class NiControllerSequence : public NiObject
{
public:
	NiControllerSequence();
	~NiControllerSequence();

	virtual void	Unk_23(void);

	struct ControlledBlock
	{
		NiInterpolator* interpolator;
		NiMultiTargetTransformController* multiTargetCtrl;
		// More
	};

	const char* sequenceName;			// 08
	UInt32				numControlledBlocks;	// 0C
	UInt32				arrayGrowBy;			// 10
	ControlledBlock** controlledBlocks;		// 14
	const char** unkNodeName;			// 18
	float				weight;					// 1C
	NiTextKeyExtraData* textKeyData;			// 20
	UInt32				cycleType;				// 24
	float				frequency;				// 28
	float				startTime;				// 2C
	float				stopTime;				// 30
	float				flt34;					// 34
	float				flt38;					// 38
	float				flt3C;					// 3C
	NiControllerManager* manager;				// 40
	UInt32				unk44;					// 44
	UInt32				unk48;					// 48
	float				flt4C;					// 4C
	float				flt50;					// 50
	float				flt54;					// 54
	UInt32				unk58;					// 58
	const char* rootNodeName;			// 5C
	UInt32				unk60[5];				// 60
};
STATIC_ASSERT(sizeof(NiControllerSequence) == 0x74);

// 34
class NiTimeController : public NiObject
{
public:
	NiTimeController();
	~NiTimeController();

	virtual void	Unk_23(void);
	virtual void	Unk_24(void);
	virtual void	Unk_25(void);
	virtual void	SetTarget(NiNode* pTarget);
	virtual void	Unk_27(void);
	virtual void	Unk_28(void);
	virtual void	Unk_29(void);
	virtual void	Unk_2A(void);
	virtual void	Unk_2B(void);
	virtual void	Unk_2C(void);

	UInt16								flags;				// 08
	UInt16								unk0A;				// 0A
	float								frequency;			// 0C
	float								phaseTime;			// 10
	float								flt14;				// 14
	float								flt18;				// 18
	float								flt1C;				// 1C
	float								flt20;				// 20
	float								flt24;				// 24
	float								flt28;				// 28
	NiNode* target;			// 2C
	NiMultiTargetTransformController* multiTargetCtrl;	// 30
};

// 7C
class NiControllerManager : public NiTimeController
{
public:
	NiControllerManager();
	~NiControllerManager();

	virtual void	Unk_2D(void);

	NiTArray<NiControllerSequence*>					sequences;		// 34
	void* ptr44;			// 44
	UInt32											unk48;			// 48
	UInt32											unk4C;			// 4C
	NiTMapBase<const char*, NiControllerSequence*>	seqStrMap;		// 50
	UInt32											unk60;			// 60
	NiTArray<void*>* arr64;			// 64
	UInt32											unk68;			// 68
	UInt32											unk6C;			// 6C
	UInt32											unk70;			// 70
	UInt32											unk74;			// 74
	NiDefaultAVObjectPalette* defObjPlt;		// 78
};
STATIC_ASSERT(sizeof(NiControllerManager) == 0x7C);

// 34
class NiInterpController : public NiTimeController
{
public:
	NiInterpController();
	~NiInterpController();

	virtual void	Unk_2D(void);
	virtual void	Unk_2E(void);
	virtual void	Unk_2F(void);
	virtual void	Unk_30(void);
	virtual void	Unk_31(void);
	virtual void	SetInterpolator(NiInterpolator* pInterpolator, UInt32 arg2);
	virtual void	Unk_33(void);
	virtual void	Unk_34(void);
	virtual void	Unk_35(void);
	virtual void	Unk_36(void);
	virtual void	Unk_37(void);
	virtual void	Unk_38(void);
	virtual void	Unk_39(void);
};

// 38
class NiSingleInterpController : public NiInterpController
{
public:
	NiSingleInterpController();
	~NiSingleInterpController();

	virtual void	Unk_3A(void);

	NiTransformInterpolator* interpolator;		// 34
};

// 38
class NiTransformController : public NiSingleInterpController
{
public:
	NiTransformController();
	~NiTransformController();

	static NiTransformController* __stdcall Create(NiNode* pTarget, NiTransformInterpolator* pInterpolator);
};
STATIC_ASSERT(sizeof(NiTransformController) == 0x38);

// 0C
class NiExtraData : public NiObject
{
public:
	NiExtraData();
	~NiExtraData();

	virtual void	Unk_23(void);
	virtual void	Unk_24(void);

	UInt32			unk08;		// 08
};

// 10
class BSXFlags : public NiExtraData
{
public:
	BSXFlags();
	~BSXFlags();

	enum
	{
		kBSXFlag_Animated = 1 << 0,
		kBSXFlag_Havok = 1 << 1,
		kBSXFlag_Ragdoll = 1 << 2,
		kBSXFlag_Complex = 1 << 3,
		kBSXFlag_Addon = 1 << 4,
		kBSXFlag_EditorMarker = 1 << 5,
		kBSXFlag_Dynamic = 1 << 6,
		kBSXFlag_Articulated = 1 << 7,
		kBSXFlag_NeedsTransformUpdates = 1 << 8,
		kBSXFlag_ExternalEmit = 1 << 9,
	};

	UInt32			flags;		// 0C
};

// 14
class TileExtra : public NiExtraData
{
public:
	TileExtra();
	~TileExtra();

	Tile* parentTile;	// 0C
	NiNode* parentNode;	// 10
};

// 18
class NiObjectNET : public NiObject
{
public:
	NiObjectNET();
	~NiObjectNET();

	const char* m_blockName;				// 08
	NiTimeController* m_controller;				// 0C
	NiExtraData** m_extraDataList;			// 10
	UInt16				m_extraDataListLen;			// 14
	UInt16				m_extraDataListCapacity;	// 16

	void DumpExtraData();
};

// 18
class NiProperty : public NiObjectNET
{
public:
	NiProperty();
	~NiProperty();

	virtual UInt32	GetPropertyType();
	virtual void	UpdateController(float arg);

	enum
	{
		kPropertyType_Alpha = 0,
		kPropertyType_Culling = 1,
		kPropertyType_Material = 2,
		kPropertyType_Shade = 3,
		kPropertyType_TileShader = kPropertyType_Shade,
		kPropertyType_Stencil = 4,
		kPropertyType_Texturing = 5,
		kPropertyType_Dither = 8,
		kPropertyType_Specular = 9,
		kPropertyType_VertexColor = 10,
		kPropertyType_ZBuffer = 11,
		kPropertyType_Fog = 13,
	};
};

// 4C
class NiMaterialProperty : public NiProperty
{
public:
	NiMaterialProperty();
	~NiMaterialProperty();

	UInt32				unk18;			// 18
	float				specularRGB[3];	// 1C
	float				emissiveRGB[3];	// 28
	UInt32				unk34;			// 34
	float				glossiness;		// 38
	float				alpha;			// 3C
	float				emitMult;		// 40
	UInt32				unk44[2];		// 44
};

// 1C
class NiAlphaProperty : public NiProperty
{
public:
	NiAlphaProperty();
	~NiAlphaProperty();

	enum
	{
		kFlag_EnableBlending = 1 << 0,

	};

	UInt16				flags;		// 18
	UInt8				threshold;	// 1A
	UInt8				byte1B;		// 1B
};

// 30
class NiTexturingProperty : public NiProperty
{
public:
	NiTexturingProperty();
	~NiTexturingProperty();

	UInt32				unk18[6];	// 18
};

// 24
class NiStencilProperty : public NiProperty
{
public:
	NiStencilProperty();
	~NiStencilProperty();

	UInt16				flags;		// 18
	UInt16				word1A;		// 1A
	UInt32				unk1C;		// 1C
	UInt32				mask;		// 20
};
STATIC_ASSERT(sizeof(NiStencilProperty) == 0x24);

// 1C
class NiCullingProperty : public NiProperty
{
public:
	NiCullingProperty();
	~NiCullingProperty();

	UInt32				unk18;		// 18
};

// 60
class BSShaderProperty : public NiProperty
{
public:
	BSShaderProperty();
	~BSShaderProperty();

	virtual void	Unk_25(void);
	virtual void	Unk_26(UInt32 arg1);
	virtual void	Unk_27(UInt32 arg1);
	virtual void	Unk_28(UInt32 arg1, UInt32 arg2, UInt32 arg3, UInt32 arg4, UInt32 arg5, UInt32 arg6);
	virtual void	Unk_29(UInt32 arg1);
	virtual void	Unk_2A(void);
	virtual void	Unk_2B(UInt32 arg1);
	virtual void	Unk_2C(UInt32 arg1, UInt32 arg2, UInt32 arg3);
	virtual void	Unk_2D(void);
	virtual void	Unk_2E(UInt32 arg1);
	virtual void	Unk_2F(UInt32 arg1, UInt32 arg2);
	virtual void	Unk_30(void);

	UInt16			unk18;		// 18
	UInt16			unk1A;		// 1A
	UInt32			unk1C;		// 1C
	UInt32			unk20;		// 20
	UInt32			unk24;		// 24
	float			flt28;		// 28
	float			flt2C;		// 2C
	float			flt30;		// 30
	float			flt34;		// 34
	UInt32			unk38;		// 38
	void* ptr03C;	// 3C	Seen 010B8480
	UInt32			unk40;		// 40
	UInt32			unk44;		// 44
	UInt32			unk48;		// 48
	UInt32			unk4C;		// 4C
	UInt32			unk50;		// 50
	UInt32			unk54;		// 54
	UInt32			unk58;		// 58
	float			flt5C;		// 5C
};
STATIC_ASSERT(sizeof(BSShaderProperty) == 0x60);

// 80
class BSShaderNoLightingProperty : public BSShaderProperty
{
public:
	BSShaderNoLightingProperty();
	~BSShaderNoLightingProperty();

	virtual void	Unk_31(void);
	virtual void	Unk_32(void);
	virtual void	Unk_33(void);
	virtual void	Unk_34(void);

	UInt32			unk60;			// 60
	UInt32			unk64;			// 64
	UInt16			word68;			// 68
	UInt16			word6A;			// 6A
	UInt32			unk6C;			// 6C
	float			flt70;			// 70
	float			flt74;			// 74
	float			flt78;			// 78
	float			flt7C;			// 7C
};
STATIC_ASSERT(sizeof(BSShaderNoLightingProperty) == 0x80);

// 7C
class BSShaderLightingProperty : public BSShaderProperty
{
public:
	BSShaderLightingProperty();
	~BSShaderLightingProperty();

	virtual void	Unk_31(void);
	virtual void	Unk_32(void);

	UInt32			unk60[3];		// 60
	float			flt6C;			// 6C
	UInt32			unk70;			// 70
	UInt8			byte74;			// 74
	UInt8			pad75[3];		// 75
	UInt32			unk78;			// 78
};
STATIC_ASSERT(sizeof(BSShaderLightingProperty) == 0x7C);

// 104
class BSShaderPPLightingProperty : public BSShaderLightingProperty
{
public:
	BSShaderPPLightingProperty();
	~BSShaderPPLightingProperty();

	virtual void	Unk_33(void);
	virtual void	Unk_34(void);
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
	virtual void	Unk_40(void);
	virtual void	Unk_41(void);
	virtual void	Unk_42(void);
	virtual void	Unk_43(void);
	virtual void	Unk_44(void);
	virtual void	Unk_45(void);
	virtual void	Unk_46(void);
	virtual void	Unk_47(void);

	UInt32			unk07C;			// 07C
	UInt32			unk080;			// 080
	float			flt084[8];		// 084
	UInt32			unk0A4;			// 0A4
	UInt16			word0A8;		// 0A8
	UInt16			word0AA;		// 0AA
	UInt32			unk0AC[6];		// 0AC
	void* ptr0C4;		// 0C4
	UInt32			unk0C8;			// 0C8
	void* ptr0CC;		// 0CC
	UInt32			unk0D0[4];		// 0D0
	float			flt0E0;			// 0E0
	UInt32			unk0E4;			// 0E4
	float			flt0E8;			// 0E8
	float			flt0EC;			// 0EC
	UInt32			unk0F0[5];		// 0F0
};
STATIC_ASSERT(sizeof(BSShaderPPLightingProperty) == 0x104);

// 150
class WaterShaderProperty : public BSShaderProperty
{
public:
	WaterShaderProperty();
	~WaterShaderProperty();

	UInt8				byte060;		// 060
	UInt8				byte061;		// 061
	UInt8				byte062;		// 062
	UInt8				byte063;		// 063
	UInt32				unk064;			// 064
	UInt32				unk068;			// 068
	float				flt06C;			// 06C
	float				flt070;			// 070
	UInt32				unk074;			// 074
	UInt32				unk078;			// 078
	UInt8				byte07C;		// 07C
	UInt8				byte07D;		// 07D
	UInt8				byte07E;		// 07E
	UInt8				byte07F;		// 07F
	UInt8				byte080;		// 080
	UInt8				byte081;		// 081
	UInt8				byte082;		// 082
	UInt8				byte083;		// 083
	UInt32				unk084;			// 084
	float				flt088[40];		// 088
	UInt32				unk128[3];		// 128
	NiSourceTexture* srcTexture;	// 134
	NiObject* noDepth;		// 138	Seen 010AE500
	NiObject* reflections;	// 13C		"
	NiObject* refractions;	// 140
	NiObject* depth;			// 144		"
	UInt32				unk148;			// 148
	UInt32				unk14C;			// 14C
};
STATIC_ASSERT(sizeof(WaterShaderProperty) == 0x150);

// 90
class SkyShaderProperty : public BSShaderProperty
{
public:
	SkyShaderProperty();
	~SkyShaderProperty();

	virtual void	Unk_31(void);
	virtual void	Unk_32(void);
	virtual void	Unk_33(void);
	virtual void	Unk_34(void);

	NiColorAlpha		color;			// 60
	NiSourceTexture* srcTexture70;	// 70
	String				filePath;		// 74
	UInt32				unk7C;			// 7C
	NiSourceTexture* srcTexture80;	// 80
	float				flt84;			// 84	Maybe texture % for animation?
	UInt32				unk88;			// 88
	UInt32				unk8C;			// 8C
};
STATIC_ASSERT(sizeof(SkyShaderProperty) == 0x90);

// B0
class TileShaderProperty : public BSShaderProperty
{
public:
	TileShaderProperty();
	~TileShaderProperty();

	NiTexture* srcTexture;	// 60
	UInt32				unk64;			// 64
	NiColorAlpha		overlayColor;	// 68
	float				alpha;			// 78
	UInt32				unk7C;			// 7C
	UInt32				unk80;			// 80
	float				flt84;			// 84
	float				flt88;			// 88
	UInt32				unk8C;			// 8C
	UInt8				byte90;			// 90
	UInt8				byte91;			// 91
	UInt8				hasVtxColors;	// 92
	UInt8				byte93;			// 93
	UInt32				unk94[7];		// 94
};
STATIC_ASSERT(sizeof(TileShaderProperty) == 0xB0);

// 9C
class NiAVObject : public NiObjectNET
{
public:
	NiAVObject();
	~NiAVObject();

	virtual void	Unk_23(UInt32 arg1);
	virtual void	Unk_24(NiMatrix33* arg1, NiVector3* arg2, bool arg3);
	virtual void	Unk_25(UInt32 arg1);
	virtual void	Unk_26(UInt32 arg1);
	virtual NiAVObject* GetObjectByName(char** objName);	// Must be strings generated at startup by sub_4B7920
	virtual void	Unk_28(UInt32 arg1, UInt32 arg2, UInt32 arg3);
	virtual void	Unk_29(UInt32 arg1, UInt32 arg2);
	virtual void	Unk_2A(UInt32 arg1, UInt32 arg2);
	virtual void	Unk_2B(UInt32 arg1, UInt32 arg2);
	virtual void	Unk_2C(UInt32 arg1);
	virtual void	Unk_2D(UInt32 arg1);
	virtual void	UpdateTransform(UInt32 arg1);
	virtual void	Unk_2F(void);
	virtual void	UpdateBounds(UInt32 arg1);
	virtual void	Unk_31(UInt32 arg1, UInt32 arg2);
	virtual void	Unk_32(UInt32 arg1);
	virtual void	Unk_33(UInt32 arg1);
	virtual void	Unk_34(void);
	virtual void	Unk_35(void);
	virtual void	Unk_36(UInt32 arg1);

	enum NiFlags
	{
		kNiFlag_Hidden = 0x00000001,
		kNiFlag_SelectiveUpdate = 0x00000002,
		kNiFlag_SelUpdTransforms = 0x00000004,
		kNiFlag_SelUpdController = 0x00000008,
		kNiFlag_SelUpdRigid = 0x00000010,
		kNiFlag_DisplayObject = 0x00000020,
		kNiFlag_DisableSorting = 0x00000040,
		kNiFlag_SelUpdTransformsOverride = 0x00000080,
		kNiFlag_UnkBit8 = 0x00000100,
		kNiFlag_SaveExternalGeomData = 0x00000200,
		kNiFlag_NoDecals = 0x00000400,
		kNiFlag_AlwaysDraw = 0x00000800,
		kNiFlag_MeshLOD = 0x00001000,
		kNiFlag_FixedBound = 0x00002000,
		kNiFlag_TopFadeNode = 0x00004000,
		kNiFlag_IgnoreFade = 0x00008000,
		kNiFlag_NoAnimSyncX = 0x00010000,
		kNiFlag_NoAnimSyncY = 0x00020000,
		kNiFlag_NoAnimSyncZ = 0x00040000,
		kNiFlag_NoAnimSyncS = 0x00080000,
		kNiFlag_NoDismember = 0x00100000,
		kNiFlag_NoDismemberValidity = 0x00200000,
		kNiFlag_RenderUse = 0x00400000,
		kNiFlag_MaterialsApplied = 0x00800000,
		kNiFlag_HighDetail = 0x01000000,
		kNiFlag_ForceUpdate = 0x02000000,
		kNiFlag_PreProcessedNode = 0x04000000,
		kNiFlag_UnkBit27 = 0x08000000,
		kNiFlag_UnkBit28 = 0x10000000,
		kNiFlag_UnkBit29 = 0x20000000,
		kNiFlag_UnkBit30 = 0x40000000,
		kNiFlag_UnkBit31 = 0x80000000
	};

	NiAVObject* m_parent;				// 18
	bhkNiCollisionObject* m_collisionObject;		// 1C
	NiSphere* m_kWorldBound;			// 20
	DList<NiProperty>		m_propertyList;			// 24
	UInt32					m_flags;				// 30
	NiMatrix33				m_localRotate;			// 34
	NiVector3				m_localTranslate;		// 58
	float					m_localScale;			// 64
	NiMatrix33				m_worldRotate;			// 68
	NiVector3				m_worldTranslate;		// 8C
	float					m_worldScale;			// 98

	NiProperty* GetProperty(UInt32 propID);

	void DumpProperties();
	void DumpParents();
};

// AC
class NiNode : public NiAVObject
{
public:
	NiNode();
	~NiNode();

	virtual void	AddObject(NiAVObject* object, bool arg2);
	virtual void	Unk_38(void);
	virtual void	RemoveObject(NiAVObject* toRemove, NiAVObject** out);
	virtual void	Unk_3A(void);
	virtual void	Unk_3B(void);
	virtual void	Unk_3C(void);
	virtual void	Unk_3D(void);
	virtual void	Unk_3E(void);
	virtual void	Unk_3F(void);

	NiTArray<NiAVObject*>	m_children;		// 9C

	static NiNode* __stdcall Create(const char* nodeName);
	NiAVObject* GetBlock(const char* blockName);
	NiNode* GetNode(const char* nodeName);
	bool IsMovable();
	void ToggleCollision(bool enable);
	void GetContactObjects(ContactObjects* contactObjs);
	bool HasPhantom();
	void GetBodyMass(float* totalMass);
	void ApplyForce(hkVector4* forceVector);
	void Dump();
};
STATIC_ASSERT(sizeof(NiNode) == 0xAC);

// E4
class BSFadeNode : public NiNode
{
public:
	BSFadeNode();
	~BSFadeNode();

	float			fltAC[4];		// AC
	UInt32			unkBC[2];		// BC
	UInt32			unkC4;			// C4
	UInt32			unkC8;			// C8
	TESObjectREFR* linkedObj;		// CC
	UInt32			unkD0[5];		// D0
};

// B4
class BSMultiBoundNode : public NiNode
{
public:
	BSMultiBoundNode();
	~BSMultiBoundNode();

	virtual void	Unk_40(UInt32 arg1, UInt32 arg2);
	virtual void	Unk_41(void);
	virtual void	Unk_42(UInt32 arg1);
	virtual void	Unk_43(UInt32 arg1);
	virtual void	Unk_44(UInt32 arg1);

	UInt32			unkAC[2];		// AC
};

// B8
class BSParticleSystemManager : public NiNode
{
public:
	BSParticleSystemManager();
	~BSParticleSystemManager();

	virtual void	Unk_40(void);

	UInt32			unkAC[3];		// AC
};

// B4
class NiBillboardNode : public NiNode
{
public:
	NiBillboardNode();
	~NiBillboardNode();

	virtual void	Unk_40(void);

	UInt32			unkAC[2];		// AC
};

// 64
class BSFogProperty : public NiProperty
{
public:
	BSFogProperty();
	~BSFogProperty();

	UInt16				unk18;		// 18
	UInt16				unk1A;		// 1A
	float				flt1C;		// 1C
	NiColor				color;		// 20
	float				distNear;	// 2C
	float				distFar;	// 30
	UInt32				unk34;		// 34
	UInt32				unk38;		// 38
	float				flt3C;		// 3C
	float				flt40;		// 40
	float				flt44;		// 44
	float				flt48;		// 48
	UInt32				unk4C;		// 4C
	UInt32				unk50;		// 50
	float				flt54;		// 54
	float				flt58;		// 58
	float				flt5C;		// 5C
	float				power;		// 60
};
STATIC_ASSERT(sizeof(BSFogProperty) == 0x64);

// 78
class BSPortalGraph : public NiRefObject
{
public:
	BSPortalGraph();
	~BSPortalGraph();

	UInt32					unk08[10];	// 08
	void* ptr30;		// 30
	void* ptr34;		// 34
	UInt32					unk38;		// 38
	NiTArray<NiAVObject*>	array3C;	// 3C
	NiNode* node4C;	// 4C
	UInt32					unk50[6];	// 50
	BSSimpleArray<NiNode>	array68;	// 68
};
STATIC_ASSERT(sizeof(BSPortalGraph) == 0x78);

// 250
class LightingData : public NiRefObject	//	010B7EB8
{
public:
	LightingData();
	~LightingData();

	UInt32					unk008;			// 008
	float					flt00C[53];		// 00C
	DList<NiTriStrips>		lgtList0E0;		// 0E0
	UInt8					byte0EC;		// 0EC
	UInt8					byte0ED;		// 0ED
	UInt8					byte0EE[2];		// 0EE
	UInt32					unk0F0;			// 0F0
	UInt32					unk0F4;			// 0F4
	NiLight* light;			// 0F8
	UInt32					unk0FC;			// 0FC
	UInt32					unk100[6];		// 100
	UInt8					byte118;		// 118
	UInt8					pad119[3];		// 119
	float					flt11C;			// 11C
	float					flt120;			// 120
	UInt8					byte124;		// 124
	UInt8					pad125[3];		// 125
	UInt32					unk128[66];		// 128
	BSSimpleArray<NiNode>	array230;		// 230
	BSPortalGraph* portalGraph;	// 240
	UInt32					unk244[3];		// 244
};
STATIC_ASSERT(sizeof(LightingData) == 0x250);

// 200
class ShadowSceneNode : public NiNode
{
public:
	ShadowSceneNode();
	~ShadowSceneNode();

	UInt32							unk0AC[2];		// 0AC
	DList<LightingData>				lgtList0B4;		// 0B4
	DList<LightingData>				lgtList0C0;		// 0C0
	UInt32							unk0CC;			// 0CC
	DListNode<LightingData>* node0D0;		// 0D0
	DListNode<LightingData>* node0D4;		// 0D4
	LightingData* data0D8;		// 0D8
	LightingData* data0DC;		// 0DC
	LightingData* data0E0;		// 0E0
	UInt32							unk0E4[6];		// 0E4
	void* ptr0FC;		// 0FC
	void* ptr100;		// 100
	UInt32							unk104;			// 104
	UInt32							unk108[3];		// 108
	void* ptr114;		// 114
	void* ptr118;		// 118
	UInt32							unk11C;			// 11C
	UInt32							unk120;			// 120
	UInt32							unk124;			// 124
	BSCubeMapCamera* cubeMapCam;	// 128
	UInt32							unk12C;			// 12C
	UInt8							byte130;		// 130
	UInt8							byte131;		// 131
	UInt8							pad132[2];		// 132
	BSFogProperty* fogProperty;	// 134
	UInt32							unk138;			// 138
	BSSimpleArray<NiFrustumPlanes>	array13C;		// 13C
	BSSimpleArray<void>				array14C;		// 14C	010C1E9C
	UInt32							unk15C[3];		// 15C
	NiVector4						unk168;			// 168
	NiVector4						unk178;			// 178
	NiVector4						unk188;			// 188
	NiVector4						unk198;			// 198
	NiVector4						unk1A8;			// 1A8
	NiVector4						unk1B8;			// 1B8
	UInt32							lightingPasses;	// 1C8
	float							flt1CC[3];		// 1CC
	UInt32							unk1D8;			// 1D8
	UInt8							byte1DC;		// 1DC
	UInt8							pad1DD[3];		// 1DD
	BSPortalGraph* portalGraph;	// 1E0
	UInt32							unk1E4[3];		// 1E4
	float							flt1F0[3];		// 1F0
	UInt8							byte1FC;		// 1FC
	UInt8							pad1FD[3];		// 1FD
};
STATIC_ASSERT(sizeof(ShadowSceneNode) == 0x200);

// 114
class NiCamera : public NiAVObject
{
public:
	NiCamera();
	~NiCamera();

	float			worldToCam[4][4];	// 09C
	NiFrustum		frustum;			// 0DC
	float			minNearPlaneDist;	// 0F8
	float			maxFarNearRatio;	// 0FC
	NiViewport		viewPort;			// 100
	float			LODAdjust;			// 110
};
STATIC_ASSERT(sizeof(NiCamera) == 0x114);

// C4
class NiDynamicEffect : public NiAVObject
{
public:
	NiDynamicEffect();
	~NiDynamicEffect();

	UInt8			byte9C;			// 9C
	UInt8			byte9D;			// 9D
	UInt8			byte9E;			// 9E
	UInt8			byte9F;			// 9F
	UInt32			unkA0;			// A0
	UInt32			unkA4;			// A4
	UInt32			unkA8;			// A8
	UInt32			unkAC;			// AC
	UInt32			unkB0;			// B0
	UInt32			unkB4;			// B4
	UInt32			unkB8;			// B8
	UInt32			unkBC;			// BC
	UInt32			unkC0;			// C0
};

// F0
class NiLight : public NiDynamicEffect
{
public:
	NiLight();
	~NiLight();

	float			fadeValue;			// C4
	NiColor			ambientColor;		// C8
	NiColor			directionalColor;	// D4
};

// FC
class NiPointLight : public NiLight
{
public:
	NiPointLight();
	~NiPointLight();

	float			radius;				// E0
	float			fltE4;				// E4
	float			fltE8;				// E8
	UInt32			unkEC;				// EC
	float			attenuation1;		// F0
	float			attenuation2;		// F4
	float			attenuation3;		// F8
};

// FC
class NiDirectionalLight : public NiLight
{
public:
	NiDirectionalLight();
	~NiDirectionalLight();

	NiColor			fogColor;			// E0
	UInt32			unkEC;				// EC
	NiVector3		direction;			// F0
};

class BSSceneGraph : public NiNode
{
public:
	BSSceneGraph();
	~BSSceneGraph();

	virtual void	Unk_40(void);
	virtual void	Unk_41(void);
};

// C0
class SceneGraph : public BSSceneGraph
{
public:
	SceneGraph();
	~SceneGraph();

	NiCamera* camera;			// AC
	UInt32				unkB0;				// B0
	NiCullingProcess* cullingProc;		// B4
	UInt32				isMinFarPlaneDist;	// B8 The farplane is set to 20480.0 when the flag is true. Probably used for interiors.
	float				cameraFOV;			// BC
};

// 3C
class TESAnimGroup : public NiRefObject
{
public:
	TESAnimGroup();
	~TESAnimGroup();

	UInt32			unk08[2];	// 08
	UInt8			index;		// 10
	UInt8			unk11;		// 11
	UInt8			unk12[1];	// 12
	UInt32			unk14[10];	// 14
};

// 78
class BSAnimGroupSequence : public NiControllerSequence
{
public:
	BSAnimGroupSequence();
	~BSAnimGroupSequence();

	TESAnimGroup* animGroup;		// 74
};

// 210
class NiRenderer : public NiObject
{
public:
	NiRenderer();
	~NiRenderer();

	virtual void	Unk_23(void);

	UInt32			unk008[126];	// 008
	UInt32			sceneState;		// 200
	UInt32			unk204;			// 204
	UInt32			unk208;			// 208
	UInt32			unk20C;			// 20C
};
STATIC_ASSERT(sizeof(NiRenderer) == 0x210);

class NiDX9Renderer : public NiRenderer
{
public:
	NiDX9Renderer();
	~NiDX9Renderer();

	enum ClearFlags
	{
		kClear_NONE = 0,
		kClear_BACKBUFFER = 1,
		kClear_STENCIL = 2,
		kClear_ZBUFFER = 4,
		kClear_ALL = kClear_BACKBUFFER | kClear_STENCIL | kClear_ZBUFFER
	};

	virtual void		Unk_24(void);
	virtual void		Unk_25(void);
	virtual void		Unk_26(void);
	virtual void		Unk_27(void);
	virtual void		Unk_28(void);
	virtual void		Unk_29(void);
	virtual void		Unk_2A(void);
	virtual void		SetBackgroundColor(NiVector4* inARGB);
	virtual void		Unk_2C(void);
	virtual void		GetBackgroundColor(NiVector4* outARGB);
	virtual void		Unk_2E(void);
	virtual void		Unk_2F(void);
	virtual void		Unk_30(void);
	virtual void		Unk_31(void);
	virtual NiRenderTargetGroup* GetDefaultRT();	// get back buffer rt
	virtual NiRenderTargetGroup* GetCurrentRT();	// get currentRTGroup
	virtual void		Unk_34(void);
	virtual void		Unk_35(void);
	virtual void		Unk_36(void);
	virtual void		Unk_37(void);
	virtual void 		Unk_38(void);
	virtual void 		Unk_39(void);
	virtual void		Unk_3A(void);
	virtual void		Unk_3B(void);
	virtual void		PurgeGeometry(NiGeometryData* geo);
	virtual void		PurgeMaterial(NiMaterialProperty* material);
	virtual void		PurgeEffect(NiDynamicEffect* effect);
	virtual void		PurgeScreenTexture();
	virtual void		PurgeSkinPartition(NiSkinPartition* skinPartition);
	virtual void		PurgeSkinInstance(NiSkinInstance* skinInstance);
	virtual void		Unk_42(void);
	virtual bool		Unk_43(void);
	virtual void		Unk_44(void);
	virtual bool		FastCopy(void* src, void* dst, RECT* srcRect, SInt32 xOffset, SInt32 yOffset);
	virtual bool		Copy(void* src, void* dst, RECT* srcRect, RECT* dstRect, UInt32 filterMode);
	virtual void		Unk_47(void);
	virtual bool		Unk_48(void* arg);
	virtual void		Unk_49(void);
	virtual void		Unk_4A(float arg);
	virtual void 		Unk_4B(UInt32 size);
	virtual void		Unk_4C(UInt32 arg0, UInt32 arg1);
	virtual void		Unk_4D(UInt32 arg0, UInt32 arg1);
	virtual void		Unk_4E(void* buf);
	virtual void		CreateSourceTexture(NiSourceTexture* texture);
	virtual bool		CreateRenderedTexture(NiRenderedTexture* arg);
	virtual bool		CreateSourceCubeMap(NiSourceCubeMap* arg);
	virtual bool		CreateRenderedCubeMap(NiRenderedCubeMap* arg);
	virtual bool		CreateDynamicTexture(void* arg);
	virtual void		Unk_54(void);
	virtual bool		CreateDepthStencil(NiDepthStencilBuffer* arg, void* textureFormat);
	virtual void		Unk_56(void);
	virtual void		Unk_57(void);
	virtual void		Unk_58(void);
	virtual void		Unk_59(void);
	virtual void		Unk_5A(void);
	virtual void		Unk_5B(void);
	virtual void		Unk_5C(void);
	virtual void		Unk_5D(void);
	virtual void		Unk_5E(void);
	virtual bool		BeginScene();
	virtual bool		EndScene();
	virtual void		DisplayScene();
	virtual void		Clear(float* rect, UInt32 flags);
	virtual void		SetupCamera(NiVector3* pos, NiVector3* at, NiVector3* up, NiVector3* right, NiFrustum* frustum, float* viewport);
	virtual void		SetupScreenSpaceCamera(float* viewport);
	virtual bool		BeginUsingRenderTargetGroup(NiRenderTargetGroup* renderTarget, ClearFlags clearFlags);
	virtual bool		EndUsingRenderTargetGroup();
	virtual void		BeginBatch(UInt32 arg0, UInt32 arg1);
	virtual void		EndBatch();
	virtual void		BatchRenderShape(void* arg);
	virtual void		BatchRenderStrips(void* arg);
	virtual void		RenderTriShape(NiTriShape* obj);
	virtual void		RenderTriStrips(NiTriStrips* obj);
	virtual void		RenderTriShape2(NiTriShape* obj);
	virtual void		RenderTriStrips2(NiTriStrips* obj);
	virtual void		RenderParticles(NiParticles* obj);
	virtual void		RenderLines(NiLines* obj);
	virtual void		RenderScreenTexture();

	class PrePackObject;

	enum FrameBufferFormat
	{
		FBFMT_UNKNOWN = 0,
		FBFMT_R8G8B8,
		FBFMT_A8R8G8B8,
		FBFMT_X8R8G8B8,
		FBFMT_R5G6B5,
		FBFMT_X1R5G5B5,
		FBFMT_A1R5G5B5,
		FBFMT_A4R4G4B4,
		FBFMT_R3G3B2,
		FBFMT_A8,
		FBFMT_A8R3G3B2,
		FBFMT_X4R4G4B4,
		FBFMT_R16F,
		FBFMT_G16R16F,
		FBFMT_A16B16G16R16F,
		FBFMT_R32F,
		FBFMT_G32R32F,
		FBFMT_A32B32G32R32F,
		FBFMT_NUM
	};

	enum DepthStencilFormat
	{
		DSFMT_UNKNOWN = 0,
		DSFMT_D16_LOCKABLE = 70,
		DSFMT_D32 = 71,
		DSFMT_D15S1 = 73,
		DSFMT_D24S8 = 75,
		DSFMT_D16 = 80,
		DSFMT_D24X8 = 77,
		DSFMT_D24X4S4 = 79,
	};

	enum PresentationInterval
	{
		PRESENT_INTERVAL_IMMEDIATE = 0,
		PRESENT_INTERVAL_ONE = 1,
		PRESENT_INTERVAL_TWO = 2,
		PRESENT_INTERVAL_THREE = 3,
		PRESENT_INTERVAL_FOUR = 4,
		PRESENT_INTERVAL_NUM
	};

	enum SwapEffect
	{
		SWAPEFFECT_DEFAULT,
		SWAPEFFECT_DISCARD,
		SWAPEFFECT_FLIP,
		SWAPEFFECT_COPY,
		SWAPEFFECT_NUM
	};

	enum FrameBufferMode
	{
		FBMODE_DEFAULT,
		FBMODE_LOCKABLE,
		FBMODE_MULTISAMPLES_2 = 0x00010000,
		FBMODE_MULTISAMPLES_3 = 0x00020000,
		FBMODE_MULTISAMPLES_4 = 0x00030000,
		FBMODE_MULTISAMPLES_5 = 0x00040000,
		FBMODE_MULTISAMPLES_6 = 0x00050000,
		FBMODE_MULTISAMPLES_7 = 0x00060000,
		FBMODE_MULTISAMPLES_8 = 0x00070000,
		FBMODE_MULTISAMPLES_9 = 0x00080000,
		FBMODE_MULTISAMPLES_10 = 0x00090000,
		FBMODE_MULTISAMPLES_11 = 0x000a0000,
		FBMODE_MULTISAMPLES_12 = 0x000b0000,
		FBMODE_MULTISAMPLES_13 = 0x000c0000,
		FBMODE_MULTISAMPLES_14 = 0x000d0000,
		FBMODE_MULTISAMPLES_15 = 0x000e0000,
		FBMODE_MULTISAMPLES_16 = 0x000f0000,
		FBMODE_MULTISAMPLES_NONMASKABLE = 0x80000000,
		FBMODE_QUALITY_MASK = 0x0000FFFF,
		FBMODE_NUM = 18
	};

	enum RefreshRate
	{
		REFRESHRATE_DEFAULT = 0
	};

	UInt32								unk210[30];					// 210
	IDirect3DDevice9* device;					// 288
	UInt32								unk28C[76];					// 28C
	HANDLE								deviceWindow;				// 3BC
	HANDLE								focusWindow;				// 3C0
	char								rendererInfo[0x200];		// 3C4
	UInt32								adapterIdx;					// 5C4
	UInt32								d3dDevType;					// 5C8 - D3DDEVTYPE
	UInt32								d3dDevFlags;				// 5CC - D3DCREATE
	UInt8								softwareVertexProcessing;	// 5D0 - !D3DCREATE_HARDWARE_VERTEXPROCESSING
	UInt8								mixedVertexProcessing;		// 5D1 - D3DCREATE_MIXED_VERTEXPROCESSING
	UInt8								pad5D2[2];					// 5D2
	UInt32								unk5D4[3];					// 5D4
	UInt32								backgroundColor;			// 5E0	ARGB
	UInt32								unk5E4[11];					// 5E4
	NiTPointerMap<PrePackObject*>		prePackObjects;				// 610 - NiTPointerMap <NiVBBlock *, NiDX9Renderer::PrePackObject *>
	UInt32								unk620[153];				// 620
	NiRenderTargetGroup* defaultRTGroup;			// 884 - back buffer
	NiRenderTargetGroup* currentRTGroup;			// 888
	NiRenderTargetGroup* currentscreenRTGroup;		// 88C
	NiTPointerMap<NiRenderTargetGroup*>	screenRTGroups;				// 890 - NiTPointerMap <HWND *, NiPointer <NiRenderTargetGroup> >
	UInt32								unk8A0[6];					// 8A0
	NiDX9RenderState* renderState;				// 8B8
	UInt32								unk8BC[3];					// 8BC
	NiTMapBase<NiLight*, void*>* lightsMap;					// 8C8
	UInt32								unk8CC[115];				// 8CC
	UInt32								width;						// A98
	UInt32								height;						// A9C
	UInt32								flags;						// AA0
	UInt32								windowDevice;				// AA4
	UInt32								windowFocus;				// AA8
	UInt32								adapterType;				// AAC
	UInt32								deviceType;					// AB0
	FrameBufferFormat					frameBufferFormat;			// AB4
	DepthStencilFormat					depthStencilFormat;			// AB8
	PresentationInterval				presentationInterval;		// ABC
	SwapEffect							swapEffect;					// AC0
	FrameBufferMode						frameBufferMode;			// AC4
	UInt32								backBufferCount;			// AC8
	RefreshRate							refreshRate;				// ACC
	UInt32								unkAD0[44];					// AD0
};
STATIC_ASSERT(sizeof(NiDX9Renderer) == 0xB80);

// 70
class NiDX9TextureData : public NiObject
{
public:
	NiDX9TextureData();
	~NiDX9TextureData();

	virtual void	Unk_23(void);
	virtual void	Unk_24(void);
	virtual void	Unk_25(void);
	virtual void	Unk_26(void);
	virtual void	Unk_27(void);
	virtual void	Unk_28(void);
	virtual void	Unk_29(void);
	virtual void	Unk_2A(void);
	virtual void	Unk_2B(void);
	virtual void	Unk_2C(void);

	NiTexture* owningTexture;	// 08
	UInt32				unk0C[6];		// 0C
	UInt32				unk24;			// 24
	UInt32				unk28[14];		// 28
	NiDX9Renderer* renderer;		// 60
	UInt32				unk64;			// 64
	UInt32				unk68;			// 68
	UInt32				unk6C;			// 6C
};

// 30
class NiTexture : public NiObjectNET
{
public:
	NiTexture();
	~NiTexture();

	virtual void	Unk_23(void);
	virtual void	Unk_24(void);
	virtual void	Unk_25(void);
	virtual void	Unk_26(void);
	virtual void	Unk_27(void);
	virtual void	Unk_28(void);

	enum
	{
		kPxlLayout_Palette8BPP = 0,
		kPxlLayout_Raw16BPP,
		kPxlLayout_Raw32BPP,
		kPxlLayout_Compressed,
		kPxlLayout_Bumpmap,
		kPxlLayout_Palette4BPP,
		kPxlLayout_Default,

		kAlphaFmt_None = 0,
		kAlphaFmt_Binary1BPP,
		kAlphaFmt_Smooth8BPP,
		kAlphaFmt_Default,

		kMipMapFmt_Disabled = 0,
		kMipMapFmt_Enabled,
		kMipMapFmt_Default,
	};

	UInt32				pixelLayout;	// 18
	UInt32				alphaFormat;	// 1C
	UInt32				mipmapFormat;	// 20
	NiDX9TextureData* textureData;	// 24
	NiTexture* prev;			// 28
	NiTexture* next;			// 2C
};

// 48
class NiSourceTexture : public NiTexture
{
public:
	NiSourceTexture();
	~NiSourceTexture();

	virtual void	Unk_29(void);
	virtual void	Unk_2A(void);
	virtual void	Unk_2B(void);

	char* ddsPath1;		// 30
	char* ddsPath2;		// 34
	UInt32			unk38;			// 38
	UInt32			unk3C;			// 3C
	UInt8			byte40;			// 40
	UInt8			byte41;			// 41
	UInt8			byte42;			// 42
	UInt8			byte43;			// 43
	UInt32			unk44;			// 44
};

// 14
class Ni2DBuffer : public NiObject
{
public:
	Ni2DBuffer();
	~Ni2DBuffer();

	virtual void	Unk_23(void);
	virtual void	Unk_24(void);
	virtual void	Unk_25(void);

	UInt32		unk08[3];		// 08
};

// 48
class NiRenderedTexture : public NiTexture
{
public:
	NiRenderedTexture();
	~NiRenderedTexture();

	virtual void	Unk_29(void);

	Ni2DBuffer* buffer;	// 30
	UInt32				unk34;		// 34
	UInt32				unk38;		// 38
	UInt32				unk3C;		// 3C
	UInt32				unk40;		// 40
	UInt32				unk44;		// 44
};

class NiAccumulator : public NiObject
{
public:
	NiAccumulator();
	~NiAccumulator();

	virtual void	Unk_23(void);
	virtual void	Unk_24(void);
	virtual void	Unk_25(void);
	virtual void	Unk_26(UInt32 arg);
	virtual void	Unk_27(void);
	virtual void	Unk_28(void);
};

class NiBackToFrontAccumulator : public NiAccumulator
{
public:
	NiBackToFrontAccumulator();
	~NiBackToFrontAccumulator();

	virtual void	Unk_29(void);
};

// 34
class NiAlphaAccumulator : public NiBackToFrontAccumulator
{
public:
	NiAlphaAccumulator();
	~NiAlphaAccumulator();

	UInt32			unk08[11];		// 08
};

// 98
class BSBatchRenderer : public NiObject
{
public:
	BSBatchRenderer();
	~BSBatchRenderer();

	virtual void	Unk_23(void);
	virtual void	Unk_24(void);

	UInt32			unk08[36];		// 08
};
STATIC_ASSERT(sizeof(BSBatchRenderer) == 0x98);

// 280
class BSShaderAccumulator : public NiAlphaAccumulator
{
public:
	BSShaderAccumulator();
	~BSShaderAccumulator();

	virtual void	Unk_2A(void);
	virtual void	Unk_2B(void);
	virtual void	Unk_2C(UInt32 arg1, UInt32 arg2);

	struct AccumStruct	//	Temp name
	{
		void* _vtbl;	// 0x10B7DC0
		UInt32		unk04;
		UInt32		unk08;
		UInt32		unk0C;
		UInt32		unk10;
	};

	UInt32				unk034[4];		// 034
	float				flt044;			// 044
	UInt32				unk048[21];		// 048
	AccumStruct			accum09C;		// 09C
	AccumStruct			accum0B0;		// 0B0
	AccumStruct			accum0C4;		// 0C4
	AccumStruct			accum0D8;		// 0D8
	AccumStruct			accum0EC;		// 0EC
	AccumStruct			accum100;		// 100
	AccumStruct			accum114;		// 114
	AccumStruct			accum128;		// 128
	UInt32				unk13C[6];		// 13C
	float				flt154;			// 154
	float				flt158;			// 158
	float				flt15C;			// 15C
	float				flt160;			// 160
	UInt32				unk164[4];		// 164
	BSBatchRenderer* batchRenderer;	// 174
	UInt32				unk178[7];		// 178
	ShadowSceneNode* shadowScene;	// 194
	UInt32				unk198;			// 198
	UInt32				unk19C;			// 19C
	UInt32				unk1A0[56];		// 1A0
};
STATIC_ASSERT(sizeof(BSShaderAccumulator) == 0x280);

struct UVCoord
{
	float		x;
	float		y;

	UVCoord() {}
	UVCoord(float _x, float _y) : x(_x), y(_y) {}
};

struct NiTriangle
{
	UInt16		point1;
	UInt16		point2;
	UInt16		point3;
};

// 54
class RendererData		//	010F017C
{
public:
	RendererData();
	~RendererData();

	virtual RendererData* Destructor(bool doFree);
	virtual bool			Unk_01(UInt32 arg1);

	UInt32						flags;			// 04
	NiUnsharedGeometryGroup* unsharedGeom;	// 08
	UInt32						unk0C;			// 0C
	void* ptr10;			// 10
	UInt32						unk14;			// 14
	UInt32						unk18;			// 18	Vertices/Normals count
	UInt32						unk1C;			// 1C		"			"
	UInt32						finished;		// 20
	void* ptr24;			// 24
	void* ptr28;			// 28
	UInt32						trianglePoints;	// 2C
	UInt32						unk30;			// 30	Byte size of triangles array
	void* ptr34;			// 34
	UInt32						unk38;			// 38
	UInt32						unk3C;			// 3C
	UInt32						unk40;			// 40	Triangle count
	UInt32						unk44;			// 44		"
	UInt32						unk48;			// 48
	UInt32						unk4C;			// 4C
	NiTriangle* triangles;		// 50	Same ptr as in NiTriShapeData
};
STATIC_ASSERT(sizeof(RendererData) == 0x54);

// 40
class NiGeometryData : public NiObject
{
public:
	NiGeometryData();
	~NiGeometryData();

	virtual void	Unk_23(UInt32 arg);
	virtual void	Unk_24(void);
	virtual void	Unk_25(void);
	virtual void	Unk_26(void);
	virtual bool	Unk_27(UInt32 arg);
	virtual void	Unk_28(void);

	UInt16			numVertices;	// 08
	UInt16			word0A;			// 0A
	UInt16			word0C;			// 0C
	UInt16			word0E;			// 0E
	NiSphere		bounds;			// 10
	NiVector3* vertices;		// 20
	NiVector3* normals;		// 24
	NiColorAlpha* vertexColors;	// 28
	UVCoord* uvCoords;		// 2C
	UInt32			unk30;			// 30
	RendererData* rendererData;	// 34
	UInt8			byte38;			// 38
	UInt8			byte39;			// 39
	UInt8			byte3A;			// 3A
	UInt8			byte3B;			// 3B
	UInt32			unk3C;			// 3C
};
STATIC_ASSERT(sizeof(NiGeometryData) == 0x40);

// 44
class NiTriBasedGeomData : public NiGeometryData
{
public:
	NiTriBasedGeomData();
	~NiTriBasedGeomData();

	virtual void	Unk_29(UInt32 arg);
	virtual void	Unk_2A(void);
	virtual void	Unk_2B(UInt32 arg1, UInt32 arg2, UInt32 arg3, UInt32 arg4);
	virtual void	Unk_2C(UInt32 arg1, UInt32 arg2, UInt32 arg3, UInt32 arg4);

	UInt16			numTriangles;		// 40
	UInt8			pad42[2];			// 42
};

// 58
class NiTriShapeData : public NiTriBasedGeomData
{
public:
	NiTriShapeData();
	~NiTriShapeData();

	UInt32			trianglePoints;	// 44
	NiTriangle* triangles;		// 48
	UInt16* points;		// 4C
	UInt32			unk50;			// 50
	UInt32			unk54;			// 54
};
STATIC_ASSERT(sizeof(NiTriShapeData) == 0x58);

// 50
class NiTriStripsData : public NiTriBasedGeomData
{
public:
	NiTriStripsData();
	~NiTriStripsData();

	UInt16			unk44;			// 44
	UInt16			unk46;			// 46
	UInt32			unk48;			// 48
	UInt32			unk4C;			// 4C
};

// 14
class NiShader : public NiRefObject
{
public:
	NiShader();
	~NiShader();

	virtual void	Unk_02(void);
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
	virtual void	Unk_10(void);
	virtual void	Unk_11(void);
	virtual void	Unk_12(void);
	virtual void	Unk_13(void);
	virtual void	Unk_14(void);
	virtual void	Unk_15(void);
	virtual void	Unk_16(void);
	virtual void	Unk_17(void);
	virtual void	Unk_18(void);
	virtual void	Unk_19(void);
	virtual void	Unk_1A(void);
	virtual void	Unk_1B(void);
	virtual void	Unk_1C(void);
	virtual void	Unk_1D(void);
	virtual void	Unk_1E(void);
	virtual void	Unk_1F(void);
	virtual void	Unk_20(void);
	virtual void	Unk_21(void);
	virtual void	Unk_22(void);
	virtual void	Unk_23(void);
	virtual void	Unk_24(void);
	virtual void	Unk_25(void);
	virtual void	Unk_26(void);
	virtual void	Unk_27(void);
	virtual void	Unk_28(void);
	virtual void	Unk_29(void);
	virtual void	Unk_2A(void);
	virtual void	Unk_2B(void);
	virtual void	Unk_2C(void);
	virtual void	Unk_2D(void);
	virtual void	Unk_2E(void);

	UInt32			unk08;		// 08
	UInt32			unk0C;		// 0C
	UInt32			unk10;		// 10
};

// 90
class TileShader : public NiShader
{
public:
	TileShader();
	~TileShader();

	virtual void	Unk_2F(void);
	virtual void	Unk_30(void);
	virtual void	Unk_31(void);
	virtual void	Unk_32(void);
	virtual void	Unk_33(void);
	virtual void	Unk_34(void);
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
	virtual void	Unk_40(void);
	virtual void	Unk_41(void);
	virtual void	Unk_42(void);
	virtual void	Unk_43(void);
	virtual void	Unk_44(void);
	virtual void	Unk_45(void);
	virtual void	Unk_46(void);
	virtual void	Unk_47(void);
	virtual void	Unk_48(void);
	virtual void	Unk_49(void);
	virtual void	Unk_4A(void);
	virtual void	Unk_4B(void);
	virtual void	Unk_4C(void);
	virtual void	Unk_4D(void);
	virtual void	Unk_4E(void);
	virtual void	Unk_4F(void);
	virtual void	Unk_50(void);
	virtual void	Unk_51(void);
	virtual void	Unk_52(void);
	virtual void	Unk_53(void);

	UInt32			unk14[31];		// 14
};
STATIC_ASSERT(sizeof(TileShader) == 0x90);

// C4
class NiGeometry : public NiAVObject
{
public:
	NiGeometry();
	~NiGeometry();

	virtual void	Unk_37(UInt32 arg1);
	virtual void	Unk_38(UInt32 arg1);
	virtual void	Unk_39(void);
	virtual void	Unk_3A(void);
	virtual void	Unk_3B(UInt32 arg1);

	NiAlphaProperty* alphaProp;		// 9C
	NiCullingProperty* cullingProp;	// A0
	NiMaterialProperty* materialProp;	// A4
	BSShaderProperty* shaderProp;	// A8
	NiStencilProperty* stencilProp;	// AC
	NiTexturingProperty* texturingProp;	// B0
	UInt32					unkB4;			// B4
	NiGeometryData* geometryData;	// B8
	BSDismemberSkinInstance* skinInstance;	// BC
	NiShader* shader;		// C0
};
STATIC_ASSERT(sizeof(NiGeometry) == 0xC4);

// C4
class NiTriBasedGeom : public NiGeometry
{
public:
	NiTriBasedGeom();
	~NiTriBasedGeom();

	virtual void	Unk_3C(UInt32 arg1, UInt32 arg2, UInt32 arg3, UInt32 arg4);
};

// C4
class NiTriShape : public NiTriBasedGeom
{
public:
	NiTriShape();
	~NiTriShape();
};

// C4
class NiTriStrips : public NiTriBasedGeom
{
public:
	NiTriStrips();
	~NiTriStrips();
};

// D4
class BSScissorTriShape : public NiTriShape
{
public:
	BSScissorTriShape();
	~BSScissorTriShape();

	UInt32			unkC4;			// C4
	UInt32			unkC8;			// C8
	UInt32			width;			// CC
	UInt32			height;			// D0
};
STATIC_ASSERT(sizeof(BSScissorTriShape) == 0xD4);

// 14C
class ParticleShaderProperty : public BSShaderProperty
{
public:
	ParticleShaderProperty();
	~ParticleShaderProperty();

	UInt32			unk060[59];		// 060
};
STATIC_ASSERT(sizeof(ParticleShaderProperty) == 0x14C);

struct NiCulledGeoList
{
	NiGeometry** m_geo;		// 00
	UInt32			m_numItems;		// 04
	UInt32			m_bufLen;		// 08
	UInt32			m_bufGrowSize;	// 0C
};

// 90
class NiCullingProcess
{
public:
	NiCullingProcess();
	~NiCullingProcess();

	virtual void	Unk_00(void);
	virtual void	Unk_01(void);
	virtual void	Unk_02(void);
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
	virtual void	Destructor(bool freeMemory);
	virtual void	Unk_11(void* arg);
	virtual void	Cull(NiCamera* camera, ShadowSceneNode* scene, NiCulledGeoList* culledGeo);
	virtual void	AddGeo(NiGeometry* arg);

	UInt8				m_useAddGeoFn;	// 04 - call AddGeo when true, else just add to the list
	UInt8				pad05[3];		// 05
	NiCulledGeoList* m_culledGeo;	// 08
	UInt32				unk0C[33];		// 0C
};
STATIC_ASSERT(sizeof(NiCullingProcess) == 0x90);

// CC
class BSCullingProcess : public NiCullingProcess
{
public:
	BSCullingProcess();
	~BSCullingProcess();

	UInt32					unk90;			// 90
	UInt32					unk94[10];		// 94
	UInt32					unkBC;			// BC
	UInt32					unkC0;			// C0
	BSShaderAccumulator* shaderAccum;	// C4
	UInt32					unkC8;			// C8
};
STATIC_ASSERT(sizeof(BSCullingProcess) == 0xCC);