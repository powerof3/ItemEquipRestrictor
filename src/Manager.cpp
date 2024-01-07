#include "Manager.h"
#include "Settings.h"

namespace ItemRestrictor
{
	void Manager::Register()
	{
		if (const auto scriptEventSourceHolder = RE::ScriptEventSourceHolder::GetSingleton()) {
			scriptEventSourceHolder->AddEventSink<RE::TESEquipEvent>(GetSingleton());
			scriptEventSourceHolder->AddEventSink<RE::TESObjectLoadedEvent>(GetSingleton());
			scriptEventSourceHolder->AddEventSink<RE::TESSwitchRaceCompleteEvent>(GetSingleton());
		}
	}

	void Manager::AddAnimationEvent(const RE::Actor* a_actor)
	{
		a_actor->AddAnimationGraphEventSink(GetSingleton());
	}

	void Manager::RemoveAnimationEvent(const RE::Actor* a_actor)
	{
		a_actor->RemoveAnimationGraphEventSink(GetSingleton());
	}

	std::pair<bool, RE::BGSPerk*> Manager::ShouldSkip(const std::string& a_keywordEDID, RE::Actor* a_actor, const RE::TESNPC* a_npc, RE::TESBoundObject* a_object, RestrictParams& a_params)
	{
		if (a_params.restrictOn == RESTRICT_ON::kEquip && !a_keywordEDID.starts_with("RestrictEquip:") || a_params.restrictOn == RESTRICT_ON::kCast && !a_keywordEDID.starts_with("RestrictCast:")) {
			return { false, nullptr };
		}

		const auto restrict_kywd = string::split(a_keywordEDID, ":");
		const auto isPlayer = a_actor->IsPlayerRef();

		// contains debuff perk param
		if (a_params.restrictOn == RESTRICT_ON::kEquip && a_params.restrictType == RESTRICT_TYPE::kRestrict && restrict_kywd.size() > 2 && isPlayer) {
			return { false, nullptr };
		}

		const auto sex = a_npc->GetSex();
		const auto actorLevel = a_npc->GetLevel();

		std::vector<std::string> edids{};
		if (const auto extraLvlCreature = a_actor->extraList.GetByType<RE::ExtraLeveledCreature>()) {
			if (const auto originalBase = extraLvlCreature->originalBase) {
				edids.emplace_back(edid::get_editorID(originalBase));
			}
			if (const auto templateBase = extraLvlCreature->templateBase) {
				edids.emplace_back(edid::get_editorID(templateBase));
			}
		} else {
			edids.emplace_back(edid::get_editorID(a_npc));
		}

		const auto split_filters = string::split(restrict_kywd[1], "+");

		bool shouldSkip = std::ranges::all_of(split_filters, [&](const std::string& a_filter) {
			switch (string::const_hash(a_filter)) {
			case "Male"_h:
				return sex == RE::SEX::kMale;
			case "Female"_h:
				return sex == RE::SEX::kFemale;
			case "Player"_h:
				return isPlayer;
			case "NPC"_h:
				return !isPlayer;
			default:
				{
					if (a_filter.starts_with("Level(")) {
						auto filterLevel = a_filter;
						string::remove_non_numeric(filterLevel);

						const auto level = string::to_num<std::uint16_t>(filterLevel);
						if (actorLevel < level) {
							a_params.restrictReason = RESTRICT_REASON::kLevel;
							return true;
						}
						return false;
					}

					if (a_filter.contains("(")) {
						auto filterSkill = a_filter;
						string::remove_non_alphanumeric(filterSkill);

						if (const auto skills = string::split(filterSkill, " "); !skills.empty()) {
							RE::ActorValue av;
							if (string::is_only_digit(skills[0])) {
								av = string::to_num<RE::ActorValue>(skills[0]);
							} else {
								av = RE::ActorValueList::GetSingleton()->LookupActorValueByName(skills[0]);
							}
							const auto minLevel = string::to_num<std::uint8_t>(skills[1]);

							if (a_actor->GetActorValue(av) < minLevel) {
								a_params.restrictReason = RESTRICT_REASON::kSkill;
								return true;
							}
						}
						return false;
					}

					if (const auto filterForm = RE::TESForm::LookupByEditorID(a_filter)) {
						switch (filterForm->GetFormType()) {
						case RE::FormType::Faction:
							{
								const auto faction = filterForm->As<RE::TESFaction>();
								return faction && a_actor->IsInFaction(faction);
							}
						case RE::FormType::Perk:
							{
								const auto perk = filterForm->As<RE::BGSPerk>();
								return perk && a_actor->HasPerk(perk);
							}
						case RE::FormType::Race:
							return a_actor->GetRace() == filterForm;
						default:
							break;
						}
					}

					return a_actor->HasKeywordString(a_filter) || std::ranges::any_of(edids, [&](const auto& edid) { return string::iequals(edid, a_filter); });
				}
			}
		});

		return { shouldSkip, restrict_kywd.size() > 2 ? RE::TESForm::LookupByEditorID<RE::BGSPerk>(restrict_kywd[2]) : nullptr };
	}

	std::pair<bool, RE::BGSPerk*> Manager::ShouldSkip(RE::Actor* a_actor, RE::TESBoundObject* a_object, RestrictParams& a_params)
	{
		bool         skipEquip = false;
		RE::BGSPerk* debuffPerk = nullptr;

		const auto base = a_actor->GetActorBase();
		if (!base) {
			return { skipEquip, debuffPerk };
		}

		if (const auto keywordForm = a_object->As<RE::BGSKeywordForm>()) {
			keywordForm->ForEachKeyword([&](const RE::BGSKeyword& a_keyword) {
				if (std::tie(skipEquip, debuffPerk) = ShouldSkip(a_keyword.GetFormEditorID(), a_actor, base, a_object, a_params); skipEquip) {
					return RE::BSContainer::ForEachResult::kStop;
				}
				return RE::BSContainer::ForEachResult::kContinue;
			});
		}

		return { skipEquip, debuffPerk };
	}

	void Manager::AddDebuffPerk(const RE::TESBoundObject* a_item, RE::BGSPerk* a_perk)
	{
		RE::PlayerCharacter::GetSingleton()->AddPerk(a_perk);
		_debuffPerkMap[a_item->GetFormID()].insert(a_perk->GetFormID());
	}

	void Manager::RemoveDebuffPerk(const RE::TESBoundObject* a_item)
	{
		if (const auto it = _debuffPerkMap.find(a_item->GetFormID()); it != _debuffPerkMap.end()) {
			for (const auto& perkID : it->second) {
				if (const auto perk = RE::TESForm::LookupByID<RE::BGSPerk>(perkID)) {
					RE::PlayerCharacter::GetSingleton()->RemovePerk(perk);
				}
			}
			_debuffPerkMap.erase(it);
		}
	}

	void Manager::ProcessShouldSkipCast(RE::Actor* a_actor, RE::MagicCaster* a_caster)
	{
		if (!a_caster || !a_caster->currentSpell) {
			return;
		}

		RestrictParams params{ RESTRICT_ON::kCast, RESTRICT_TYPE::kRestrict, RESTRICT_REASON::kGeneric };
		if (auto [skipEquip, debuffPerk] = ShouldSkip(a_actor, a_caster->currentSpell, params); skipEquip) {
			if (a_actor->IsPlayerRef()) {
				const auto notification = Settings::GetSingleton()->GetNotification(a_caster->currentSpell, params);
				if (!notification.empty()) {
					RE::DebugNotification(notification.c_str());
				}
			}
			a_caster->InterruptCast(true);
			RE::PlaySound("MAGFail");
		}
	}

	namespace Equip
	{
		struct DoEquip
		{
			static void thunk(RE::ActorEquipManager* a_manager, RE::Actor* a_actor, RE::TESBoundObject* a_object, const RE::ObjectEquipParams& a_objectEquipParams)
			{
				if (a_actor && a_object && !a_objectEquipParams.forceEquip) {
					RestrictParams params{ RESTRICT_ON::kEquip, RESTRICT_TYPE::kRestrict, RESTRICT_REASON::kGeneric };
					if (auto [skipEquip, debuffPerk] = Manager::ShouldSkip(a_actor, a_object, params); skipEquip) {
						if (a_actor->IsPlayerRef()) {
							const auto notification = Settings::GetSingleton()->GetNotification(a_object, params);
							if (!notification.empty()) {
								RE::DebugNotification(notification.c_str());
							}
						}
						return;
					}
				}

				return func(a_manager, a_actor, a_object, a_objectEquipParams);
			}
			static inline REL::Relocation<decltype(thunk)> func;
		};

		void Install()
		{
			std::array targets{
				std::make_pair(RELOCATION_ID(37938, 38894), OFFSET(0xE5, 0x170)),  //ActorEquipManager::EquipObject
				std::make_pair(RELOCATION_ID(37937, 38893), 0xBC),                 //ActorEquipManager::EquipImpl?
			};

			for (const auto& [id, offset] : targets) {
				REL::Relocation<std::uintptr_t> target{ id, offset };
				stl::write_thunk_call<DoEquip>(target.address());
			}
		}
	}

	namespace Unequip
	{
		struct DoUnequip
		{
			static void thunk(RE::ActorEquipManager* a_manager, RE::Actor* a_actor, RE::TESBoundObject* a_object, const RE::ObjectEquipParams& a_objectEquipParams)
			{
				RestrictParams params{ RESTRICT_ON::kEquip, RESTRICT_TYPE::kRestrict, RESTRICT_REASON::kGeneric };
				if (auto [skipEquip, debuffPerk] = Manager::ShouldSkip(a_actor, a_object, params); skipEquip) {
					if (a_actor->IsPlayerRef()) {
						const auto notification = Settings::GetSingleton()->GetNotification(a_object, params);
						if (!notification.empty()) {
							RE::DebugNotification(notification.c_str());
						}
					}
					return;
				}

				return func(a_manager, a_actor, a_object, a_objectEquipParams);
			}
			static inline REL::Relocation<decltype(thunk)> func;
		};

		void Install()
		{
			std::array targets{
				std::make_pair(RELOCATION_ID(37938, 38894), OFFSET(0xE5, 0x170)),  //ActorEquipManager::EquipObject
				std::make_pair(RELOCATION_ID(37937, 38893), 0xBC),                 //ActorEquipManager::EquipImpl?
			};

			for (const auto& [id, offset] : targets) {
				REL::Relocation<std::uintptr_t> target{ id, offset };
				stl::write_thunk_call<DoUnequip>(target.address());
			}
		}
	}

	RE::BSEventNotifyControl Manager::ProcessEvent(RE::TESEquipEvent const* a_evn, RE::BSTEventSource<RE::TESEquipEvent>*)
	{
		if (!a_evn || !a_evn->actor) {
			return RE::BSEventNotifyControl::kContinue;
		}

		const auto actor = a_evn->actor->As<RE::Actor>();
		if (!actor || !a_evn->actor->IsPlayerRef()) {
			return RE::BSEventNotifyControl::kContinue;
		}

		const auto item = RE::TESForm::LookupByID<RE::TESBoundObject>(a_evn->baseObject);
		if (!item) {
			return RE::BSEventNotifyControl::kContinue;
		}

		if (a_evn->equipped) {
			RestrictParams params{ RESTRICT_ON::kEquip, RESTRICT_TYPE::kDebuff, RESTRICT_REASON::kGeneric };
			if (auto [skipEquip, debuffPerk] = ShouldSkip(actor, item, params); skipEquip && debuffPerk) {
				AddDebuffPerk(item, debuffPerk);
				const auto notification = Settings::GetSingleton()->GetNotification(item, params);
				if (!notification.empty()) {
					RE::DebugNotification(notification.c_str());
				}
			}
		} else {
			RemoveDebuffPerk(item);
		}

		return RE::BSEventNotifyControl::kContinue;
	}

	RE::BSEventNotifyControl Manager::ProcessEvent(RE::TESObjectLoadedEvent const* a_evn, RE::BSTEventSource<RE::TESObjectLoadedEvent>*)
	{
		if (!a_evn) {
			return RE::BSEventNotifyControl::kContinue;
		}

		if (const auto actor = RE::TESForm::LookupByID<RE::Actor>(a_evn->formID)) {
			AddAnimationEvent(actor);
		}

		return RE::BSEventNotifyControl::kContinue;
	}

	RE::BSEventNotifyControl Manager::ProcessEvent(RE::TESSwitchRaceCompleteEvent const* a_evn, RE::BSTEventSource<RE::TESSwitchRaceCompleteEvent>*)
	{
		if (!a_evn || !a_evn->subject) {
			return RE::BSEventNotifyControl::kContinue;
		}

		if (const auto actor = a_evn->subject->As<RE::Actor>()) {
			AddAnimationEvent(actor);
		}

		return RE::BSEventNotifyControl::kContinue;
	}

	RE::BSEventNotifyControl Manager::ProcessEvent(RE::BSAnimationGraphEvent const* a_evn, RE::BSTEventSource<RE::BSAnimationGraphEvent>*)
	{
		if (!a_evn || !a_evn->holder) {
			return RE::BSEventNotifyControl::kContinue;
		}

		const auto actor = const_cast<RE::Actor*>(a_evn->holder->As<RE::Actor>());
		if (!actor) {
			return RE::BSEventNotifyControl::kContinue;
		}

		switch (string::const_hash(a_evn->tag)) {
		case "BeginCastLeft"_h:
			ProcessShouldSkipCast(actor, actor->magicCasters[0]);
			break;
		case "BeginCastRight"_h:
			ProcessShouldSkipCast(actor, actor->magicCasters[1]);
			break;
		case "BeginCastVoice"_h:
			ProcessShouldSkipCast(actor, actor->magicCasters[2]);
			break;
		default:
			break;
		}

		return RE::BSEventNotifyControl::kContinue;
	}

	void Install()
	{
		Settings::GetSingleton()->LoadSettings();
		Manager::Register();

		Equip::Install();
		//Unequip::Install();
	}
}
