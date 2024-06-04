#pragma once

#ifdef RUNTIME
namespace StackVariables
{
	using Index_t = int;
	using Value_t = double;

	struct LocalStackFrame {
		std::vector<Value_t> vars{};

		Value_t& get(Index_t idx) {
			if (idx >= vars.size()) {
				vars.resize(idx + 10, 0);
			}

			return vars[idx];
		}

		void set(Index_t idx, Value_t val) {
			if (idx >= vars.size()) {
				vars.resize(idx + 10, 0);
			}

			vars[idx] = val;
		}
	};

	inline thread_local std::vector<LocalStackFrame> g_localStackVars;
	inline thread_local Index_t g_localStackPtr{ -1 };

	void SetLocalStackVarVal(Index_t idx, Value_t val);
	Value_t& GetLocalStackVarVal(Index_t idx);

	void PushLocalStack();
	void PopLocalStack();
}
#endif