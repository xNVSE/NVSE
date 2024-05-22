#include <iostream>
#include <string>
#include "NVSEParser.h"
#include "NVSETreePrinter.h"

int main() {
	NVSELexer lexer(
		"{						\n"
		"    x = fn(int x == 2, ref y) {};	\n"
		"}   "
	);
	NVSEParser parser(lexer);
	NVSETreePrinter treePrinter{};
	if (auto ast = parser.parse()) {
		ast.get()->accept(&treePrinter);
	}

	printf("\n");
	return 0;
}