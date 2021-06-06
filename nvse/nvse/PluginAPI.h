#pragma once

#include "nvse/CommandTable.h"
#include "nvse/Utilities.h"

#if RUNTIME
#include "GameAPI.h"
#endif

struct CommandInfo;
struct ParamInfo;
class TESObjectREFR;
class Script;
class TESForm;
struct ScriptEventList;
struct ArrayKey;
namespace PluginAPI { class ArrayAPI; }
struct PluginInfo;

typedef UInt32	PluginHandle;	// treat this as an opaque type

enum
{
	kPluginHandle_Invalid = 0xFFFFFFFF,
};

enum
{
	kPluginOpcode_Debug = kNVSEOpcodeTest,
};

enum
{
	kInterface_Serialization = 0,
	kInterface_Console,

	// Added v0002
	kInterface_Messaging,
	kInterface_CommandTable,

	// Added v0004
	kInterface_StringVar,
	kInterface_ArrayVar,
	kInterface_Script,

	// Added v0005 - version bumped to 3
	kInterface_Data,

	kInterface_Max
};

struct ExpressionEvaluatorUtils;

struct NVSEInterface
{
	UInt32	nvseVersion;
	UInt32	runtimeVersion;
	UInt32	editorVersion;
	UInt32	isEditor;
	bool	(* RegisterCommand)(CommandInfo * info);	// returns true for success, false for failure
	void	(* SetOpcodeBase)(UInt32 opcode);
	void *	(* QueryInterface)(UInt32 id);

	// call during your Query or Load functions to get a PluginHandle uniquely identifying your plugin
	// invalid if called at any other time, so call it once and save the result
	PluginHandle	(* GetPluginHandle)(void);

	// CommandReturnType enum defined in CommandTable.h
	// does the same as RegisterCommand but includes return type; *required* for commands returning arrays
	bool	(* RegisterTypedCommand)(CommandInfo * info, CommandReturnType retnType);
	// returns a full path the the game directory
	const char* (* GetRuntimeDirectory)();

	// Allows checking for nogore edition
	UInt32	isNogore;

	void		(*InitExpressionEvaluatorUtils)(ExpressionEvaluatorUtils *utils);
};

struct NVSEConsoleInterface
{
	enum
	{
		kVersion = 3
	};

	UInt32	version;

	// return type changed from void to bool in kVersion == 2
	bool	(* RunScriptLine)(const char * buf, TESObjectREFR * object);	// NULL acceptable for object parameter

	// like RunScriptLine, but accepts a calling refr (or NULL) and optionally suppresses all console output for the duration
	bool	(* RunScriptLine2)(const char * buf, TESObjectREFR* callingRefr, bool bSuppressConsoleOutput);
};

/***** string_var API *****************************
*
* string_var is NVSE's string pseudo-datatype. Strings are represented internally by
* integer IDs, however the details of the implementation are opaque to scripts and
* ideally to plugins as well.
*
* Plugin authors should rely primarily on Assign() to return a string as the result of a script function.
* It takes the COMMAND_ARGS passed to the script function followed by a pointer to the new string.
* i.e. Assign(PASS_COMMAND_ARGS, "some string") assigns "some string" to the  string variable on the lefthand
* side of the script assignment statement, initializing the variable if necessary. Generates a logged error if 
* the scripter does not provide a variable in which to store the result.
*
* GetString(), CreateString(), and SetString() are slightly lower-level functions; use them only if you have a 
* genuine need to directly create and manipulate new string variables outside of script commands. CreateString()
* returns the integer ID of the newly-created string var.
*
* If you want your script commands to support NVSE's %z format specifier (for inserting the contents of a string_var
* into another string), you must pass an NVSEStringVarInterface pointer (obtained via QueryInterface()) to 
* RegisterStringVarInterface() defined in GameAPI.h. This only needs to be done once, preferably during plugin load.
*
**************************************************/

struct NVSEStringVarInterface
{
	enum {
		kVersion = 1
	};

	UInt32		version;
	const char* (* GetString)(UInt32 stringID);
	void		(* SetString)(UInt32 stringID, const char* newValue);
	UInt32		(* CreateString)(const char* value, void* owningScript);
	void		(* Register)(NVSEStringVarInterface* intfc);			// is RegisterStringVarInterface() in GameAPI.h
	bool		(* Assign)(COMMAND_ARGS, const char* newValue);
};

// IsKeyPressed() takes a DirectInput scancode; values above 255 represent mouse buttons
// codes are the same as those used by OBSE's IsKeyPressed2 command
struct NVSEIOInterface
{
	enum {
		kVersion = 1
	};

	UInt32		version;
	bool		(* IsKeyPressed)(UInt32 scancode);
};

/**** Messaging API docs ********************************************************************
 *
 *	Messaging API allows inter-plugin communication at run-time. A plugin may register
 *	one callback for each plugin from which it wishes to receive messages, specifying
 *	the sender by name in the call to RegisterListener(). RegisterListener returns false
 *	if the specified plugin is not loaded, true otherwise. Any messages dispatched by
 *	the specified plugin will then be forwarded to the listener as they occur. Passing NULL as 
 *	the sender registers the calling plugin as a listener to every loaded plugin.
 *
 *	Messages may be dispatched via Dispatch() to either a specific listener (specified
 *	by name) or to all listeners (with NULL passed as the receiver). The contents and format of
 *	messageData are left up to the sender, and the receiver is responsible for casting the message
 *	to the expected type. If no plugins are registered as listeners for the sender, 
 *	Dispatch() returns false, otherwise it returns true.
 *
 *	Calling RegisterListener() or Dispatch() during plugin load is not advised as the requested plugin
 *	may not yet be loaded at that point. Instead, if you wish to register as a listener or dispatch a
 *	message immediately after plugin load, use RegisterListener() during load to register to receive
 *	messages from NVSE (sender name: "NVSE"). You will then receive a message from NVSE once 
 *	all plugins have been loaded, at which point it is safe to establish communications between
 *	plugins.
 *
 *	Some plugin authors may wish to use strings instead of integers to denote message type. In
 *	that case the receiver can pass the address of the string as an integer and require the receiver
 *	to cast it back to a char* on receipt.
 *
 *********************************************************************************************/

struct NVSEMessagingInterface
{
	struct Message {
		const char	* sender;
		UInt32		type;
		UInt32		dataLen;
		void		* data;
	};

	typedef void (* EventCallback)(Message* msg);

	enum {
		kVersion = 4
	};

	// NVSE messages
	enum {
		kMessage_PostLoad,				// sent to registered plugins once all plugins have been loaded (no data)

		kMessage_ExitGame,				// exit to windows from main menu or in-game menu

		kMessage_ExitToMainMenu,		// exit to main menu from in-game menu

		kMessage_LoadGame,				// Dispatched immediately before plugin serialization callbacks invoked, after savegame has been read by Fallout
										// dataLen: length of file path, data: char* file path of .fos savegame file
										// Receipt of this message does not *guarantee* the serialization callback will be invoked
										// as there may be no .nvse file associated with the savegame

		kMessage_SaveGame,				// as above
	
		kMessage_Precompile,			// EDITOR: Dispatched when the user attempts to save a script in the script editor.
										// NVSE first does its pre-compile checks; if these pass the message is dispatched before
										// the vanilla compiler does its own checks. 
										// data: ScriptBuffer* to the buffer representing the script under compilation
		
		kMessage_PreLoadGame,			// dispatched immediately before savegame is read by Fallout
										// dataLen: length of file path, data: char* file path of .fos savegame file

		kMessage_ExitGame_Console,		// exit game using 'qqq' console command

		kMessage_PostLoadGame,			//dispatched after an attempt to load a saved game has finished (the game's LoadGame() routine
										//has returned). You will probably want to handle this event if your plugin uses a Preload callback
										//as there is a chance that after that callback is invoked the game will encounter an error
										//while loading the saved game (eg. corrupted save) which may require you to reset some of your
										//plugin state.
										//data: bool, true if game successfully loaded, false otherwise */

		kMessage_PostPostLoad,			// sent right after kMessage_PostLoad to facilitate the correct dispatching/registering of messages/listeners
										// plugins may register as listeners during the first callback while deferring dispatches until the next
		kMessage_RuntimeScriptError,	// dispatched when an NVSE script error is encountered during runtime/
										// data: char* errorMessageText
// added for kVersion = 2
		kMessage_DeleteGame,			// sent right before deleting the .nvse cosave and the .fos save.
										// dataLen: length of file path, data: char* file path of .fos savegame file
		kMessage_RenameGame,			// sent right before renaming the .nvse cosave and the .fos save.
										// dataLen: length of old file path, data: char* old file path of .fos savegame file
										// you are expected to save the data and wait for kMessage_RenameNewGame
		kMessage_RenameNewGame,			// sent right after kMessage_RenameGame.
										// dataLen: length of new file path, data: char* new file path of .fos savegame file
		kMessage_NewGame,				// sent right before iterating through plugins newGame.
										// dataLen: 0, data: NULL
// added for kVersion == 3
		kMessage_DeleteGameName,		// version of the messages sent with a save file name instead of a save file path.
		kMessage_RenameGameName,
		kMessage_RenameNewGameName,

// added for kVersion == 4 (xNVSE)
		kMessage_DeferredInit,
		kMessage_ClearScriptDataCache,
		kMessage_MainGameLoop,			// called each game loop
		kMessage_ScriptCompile   // EDITOR: called after successful script compilation in GECK. data: pointer to Script
	};

	UInt32	version;
	bool	(* RegisterListener)(PluginHandle listener, const char* sender, EventCallback handler);
	bool	(* Dispatch)(PluginHandle sender, UInt32 messageType, void * data, UInt32 dataLen, const char* receiver);
};

/**** array_var API **************************************************************************
*
*	array_var is NVSE's pseudo-datatype for storing collections of data. Like strings, arrays 
*	are represented internally as integer IDs. However they are represented to plugins as 
*	pointers to type Array; this is an intentionally opaque type with no visible implementation.
*	They can be passed back and forth between plugins and NVSE; as far as your plugin is concerned
*	they define no other operations (not even dereferencing) and no data.
*
*	The current implementation allows plugins to create and return arrays from script commands.
*	Arrays come in three flavors: 
*		-Array, a zero-based array with consecutive integer keys
*		-Map, a mapping of floating point keys to elements
*		-StringMap, a mapping of string keys to elements
*	Container elements are of type Element, which can hold (and supports implicit conversion from) a
*	TESForm*, a C string, a double-precision float, or an Array*; the latter allows for the creation 
*	of multi-dimensional arrays.
*
*	The three "Create" functions accept a size, an array of Element values, the script from which
*	your command was invoked, and (for map-types) an array of Element keys of the appropriate type.
*	The arrays of keys and values should be of the same size and ordered so that key[i] corresponds to
*	value[i]. Pass a size of 0 to create an empty array. The creation methods return NULL if creation 
*	fails. Helper functions for populating arrays from STL containers are included in the example
*	plugin project along with code for sample usage.
*
*	SetElement() inserts a key-value pair, or modifies the value of an existing key.
*	AppendElement() applies only to Array-type arrays, inserting a new element at the end of the array.
*
*	A command returning an array should *always* return an array of the expected type; return
*	an empty array if necessary (e.g. if an error occurs in extracting the command arguments).
*	Assign() makes sure the Array's ID is stored in the command result - pass 'result' defined
*	in COMMAND_ARGS as the 'dest'. When registering your command with NVSE at plugin load,
*	make sure to use RegisterTypedCommand() and indicate the return type as kRetnType_Array.
*
*	Conventionally, you'll want to:
*		-Create and populate one or more STL containers holding your array elements,
*		-Pass the container(s) to the plugin helper functions to turn them into Array*(s), and finally
*		-Call Assign(yourArray, result) to return the array from your function (yourArray
*		 would be the first dimension for multi-dimensional arrays).
*	STL containers and helper functions are optional but convenient; calling Assign() is required.
*	See Cmd_ExamplePlugin_MakeArray_Execute() in the example plugin project for sample code.
*
*	Array pointers returned from the interface are valid only until the function creating them
*	goes out of scope; don't try to save them to use later. Arrays created by plugins only
*	persist as long as at least one reference to them exists; if your command creates a
*	huge multi-dimensional array and returns it, but the scripter doesn't assign the result
*	to a variable, the array and its sub-arrays will be destroyed.
*
*	-Use GetArraySize() to get the number of elements in an array. Returns -1 if the array
*	 does not exist.
*	-Use GetElement() to retrieve a single element given an Array* and a key. Returns false if
*	 the element could not be retrieved.
*	-Use GetElements() to retrieve a C array containing all of the elements of an array. If the
*	 array is a Map-type and the 'keys' argument is non-null, 'keys' will contain the keys
*	 associated with each element. Both 'keys' and 'elements' must be of a size large enough
*	 to hold all of the array data; use GetArraySize() to determine the size.
*	-Use LookupArrayByID to attempt to get an Array* given its unique integer ID. This allows
*	 plugin commands to accept arrays as arguments by defining the parameter as an integer.
*	 See the nvse_plugin_example project for sample usage. Or, better, see below for info
*	 on using kParamType_Array to accept array arguments directly.
*
*	-Plugin commands can now accept arrays as arguments provided the script calling the 
*	 command is compiled with NVSE's compiler override enabled. Array params should be defined
*	 as kParamType_Array. The argument can be extracted at run-time to an Array*, e.g.:
*		Array* arr;
*		ExtractArgs(PASS_EXTRACT_ARGS, &arr);
*		
*********************************************************************************************/

#if RUNTIME

struct NVSEArrayVarInterface
{
	enum {
		kVersion = 2
	};

	struct Array;

	struct Element
	{
	protected:
		union
		{
			char	* str;
			Array		* arr;
			TESForm		* form;
			double		num;
		};
		UInt8		type;

		friend class PluginAPI::ArrayAPI;
		friend class ArrayVar;
		void Reset() { if (type == kType_String) { FormHeap_Free(str); type = kType_Invalid; str = NULL; } }
	public:
		enum
		{
			kType_Invalid,

			kType_Numeric,
			kType_Form,
			kType_String,
			kType_Array,
		};

		~Element() { Reset(); }

		Element() : type(kType_Invalid) { }
		Element(const char* _str) : type(kType_String) { str = CopyCString(_str); }
		Element(double _num) : num(_num), type(kType_Numeric) { }
		Element(TESForm* _form) : form(_form), type(kType_Form) { }
		Element(Array* _array) : arr(_array), type(kType_Array) { }
		Element(const Element& rhs) { if (rhs.type == kType_String) { str = CopyCString(rhs.str); } else { num = rhs.num; } type = rhs.type; }
		Element& operator=(const Element& rhs) { if (this != &rhs) { Reset(); if (rhs.type == kType_String) str = CopyCString(rhs.str); else num = rhs.num; type = rhs.type; } return *this; }

		bool IsValid() const { return type != kType_Invalid; }
		UInt8 GetType() const { return type; }

		const char* String() { return type == kType_String ? str : NULL; }
		Array * Array() { return type == kType_Array ? arr : NULL; }
		TESForm * Form() { return type == kType_Form ? form : NULL; }
		double Number() { return type == kType_Numeric ? num : 0.0; }
	};

	Array* (* CreateArray)(const Element* data, UInt32 size, Script* callingScript);
	Array* (* CreateStringMap)(const char** keys, const NVSEArrayVarInterface::Element* values, UInt32 size, Script* callingScript);
	Array* (* CreateMap)(const double* keys, const NVSEArrayVarInterface::Element* values, UInt32 size, Script* callingScript);

	bool	(* AssignCommandResult)(Array* arr, double* dest);
	void	(* SetElement)(Array* arr, const Element& key, const Element& value);
	void	(* AppendElement)(Array* arr, const Element& value);

	UInt32	(* GetArraySize)(Array* arr);
	Array*	(* LookupArrayByID)(UInt32 id);
	bool	(* GetElement)(Array* arr, const Element& key, Element& outElement);
	bool	(* GetElements)(Array* arr, Element* elements, Element* keys);

	// version 2
	UInt32	(* GetArrayPacked)(Array* arr);

};

#endif
		
/**** command table API docs *******************************************************
*
*	Command table API gives plugins limited access to NVSE's internal command table.
*	The provided functionality mirrors that defined in the CommandTable class and
*	should be fairly self-explanatory. You may note that Start() and End() could be
*	used to alter the command table in memory. It probably needn't be said, but
*	doing so would be a Very Bad Idea.
*
*	GetRequiredNVSEVersion returns the minimum major release of NVSE required by
*	the specified command, i.e. the release in which it first appeared.
*	For non-NVSE commands, returns 0. For plugin commands, returns -1.
*
************************************************************************************/

struct NVSECommandTableInterface
{
	enum {
		kVersion = 1
	};

	UInt32	version;
	const CommandInfo*	(* Start)(void);
	const CommandInfo*	(* End)(void);
	const CommandInfo*	(* GetByOpcode)(UInt32 opcode);
	const CommandInfo*	(* GetByName)(const char* name);
	UInt32				(* GetReturnType)(const CommandInfo* cmd);		// return type enum defined in CommandTable.h
	UInt32				(* GetRequiredNVSEVersion)(const CommandInfo* cmd);
	const PluginInfo*	(* GetParentPlugin)(const CommandInfo* cmd);	// returns a pointer to the PluginInfo of the NVSE plugin that adds the command, if any. returns NULL otherwise
	const PluginInfo*	(* GetPluginInfoByName)(const char *pluginName);	// Returns a pointer to the PluginInfo of the NVSE plugin of the specified name; returns NULL is the plugin is not loaded.
};

/**** script API docs **********************************************************
 *
 *	Provides general functionality for interacting with scripts.
 *
 *	CallFunction() attempts to execute a script defined as a user-defined function.
 *	A calling object and containing object can be specified, or passed as NULL.
 *	If successful, it returns true, and the result is passed back from the script
 *	as an NVSEArrayVarInterface::Element. If the script returned nothing, the result
 *	is of type kType_Invalid. Up to 5 arguments can be passed in, of type
 *	int, float, or char*; support for passing arrays will be implemented later.
 *
 *	GetFunctionParams() returns the number of parameters expected by a function
 *	script. Returns -1 if the script is not a valid function script. Otherwise, if
 *	the number of params > 0 and paramTypesOut is non-NULL, paramTypesOut will be
 *	populated with the type of each param in left-to-right order (e.g. first param
 *	is in paramTypesOut[0], etc) where the type is one of Script::eVarType_XXX.
 *	To ensure that paramTypesOut has enough space to 
 *	hold all of the param types, either call GetFunctionParams(script, NULL) first
 *	or allocate at least 10 bytes in the array, since function scripts cannot
 *	currently (v0020) accept more than 10 arguments.
 *
 *	As of v0020, ExtractArgsEx() and ExtractFormatStringArgs() are no longer included
 *	in plugin builds. They are made available through NVSEScriptInterface instead.
 *	Macros are included in the example plugin project to make updating existing plugins
 *	which use either or both straightforward.
 *
 *	ExtractFormattedStringArgs() extracts and populates a format string, as well as
 *	any additional command arguments preceding or following the format string and its
 *	arguments.
 *
 *	ExtractArgsEx() behaves as the game's ExtractArgs(), except that for TESForm-derived
 *	parameters it will not perform any type checking or pointer casts, and will return
 *	true even if the passed form is NULL or not loaded in memory. This can be useful
 *	e.g. when a command accepts one or more form types not defined by the game's
 *	param type enum.
 *
 *	The game's ExtractArgs() function remains available and is generally preferred.
 *
 ******************************************************************************/

#if RUNTIME

struct NVSEScriptInterface
{
	enum {
		kVersion = 1
	};

	bool	(* CallFunction)(Script* funcScript, TESObjectREFR* callingObj, TESObjectREFR* container,
		NVSEArrayVarInterface::Element * result, UInt8 numArgs, ...);

	UInt32	(* GetFunctionParams)(Script* funcScript, UInt8* paramTypesOut);
	bool	(* ExtractArgsEx)(ParamInfo * paramInfo, void * scriptDataIn, UInt32 * scriptDataOffset, Script * scriptObj,
		ScriptEventList * eventList, ...);
	bool	(* ExtractFormatStringArgs)(UInt32 fmtStringPos, char* buffer, ParamInfo * paramInfo, void * scriptDataIn, 
		UInt32 * scriptDataOffset, Script * scriptObj, ScriptEventList * eventList, UInt32 maxParams, ...);
};

#endif

#if RUNTIME

// Gives access to internal data without reverse engineering NVSE
struct NVSEDataInterface
{
	enum {
		kVersion = 3
	};

	UInt32		version;
	enum {
		kNVSEData_DIHookControl = 1,
		kNVSEData_ArrayMap,
		kNVSEData_StringMap,
		kNVSEData_InventoryReferenceMap,

		kNVSEData_SingletonMax,
	};
	void * (* GetSingleton)(UInt32 singletonID);
	enum  {
		kNVSEData_InventoryReferenceCreate = 1,
		kNVSEData_InventoryReferenceGetForRefID,
		kNVSEData_InventoryReferenceGetRefBySelf,
		kNVSEData_ArrayVarMapDeleteBySelf,
		kNVSEData_StringVarMapDeleteBySelf,
		kNVSEData_LambdaDeleteAllForScript,
		kNVSEData_FuncMax,
	};
	void * (* GetFunc)(UInt32 funcID);
	enum  {
		kNVSEData_NumPreloadMods = 1,

		kNVSEData_DataMax,
	};
	void * (* GetData)(UInt32 dataID);
	// v2: xNVSE caches script data for additional performance and short circuit evaluation, if you are manipulating script data then you can clear the cache 
	void (*ClearScriptDataCache)();
	// v3
	
	
};
#endif

/**** serialization API docs ***************************************************
 *	
 *	The serialization API adds a separate save data file plugins can use to save
 *	data along with a game save. It is be stored separately from the main save
 *	(.ess) file to prevent any potential compatibility issues. The actual
 *	implementation is opaqe, so it can be changed if needed, but the APIs
 *	provided will always remain the same and will operate the same way.
 *	
 *	Each plugin that has registered the proper callbacks will be able to write
 *	typed and versioned records in to the file, similarly to the way Fallout
 *	.esp files work. Chunk types are just simple 32-bit values, you can use
 *	them for whatever you want as they are only meaningful to your code.
 *	Multiple records of the same type can be added, and records will be
 *	returned to you in the order they were originally written. Versioning is
 *	important so you can update the format of your data without breaking
 *	people's save files. Note that this means that if you have created multiple
 *	versions of a record you will have to check the version number and do some
 *	data conversion on load. Of course it isn't strictly /mandatory/ that you do
 *	this, but I consider not breaking people's save files to be very important.
 *	Also, note that your record data will be uniquely identified by your
 *	assigned opcode base, so make sure that is set up correctly (you really
 *	have to be doing that anyway, but I thought I'd mention it again).
 *	
 *	At any point, a plugin can call the
 *	NVSEStorageInterface::SetSave/Load/NewGameCallback functions to register a
 *	callback that will be run when a game is being saved, loaded, or started
 *	fresh. You must provide the PluginHandle you were given during your startup
 *	function so the core code knows which plugin to associate with the callback.
 *	
 *	When the save callback is called, there are two APIs you can use to write
 *	your records. WriteRecord is the simplest API - just give it your record
 *	type, the version, and a buffer storing your data. This is good when you can
 *	write your entire record all at one time. If you have more complex needs,
 *	use the OpenRecord/WriteRecordData APIs. When starting to write a new
 *	record, call OpenRecord, passing in the record type and version. Then call
 *	WriteRecordData as many times as needed to fill in the data for the record.
 *	To start the next record, just call OpenRecord again. Calling OpenRecord or
 *	exiting your save callback will automatically close the record and perform
 *	behind-the-scenes cleanup.
 *	
 *	The load callback is simpler. First call GetNextRecordInfo. It will move to
 *	the next record (or the first record if it is the first time it has been
 *	called) and return information about it. GetNextRecordInfo returns true if
 *	it found a "next" record for your plugin, so you know you are done reading
 *	when it returns false. After calling GetNextRecordInfo, call ReadRecordData
 *	to retrieve the data stored in the record. It may be called multiple times,
 *	and returns the number of bytes read from the record (really only useful if
 *	you accidentally try to read off the end of the record).
 *	
 *	The new game callback should be used to reset all of your internal data
 *	structures. It is called when a new game is started or a game is loaded with
 *	no save file.
 *	
 *	RefIDs stored in this file involve one complication. The upper 8 bits of the
 *	ID store the index of the mod that "owns" the form. This index may change in
 *	between save and load if a user changes their mod list. To fix this, run the
 *	ID through the ResolveRefID API. It fixes up the ID to reflect any changes
 *	in the mod list. If the mod owning that ID is no longer loaded, the function
 *	returns false.
 *	
 *	A Preload callback has been added. This callback is invoked immediately
 *	before a savegame is loaded - in contrast to the load callback, which is
 *	invoked *after* the runtime has loaded the savegame. This can be useful if 
 *	you need to modify objects before the game loads them. All of your plugin's
 *	saved data will be available from within both the Preload and Load callbacks. 
 *	Register this callback only if you have a legitimate need for it, as it requires
 *	NVSE to parse the co-save file twice.
 *	NOTE: during preload, ResolveRefID can still be used to fix up saved refIDs,
 *	but for obvious reasons objects stored in the savegame are not available.
 *
 ******************************************************************************/

struct NVSESerializationInterface
{
	enum
	{
		kVersion = 2,
	};

	typedef void (* EventCallback)(void * reserved);

	UInt32	version;
	void	(* SetSaveCallback)(PluginHandle plugin, EventCallback callback);
	void	(* SetLoadCallback)(PluginHandle plugin, EventCallback callback);
	void	(* SetNewGameCallback)(PluginHandle plugin, EventCallback callback);

	bool	(* WriteRecord)(UInt32 type, UInt32 version, const void * buf, UInt32 length);
	bool	(* OpenRecord)(UInt32 type, UInt32 version);
	bool	(* WriteRecordData)(const void * buf, UInt32 length);

	bool	(* GetNextRecordInfo)(UInt32 * type, UInt32 * version, UInt32 * length);
	UInt32	(* ReadRecordData)(void * buf, UInt32 length);

	// take a refid as stored in the loaded save file and resolve it using the currently
	// loaded list of mods. All refids stored in a save file must be run through this
	// function to account for changing mod lists. This returns true on success, and false
	// if the mod owning the RefID was unloaded.
	bool	(* ResolveRefID)(UInt32 refID, UInt32 * outRefID);

	void	(* SetPreLoadCallback)(PluginHandle plugin, EventCallback callback);

	// returns a full path to the last loaded save game
	const char* (* GetSavePath)();

	// Peeks at the data without interfiring with the current position
	UInt32	(* PeekRecordData)(void * buf, UInt32 length);

	void	(*WriteRecord8)(UInt8 inData);
	void	(*WriteRecord16)(UInt16 inData);
	void	(*WriteRecord32)(UInt32 inData);
	void	(*WriteRecord64)(const void *inData);

	UInt8	(*ReadRecord8)();
	UInt16	(*ReadRecord16)();
	UInt32	(*ReadRecord32)();
	void	(*ReadRecord64)(void *outData);

	void	(*SkipNBytes)(UInt32 byteNum);
};

struct PluginInfo
{
	enum
	{
		kInfoVersion = 1
	};

	UInt32			infoVersion;
	const char *	name;
	UInt32			version;
};

typedef bool (* _NVSEPlugin_Query)(const NVSEInterface * nvse, PluginInfo * info);
typedef bool (* _NVSEPlugin_Load)(const NVSEInterface * nvse);

/**** plugin API docs **********************************************************
 *	
 *	IMPORTANT: Before releasing a plugin, you MUST contact the NVSE team at the
 *	contact addresses listed in nvse_readme.txt to register a range of opcodes.
 *	This is required to prevent conflicts between multiple plugins, as each
 *	command must be assigned a unique opcode.
 *	
 *	The base API is pretty simple. Create a project based on the
 *	nvse_plugin_example project included with the NVSE source code, then define
 *	and export these functions:
 *	
 *	bool NVSEPlugin_Query(const NVSEInterface * nvse, PluginInfo * info)
 *	
 *	This primary purposes of this function are to fill out the PluginInfo
 *	structure, and to perform basic version checks based on the info in the
 *	NVSEInterface structure. Return false if your plugin is incompatible with
 *	the version of NVSE or Fallout passed in, otherwise return true. In either
 *	case, fill out the PluginInfo structure.
 *	
 *	If the plugin is being loaded in the context of the editor, isEditor will be
 *	non-zero, editorVersion will contain the current editor version, and
 *	falloutVersion will be zero. In this case you can probably just return
 *	true, however if you have multiple DLLs implementing the same behavior, for
 *	example one for each version of Fallout, only one of them should return
 *	true.
 *	
 *	The PluginInfo fields should be filled out as follows:
 *	- infoVersion should be set to PluginInfo::kInfoVersion
 *	- name should be a pointer to a null-terminated string uniquely identifying
 *	  your plugin, it will be used in the plugin querying API
 *	- version is only used by the plugin query API, and will be returned to
 *	  scripts requesting the current version of your plugin
 *	
 *	bool NVSEPlugin_Load(const NVSEInterface * nvse)
 *	
 *	In this function, use the SetOpcodeBase callback in NVSEInterface to set the
 *	opcode base to your assigned value, then use RegisterCommand to register all
 *	of your commands. NVSE will fix up your CommandInfo structure when loaded
 *	in the context of the editor, and will fill in any NULL callbacks with their
 *	default values, so don't worry about having a unique 'execute' callback for
 *	the editor, and don't provide a 'parse' callback unless you're actually
 *	overriding the default behavior. The opcode field will also be automatically
 *	updated with the next opcode in the sequence started by SetOpcodeBase.
 *	
 *	At this time, or at any point forward you can call the QueryInterface
 *	callback to retrieve an interface structure for the base services provided
 *	by the NVSE core.
 *	
 *	You may optionally return false from this function to unload your plugin,
 *	but make sure that you DO NOT register any commands if you do.
 *	
 *	Note that all structure versions are backwards-compatible, so you only need
 *	to check against the latest version that you need. New fields will be only
 *	added to the end, and all old fields will remain compatible with their
 *	previous implementations.
 *	
 ******************************************************************************/

class ExpressionEvaluator;
struct PluginScriptToken;

struct ExpressionEvaluatorUtils
{
	ExpressionEvaluator*	(__stdcall *CreateExpressionEvaluator)(COMMAND_ARGS);
	void					(__fastcall *DestroyExpressionEvaluator)(ExpressionEvaluator *eval);
	bool					(__fastcall *ExtractArgsEval)(ExpressionEvaluator *eval);
	UInt8					(__fastcall *GetNumArgs)(ExpressionEvaluator *eval);
	PluginScriptToken*		(__fastcall *GetNthArg)(ExpressionEvaluator *eval, UInt32 idx);

	UInt8					(__fastcall *ScriptTokenGetType)(PluginScriptToken *scrToken);
	double					(__fastcall *ScriptTokenGetNumber)(PluginScriptToken *scrToken);
	TESForm*				(__fastcall *ScriptTokenGetForm)(PluginScriptToken *scrToken);
	const char*				(__fastcall *ScriptTokenGetString)(PluginScriptToken *scrToken);
	UInt32					(__fastcall *ScriptTokenGetArrayID)(PluginScriptToken *scrToken);
};

extern ExpressionEvaluatorUtils s_expEvalUtils;

class PluginExpressionEvaluator
{
	ExpressionEvaluator		*eval;

public:
	PluginExpressionEvaluator(COMMAND_ARGS)
	{
		eval = s_expEvalUtils.CreateExpressionEvaluator(PASS_COMMAND_ARGS);
	}
	~PluginExpressionEvaluator()
	{
		s_expEvalUtils.DestroyExpressionEvaluator(eval);
	}

	bool ExtractArgs()
	{
		return s_expEvalUtils.ExtractArgsEval(eval);
	}

	UInt8 NumArgs()
	{
		return s_expEvalUtils.GetNumArgs(eval);
	}

	PluginScriptToken *Arg(UInt32 idx)
	{
		return s_expEvalUtils.GetNthArg(eval, idx);
	}
};

struct PluginScriptToken
{
	UInt8 GetType()
	{
		return s_expEvalUtils.ScriptTokenGetType(this);
	}

	double GetNumber()
	{
		return s_expEvalUtils.ScriptTokenGetNumber(this);
	}

	TESForm *GetTESForm()
	{
		return s_expEvalUtils.ScriptTokenGetForm(this);
	}

	const char *GetString()
	{
		return s_expEvalUtils.ScriptTokenGetString(this);
	}
#if RUNTIME
	NVSEArrayVarInterface::Array *GetArrayVar()
	{
		return (NVSEArrayVarInterface::Array*)s_expEvalUtils.ScriptTokenGetArrayID(this);
	}
#endif
};
