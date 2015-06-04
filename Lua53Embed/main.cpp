#include <iostream>
#include <unordered_map>
#include <vector>
#include <string>

#include <lua.hpp>

/* 
 * Global variable declarations 
 * Typically, these variables would be wrapped nicely into a class and would be exposed by the game engine
 * For sake of the tutorial, I kept everything into one file
 */
typedef unsigned int luaHookID_t;
std::unordered_map<std::string, std::vector<luaHookID_t> > eventHooks;
lua_State *luaState;
float deltaTime;

/* Function declarations */
lua_State *loadScript(const char *scriptPath);
void tick(float deltaTime);
void fireEvent(const char *eventName);
int hookEvent(lua_State *luaState);
int luaGetTimeMillis(lua_State *luaState);
int luaGetDeltaTime(lua_State *luaState);

void registerGlobalObject(const char *objectName, luaL_Reg *regs)
{
	/* Create a new object */
	luaL_newmetatable(luaState, objectName);

	/* Duplicate the object on top of the stack */
	lua_pushvalue(luaState, -1);

	/* Tell the object to point to itself */
	lua_setfield(luaState, -2, "__index");

	/* Push the functions into the object */
	luaL_setfuncs(luaState, regs, 0);

	/* Push the object into Lua's global space */
	lua_setglobal(luaState, objectName);
}

void prepareLuaState()
{
	luaState = luaL_newstate();
	luaL_openlibs(luaState);

	static const luaL_Reg engineLibs[] =
	{
		{"addEventListener", hookEvent},
		{NULL, NULL}
	};

	/* loop through all functions and register them to Lua's global space */
	const luaL_Reg *engineLibsPtr = engineLibs;
	for (; engineLibsPtr->func != NULL; engineLibsPtr++)
	{
		lua_register(luaState, engineLibsPtr->name, engineLibsPtr->func);
	}


	luaL_Reg timefuncs[] =
	{
		{"getTimeMillis", luaGetTimeMillis},
		{"getDeltaTime", luaGetDeltaTime},
		{NULL, NULL}
	};

	/*
	 * Register the following object with functions into Lua's global space
	 *    - Time
	 *        -- getTimeMillis()
	 *        -- getDeltaTime()
	 */
	registerGlobalObject("Time", timefuncs);

	loadScript("test.lua");

}

lua_State *loadScript(const char *scriptPath)
{

	int status = luaL_loadfile(luaState, scriptPath);
	int result = 0;
	if (status == LUA_OK)
	{
		/* Run the script */
		result = lua_pcall(luaState, 0, LUA_MULTRET, 0);

		/* Check if any values were returned */

		int argc = lua_gettop(luaState);
		if (argc != 0)
		{
			if (lua_istable(luaState, lua_gettop(luaState)))
			{
				/*
				 * Returning tables can be used to make a very nice configuration system
				 * A lot like how people use XML/JSON to store information
				 * This will be saved for another tutorial
				 */
			}
		}
	} else
	{
		std::cerr << "[ERROR]: " << lua_tostring(luaState, lua_gettop(luaState)) << std::endl;
		lua_pop(luaState, 1);
	}

	return luaState;
}

void tick(float deltaTime)
{
	::deltaTime = deltaTime;
	fireEvent("Update");
}

void fireEvent(const char *eventName)
{

	if (eventHooks[eventName].size() > 0)
	{
		for (unsigned int i=0; i < eventHooks[eventName].size(); i++)
		{
			/* Check Lua's registry for the function name */
			lua_rawgeti(luaState, LUA_REGISTRYINDEX, eventHooks[eventName][i]);

			/* If the top of the stack is a function */
			if (lua_isfunction(luaState, lua_gettop(luaState)))
			{
				/* Run function on top of stack */
				int result = lua_pcall(luaState, 0, LUA_MULTRET, 0);
			}
			else
			{
				/* 
				 * lua_rawgeti() returned a nil pointer
				 * Clean the stack
				 */
				lua_pop(luaState, 1);
			}
		}
	}
}

int hookEvent(lua_State *luaState)
{
	int argc = lua_gettop(luaState);

	if (argc != 2)
	{
		/* Failed hook due to incorrect argument count */
		lua_pop(luaState, argc); /* clean stack */
		lua_pushnumber(luaState, -1);

		return 0;
	}

	int functionId;
	std::string functionName;

	/* Add the function on top of the stack to Lua's registry */
	functionId = luaL_ref(luaState, LUA_REGISTRYINDEX);

	/* Grab the function name and clean the stack */
	functionName = strdup(lua_tostring(luaState, lua_gettop(luaState)));
	lua_pop(luaState, 1);

	/* Add the registry index ID at 'functionName' to a vector that contains a list of ids that have already registered */
	eventHooks[functionName].push_back(functionId);

	/* successful hook*/
	lua_pushnumber(luaState, 1);

	return 1;
}

int luaGetTimeMillis(lua_State *luaState)
{
	/* 
	 * Use whatever function/library you want to get the time
	 * For my engine, I use GLFW and the function 'glfwGetTime()'
	 * In this example, we're simply going to hardcode the value '1234567'
	 */
	lua_pushnumber(luaState, 1234567 /* glfwGetTime() */);
	return 1;
}

int luaGetDeltaTime(lua_State *luaState)
{
	lua_pushnumber(luaState, deltaTime);
	return 1;
}


int main()
{

	prepareLuaState();
	
	for (unsigned int i=0; i < 60; i++)
	{
		tick(12.0f);
	}

	return 0;
}