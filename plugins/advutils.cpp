// Intention: adventure mode teleportation
//
//   Expose locate
//
#include <iostream>
#include <iomanip>
#include <climits>
#include <vector>
#include <algorithm>
#include <string>
#include <sstream>
#include <ctime>
#include <cstdio>
using namespace std;

#include "Core.h"
#include "Console.h"
#include "Export.h"
#include "PluginManager.h"
#include "MiscUtils.h"

#include "LuaTools.h"
#include "DataFuncs.h"

#include "modules/Units.h"
#include "modules/Maps.h"
#include "modules/Gui.h"
#include "modules/Materials.h"
#include "modules/MapCache.h"
#include "modules/Buildings.h"
#include "modules/World.h"
#include "modules/Screen.h"
#include "MiscUtils.h"
#include <VTableInterpose.h>

#include "df/ui.h"
#include "df/world.h"
#include "df/world_data.h"
#include "df/world_raws.h"
#include "modules/Translation.h"

#if defined(_MSC_VER) && (_MSC_VER >= 1400)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif


using std::vector;
using std::string;
using namespace DFHack;
using namespace DFHack::Units;
using namespace df::enums;

DFHACK_PLUGIN("advutils");
DFHACK_PLUGIN_IS_ENABLED(is_enabled);

REQUIRE_GLOBAL(world);
REQUIRE_GLOBAL(cursor);
REQUIRE_GLOBAL(ui);
REQUIRE_GLOBAL(gps);
REQUIRE_GLOBAL(cur_year);
REQUIRE_GLOBAL(cur_year_tick);

REQUIRE_GLOBAL(ui_menu_width);
REQUIRE_GLOBAL(ui_area_map_width);

using namespace DFHack::Gui;

DFhackCExport command_result plugin_enable ( color_ostream &out, bool enable);

#if defined(_MSC_VER) && (_MSC_VER >= 1400)
struct df_updatemap_struct
{
	int mode;
	int a1;
	int a2;
	int a3;
	int a4;
};

typedef int(__cdecl *DF_UPDATEMAP)(df::world* a2, __int16 a3, __int16 a4, int a5, int a6, int a7, df_updatemap_struct* a8);
typedef void (__cdecl *DF_CLEARUNITS)(); // esi pass by reference

static DF_UPDATEMAP df_updatemap = NULL;
static DF_CLEARUNITS df_clearunits = NULL;

#endif

const string adv_teleport_help =
    "Performs adventure mode teleport for the unit.\n"
    "Options:\n"
    "  examples     - print some usage examples\n"
    ;

const string adv_teleport_help_examples =
    "Example for teleporting adventurer:\n"
    "  (dfhack) 'zone set' to use this zone for future assignments\n"
    ;


DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event)
{
    switch (event)
    {
    case DFHack::SC_MAP_LOADED:
        // initialize from the world just loaded
        break;
    case DFHack::SC_MAP_UNLOADED:
        // cleanup
        break;
    default:
        break;
    }
    return CR_OK;
}

DFhackCExport command_result plugin_onupdate ( color_ostream &out )
{
    return CR_OK;
}

void internal_teleport(int reg_x, int reg_y, int emb_x, int emb_y)
{
#if defined(_MSC_VER) && (_MSC_VER >= 1400)
	auto advmode = df::global::ui_advmode;
	if (advmode == NULL || df_clearunits == NULL || df_updatemap == NULL)
		return;

	if (!World::isAdventureMode())
		return;

	if (df_clearunits)
	{
		// Clear current creatures
		__asm 
		{
			pushad
			mov esi, advmode
			call df_clearunits
			popad
		}
	}
    // make copy of character and move to specified area
	if (df_updatemap)
	{
		// this function is a thisfunc function but supports null this values in ecx
		// we go ahead an push 0 into ecx via assembly to make this call
		auto world = df::global::world;
		if (world && world->units.active.empty())
		{
			auto data = world->world_data;
			df_updatemap_struct a1 = { 0, -1, 0, -1, -1 }, *p1 = &a1;
			__asm 
			{
				pushad
				push p1
				push 0
				push emb_y
				push emb_x
				push reg_y
				push reg_x
				push world
				xor ecx, ecx  // this ptr
				call df_updatemap
				popad
			}
		}
	}
#endif
}

command_result df_adv_teleport (color_ostream &out, vector <string> & parameters)
{
    CoreSuspender suspend;

    for (size_t i = 0; i < parameters.size(); i++)
    {
        string & p = parameters[i];

        if (p == "help" || p == "?")
        {
			out << adv_teleport_help << endl;
            return CR_OK;
        }
        else
        {
            out << "Unknown command: " << p << endl;
            return CR_WRONG_USAGE;
        }
    }

#if 0
    if (!Maps::IsValid())
    {
        out.printerr("Map is not available!\n");
        return CR_FAILURE;
    }
#endif

	auto advmode = df::global::ui_advmode;
	if (advmode == NULL)
	{
		out.printerr("Unable to resolve ui_advmode\n");
		return CR_FAILURE;
	}
#if defined(_MSC_VER) && (_MSC_VER >= 1400)
	if (df_clearunits == NULL || df_updatemap == NULL)
	{
		out.printerr("Unable to resolve internal function pointers\n");
		return CR_FAILURE;
	}
#endif
	
	auto world = df::global::world;
	if (world == NULL)
	{
		out.printerr("World Pointer is not valid\n");
		return CR_FAILURE;
	}

	auto data = world->world_data;
	if (data == NULL)
	{
		out.printerr("World Data Pointer is not valid\n");
		return CR_FAILURE;
	}

	internal_teleport(data->adv_region_x, data->adv_region_y, data->adv_emb_x, data->adv_emb_y);
    return CR_OK;
}

static int adv_teleport(lua_State *L)
{
    color_ostream &out = *Lua::GetOutput(L);

	short region_x, region_y, emb_x, emb_y;
	region_x = region_y = emb_x = emb_y = 0;

#if defined(_MSC_VER) && (_MSC_VER >= 1400)
	if (df_clearunits == NULL || df_updatemap == NULL)
	{
		out.printerr("Unable to resolve internal function pointers\n");
		return CR_FAILURE;
	}
#endif

	auto advmode = df::global::ui_advmode;
	if (advmode == NULL)
	{
		out.printerr("Unable to resolve ui_advmode\n");
		return CR_FAILURE;
	}

	auto world = df::global::world;
	if (world == NULL)
	{
		out.printerr("World Pointer is not valid\n");
		return CR_FAILURE;
	}

	auto data = world->world_data;
	if (data == NULL)
	{
		out.printerr("World Data Pointer is not valid\n");
		return CR_FAILURE;
	}
	region_x = luaL_optinteger(L,1,data->adv_region_x);
	region_y = luaL_optinteger(L,2,data->adv_region_y);
	emb_x = luaL_optinteger(L,3,data->adv_emb_x);
	emb_y = luaL_optinteger(L,4,data->adv_emb_y);

	out.print("Teleporting to W:(%3d, %3d) R:(%3d, %3d)\n", (int)region_x, (int)region_y, (int)emb_x, (int)emb_y);
	out.flush();
	internal_teleport(region_x, region_y, emb_x, emb_y);

	lua_pushnil(L);
	return 1;
}

DFHACK_PLUGIN_LUA_COMMANDS {
	DFHACK_LUA_COMMAND(adv_teleport),
    DFHACK_LUA_END
};

// end lua API

DFhackCExport command_result plugin_enable ( color_ostream &out, bool enable)
{
	// only enable teleport on windows for now
#if defined(_MSC_VER) && (_MSC_VER >= 1400)
    if (enable != is_enabled)
    {
        is_enabled = enable;
    }

    return CR_OK;
#else
	return CR_FAILURE;
#endif
}

#if defined(_MSC_VER) 

typedef void *(*FNRESOLVEPOINTER)(unsigned char *start, unsigned char *end, int align, 
					int length, unsigned char *data, unsigned char *mask);

// Simple pointer resolution.  Brute force search ignoring mask
void *ResolvePointerSimple(unsigned char *start, unsigned char *end, int align, 
					int length, unsigned char *data, unsigned char *mask)
{
	if (align == 0) align = 1;
	for (unsigned char *addr = start; addr < end; addr += align)
	{
		if (*addr == *data)
		{
			if (memcmp(addr, data, length) == 0)
				return addr;
		}
	}
	return NULL;
}

// Complex pointer resolution. Search using mask
void *ResolvePointer(unsigned char *start, unsigned char *end, int align, 
					int length, unsigned char *data, unsigned char *mask)
{
	if (align == 0) align = 1;
	for (unsigned char *addr = start; addr < end; addr += align)
	{
		if (*addr == *data) //if (((*addr)&(*mask)) == *data) // assume start always matches
		{
			int match = 0;
			unsigned char *a = addr;
			unsigned char *aend = addr+length;
			unsigned char *d = data; 
			unsigned char *m = mask;
			do
			{
				if ((*a++ & *m++) != *d++)
					break;
			} while (++match < length);
			if (match == length)
				return addr;
		}
	}
	return NULL;
}

bool IsSimpleMask(int length, unsigned char *mask)
{
	if (mask == NULL || length == 0) return true;
	bool ok = true;
	for (int i = 0; ok && i<length; ++i)
		ok &= (mask[i] == 0xFF);
	return ok;
}

// Resolve Pointer in the base EXE
void *ResolveExePointer(LPCVOID startaddr, int align, int length, unsigned char *data, unsigned char *mask)
{
	MEMORY_BASIC_INFORMATION memory = { 0 };
	PROCESS_HEAP_ENTRY entry = { 0 };
	bool found = false;
	FNRESOLVEPOINTER fnResolvePointer = IsSimpleMask(length, mask) ? ResolvePointerSimple : ResolvePointer;
	while (!found)
	{
		if (0 == VirtualQueryEx(GetCurrentProcess(), startaddr, &memory, sizeof(memory)))
			break;
		if (memory.Type == MEM_IMAGE)
		{
			unsigned char *baddr = (unsigned char *)memory.BaseAddress;
			void * ptr = fnResolvePointer(baddr, baddr + memory.RegionSize, align, length, data, mask); 
			if (ptr) return ptr;
		}
		else
		{
			return NULL;
		}
		startaddr = (unsigned char*)memory.BaseAddress + memory.RegionSize;
	}
	return NULL;
}
#endif

DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
	// only enable teleport on windows for now
#if defined(_MSC_VER) && (_MSC_VER >= 1400)
	LPCVOID startaddr = GetModuleHandle(NULL); // load the EXE;
	unsigned char mask[64];
	memset(mask, 0xFF, sizeof(mask));
	// this is just a "stable" reference to the routine which updates the map and places the adventurer
	unsigned char update_map_data[] =
	{
		0x50, 0x8B, 0x4C, 0x24, 0x64, 0x8B, 0xC5, 0x99, 0x83, 0xE2, 0x0F, 0x03, 0xC2, 0xC1, 0xF8, 0x04,
		0x50, 0x8B, 0xC6, 0x99, 0x83, 0xE2, 0x0F, 0x03, 0xC2, 0xC1, 0xF8, 0x04, 0x50, 0x51, 0x8B, 0xCF,
	};
	void *df_updatemap_start = ResolveExePointer(startaddr, 1, sizeof(update_map_data),update_map_data,mask);
	if (df_updatemap_start)
	{
		// this is a relative jump instruction
		unsigned char *vaddr = (unsigned char*)df_updatemap_start + sizeof(update_map_data);
		int offset = vaddr[4] << 24 | vaddr[3] << 16 | vaddr[2] << 8 | vaddr[1];
		unsigned char *ptr = (vaddr + 5 + offset);
		if (ptr > startaddr) // verify jump is in module
			df_updatemap = (DF_UPDATEMAP)(vaddr + 5 + offset);
	}

	// this routine will clear the units list which is used before placing the adventurer
	unsigned char df_clearunits_data[] = {
		0x51, 0x53, 0x57, 0x56, 0xC6, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE8, 0x00, 0x00, 0x00, 0x00, 0x6A, 0x01, 0x6A, 0x00, 0x68
	};
	unsigned char df_clearunits_mask[] = {
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
	};
	df_clearunits = (DF_CLEARUNITS)ResolveExePointer(startaddr, 16, sizeof(df_clearunits_data),df_clearunits_data,df_clearunits_mask);

	if (df_updatemap != NULL && df_clearunits != NULL)
	{
		//commands.push_back(PluginCommand(
		//	"adv_teleport", "adventure mode teleport",
		//	df_adv_teleport, false,
		//	adv_teleport_help.c_str()
		//	));
		plugin_enable(out, true);
	}
#endif
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
#if defined(_MSC_VER) && (_MSC_VER >= 1400)
	df_clearunits = NULL;
	df_updatemap = NULL;
#endif
    return CR_OK;
}
