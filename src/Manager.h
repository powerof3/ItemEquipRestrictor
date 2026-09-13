#pragma once

#include "RestrictData.h"

namespace ItemRestrictor
{
	class Manager :
		public REX::TSingleton<Manager>,
		public RE::BSTEventSink<RE::TESEquipEvent>,
		public RE::BSTEventSink<RE::TESObjectLoadedEvent>,
		public RE::BSTEventSink<RE::TESSwitchRaceCompleteEvent>,
		public RE::BSTEventSink<RE::BSAnimationGraphEvent>,
		public RE::BSTEventSink<SKSE::ModCallbackEvent>
	{
	public:
		static void Register();
		static void AddAnimationEvent(const RE::Actor* a_actor);
		static void RemoveAnimationEvent(const RE::Actor* a_actor);

		static void OnDataLoaded();

		RestrictResult ShouldSkip(RestrictParams& a_params);
		RestrictResult ShouldSkip(RE::BGSKeywordForm* a_keyword, const RestrictData& a_data, RestrictParams& a_params);

		void AddDebuff(const RE::TESBoundObject* a_item, RE::TESForm* a_debuffForm, bool equip);
		void RemoveDebuff(const RE::TESBoundObject* a_item, bool equip);

	private:
		static void LoadKeywords();
		
		void ProcessShouldSkipCast(RE::Actor* a_actor, RE::MagicCaster* a_caster);

		RE::BSEventNotifyControl ProcessEvent(RE::TESEquipEvent const* a_evn, RE::BSTEventSource<RE::TESEquipEvent>*) override;
		RE::BSEventNotifyControl ProcessEvent(RE::TESObjectLoadedEvent const* a_evn, RE::BSTEventSource<RE::TESObjectLoadedEvent>*) override;
		RE::BSEventNotifyControl ProcessEvent(RE::TESSwitchRaceCompleteEvent const* a_evn, RE::BSTEventSource<RE::TESSwitchRaceCompleteEvent>*) override;
		RE::BSEventNotifyControl ProcessEvent(RE::BSAnimationGraphEvent const* a_evn, RE::BSTEventSource<RE::BSAnimationGraphEvent>*) override;
		RE::BSEventNotifyControl ProcessEvent(SKSE::ModCallbackEvent const* a_evn, RE::BSTEventSource<SKSE::ModCallbackEvent>*) override;

		// members
		FlatMap<const RE::BGSKeyword*, RestrictFilter> _restrictKeywords{};
		FlatMap<RE::FormID, FlatSet<RE::FormID>> _objectEquipDebuffs{};
		FlatMap<RE::FormID, FlatSet<RE::FormID>> _debuffEquipObjects{};
		FlatMap<RE::FormID, FlatSet<RE::FormID>> _objectPickUpDebuffs{};
		FlatMap<RE::FormID, FlatSet<RE::FormID>> _debuffPickUpObjects{};
	};
}
