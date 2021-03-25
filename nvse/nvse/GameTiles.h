#pragma once

#include "NiNodes.h"
#include "GameTypes.h"

typedef UInt32 (* _TraitNameToID)(const char* traitName);
extern const _TraitNameToID TraitNameToID;

const char * TraitIDToName(int id);	// slow

//	Tile			
//		TileRect		3C	ID=385
//			TileMenu	40	ID=389
//		TileImage		48	ID=386
//			RadialTile		ID=38C but answer as TileImage !
//		TileText		4C	ID=387
//		Tile3D			50	ID=388
//	Close by:
//		HotRect	ID=38A
//		Window	ID=38B

class NiNode;
class Menu;
class NiObject;
class RefNiObject;

// 38+
class Tile
{
public:
	Tile();
	~Tile();

	enum eTileValue {
		kTileValue_x					= 0x0FA1,
		kTileValue_y,
		kTileValue_visible,
		kTileValue_class,
		kTileValue_clipwindow			= 0x0FA6,
		kTileValue_stackingtype,
		kTileValue_locus,
		kTileValue_alpha,
		kTileValue_id,
		kTileValue_disablefade,
		kTileValue_listindex,
		kTileValue_depth,
		kTileValue_clips,
		kTileValue_target,
		kTileValue_height,
		kTileValue_width,
		kTileValue_red,
		kTileValue_green,
		kTileValue_blue,
		kTileValue_tile,
		kTileValue_childcount,
		kTileValue_child_count			= kTileValue_childcount,
		kTileValue_justify,
		kTileValue_zoom,
		kTileValue_font,
		kTileValue_wrapwidth,
		kTileValue_wraplimit,
		kTileValue_wraplines,
		kTileValue_pagenum,
		kTileValue_ishtml,
		kTileValue_cropoffsety,
		kTileValue_cropy				= kTileValue_cropoffsety,
		kTileValue_cropoffsetx,
		kTileValue_cropx				= kTileValue_cropoffsetx,
		kTileValue_menufade,
		kTileValue_explorefade,
		kTileValue_mouseover,
		kTileValue_string,
		kTileValue_shiftclicked,
		kTileValue_clicked				= 0x0FC7,
		kTileValue_clicksound			= 0x0FCB,
		kTileValue_filename,
		kTileValue_filewidth,
		kTileValue_fileheight,
		kTileValue_repeatvertical,
		kTileValue_repeathorizontal,
		kTileValue_animation			= 0x0FD2,
		kTileValue_linecount			= 0x0DD4,
		kTileValue_pagecount,
		kTileValue_xdefault,
		kTileValue_xup,
		kTileValue_xdown,
		kTileValue_xleft,
		kTileValue_xright,
		kTileValue_xbuttona				= 0x0FDD,
		kTileValue_xbuttonb,
		kTileValue_xbuttonx,
		kTileValue_xbuttony,
		kTileValue_xbuttonlt,
		kTileValue_xbuttonrt,
		kTileValue_xbuttonlb,
		kTileValue_xbuttonrb,
		kTileValue_xbuttonstart			= 0x0FE7,
		kTileValue_mouseoversound,
		kTileValue_draggable,
		kTileValue_dragstartx,
		kTileValue_dragstarty,
		kTileValue_dragoffsetx,
		kTileValue_dragoffsety,
		kTileValue_dragdeltax,
		kTileValue_dragdeltay,
		kTileValue_dragx,
		kTileValue_dragy,
		kTileValue_wheelable,
		kTileValue_wheelmoved,
		kTileValue_systemcolor,
		kTileValue_brightness,
		kTileValue_linegap				= 0x0FF7,
		kTileValue_resolutionconverter,
		kTileValue_texatlas,
		kTileValue_rotateangle,
		kTileValue_rotateaxisx,
		kTileValue_rotateaxisy,

		kTileValue_user0				= 0x01004,
		kTileValue_user1,
		kTileValue_user2,
		kTileValue_user3,
		kTileValue_user4,
		kTileValue_user5,
		kTileValue_user6,
		kTileValue_user7,
		kTileValue_user8,
		kTileValue_user9,
		kTileValue_user10,
		kTileValue_user11,
		kTileValue_user12,
		kTileValue_user13,
		kTileValue_user14,
		kTileValue_user15,
		kTileValue_user16,

		kTileValue_max
	};

	enum eTileID {
		kTileID_rect		= 0x0385,
		kTileID_image,
		kTileID_text,
		kTileID_3D,
		kTileID_nif			= kTileID_3D,
		kTileID_menu,

		// Not a Tile descendend
		kTileID_hotrect,
		kTileID_window,
		// This one descend from TileImage
		kTileID_radial,

		kTileID_max
	};

	MEMBER_FN_PREFIX(Tile);
#if RUNTIME
	DEFINE_MEMBER_FN(SetStringValue, void, 0x00A01350, UInt32 valueID, const char* str, bool bPropagate);
	DEFINE_MEMBER_FN(SetFloatValue, void, 0x00A012D0, UInt32 valueID, float num, bool bPropagate);
#endif

	virtual void			Destroy(bool noDealloc);
	virtual void			Init(Tile * parent, const char * name, Tile* replacedChild);
	virtual NiNode *		CalcNode(void);
	virtual UInt32			GetType(void);		// returns one of kTileValue_XXX
	virtual const char *	GetTypeStr(void);	// 4-byte id
	virtual bool			Unk_05(UInt32 arg0, UInt32 arg1);
	virtual UInt32			Unk_06(UInt32 arg0, UInt32 arg1, UInt32 arg2);
	virtual void			Unk_07(void);
	virtual UInt32			Unk_08(void);
	virtual void			Unk_09(UInt32 arg0, UInt32 arg1, UInt32 arg2);

	struct Value;

	enum ActionType
	{
		kAction_copy = 0x7D0,
		kAction_add,
		kAction_sub,
		kAction_mul,
		kAction_div,
		kAction_min,
		kAction_max,
		kAction_mod,
		kAction_floor,
		kAction_ceil,
		kAction_abs,
		kAction_round,
		kAction_gt,
		kAction_gte,
		kAction_eq,
		kAction_neq,
		kAction_lt,
		kAction_lte,
		kAction_and,
		kAction_or,
		kAction_not,
		kAction_onlyif,
		kAction_onlyifnot,
		kAction_ref,
		kAction_begin,
		kAction_end,
	};

	// 0C
	class Action
	{
	public:
		Action();
		~Action();

		virtual float	GetFloat();
		virtual Value	*GetValue();

		UInt32		type;		// 04
		Action		*next;		// 08
	};

	// 10
	class RefValueAction : public Action
	{
	public:
		RefValueAction();
		~RefValueAction();

		Value		*tileVal;	// 0C
	};

	// 10
	class FloatAction : public Action
	{
	public:
		FloatAction();
		~FloatAction();

		float		value;		// 0C
	};
	
	// 14
	struct Value
	{
		UInt32		id;			// 00
		Tile		* parent;	// 04
		float		num;		// 08
		char		* str;		// 0C
		Action		* action;	// 10
	};

	struct ChildNode
	{
		ChildNode	* next;		// 000
		ChildNode	* prev;		// 004
		Tile		* child;	// 008
	};

	tList <ChildNode>			childList;	// 04
	UInt32						unk0C;		// 0C looks like childcount, share initiator with EntryData
	BSSimpleArray <Value *>		values;		// 10
	String						name;		// 20
	Tile						* parent;	// 28
	NiNode						* node;		// 2C
	UInt32						flags;		// 30
	UInt8						unk34;		// 34
	UInt8						unk35;		// 35
	UInt8						pad35[2];	// 36

	static UInt32	TraitNameToID(const char * traitName);
	Value *			GetValue(UInt32 typeID);
	Value *			GetValueName(const char * valueName);
	Tile *			GetChild(const char * childName);
	Tile *			GetComponent(const char * componentTile, const char **trait);
	Tile *			GetComponentTile(const char * componentTile);
	Value *			GetComponentValue(const char * componentPath);
	char			*GetComponentFullName(char *resStr);

	void			Dump(void);

	Tile *GetChildAlt(const char *childName);
	Value *GetComponentValueAlt(const char *componentPath);
};

// 3C
class TileRect : public Tile
{
public:
	UInt32	unk38;	// 38
};

// 40
class TileMenu : public TileRect
{
public:
	Menu	* menu;	// 3C - confirmed
};

// 48
class TileImage : public Tile
{
public:
	float		flt038;		// 38
	RefNiObject unk03C;		// 3C
	RefNiObject unk040;		// 40
	UInt8		byt044;		// 44
	UInt8		fill[3];	// 45-47
};

class TileText : public Tile
{
public:
};

void Debug_DumpTraits(void);
void Debug_DumpTileImages(void);
