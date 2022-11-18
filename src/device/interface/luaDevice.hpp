/*
 * devices.hpp
 *
 *  Created on: Sep 29, 2021
 *      Author: dwd
 */

#pragma once

#include <device.hpp>

extern "C"
{
	#if __has_include(<lua5.3/lua.h>)
		#include <lua5.3/lua.h>
		#include <lua5.3/lualib.h>
		#include <lua5.3/lauxlib.h>
	#elif  __has_include(<lua.h>)
		#include <lua.h>
		#include <lualib.h>
		#include <lauxlib.h>
	#else
		#error("No lua libraries found")
	#endif
}
#include <LuaBridge/LuaBridge.h>

#include <cstring>
#include <string>
#include <QPoint>



class LuaDevice : public Device {
	luabridge::LuaRef m_env;

	luabridge::LuaRef m_getGraphBufferLayout = m_env["getGraphBufferLayout"];
	luabridge::LuaRef m_initializeGraphBuffer = m_env["initializeGraphBuffer"];
	lua_State* L;				// to register functions and Format

	bool implementsGraphFunctions();
	static void declarePixelFormat(lua_State* L);

public:

	const DeviceClass getClass() const;

	class PIN_Interface_Lua : public Device::PIN_Interface {
		luabridge::LuaRef m_getPinLayout;
		luabridge::LuaRef m_getPin;
		luabridge::LuaRef m_setPin;

	public:
		PIN_Interface_Lua(luabridge::LuaRef& ref);
		~PIN_Interface_Lua();
		PinLayout getPinLayout() override;
		gpio::Tristate getPin(PinNumber num) override;
		void setPin(PinNumber num, gpio::Tristate val) override;
		static bool implementsInterface(const luabridge::LuaRef& ref);
	};

	class SPI_Interface_Lua : public Device::SPI_Interface {
		luabridge::LuaRef m_send;
	public:
		SPI_Interface_Lua(luabridge::LuaRef& ref);
		~SPI_Interface_Lua();
		gpio::SPI_Response send(gpio::SPI_Command byte) override;
		static bool implementsInterface(const luabridge::LuaRef& ref);
	};

	class Config_Interface_Lua : public Device::Config_Interface {
		luabridge::LuaRef m_getConf;
		luabridge::LuaRef m_setConf;
		luabridge::LuaRef& m_env;	// for building table
	public:
		Config_Interface_Lua(luabridge::LuaRef& ref);
		~Config_Interface_Lua();
		Config getConfig() override;
		bool setConfig(Config conf) override;
		static bool implementsInterface(const luabridge::LuaRef& ref);
	};

	class Input_Interface_Lua : public Device::Input_Interface {
		luabridge::LuaRef m_onClick;
		luabridge::LuaRef m_onKeypress;
		luabridge::LuaRef& m_env; // for building table
	public:
		Input_Interface_Lua(luabridge::LuaRef& ref);
		~Input_Interface_Lua();
		void onClick(bool active) override;
		void onKeypress(Key key, bool active) override;

		static bool implementsInterface(const luabridge::LuaRef& ref);
	};

	LuaDevice(const DeviceID& id, luabridge::LuaRef env, lua_State* L);
	~LuaDevice();

	Layout getLayout() override;
	void initializeBuffer() override;
	void createBuffer(unsigned iconSizeMinimum, QPoint offset) override;

	template<typename FunctionFootprint>
	void registerGlobalFunctionAndInsertLocalAlias(const std::string name, FunctionFootprint fun);
};

