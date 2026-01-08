#pragma once

#include "RestrictData.h"

namespace ItemRestrictor
{
	class Manager :
		public REX::Singleton<Manager>,
		public RE::BSTEventSink<RE::TESEquipEvent>,
		public RE::BSTEventSink<RE::TESObjectLoadedEvent>,
		public RE::BSTEventSink<RE::TESSwitchRaceCompleteEvent>,
		public RE::BSTEventSink<RE::BSAnimationGraphEvent>
	{
	public:
		static void Register();
		static void AddAnimationEvent(const RE::Actor* a_actor);
		static void RemoveAnimationEvent(const RE::Actor* a_actor);

		RestrictResult ShouldSkip(const std::string& a_keywordEDID, const RestrictData& a_data, RestrictParams& a_params);
		RestrictResult ShouldSkip(RestrictParams& a_params);
		RestrictResult ShouldSkip(RE::BGSKeywordForm* a_keyword, RestrictParams& a_params);

		void AddDebuff(const RE::TESBoundObject* a_item, RE::TESForm* a_debuffForm, bool equip);
		void RemoveDebuff(const RE::TESBoundObject* a_item, bool equip);

	private:
		void ProcessShouldSkipCast(RE::Actor* a_actor, RE::MagicCaster* a_caster);

		RE::BSEventNotifyControl ProcessEvent(RE::TESEquipEvent const* a_evn, RE::BSTEventSource<RE::TESEquipEvent>*) override;
		RE::BSEventNotifyControl ProcessEvent(RE::TESObjectLoadedEvent const* a_evn, RE::BSTEventSource<RE::TESObjectLoadedEvent>*) override;
		RE::BSEventNotifyControl ProcessEvent(RE::TESSwitchRaceCompleteEvent const* a_evn, RE::BSTEventSource<RE::TESSwitchRaceCompleteEvent>*) override;
		RE::BSEventNotifyControl ProcessEvent(RE::BSAnimationGraphEvent const* a_evn, RE::BSTEventSource<RE::BSAnimationGraphEvent>*) override;

		// members
		StringMap<RestrictFilter>                _restrictKeywords{};
		StringSet                                _rejectedKeywords{};
		FlatMap<RE::FormID, FlatSet<RE::FormID>> _objectEquipDebuffs{};
		FlatMap<RE::FormID, FlatSet<RE::FormID>> _debuffEquipObjects{};
		FlatMap<RE::FormID, FlatSet<RE::FormID>> _objectPickUpDebuffs{};
		FlatMap<RE::FormID, FlatSet<RE::FormID>> _debuffPickUpObjects{};
	};
}
