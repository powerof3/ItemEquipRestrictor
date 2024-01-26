#pragma once

enum class RESTRICT_ON
{
	kEquip,
	kCast
};

enum class RESTRICT_TYPE
{
	kRestrict,
	kDebuff
};

enum class RESTRICT_REASON
{
	kGeneric,
	kSkill,
	kLevel
};

struct RestrictParams
{
	RESTRICT_ON     restrictOn;
	RESTRICT_TYPE   restrictType;
	RESTRICT_REASON restrictReason;
};

namespace ItemRestrictor
{
	class Manager :
		public ISingleton<Manager>,
		public RE::BSTEventSink<RE::TESEquipEvent>,
		public RE::BSTEventSink<RE::TESObjectLoadedEvent>,
		public RE::BSTEventSink<RE::TESSwitchRaceCompleteEvent>,
		public RE::BSTEventSink<RE::BSAnimationGraphEvent>
	{
	public:
		static void Register();
		static void AddAnimationEvent(const RE::Actor* a_actor);
		static void RemoveAnimationEvent(const RE::Actor* a_actor);

		static std::pair<bool, RE::TESForm*> ShouldSkip(const std::string& a_keywordEDID, RE::Actor* a_actor, const RE::TESNPC* a_npc, const RE::TESBoundObject* a_object, RestrictParams& a_params);
		static std::pair<bool, RE::TESForm*> ShouldSkip(RE::Actor* a_actor, RE::TESBoundObject* a_object, RestrictParams& a_params);

		void AddDebuff(const RE::TESBoundObject* a_item, RE::TESForm* a_debuffForm);
		void RemoveDebuff(const RE::TESBoundObject* a_item);

	private:
		static void get_npc_edids(RE::Actor* a_actor, const RE::TESNPC* a_npc, std::vector<std::string>& a_edids);
		static bool is_bow_or_crossbow(RE::TESForm* a_object);

	    static void ProcessShouldSkipCast(RE::Actor* a_actor, RE::MagicCaster* a_caster);

		RE::BSEventNotifyControl ProcessEvent(RE::TESEquipEvent const* a_evn, RE::BSTEventSource<RE::TESEquipEvent>*) override;
		RE::BSEventNotifyControl ProcessEvent(RE::TESObjectLoadedEvent const* a_evn, RE::BSTEventSource<RE::TESObjectLoadedEvent>*) override;
		RE::BSEventNotifyControl ProcessEvent(RE::TESSwitchRaceCompleteEvent const* a_evn, RE::BSTEventSource<RE::TESSwitchRaceCompleteEvent>*) override;
		RE::BSEventNotifyControl ProcessEvent(RE::BSAnimationGraphEvent const* a_evn, RE::BSTEventSource<RE::BSAnimationGraphEvent>*) override;

		// members
		std::unordered_map<RE::FormID, std::unordered_set<RE::FormID>> _objectDebuffsMap{};
		std::unordered_map<RE::FormID, std::unordered_set<RE::FormID>> _debuffObjectsMap{};
	};

	void Install();
}
