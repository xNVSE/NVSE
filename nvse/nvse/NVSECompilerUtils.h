#pragma once

enum class ArgType {
	Double, Int, String, Array, Ref
};

struct BeginInfo {
	uint16_t opcode;
	bool arg;
	bool argRequired;
	ArgType argType;
};

static std::unordered_map<std::string, BeginInfo> mpBeginInfo = {
	{"GameMode",                 BeginInfo{0,  false, false,  ArgType::Double}},
	{"MenuMode",                 BeginInfo{1,  true,  false,  ArgType::Int}},
	{"OnActivate",               BeginInfo{2,  false, false,  ArgType::Double}},
	{"OnAdd",                    BeginInfo{3,  true,  false,  ArgType::Ref}},
	{"OnEquip",                  BeginInfo{4,  true,  false,  ArgType::Ref}},
	{"OnUnequip",                BeginInfo{5,  true,  false,  ArgType::Ref}},
	{"OnDrop",                   BeginInfo{6,  true,  false,  ArgType::Ref}},
	{"SayToDone",                BeginInfo{7,  true,  false,  ArgType::Ref}},
	{"OnHit",                    BeginInfo{8,  true,  false,  ArgType::Ref}},
	{"OnHitWith",                BeginInfo{9,  true,  false,  ArgType::Ref}},
	{"OnDeath",                  BeginInfo{10, true,  false,  ArgType::Ref}},
	{"OnMurder",                 BeginInfo{11, true,  false,  ArgType::Ref}},
	{"OnCombatEnd",              BeginInfo{12, false, false,  ArgType::Double}},
	{"OnPackageStart",           BeginInfo{15, true,  true,   ArgType::Ref}},
	{"OnPackageDone",            BeginInfo{16, true,  true,   ArgType::Ref}},
	{"ScriptEffectStart",        BeginInfo{17, false, false,  ArgType::Double}},
	{"ScriptEffectFinish",       BeginInfo{18, false, false,  ArgType::Double}},
	{"ScriptEffectUpdate",       BeginInfo{19, false, false,  ArgType::Double}},
	{"OnPackageChange",          BeginInfo{20, true,  true,   ArgType::Ref}},
	{"OnLoad",                   BeginInfo{21, false, false,  ArgType::Double}},
	{"OnMagicEffectHit",         BeginInfo{22, true,  false,  ArgType::Ref}},
	{"OnSell",                   BeginInfo{23, true,  false,  ArgType::Ref}},
	{"OnTrigger",                BeginInfo{24, false, false,  ArgType::Double}},
	{"OnStartCombat",            BeginInfo{25, true,  false,  ArgType::Ref}},
	{"OnTriggerEnter",           BeginInfo{26, true,  false,  ArgType::Ref}},
	{"OnTriggerLeave",           BeginInfo{27, true,  false,  ArgType::Ref}},
	{"OnActorEquip",             BeginInfo{28, true,  true,   ArgType::Ref}},
	{"OnActorUnequip",           BeginInfo{29, true,  true,   ArgType::Ref}},
	{"OnReset",                  BeginInfo{30, false, false,  ArgType::Double}},
	{"OnOpen",                   BeginInfo{31, false, false,  ArgType::Double}},
	{"OnClose",                  BeginInfo{32, false, false,  ArgType::Double}},
	{"OnGrab",                   BeginInfo{33, false, false,  ArgType::Double}},
	{"OnRelease",                BeginInfo{34, false, false,  ArgType::Double}},
	{"OnDestructionStageChange", BeginInfo{35, false, false,  ArgType::Double}},
	{"OnFire",                   BeginInfo{36, false, false,  ArgType::Double}},
	{"OnNPCActivate",            BeginInfo{37, false, false,  ArgType::Double}},
};

// Define tokens
static std::unordered_map<std::string, uint8_t> operatorMap{
	{"=", 0},
	{"||", 1},
	{"&&", 2},
	{":", 3},
	{"==", 4},
	{"!=", 5},
	{">", 6},
	{"<", 7},
	{">=", 8},
	{"<=", 9},
	{"|", 10},
	{"&", 11},
	{"<<", 12},
	{">>", 13},
	{"+", 14},
	{"-", 15},
	{"*", 16},
	{"/", 17},
	{"%", 18},
	{"^", 19},
	{"-", 20},
	{"!", 21},
	{"(", 22},
	{")", 23},
	{"[", 24},
	{"]", 25},
	{"<-", 26},
	{"$", 27},
	{"+=", 28},
	{"*=", 29},
	{"/=", 30},
	{"^=", 31},
	{"-=", 32},
	{"#", 33},
	{"*", 34},
	{"->", 35},
	{"::", 36},
	{"&", 37},
	{"{", 38},
	{"}", 39},
	{".", 40},
	{"|=", 41},
	{"&=", 42},
	{"%=", 43},
};

inline uint8_t getArgType(NVSEToken t) {
	switch (t.type) {
	case NVSETokenType::DoubleType:
		return 0;
	case NVSETokenType::IntType:
		return 1;
	case NVSETokenType::RefType:
		return 4;
	case NVSETokenType::ArrayType:
		return 3;
	case NVSETokenType::StringType:
		return 2;
	}

	return 0;
}