#pragma once

#define NOMINMAX

#include <shared_mutex>

#include "RE/Skyrim.h"
#include "REX/REX/Singleton.h"
#include "SKSE/SKSE.h"

#ifdef NDEBUG
#	include <spdlog/sinks/basic_file_sink.h>
#else
#	include <spdlog/sinks/msvc_sink.h>
#endif
#include <srell.hpp>
#include <xbyak/xbyak.h>

#include <boost/unordered/unordered_flat_map.hpp>
#include <boost/unordered/unordered_flat_set.hpp>

#include "ClibUtil/editorID.hpp"
#include "ClibUtil/simpleINI.hpp"
#include "ClibUtil/string.hpp"

namespace logger = SKSE::log;
namespace string = clib_util::string;
namespace edid = clib_util::editorID;
namespace ini = clib_util::ini;

using namespace std::literals;
using namespace clib_util::string::literals;

// for visting variants
template <class... Ts>
struct overload : Ts...
{
	using Ts::operator()...;
};

template <class K, class D, class H = boost::hash<K>, class KEqual = std::equal_to<K>>
using FlatMap = boost::unordered_flat_map<K, D, H, KEqual>;

template <class K, class H = boost::hash<K>, class KEqual = std::equal_to<K>>
using FlatSet = boost::unordered_flat_set<K, H, KEqual>;

template <class K, class D, class H = boost::hash<K>, class KEqual = std::equal_to<K>>
using LockedMap = boost::concurrent_flat_map<K, D, H, KEqual>;

template <class K, class H = boost::hash<K>, class KEqual = std::equal_to<K>>
using LockedSet = boost::unordered_flat_set<K, H, KEqual>;

struct string_hash
{
	using is_transparent = void;  // enable heterogeneous overloads

	std::size_t operator()(std::string_view str) const
	{
		std::size_t seed = 0;
		for (auto it = str.begin(); it != str.end(); ++it) {
			boost::hash_combine(seed, std::tolower(*it));
		}
		return seed;
	}
};

struct string_cmp
{
	using is_transparent = void;  // enable heterogeneous overloads

	bool operator()(const std::string& str1, const std::string& str2) const
	{
		return string::iequals(str1, str2);
	}
	bool operator()(std::string_view str1, std::string_view str2) const
	{
		return string::iequals(str1, str2);
	}
};

template <class D>
using StringMap = FlatMap<std::string, D, string_hash, string_cmp>;

using StringSet = FlatSet<std::string, string_hash, string_cmp>;

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
