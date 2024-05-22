#include <iostream>
#include <string>
#include "NVSEParser.h"
#include "NVSETreePrinter.h"

int main() {
	NVSELexer nvl(
		""
		"while (true) {						\n"
		"  print(true);						\n"
		"  GetPlayer().name = \"foobar\";	\n"
		"  if (10 / 2 == 5) {				\n"
		"     print(\"foo\");               \n"
		"  } else {                         \n"
		"     print(\"bar\");               \n"
		"  }								\n"
		"}"
	);

	NVSEParser p(nvl);

	const auto a = p.parse();

	NVSETreePrinter tp{};
	if (a) {
		a.get()->accept(&tp);
	}

	std::cout << std::endl;
	return 0;
}