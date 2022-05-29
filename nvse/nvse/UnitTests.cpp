#if RUNTIME

#include "UnitTests.h"
#include "GameAPI.h"
#include "FunctionScripts.h"
#include <fstream>
#include <string>
#include <sstream>
#include <filesystem>

namespace fs = std::filesystem;

std::string get_stem(const fs::path& p) { return p.stem().string(); }

namespace ScriptFunctionTests
{
	void RunTests()
	{
		for (const auto& entry : fs::directory_iterator(fs::current_path() / "data/nvse/unit_tests"))
		{
			if (!entry.is_regular_file())
				continue;

			std::ifstream f(entry.path());
			if (!f)
				continue;
			std::ostringstream ss;
			ss << f.rdbuf();
			const std::string& str = ss.str();
			std::string fileName = get_stem(entry);

			if (auto const script = CompileScriptEx(str.c_str(), fileName.c_str())) [[likely]]
			{
				PluginAPI::CallFunctionScriptAlt(script, nullptr, 0);
			}
			else
			{
				Console_Print("Error in xNVSE unit test file %s: script failed to compile.", 
					fileName.c_str());
			}
		}
		Console_Print("Finished running xNVSE script unit tests.");
	}
}

namespace JIPContainerTests
{
	void TestUnorderedMap()
	{
		// Test JIP UnorderedMap's reference validity.
		UnorderedMap<const char*, std::string> map;

		std::string* outStr;

		map.Insert("1", &outStr);
		*outStr = "test";
		std::string& ref = *outStr;

		for (int i = 2; i < 200; i++)
		{
			map.Insert(std::to_string(i).c_str(), &outStr);
			*outStr = "testR" + std::to_string(i);
		}

		map.Insert("200", &outStr);
		*outStr = "test2";
		std::string& ref2 = *outStr;

		for (int i = 201; i < 400; i++)
		{
			map.Insert(std::to_string(i).c_str(), &outStr);
			*outStr = "testR" + std::to_string(i);
		}

		ASSERT(ref == "test");
		ASSERT(ref2 == "test2");

		//... add more?

		Console_Print("Finished running xNVSE UnorderedMap unit tests.");
	}
}

void ExecuteRuntimeUnitTests()
{
	if (!s_AreRuntimeTestsEnabled)
		return;

	ScriptFunctionTests::RunTests();
	JIPContainerTests::TestUnorderedMap();


}

#endif