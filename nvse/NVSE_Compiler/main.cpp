#include <iostream>
#include <string>

#include "NVSECompiler.h"
#include "NVSEParser.h"
#include "NVSETreePrinter.h"

int main() {
	NVSELexer lexer(""
		"if (57005) { if (100) { 255 + 255; } }"
	);
	NVSEParser parser(lexer);
	NVSETreePrinter treePrinter{};
	NVSECompiler comp{};

	if (auto ast = parser.parse()) {
		ast.get()->accept(&treePrinter);

		printf("\n");

		auto bytes = comp.compile(ast);
		for (int i = 0; i < bytes.size(); i++) {
			printf("%.2X ", bytes[i]);
		}

		printf("\nNum compiled bytes: %d\n", bytes.size());
	}
	return 0;
}