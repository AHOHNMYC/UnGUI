#define MAX_PARAMS_COUNT 32
typedef struct {
	CHAR title[256];
	CHAR hint[256];
	CHAR type[256];
	CHAR group_prefix[256];
	CHAR prefix[256];

	HWND group_box_hndl;
	HWND group_box_hint_hndls[2];

	DWORD key_count;
	CHAR values[MAX_PARAMS_COUNT][256];
	CHAR keys[MAX_PARAMS_COUNT][256];
	BOOL defaults[MAX_PARAMS_COUNT];
	CHAR hints[MAX_PARAMS_COUNT][256];
	HWND handles[MAX_PARAMS_COUNT];
	HWND hint_handles[MAX_PARAMS_COUNT];
} groupOfParamsEntry;

// uses in options.options
#define OPTION_ON_TOP		0x00000001
#define OPTION_SAVE_ON_EXIT	0x00000002
typedef struct {
//	CHAR command[256];			// path to exec file
	BOOL do_exit_after_launch;
	DWORD group_count;	// count of groups of options
	groupOfParamsEntry* groups[256];	// groups of options
} parsed_config;


typedef struct {
	LPCSTR config;
//	LPCSTR lang;
	DWORD on_top;
	DWORD autoclose;
	DWORD optional_count;
	LPCSTR optionals[256];
} accepted_params;

typedef struct {
	BOOL is_on_top;
//	LPCSTR curr_lang;

	BOOL is_config_in_resources;
	LPCSTR config_path;
} runtime_options;


