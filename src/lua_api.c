#include <string.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "termit.h"
#include "termit_core_api.h"
#include "keybindings.h"
#include "configs.h"
#include "lua_api.h"

extern lua_State* L;

TermitLuaTableLoaderResult termit_load_lua_table(lua_State* ls, TermitLuaTableLoaderFunc func, void* data)
{
    if (!data) {
        TRACE_MSG("data is NULL: skipping");
        return TERMIT_LUA_TABLE_LOADER_FAILED;
    }
    if (lua_isnil(ls, 1)) {
        TRACE_MSG("tabInfo not defined: skipping");
        return TERMIT_LUA_TABLE_LOADER_FAILED;
    }
    if (!lua_istable(ls, 1)) {
        TRACE_MSG("tabInfo is not table: skipping");
        lua_pop(ls, 1);
        return TERMIT_LUA_TABLE_LOADER_FAILED;
    }
    lua_pushnil(ls);
    while (lua_next(ls, 1) != 0) {
        if (lua_isstring(ls, -2)) {
            const gchar* name = lua_tostring(ls, -2);
            func(name, ls, -1, data);
        }
        lua_pop(ls, 1);
    }
    lua_pop(ls, 1);
    return TERMIT_LUA_TABLE_LOADER_OK;
}


void termit_lua_execute(const gchar* cmd)
{
    TRACE("executing script: %s", cmd);
    int s = luaL_dostring(L, cmd);
    termit_report_lua_error(__FILE__, __LINE__, s);
}

void termit_report_lua_error(const char* file, int line, int status)
{
    if (status == 0)
        return;
    TRACE_MSG("lua error:");
    g_fprintf(stderr, "%s:%d %s\n", file, line, lua_tostring(L, -1));
    lua_pop(L, 1);
}

static int termit_lua_setOptions(lua_State* ls)
{
    TRACE_MSG(__FUNCTION__);
    termit_load_lua_table(ls, termit_options_loader, &configs);
    trace_configs();
    return 0;
}

int termit_lua_dofunction(int f)
{
    lua_State* ls = L;
    if(f != LUA_REFNIL) {
        lua_rawgeti(ls, LUA_REGISTRYINDEX, f);
        if (lua_pcall(ls, 0, 0, 0)) {
            TRACE("error running function: %s", lua_tostring(ls, -1));
            lua_pop(ls, 1);
            return 0;
        }
        return 1;
    }
    return 0;
}

void termit_lua_unref(int* lua_callback)
{
    if (*lua_callback) {
        luaL_unref(L, LUA_REGISTRYINDEX, *lua_callback);
        *lua_callback = 0;
    }
}

gchar* termit_lua_getTitleCallback(int f, const gchar* title)
{
    lua_State* ls = L;
    if(f != LUA_REFNIL) {
        lua_rawgeti(ls, LUA_REGISTRYINDEX, f);
        lua_pushstring(ls, title);
        if (lua_pcall(ls, 1, 1, 0)) {
            TRACE("error running function: %s", lua_tostring(ls, -1));
            lua_pop(ls, 1);
            return NULL;
        }
        if (lua_isstring(ls, -1))
            return g_strdup(lua_tostring(ls, -1));
    }
    return NULL;
}

static int termit_lua_bindKey(lua_State* ls)
{
    TRACE_MSG(__FUNCTION__);
    if (lua_isnil(ls, 1)) {
        TRACE_MSG("nil args: skipping");
    } else if (!lua_isstring(ls, 1)) {
        TRACE_MSG("bad args: skipping");
    } else if (lua_isnil(ls, 2)) {
        const char* keybinding = lua_tostring(ls, 1);
        termit_unbind_key(keybinding);
        TRACE("unbindKey: %s", keybinding);
    } else if (lua_isfunction(ls, 2)) {
        const char* keybinding = lua_tostring(ls, 1);
        int func = luaL_ref(ls, LUA_REGISTRYINDEX);
        termit_bind_key(keybinding, func);
        TRACE("bindKey: %s - %d", keybinding, func);
    }
    return 0;
}

static int termit_lua_bindMouse(lua_State* ls)
{
    TRACE_MSG(__FUNCTION__);
    if (lua_isnil(ls, 1)) {
        TRACE_MSG("nil args: skipping");
    } else if (!lua_isstring(ls, 1)) {
        TRACE_MSG("bad args: skipping");
    } else if (lua_isnil(ls, 2)) {
        const char* mousebinding = lua_tostring(ls, 1);
        termit_unbind_mouse(mousebinding);
        TRACE("unbindMouse: %s", mousebinding);
    } else if (lua_isfunction(ls, 2)) {
        const char* mousebinding = lua_tostring(ls, 1);
        int func = luaL_ref(ls, LUA_REGISTRYINDEX);
        termit_bind_mouse(mousebinding, func);
        TRACE("bindMouse: %s - %d", mousebinding, func);
    }
    return 0;
}

static int termit_lua_toggleMenubar(lua_State* ls)
{
    TRACE_MSG(__FUNCTION__);
    termit_toggle_menubar();
    return 0;
}

static void tabLoader(const gchar* name, lua_State* ls, int index, void* data)
{
    struct TabInfo* ti = (struct TabInfo*)data;
    if (!strcmp(name, "name") && lua_isstring(ls, index)) {
        const gchar* value = lua_tostring(ls, index);
        TRACE("  %s - %s", name, value);
        ti->name = g_strdup(value);
    } else if (!strcmp(name, "command") && lua_isstring(ls, index)) {
        const gchar* value = lua_tostring(ls, index);
        TRACE("  %s - %s", name, value);
        ti->command = g_strdup(value);
    } else if (!strcmp(name, "encoding") && lua_isstring(ls, index)) {
        const gchar* value = lua_tostring(ls, index);
        TRACE("  %s - %s", name, value);
        ti->encoding = g_strdup(value);
    } else if (!strcmp(name, "working_dir") && lua_isstring(ls, index)) {
        const gchar* value = lua_tostring(ls, index);
        TRACE("  %s - %s", name, value);
        ti->working_dir = g_strdup(value);
    }
}

static int termit_lua_nextTab(lua_State* ls)
{
    TRACE_MSG(__FUNCTION__);
    termit_next_tab();
    return 0;
}

static int termit_lua_prevTab(lua_State* ls)
{
    TRACE_MSG(__FUNCTION__);
    termit_prev_tab();
    return 0;
}

static int termit_lua_copy(lua_State* ls)
{
    TRACE_MSG(__FUNCTION__);
    termit_copy();
    return 0;
}

static int termit_lua_paste(lua_State* ls)
{
    TRACE_MSG(__FUNCTION__);
    termit_paste();
    return 0;
}

static int termit_lua_openTab(lua_State* ls)
{
    TRACE_MSG(__FUNCTION__);
    if (lua_istable(ls, 1)) {
        struct TabInfo ti = {0};
        if (termit_load_lua_table(ls, tabLoader, &ti) != TERMIT_LUA_TABLE_LOADER_OK)
            return 0;
        termit_append_tab_with_details(&ti);
        g_free(ti.name);
        g_free(ti.command);
        g_free(ti.encoding);
        g_free(ti.working_dir);
    } else {
        termit_append_tab();
    }
    return 0;
}

static int termit_lua_closeTab(lua_State* ls)
{
    termit_close_tab();
    TRACE_FUNC;
    return 0;
}

static int termit_lua_setMatches(lua_State* ls)
{
    TRACE_MSG(__FUNCTION__);
    termit_load_lua_table(ls, termit_matches_loader, configs.matches);
    trace_configs();
    return 0;
}

static int termit_lua_setKbPolicy(lua_State* ls)
{
    if (lua_isnil(ls, 1)) {
        TRACE_MSG("no kbPolicy defined: skipping");
        return 0;
    } else if (!lua_isstring(ls, 1)) {
        TRACE_MSG("kbPolicy is not string: skipping");
        return 0;
    }
    const gchar* val =  lua_tostring(ls, 1);
    TRACE("setKbPolicy: %s", val);
    if (!strcmp(val, "keycode"))
        termit_set_kb_policy(TermitKbUseKeycode);
    else if (!strcmp(val, "keysym"))
        termit_set_kb_policy(TermitKbUseKeysym);
    else {
        ERROR("unknown kbPolicy: %s", val);
    }
    return 0;
}

static int loadMenu(lua_State* ls, GArray* menus)
{
    int res = 0;
    if (lua_isnil(ls, 1) || lua_isnil(ls, 2)) {
        TRACE_MSG("menu not defined: skipping");
        res = -1;
    } else if (!lua_istable(ls, 1) || !lua_isstring(ls, 2)) {
        TRACE_MSG("menu is not table: skipping");
        res = -1;
    } else {
        const gchar* menuName = lua_tostring(ls, 2);
        TRACE("Menu: %s", menuName);
        lua_pushnil(ls);
        struct UserMenu um;
        um.name = g_strdup(menuName);
        um.items = g_array_new(FALSE, TRUE, sizeof(struct UserMenuItem));
        while (lua_next(ls, 1) != 0) {
            if (lua_isstring(ls, -2)) {
                struct UserMenuItem umi = {0};
                lua_pushnil(ls);
                while (lua_next(ls, -2) != 0) {
                    if (lua_isstring(ls, -2)) {
                        const gchar* name = lua_tostring(ls, -2);
                        const gchar* value = lua_tostring(ls, -1);
                        if (!strcmp(name, "name"))
                            umi.name = g_strdup(value);
                        else if (!strcmp(name, "action"))
                            umi.userFunc = g_strdup(value);
                    }
                    lua_pop(ls, 1);
                }
                g_array_append_val(um.items, umi);
            }
            lua_pop(ls, 1);
        }
        g_array_append_val(menus, um);
    }
    TRACE_MSG(__FUNCTION__);
    lua_pop(ls, 1);
    TRACE_MSG(__FUNCTION__);

    return res;
}

static int termit_lua_addMenu(lua_State* ls)
{
    if (loadMenu(ls, configs.user_menus) < 0) {
        ERROR("addMenu failed");
    }
    return 0;
}

static int termit_lua_addPopupMenu(lua_State* ls)
{
    if (loadMenu(ls, configs.user_popup_menus) < 0) {
        ERROR("addMenu failed");
    }
    return 0;
}

static int termit_lua_setEncoding(lua_State* ls)
{
    if (lua_isnil(ls, 1)) {
        TRACE_MSG("no encoding defined: skipping");
        return 0;
    } else if (!lua_isstring(ls, 1)) {
        TRACE_MSG("encoding is not string: skipping");
        return 0;
    }
    const gchar* val =  lua_tostring(ls, 1);
    termit_set_encoding(val);
    return 0;
}

static int termit_lua_activateTab(lua_State* ls)
{
    if (lua_isnil(ls, 1)) {
        TRACE_MSG("no tabNum defined: skipping");
        return 0;
    } else if (!lua_isnumber(ls, 1)) {
        TRACE_MSG("tabNum is not number: skipping");
        return 0;
    }
    int tab_index =  lua_tointeger(ls, 1);
    termit_activate_tab(tab_index - 1);
    return 0;
}

static int termit_lua_currentTabIndex(lua_State* ls)
{
    lua_pushinteger(ls, termit_get_current_tab_index() + 1);
    return 1;
}

static int termit_lua_changeTab(lua_State* ls)
{
    return 0;
}

static int termit_lua_setTabTitle(lua_State* ls)
{
    if (lua_isnil(ls, 1)) {
        TRACE_MSG("no tabName defined: skipping");
        return 0;
    } else if (!lua_isstring(ls, 1)) {
        TRACE_MSG("tabName is not string: skipping");
        return 0;
    }
    const gchar* val =  lua_tostring(ls, 1);
    gint page = gtk_notebook_get_current_page(GTK_NOTEBOOK(termit.notebook));
    TERMIT_GET_TAB_BY_INDEX2(pTab, page, 0);
    termit_set_tab_title(page, val);
    pTab->custom_tab_name = TRUE;
    return 0;
}

static int termit_lua_setWindowTitle(lua_State* ls)
{
    if (lua_isnil(ls, 1)) {
        TRACE_MSG("no title defined: skipping");
        return 0;
    } else if (!lua_isstring(ls, 1)) {
        TRACE_MSG("title is not string: skipping");
        return 0;
    }
    const gchar* val =  lua_tostring(ls, 1);
    termit_set_window_title(val);
    return 0;
}

static int termit_lua_setTabColor__(lua_State* ls, void (*callback)(gint, const GdkColor*))
{
    if (lua_isnil(ls, 1)) {
        TRACE_MSG("no color defined: skipping");
        return 0;
    } else if (!lua_isstring(ls, 1)) {
        TRACE_MSG("color is not string: skipping");
        return 0;
    }
    const gchar* val =  lua_tostring(ls, 1);
    GdkColor color;
    if (gdk_color_parse(val, &color) == TRUE)
        callback(-1, &color);
    return 0;
}

static int termit_lua_setTabForegroundColor(lua_State* ls)
{
    return termit_lua_setTabColor__(ls, &termit_set_tab_foreground_color);
}

static int termit_lua_setTabBackgroundColor(lua_State* ls)
{
    return termit_lua_setTabColor__(ls, &termit_set_tab_background_color);
}

static int termit_lua_reconfigure(lua_State* ls)
{
    termit_reconfigure();
    return 0;
}

void termit_init_lua_api()
{
    TRACE_FUNC;
    lua_register(L, "setOptions", termit_lua_setOptions);
    lua_register(L, "bindKey", termit_lua_bindKey);
    lua_register(L, "bindMouse", termit_lua_bindMouse);
    lua_register(L, "setKbPolicy", termit_lua_setKbPolicy);
    lua_register(L, "setMatches", termit_lua_setMatches);
    lua_register(L, "openTab", termit_lua_openTab);
    lua_register(L, "nextTab", termit_lua_nextTab);
    lua_register(L, "prevTab", termit_lua_prevTab);
    lua_register(L, "activateTab", termit_lua_activateTab);
    lua_register(L, "changeTab", termit_lua_changeTab);
    lua_register(L, "closeTab", termit_lua_closeTab);
    lua_register(L, "copy", termit_lua_copy);
    lua_register(L, "paste", termit_lua_paste);
    lua_register(L, "addMenu", termit_lua_addMenu);
    lua_register(L, "addPopupMenu", termit_lua_addPopupMenu);
    lua_register(L, "currentTabIndex", termit_lua_currentTabIndex);
    lua_register(L, "setEncoding", termit_lua_setEncoding);
    lua_register(L, "setTabTitle", termit_lua_setTabTitle);
    lua_register(L, "setWindowTitle", termit_lua_setWindowTitle);
    lua_register(L, "setTabForegroundColor", termit_lua_setTabForegroundColor);
    lua_register(L, "setTabBackgroundColor", termit_lua_setTabBackgroundColor);
    lua_register(L, "toggleMenu", termit_lua_toggleMenubar);
    lua_register(L, "reconfigure", termit_lua_reconfigure);
}

