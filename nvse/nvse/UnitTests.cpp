#if RUNTIME

#include "UnitTests.h"
#include "GameAPI.h"
#include "FunctionScripts.h"
#include <fstream>
#include <string>
#include <sstream>
#include <filesystem>

namespace fs = std::filesystem;

void RunScriptUnitTests()
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
		const std::string &str = ss.str();

		auto const script = CompileScript(str.c_str());
		PluginAPI::CallFunctionScriptAlt(script, nullptr, 0);
	}
	Console_Print("Finished running xNVSE script unit tests.");
}

#endif