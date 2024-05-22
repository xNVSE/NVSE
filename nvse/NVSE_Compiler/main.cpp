#include <string>
#include "NVSEParser.h"
#include "NVSETreePrinter.h"

int main() {
	NVSELexer nvl(
		"GetPlayer().name = 1 == 3 ? (10 + 10) : test(1,2,3)"
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