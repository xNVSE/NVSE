#pragma once
#include "SmallObjectsAllocator.h"

namespace Expression
{
	enum ScriptOperator;
}

namespace kEvaluator
{
	
	class Token
	{
	public:
		enum class Type
		{
			kNull, kOperand, kCommandRef, kOperator
		};
		Type type;
		union
		{
			double operand;
			Expression::ScriptOperator operator_;
			UInt8* scriptData;
		} value;
		bool temporary = false;

		Expression::ScriptOperator shortCircuitParentType;
		UInt32 shortCircuitDistance;
		UInt32 shortCircuitStackOffset;

		explicit Token(const double operand) : type(Type::kOperand) { value.operand = operand; }
		explicit Token(const Expression::ScriptOperator op) : type(Type::kOperator) { value.operator_ = op; }
		explicit Token(UInt8* const scriptData) : type(Type::kCommandRef) { value.scriptData = scriptData;  }

		void* operator new(size_t size);
		void operator delete(void* ptr);

		bool IsLogicalOperator() const;

		template <typename T>
		static Token* AllocateTemporary(T arg)
		{
			auto* token = new Token(arg);
			token->temporary = true;
			return token;
		}
		
	};


	class TokenList
	{
	public:
		Vector<Token> tokens;
		UInt32 appendAmount;
	};

	class TokenListMap
	{
		static Set<TokenListMap*> tlsInstances_;
	public:
		TokenListMap();
		static void ClearAll();
		UnorderedMap<UInt8*, TokenList> map;
	};
}



namespace Expression
{
	enum ScriptOperator
	{
		kOp_LeftBracket = 0x0,
		kOp_RightBracket = 0x1,
		kOp_LogicalAnd = 0x2,
		kOp_LogicalOr = 0x3,
		kOp_LessThanOrEqual = 0x4,
		kOp_LessThan = 0x5,
		kOp_GreaterThanOrEqual = 0x6,
		kOp_GreaterThan = 0x7,
		kOp_Equals = 0x8,
		kOp_NotEquals = 0x9,
		kOp_Minus = 0xA,
		kOp_Plus = 0xB,
		kOp_Multiply = 0xC,
		kOp_Divide = 0xD,
		kOp_Modulo = 0xE,
		kOp_Tilde = 0xF,
		kOp_MAX = 0x10,
	};
	
	enum ScriptErrors
	{
		kScriptError_NoError = 0x0,
		kScriptError_OutOfMemory = 0x1,
		kScriptError_StackOverflow = 0x2,
		kScriptError_StackUnderflow = 0x3,
		kScriptError_DivideByZero = 0x4,
		kScriptError_Syntax = 0x5,
		kScriptError_BadObjectPointer = 0x6,
	};


	struct __declspec(align(4)) Expression
	{
		struct __declspec(align(4)) Stack404
		{
			struct Member
			{
				UInt32 operand;
			};

			Member members[64];
			UInt32 count;
		};


		struct __declspec(align(4)) RPNStack
		{
			struct __declspec(align(4)) Token
			{
				UInt8 isFloat;
				UInt8 gap01[3];
				UInt32 opcode;
				UInt32 tokenResult;
				UInt32 tokenResultDouble;
			};

			Token members[32];
			UInt32 count;
		};


		ScriptErrors lastErrorID;
		UInt32 unk004;
		UInt32 unk008;
		UInt32 unk00C;
		UInt32 unk010;
		UInt32 unk014;
		UInt32 unk018;
		UInt32 unk01C;
		UInt32 unk020;
		UInt32 unk024;
		UInt32 unk028;
		UInt32 unk02C;
		UInt32 unk030;
		UInt32 unk034;
		UInt32 unk038;
		UInt32 unk03C;
		UInt32 unk040;
		UInt32 unk044;
		UInt32 unk048;
		UInt32 unk04C;
		UInt32 unk050;
		UInt32 unk054;
		UInt32 unk058;
		UInt32 unk05C;
		UInt32 unk060;
		UInt32 unk064;
		UInt32 unk068;
		UInt32 unk06C;
		UInt32 unk070;
		UInt32 unk074;
		UInt32 unk078;
		UInt32 unk07C;
		UInt32 unk080;
		UInt32 unk084;
		UInt32 unk088;
		UInt32 unk08C;
		UInt32 unk090;
		UInt32 unk094;
		UInt32 unk098;
		UInt32 unk09C;
		UInt32 unk0A0;
		UInt32 unk0A4;
		UInt32 unk0A8;
		UInt32 unk0AC;
		UInt32 unk0B0;
		UInt32 unk0B4;
		UInt32 unk0B8;
		UInt32 unk0BC;
		UInt32 unk0C0;
		UInt32 unk0C4;
		UInt32 unk0C8;
		UInt32 unk0CC;
		UInt32 unk0D0;
		UInt32 unk0D4;
		UInt32 unk0D8;
		UInt32 unk0DC;
		UInt32 unk0E0;
		UInt32 unk0E4;
		UInt32 unk0E8;
		UInt32 unk0EC;
		UInt32 unk0F0;
		UInt32 unk0F4;
		UInt32 unk0F8;
		UInt32 unk0FC;
		UInt32 unk100;
		UInt32 unk104;
		UInt32 unk108;
		UInt32 unk10C;
		UInt32 unk110;
		UInt32 unk114;
		UInt32 unk118;
		UInt32 unk11C;
		UInt32 unk120;
		UInt32 unk124;
		UInt32 unk128;
		UInt32 unk12C;
		UInt32 unk130;
		UInt32 unk134;
		UInt32 unk138;
		UInt32 unk13C;
		UInt32 unk140;
		UInt32 unk144;
		UInt32 unk148;
		UInt32 unk14C;
		UInt32 unk150;
		UInt32 unk154;
		UInt32 unk158;
		UInt32 unk15C;
		UInt32 unk160;
		UInt32 unk164;
		UInt32 unk168;
		UInt32 unk16C;
		UInt32 unk170;
		UInt32 unk174;
		UInt32 unk178;
		UInt32 unk17C;
		UInt32 unk180;
		UInt32 unk184;
		UInt32 unk188;
		UInt32 unk18C;
		UInt32 unk190;
		UInt32 unk194;
		UInt32 unk198;
		UInt32 unk19C;
		UInt32 unk1A0;
		UInt32 unk1A4;
		UInt32 unk1A8;
		UInt32 unk1AC;
		UInt32 unk1B0;
		UInt32 unk1B4;
		UInt32 unk1B8;
		UInt32 unk1BC;
		UInt32 unk1C0;
		UInt32 unk1C4;
		UInt32 unk1C8;
		UInt32 unk1CC;
		UInt32 unk1D0;
		UInt32 unk1D4;
		UInt32 unk1D8;
		UInt32 unk1DC;
		UInt32 unk1E0;
		UInt32 unk1E4;
		UInt32 unk1E8;
		UInt32 unk1EC;
		UInt32 unk1F0;
		UInt32 unk1F4;
		UInt32 unk1F8;
		UInt32 unk1FC;
		UInt32 unk200;
		UInt32 unk204;
		UInt32 unk208;
		UInt32 unk20C;
		UInt32 unk210;
		UInt32 unk214;
		UInt32 unk218;
		UInt32 unk21C;
		UInt32 unk220;
		UInt32 unk224;
		UInt32 unk228;
		UInt32 unk22C;
		UInt32 unk230;
		UInt32 unk234;
		UInt32 unk238;
		UInt32 unk23C;
		UInt32 unk240;
		UInt32 unk244;
		UInt32 unk248;
		UInt32 unk24C;
		UInt32 unk250;
		UInt32 unk254;
		UInt32 unk258;
		UInt32 unk25C;
		UInt32 unk260;
		UInt32 unk264;
		UInt32 unk268;
		UInt32 unk26C;
		UInt32 unk270;
		UInt32 unk274;
		UInt32 unk278;
		UInt32 unk27C;
		UInt32 unk280;
		UInt32 unk284;
		UInt32 unk288;
		UInt32 unk28C;
		UInt32 unk290;
		UInt32 unk294;
		UInt32 unk298;
		UInt32 unk29C;
		UInt32 unk2A0;
		UInt32 unk2A4;
		UInt32 unk2A8;
		UInt32 unk2AC;
		UInt32 unk2B0;
		UInt32 unk2B4;
		UInt32 unk2B8;
		UInt32 unk2BC;
		UInt32 unk2C0;
		UInt32 unk2C4;
		UInt32 unk2C8;
		UInt32 unk2CC;
		UInt32 unk2D0;
		UInt32 unk2D4;
		UInt32 unk2D8;
		UInt32 unk2DC;
		UInt32 unk2E0;
		UInt32 unk2E4;
		UInt32 unk2E8;
		UInt32 unk2EC;
		UInt32 unk2F0;
		UInt32 unk2F4;
		UInt32 unk2F8;
		UInt32 unk2FC;
		UInt32 unk300;
		UInt32 unk304;
		UInt32 unk308;
		UInt32 unk30C;
		UInt32 unk310;
		UInt32 unk314;
		UInt32 unk318;
		UInt32 unk31C;
		UInt32 unk320;
		UInt32 unk324;
		UInt32 unk328;
		UInt32 unk32C;
		UInt32 unk330;
		UInt32 unk334;
		UInt32 unk338;
		UInt32 unk33C;
		UInt32 unk340;
		UInt32 unk344;
		UInt32 unk348;
		UInt32 unk34C;
		UInt32 unk350;
		UInt32 unk354;
		UInt32 unk358;
		UInt32 unk35C;
		UInt32 unk360;
		UInt32 unk364;
		UInt32 unk368;
		UInt32 unk36C;
		UInt32 unk370;
		UInt32 unk374;
		UInt32 unk378;
		UInt32 unk37C;
		UInt32 unk380;
		UInt32 unk384;
		UInt32 unk388;
		UInt32 unk38C;
		UInt32 unk390;
		UInt32 unk394;
		UInt32 unk398;
		UInt32 unk39C;
		UInt32 unk3A0;
		UInt32 unk3A4;
		UInt32 unk3A8;
		UInt32 unk3AC;
		UInt32 unk3B0;
		UInt32 unk3B4;
		UInt32 unk3B8;
		UInt32 unk3BC;
		UInt32 unk3C0;
		UInt32 unk3C4;
		UInt32 unk3C8;
		UInt32 unk3CC;
		UInt32 unk3D0;
		UInt32 unk3D4;
		UInt32 unk3D8;
		UInt32 unk3DC;
		UInt32 unk3E0;
		UInt32 unk3E4;
		UInt32 unk3E8;
		UInt32 unk3EC;
		UInt32 unk3F0;
		UInt32 unk3F4;
		UInt32 unk3F8;
		UInt32 unk3FC;
		UInt32 unk400;
		Stack404 operandStack;
		RPNStack ifSetExprStack;
	};

}

void Hook_Evaluator();