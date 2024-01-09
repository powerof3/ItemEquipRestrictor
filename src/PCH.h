#pragma once

#include <shared_mutex>

#include "RE/Skyrim.h"
#include "SKSE/SKSE.h"

#ifdef NDEBUG
#	include <spdlog/sinks/basic_file_sink.h>
#else
#	include <spdlog/sinks/msvc_sink.h>
#endif
#include <xbyak/xbyak.h>
#include <srell.hpp>

#include "ClibUtil/editorID.hpp"
#include "ClibUtil/simpleINI.hpp"
#include "ClibUtil/singleton.hpp"
#include "ClibUtil/string.hpp"

namespace logger = SKSE::log;
namespace string = clib_util::string;
namespace edid = clib_util::editorID;
namespace ini = clib_util::ini;

using namespace std::literals;
using namespace clib_util::string::literals;
using namespace clib_util::singleton;

namespace stl
{
	using namespace SKSE::stl;

	template <class T>
	void write_thunk_call(std::uintptr_t a_src)
	{
		SKSE::AllocTrampoline(14);

		auto& trampoline = SKSE::GetTrampoline();
		T::func = trampoline.write_call<5>(a_src, T::thunk);
	}

	template <class F, class T>
	void write_vfunc()
	{
		REL::Relocation<std::uintptr_t> vtbl{ F::VTABLE[0] };
		T::func = vtbl.write_vfunc(T::idx, T::thunk);
	}
}

#define DLLEXPORT __declspec(dllexport)

#ifdef SKYRIM_AE
#	define OFFSET(se, ae) ae
#else
#	define OFFSET(se, ae) se
#endif

#include "Version.h"
