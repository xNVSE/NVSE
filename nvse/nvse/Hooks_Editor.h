#pragma once

#if EDITOR
void Hook_Editor_Init(void);
void CreateHookWindow(void);

void FixEditorFont(void);

// Add "string_var" as alias for "long"
void CreateTokenTypedefs(void);

// Disable check on vanilla opcode range for use of commands as conditionals
void PatchConditionalCommands(void);

// Allow use of special characters '$', '[', and ']' in string params to script commands
void PatchIsAlpha(void);

// Handle new param types
void PatchDefaultCommandParser();

#endif