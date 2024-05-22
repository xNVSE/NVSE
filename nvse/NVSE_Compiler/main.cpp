#include <string>
#include "NVSEParser.h"
#include "NVSETreePrinter.h"

int main() {
	NVSELexer nvl(
		"player.name = \"hello\""
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