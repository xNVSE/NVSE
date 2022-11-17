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

namespace ScriptTokenizerTests
{
	void RunTests()
	{
		// Test single line, two tokens
		{
			std::string const scriptText = "I_am_the_first_valid_token, I'm_the_second\n";
			ScriptTokenizer tokenizer(scriptText);

			ASSERT(tokenizer.TryLoadNextLine() == true);

			auto tokenView = tokenizer.GetNextLineToken();
			ASSERT(tokenView == "I_am_the_first_valid_token,");
		}

		// Test single line, two tokens, with regular ';' comments that consume a would-be multiline comment.
		{
			std::string scriptText = "; /*\n; This is NOT counted as a multiline comment! Needs ';' just to compile.\n\n; */\n";
			scriptText += "I_am_the_first_valid_token, I'm_the_second\n";
			ScriptTokenizer tokenizer(scriptText);

			ASSERT(tokenizer.TryLoadNextLine() == true);

			auto tokenView = tokenizer.GetNextLineToken();
			ASSERT(tokenView == "I_am_the_first_valid_token,");

			tokenView = tokenizer.GetNextLineToken();
			ASSERT(tokenView == "I'm_the_second");

			ASSERT(tokenizer.TryLoadNextLine() == false);
		}

		// Test single line, two tokens, with a multi-line comment that ends in the line,
		// .. but the comment is continued by ';' in the middle of the multiline comment.
		{
			std::string scriptText = "/*\n\nI am ignored; */ This is still a comment, even though the multiline comment ended!\n\n";
			scriptText += "I_am_the_first_valid_token";
			ScriptTokenizer tokenizer(scriptText);

			ASSERT(tokenizer.TryLoadNextLine() == true);

			auto tokenView = tokenizer.GetNextLineToken();
			ASSERT(tokenView == "I_am_the_first_valid_token");

			tokenView = tokenizer.GetNextLineToken();
			ASSERT(tokenView.empty() == true);

			ASSERT(tokenizer.TryLoadNextLine() == false);
		}

		// Test two lines: 1st has 2 tokens, 2nd has 1.
		{
			std::string scriptText = "I_am_the_first_valid_token_on_line_1, I'm_the_second\n";
			scriptText += "I_am_the_first_valid_token_on_line_2";
			ScriptTokenizer tokenizer(scriptText);

			ASSERT(tokenizer.TryLoadNextLine() == true);

			auto tokenView = tokenizer.GetNextLineToken();
			ASSERT(tokenView == "I_am_the_first_valid_token_on_line_1,");

			tokenView = tokenizer.GetNextLineToken();
			ASSERT(tokenView == "I'm_the_second");

			ASSERT(tokenizer.TryLoadNextLine() == true);

			tokenView = tokenizer.GetNextLineToken();
			ASSERT(tokenView == "I_am_the_first_valid_token_on_line_2");
		}

		{
			std::string scriptText = "\n\tI_am_the_first_valid_token_on_line_1,\t  I'm_the_second\t \t \n";
			scriptText += "\n\n\t   \t\nI_am_the_first_valid_token_on_line_2  ";
			ScriptTokenizer tokenizer(scriptText);

			ASSERT(tokenizer.TryLoadNextLine() == true);

			auto tokenView = tokenizer.GetNextLineToken();
			ASSERT(tokenView == "I_am_the_first_valid_token_on_line_1,");

			tokenView = tokenizer.GetNextLineToken();
			ASSERT(tokenView == "I'm_the_second");

			ASSERT(tokenizer.TryLoadNextLine() == true);

			tokenView = tokenizer.GetNextLineToken();
			ASSERT(tokenView == "I_am_the_first_valid_token_on_line_2");
		}

		// Test two lines, with comments in-between: 1st has 2 tokens, 2nd has 1.
		{
			std::string scriptText = "/* */ \tI_am_the_first_valid_token_on_line_1,/* */\t /* */  I'm_the_second /* */ \n";
			scriptText += " \t\nI_am_the_first_valid_token_on_line_2/* */;\t\n";
			ScriptTokenizer tokenizer(scriptText);

			ASSERT(tokenizer.TryLoadNextLine() == true);

			auto tokenView = tokenizer.GetNextLineToken();
			ASSERT(tokenView == "I_am_the_first_valid_token_on_line_1,");

			tokenView = tokenizer.GetNextLineToken();
			ASSERT(tokenView == "I'm_the_second");

			ASSERT(tokenizer.TryLoadNextLine() == true);

			tokenView = tokenizer.GetNextLineToken();
			ASSERT(tokenView == "I_am_the_first_valid_token_on_line_2");
		}

		// Test one line with 1 valid token, but ending with a multiline comment.
		{
			std::string scriptText = "/* *//* */  /* */ Finally,_I_Am_A_Real_Token. /* \n */\n";
			ScriptTokenizer tokenizer(scriptText);

			ASSERT(tokenizer.TryLoadNextLine() == true);

			auto tokenView = tokenizer.GetNextLineToken();
			ASSERT(tokenView == "Finally,_I_Am_A_Real_Token.");

			tokenView = tokenizer.GetNextLineToken();
			ASSERT(tokenView.empty());

			ASSERT(tokenizer.TryLoadNextLine() == false);
			tokenView = tokenizer.GetNextLineToken();
			ASSERT(tokenView.empty());
		}

		// Test two valid lines (with the rest empty), with comments in-between: 1st has 2 tokens, 2nd has 1.
		{
			std::string scriptText = "\n\n\t\r/* */ \tI_am_the_first_valid_token_on_line_1,/* */\t /* */  I'm_the_second /* */ \n";
			scriptText += ";I am commented out\n";
			scriptText += "\n\t   ; As am I!\n";
			scriptText += "\n\t /*No way, me too!*/ ; \t \n";
			scriptText += " \t\nI_am_the_first_valid_token_on_line_2/* */;\t\n";
			scriptText += "\n\t; /*Ignore me/ \t \n";
			scriptText += "\n\t  \t \n";
			ScriptTokenizer tokenizer(scriptText);

			ASSERT(tokenizer.TryLoadNextLine() == true);

			auto tokenView = tokenizer.GetNextLineToken();
			ASSERT(tokenView == "I_am_the_first_valid_token_on_line_1,");

			tokenView = tokenizer.GetNextLineToken();
			ASSERT(tokenView == "I'm_the_second");

			ASSERT(tokenizer.TryLoadNextLine() == true);

			tokenView = tokenizer.GetNextLineToken();
			ASSERT(tokenView == "I_am_the_first_valid_token_on_line_2");
		}

		// Test one valid token, which is a string with a comment character ';' in it.
		// Should ignore the comment character, as it is in a string.
		{
			std::string scriptText = "\" ;I am a complete and valid token! \" ; ignore me";
			ScriptTokenizer tokenizer(scriptText);

			ASSERT(tokenizer.TryLoadNextLine() == true);
			auto tokenView = tokenizer.GetNextLineToken();
			ASSERT(tokenView == "\" ;I am a complete and valid token! \"");
		}

		// Same as above, but with tweaked end of script text.
		{
			std::string scriptText = "\" ;I am a complete and valid token! \"";
			ScriptTokenizer tokenizer(scriptText);

			ASSERT(tokenizer.TryLoadNextLine() == true);
			auto tokenView = tokenizer.GetNextLineToken();
			ASSERT(tokenView == "\" ;I am a complete and valid token! \"");
		}

		// Same as above, but with more tokens.
		{
			std::string scriptText = "\" ;I am a complete and valid token! \" \" I /* am */ the 2nd token!\"\n";
			scriptText += "/* */ 1 + 1;";
			ScriptTokenizer tokenizer(scriptText);

			ASSERT(tokenizer.TryLoadNextLine() == true);

			auto tokenView = tokenizer.GetNextLineToken();
			ASSERT(tokenView == "\" ;I am a complete and valid token! \"");

			tokenView = tokenizer.GetNextLineToken();
			ASSERT(tokenView == "\" I /* am */ the 2nd token!\"");

			tokenView = tokenizer.GetNextLineToken();
			ASSERT(tokenView.empty());

			// 2nd line
			ASSERT(tokenizer.TryLoadNextLine() == true);

			tokenView = tokenizer.GetNextLineToken();
			ASSERT(tokenView == "1");

			tokenView = tokenizer.GetNextLineToken();
			ASSERT(tokenView == "+");

			tokenView = tokenizer.GetNextLineToken();
			ASSERT(tokenView == "1");

			tokenView = tokenizer.GetNextLineToken();
			ASSERT(tokenView.empty());
		}

		// Test no tokens
		{
			std::string const scriptText = "\n \t   ;";
			ScriptTokenizer tokenizer(scriptText);

			ASSERT(tokenizer.TryLoadNextLine() == false);

			auto tokenView = tokenizer.GetNextLineToken();
			ASSERT(tokenView.empty());
		}

		{
			std::string const scriptText = "";
			ScriptTokenizer tokenizer(scriptText);

			ASSERT(tokenizer.TryLoadNextLine() == false);

			auto tokenView = tokenizer.GetNextLineToken();
			ASSERT(tokenView.empty());
		}

		{
			std::string const scriptText = "  ";
			ScriptTokenizer tokenizer(scriptText);

			ASSERT(tokenizer.TryLoadNextLine() == false);

			auto tokenView = tokenizer.GetNextLineToken();
			ASSERT(tokenView.empty());
		}

		{
			std::string const scriptText = "/*  */";
			ScriptTokenizer tokenizer(scriptText);

			ASSERT(tokenizer.TryLoadNextLine() == false);

			auto tokenView = tokenizer.GetNextLineToken();
			ASSERT(tokenView.empty());
		}

		{
			std::string const scriptText = "/*  */\n/* */\t";
			ScriptTokenizer tokenizer(scriptText);

			ASSERT(tokenizer.TryLoadNextLine() == false);

			auto tokenView = tokenizer.GetNextLineToken();
			ASSERT(tokenView.empty());
		}

		{
			std::string const scriptText = "/* \n \t \t  \n */ \n";
			ScriptTokenizer tokenizer(scriptText);

			ASSERT(tokenizer.TryLoadNextLine() == false);

			auto tokenView = tokenizer.GetNextLineToken();
			ASSERT(tokenView.empty());
		}

		Console_Print("Finished running xNVSE ScriptTokenizer unit tests.");
	}
}

void ExecuteRuntimeUnitTests()
{
	if (!s_AreRuntimeTestsEnabled)
		return;

	ScriptFunctionTests::RunTests();
	JIPContainerTests::TestUnorderedMap();
	ScriptTokenizerTests::RunTests();

}

#endif