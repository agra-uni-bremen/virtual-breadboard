/*
 * device.cpp
 *
 *  Created on: Sep 30, 2021
 *      Author: dwd
 */
#include "luaDevice.hpp"

#include <QKeySequence>

using std::string;
using std::cout;
using std::cerr;
using std::endl;
using luabridge::LuaRef;
using luabridge::LuaResult;


LuaDevice::LuaDevice(const DeviceID id, LuaRef env, lua_State* l) : Device(id), m_env(env), L(l){
	if(PIN_Interface_Lua::implementsInterface(m_env)) {
		pin = std::make_unique<PIN_Interface_Lua>(m_env);
	}
	if(SPI_Interface_Lua::implementsInterface(m_env)) {
		spi = std::make_unique<SPI_Interface_Lua>(m_env);
	}
	if(Config_Interface_Lua::implementsInterface(m_env)) {
		conf = std::make_unique<Config_Interface_Lua>(m_env);
	}
	if(Input_Interface_Lua::implementsInterface(m_env)) {
		input = std::make_unique<Input_Interface_Lua>(m_env);
	}

	if(implementsGraphFunctions()) {
		declarePixelFormat(L);
	}
};

LuaDevice::~LuaDevice() {}

const DeviceClass LuaDevice::getClass() const {
	// If Device exists, classname is known to exist of correct type
	return m_env["classname"].unsafe_cast<string>();
}

bool LuaDevice::implementsGraphFunctions() {
	if(!m_env["getGraphBufferLayout"].isFunction()) {
		//cout << "getGraphBufferLayout not a Function" << endl;
		return false;
	}
	LuaResult r = m_env["getGraphBufferLayout"]();
	if(r.size() != 1 || !r[0].isTable() || r[0].length() != 3) {
		//cout << "return val is " << r.size() << " " << !r[0].isTable() << r[0].length() << endl;
		return false;
	}
	const auto& type = r[0][3];
	if(!type.isString()) {
		//cout << "Type not a string" << endl;
		return false;
	}
	return true;
}

Device::Layout LuaDevice::getLayout() {
	if(!m_getGraphBufferLayout.isFunction()) {
		return Device::getLayout();
	}
	Layout ret;
	LuaResult r = m_getGraphBufferLayout();
	if(!r || r.size() != 1 || !r[0].isTable() || r[0].length() != 3) {
		cerr << "[LuaDevice] Graph Layout malformed " << r.errorMessage() << endl;
		return ret;
	}
	const auto maybe_width = r[0][1].cast<unsigned>();
	const auto maybe_height = r[0][2].cast<unsigned>();
	if(!maybe_width || !maybe_height) {
		cerr << "[LuaDevice] height or width invalid: " << maybe_width.message() << endl << maybe_height.message() << endl;
		return ret;
	}
	ret.width = maybe_width.value();
	ret.height = maybe_height.value();
	const auto& type = r[0][3];
	if(!type.isString() || type != string("rgba")) {
		cerr << "[LuaDevice] Graph Layout type may only be 'rgba' at the moment." << endl;
		return ret;
	}
	ret.data_type = type.unsafe_cast<string>();
	return ret;
}

void LuaDevice::initializeBuffer(){
	if(m_initializeGraphBuffer.isFunction()) {
		m_initializeGraphBuffer();
	} else {
		//cout << "Device " << m_deviceId << " does not implement 'initializeGraphBuffer()'" << endl;
		Device::initializeBuffer();
	}
}

void LuaDevice::declarePixelFormat(lua_State* L) {
	if(luaL_dostring (L, "graphbuf.Pixel(0,0,0,0)") != 0) {
		//cout << "Testpixel could not be created, probably was not yet registered" << endl;
		luabridge::getGlobalNamespace(L)
			.beginNamespace("graphbuf")
			  .beginClass <Pixel> ("Pixel")
			    .addConstructor (+[](void* ptr, const uint8_t r, const uint8_t g, const uint8_t b, const uint8_t a) { return new (ptr) Pixel{r,g,b,a};})
			    .addProperty ("r", &Pixel::r)
			    .addProperty ("g", &Pixel::g)
			    .addProperty ("b", &Pixel::b)
			    .addProperty ("a", &Pixel::a)
			  .endClass ()
			.endNamespace()
		;
		//cout << "Graphbuf: Declared Pixel class to lua." << endl;
	} else {
		//cout << "Pixel class already registered." << endl;
	}
}

template<typename FunctionFootprint>
void LuaDevice::registerGlobalFunctionAndInsertLocalAlias(
		const std::string name, FunctionFootprint fun) {
	if(m_id.length() == 0 || name.length() == 0) {
		cerr << "[LuaDevice] Error: Name '" << name << "' or prefix '"
				<< m_id << "' invalid!" << endl;
		return;
	}

	const auto globalFunctionName = m_id + "_" + name;
	luabridge::getGlobalNamespace(L)
		.addFunction(globalFunctionName.c_str(), fun)
	;
	//cout << "Inserted function " << globalFunctionName << " into global namespace" << endl;

	const auto global_lua_fun = luabridge::getGlobal(L, globalFunctionName.c_str());
	if(!global_lua_fun.isFunction()) {
		cerr << "[LuaDevice] Error: " << globalFunctionName  << " is not valid!" << endl;
		return;
	}
	m_env[name.c_str()] = global_lua_fun;

	//cout << "Registered function " << globalFunctionName << " to " << name << endl;
};

void LuaDevice::createBuffer(unsigned iconSizeMinimum, QPoint offset) {
	Device::createBuffer(iconSizeMinimum, offset);
	std::function<Pixel(const Xoffset, const Yoffset)> getBuf = [this](const Xoffset x, const Yoffset y){
		return getPixel(x, y);
	};
	registerGlobalFunctionAndInsertLocalAlias("getGraphbuffer", getBuf);
	std::function<void(const Xoffset, const Yoffset, Pixel)> setBuf = [this](const Xoffset x, const Yoffset y, Pixel p) {
		setPixel(x, y, p);
	};

	registerGlobalFunctionAndInsertLocalAlias<>("setGraphbuffer", setBuf);
	m_env["buffer_width"] = buffer.width();
	m_env["buffer_height"] = buffer.height();
	initializeBuffer();
}

LuaDevice::PIN_Interface_Lua::PIN_Interface_Lua(LuaRef& ref) :
		m_getPinLayout(ref["getPinLayout"]),
		m_getPin(ref["getPin"]), m_setPin(ref["setPin"]) {
	if(!implementsInterface(ref))
		cerr << "[LuaDevice] WARN: Device " << ref << " not implementing interface" << endl;
}

LuaDevice::PIN_Interface_Lua::~PIN_Interface_Lua() {}

bool LuaDevice::PIN_Interface_Lua::implementsInterface(const luabridge::LuaRef& ref) {
	// TODO: Better checks
	return ref["getPinLayout"].isFunction() &&
	       (ref["getPin"].isFunction() ||
	        ref["setPin"].isFunction());
}

PinLayout LuaDevice::PIN_Interface_Lua::getPinLayout() {
	PinLayout ret;
	LuaResult r = m_getPinLayout();
	//cout << r.size() << " elements in pinlayout" << endl;

	// TODO: check r

	ret.reserve(r.size());
	for(unsigned i = 0; i < r.size(); i++) {
		if(!r[i].isTable()){
			cerr << "[LuaDevice] Pin layout return value malformed:" << endl;
			cerr << "[LuaDevice] " << i << "\t" << r[i] << endl;
			continue;
		}
		//cout << "\tElement " << i << ": " << r[i] << " with length " << r[i].length() << endl;
		if(r[i].length() < 2 || r[i].length() > 3) {
			cerr << "[LuaDevice] Pin layout element " << i << " (" << r[i] << ") is malformed" << endl;
			continue;
		}
		PinDesc desc;
		const auto maybe_number = r[i][1].cast<PinNumber>();
		if(!maybe_number) {
			cerr << "[LuaDevice] Pin layout element " << i << " (" << r[i] << ") has invalid pin number" << endl;
			continue;
		}
		const auto number = maybe_number.value();
		desc.name = "undef";
		if(r[i].length() == 3)
			desc.name = r[i][3].tostring();

		const string direction_raw = r[i][2];
		if(direction_raw == "input") {
			desc.dir = PinDesc::Dir::input;
		} else if(direction_raw == "output") {
			desc.dir = PinDesc::Dir::output;
		} else if(direction_raw == "inout") {
			desc.dir = PinDesc::Dir::inout;
		} else {
			// TODO: Add PWM input here? Or better, lua script has to cope with ratios
			cerr << "[LuaDevice] Pin layout element " << i << " (" << r[i] << "), direction " << direction_raw << " is malformed" << endl;
			continue;
		}
		//cout << "Mapping Device's pin " << number << " (" << desc.name << ")" << endl;
		ret.emplace(number, desc);
	}
	return ret;
}


gpio::Tristate LuaDevice::PIN_Interface_Lua::getPin(PinNumber num) {
	const LuaResult r = m_getPin(num);
	if(!r || !r[0].isBool()) {
		cerr << "[LuaDevice] Device getPin returned malformed output: " << r.errorMessage() << endl;
		return gpio::Tristate::LOW;
	}
	return r[0].unsafe_cast<bool>() ? gpio::Tristate::HIGH : gpio::Tristate::LOW;
}

void LuaDevice::PIN_Interface_Lua::setPin(PinNumber num, gpio::Tristate val) {
	const LuaResult r = m_setPin(num, val == gpio::Tristate::HIGH ? true : false);
	if(!r) {
		cerr << "[LuaDevice] Device setPin error: " << r.errorMessage() << endl;
	}
}

LuaDevice::SPI_Interface_Lua::SPI_Interface_Lua(LuaRef& ref) :
		m_send(ref["receiveSPI"]) {
	if(!implementsInterface(ref))
		cerr << "[LuaDevice] " << ref << " not implementing SPI interface" << endl;
}

LuaDevice::SPI_Interface_Lua::~SPI_Interface_Lua() {}


gpio::SPI_Response LuaDevice::SPI_Interface_Lua::send(gpio::SPI_Command byte) {
	LuaResult r = m_send(byte);
	if(r.size() != 1) {
		cerr << "[LuaDevice] send SPI function failed! " << r.errorMessage() << endl;
		return 0;
	}
	if(!r[0].isNumber()) {
		cerr << "[LuaDevice] send SPI function returned invalid type " << r[0] << endl;
		return 0;
	}
	return r[0];
}

bool LuaDevice::SPI_Interface_Lua::implementsInterface(const LuaRef& ref) {
	return ref["receiveSPI"].isFunction();
}

LuaDevice::Config_Interface_Lua::Config_Interface_Lua(luabridge::LuaRef& ref) :
	m_getConf(ref["getConfig"]), m_setConf(ref["setConfig"]), m_env(ref){

};

LuaDevice::Config_Interface_Lua::~Config_Interface_Lua() {}

bool LuaDevice::Config_Interface_Lua::implementsInterface(const luabridge::LuaRef& ref) {
	return !ref["getConfig"].isNil() && !ref["setConfig"].isNil();
}

Config LuaDevice::Config_Interface_Lua::getConfig(){
	Config ret;
	LuaResult r = m_getConf();

	// TODO: Check success and print result

	for(unsigned i = 0; i < r.size(); i++) {
		if(!r[i].isTable()){
			cerr << "[LuaDevice] config return value malformed:" << endl;
			cerr << "[LuaDevice] " << i << "\t" << r[i] << endl;
			continue;
		}
		//cout << "\tElement " << i << ": " << r[i] << " with length " << r[i].length() << endl;
		if(r[i].length() != 2) {
			cerr << "[LuaDevice] Config element " << i << " (" << r[i] << ") is not a pair" << endl;
			continue;
		}

		LuaRef name = r[i][1];
		LuaRef value = r[i][2];

		if(!name.isString()) {
			cerr << "[LuaDevice] Config name " << name << " is not a string" << endl;
			continue;
		}

		switch(value.type()) {
		case LUA_TNUMBER:
			ret.emplace(
					name, ConfigElem{value.unsafe_cast<typeof(ConfigElem::Value::integer)>()}
			);
			break;
		case LUA_TBOOLEAN:
			ret.emplace(
					name, ConfigElem{value.unsafe_cast<bool>()}
			);
			break;
		case LUA_TSTRING:
			ret.emplace(
					name, ConfigElem{value.unsafe_cast<string>().c_str()}
			);
			break;
		default:
			cerr << "[LuaDevice] Config value of unknown type: " << value << endl;
		}
	}
	return ret;
}

bool LuaDevice::Config_Interface_Lua::setConfig(Config conf) {
	LuaRef c = luabridge::newTable(m_env.state());
	for(auto& [name, elem] : conf) {
		switch(elem.type) {
		case ConfigElem::Type::boolean:
			c[name] = elem.value.boolean;
			break;
		case ConfigElem::Type::integer:
			c[name] = elem.value.integer;
			break;
		case ConfigElem::Type::string:
			c[name] = string(elem.value.string);
			break;
		default:
			cerr << "[LuaDevice] Config Element of invalid type!" << endl;
		}
	}

	LuaResult r = m_setConf(c);
	if(!r) {
		std::cerr << "[LuaDevice] Error setting config of [some] device: : " << r.errorMessage() << std::endl;
	}
	return r.wasOk();
}

LuaDevice::Input_Interface_Lua::Input_Interface_Lua(luabridge::LuaRef& ref) : m_onClick(ref["onClick"]),
		m_onKeypress(ref["onKeypress"]), m_env(ref) {
	if(!implementsInterface(ref))
		cerr << "[LuaDevice] WARN: Device " << ref << " not implementing interface" << endl;
}

LuaDevice::Input_Interface_Lua::~Input_Interface_Lua() {}

void LuaDevice::Input_Interface_Lua::onClick(bool active) {
	m_onClick(active);
}

void LuaDevice::Input_Interface_Lua::onKeypress(Key key, bool active) {
	m_onKeypress(QKeySequence(key).toString().toStdString(), active);
}

bool LuaDevice::Input_Interface_Lua::implementsInterface(const luabridge::LuaRef& ref) {
	return !ref["onClick"].isNil() || !ref["onKeypress"].isNil();
}

