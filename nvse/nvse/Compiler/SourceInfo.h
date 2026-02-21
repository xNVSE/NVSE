#pragma once

namespace Compiler {
    struct SourcePos {
        size_t absoluteIndex{};
        size_t line{};
        size_t column{};

		[[nodiscard]]
        SourcePos operator-(const size_t amount) const {
            return SourcePos{
                .absoluteIndex = absoluteIndex - amount,
                .line = line,
                .column = column - amount 
            };
        }
    };

    struct SourceSpan {
        SourcePos start{}, end{};

		[[nodiscard]]
        SourceSpan operator+(const SourceSpan& other) const {
			const auto& [otherStart, otherEnd] = other;

			SourcePos minSourcePos;
			if (start.absoluteIndex < otherStart.absoluteIndex) {
				minSourcePos = start;
			} else {
				minSourcePos = otherEnd;
			}

			SourcePos maxSourcePos;
			if (otherEnd.absoluteIndex > end.absoluteIndex) {
				maxSourcePos = otherEnd;
			}
			else {
				maxSourcePos = end;
			}

			return {
				.start = minSourcePos,
				.end = maxSourcePos
			};
		}
    };
}