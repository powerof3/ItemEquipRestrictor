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
		if (const auto modCallbackSource = SKSE::GetModCallbackEventSource()) {
			modCallbackSource->AddEventSink(GetSingleton());
		}
	}

	void Manager::OnDataLoaded()
	{
		if (REX::W32::GetModuleHandleA("po3_KeywordItemDistributor.dll") != nullptr) {
			REX::INFO("KID detected, waiting for KID_KeywordDistributionDone");
			return;
		}
		LoadKeywords();
	}

	void Manager::AddAnimationEvent(const RE::Actor* a_actor)
	{
		a_actor->AddAnimationGraphEventSink(GetSingleton());
	}

	void Manager::RemoveAnimationEvent(const RE::Actor* a_actor)
	{
		a_actor->RemoveAnimationGraphEventSink(GetSingleton());
	}

	void Manager::LoadKeywords()
	{
		const auto manager = GetSingleton();
		const auto dataHandler = RE::TESDataHandler::GetSingleton();

		manager->_restrictKeywords.clear();

		for (const auto keyword : dataHandler->GetFormArray<RE::BGSKeyword>()) {
			if (!keyword) {
				continue;
			}
			const auto edid = keyword->GetFormEditorID();
			if (REX::STR::IS_EMPTY(edid)) {
				continue;
			}
			const auto restrictType = RestrictFilter::GetRestrictType(edid);
			if (restrictType == RESTRICT_ON::kInvalid) {
				continue;
			}
			manager->_restrictKeywords.try_emplace(keyword, edid, restrictType);
		}

		REX::INFO("Loaded {} restrict keyword(s)", manager->_restrictKeywords.size());
	}

	RestrictResult Manager::ShouldSkip(RestrictParams& a_params)
	{
		RestrictResult result;

		if (_restrictKeywords.empty()) {
			return result;
		}

		RestrictData restrictData(a_params);
		if (!restrictData.actor || !restrictData.object) {
			return result;
		}

		if (a_params.object->Is(RE::FormType::Shout)) {
			const auto shout = a_params.object->As<RE::TESShout>();
			for (const auto& shoutWord : shout->variations) {
				if (shoutWord.spell) {
					result = ShouldSkip(shoutWord.spell->As<RE::BGSKeywordForm>(), restrictData, a_params);
					if (result.shouldSkip) {
						break;
					}
				}
			}
		} else {
			result = ShouldSkip(a_params.object->As<RE::BGSKeywordForm>(), restrictData, a_params);
		}

		return result;
	}

	RestrictResult Manager::ShouldSkip(RE::BGSKeywordForm* a_keywordForm, const RestrictData& a_data, RestrictParams& a_params)
	{
		RestrictResult result;

		if (!a_keywordForm || a_keywordForm->GetNumKeywords() == 0) {
			return result;
		}

		a_keywordForm->ForEachKeyword([&](const RE::BGSKeyword* a_keyword) {
			if (const auto it = _restrictKeywords.find(a_keyword); it != _restrictKeywords.end()) {
				if (result = it->second.MatchFilter(a_data, a_params); result.shouldSkip) {
					return RE::BSContainer::ForEachResult::kStop;
				}
			}
			return RE::BSContainer::ForEachResult::kContinue;
		});

		return result;
	}

	void Manager::AddDebuff(const RE::TESBoundObject* a_item, RE::TESForm* a_debuffForm, bool equip)
	{
		if (a_debuffForm->Is(RE::FormType::Perk)) {
			RE::PlayerCharacter::GetSingleton()->AddPerk(a_debuffForm->As<RE::BGSPerk>());
		} else {
			RE::PlayerCharacter::GetSingleton()->AddSpell(a_debuffForm->As<RE::SpellItem>());
		}

		auto& objectDebuffs = equip ? _objectEquipDebuffs : _objectPickUpDebuffs;
		auto& debuffObjects = equip ? _debuffEquipObjects : _debuffPickUpObjects;

		objectDebuffs[a_item->GetFormID()].insert(a_debuffForm->GetFormID());
		debuffObjects[a_debuffForm->GetFormID()].insert(a_item->GetFormID());
	}

	void Manager::RemoveDebuff(const RE::TESBoundObject* a_item, bool equip)
	{
		const auto itemID = a_item->GetFormID();

		auto& objectDebuffs = equip ? _objectEquipDebuffs : _objectPickUpDebuffs;
		auto& debuffObjects = equip ? _debuffEquipObjects : _debuffPickUpObjects;

		if (const auto oIt = objectDebuffs.find(itemID); oIt != objectDebuffs.end()) {
			for (const auto& debuffID : oIt->second) {
				if (const auto dIt = debuffObjects.find(debuffID); dIt != debuffObjects.end()) {
					if (dIt->second.erase(itemID) && dIt->second.empty()) {
						if (const auto debuffForm = RE::TESForm::LookupByID(debuffID)) {
							if (debuffForm->Is(RE::FormType::Perk)) {
								RE::PlayerCharacter::GetSingleton()->RemovePerk(debuffForm->As<RE::BGSPerk>());
							} else {
								RE::PlayerCharacter::GetSingleton()->RemoveSpell(debuffForm->As<RE::SpellItem>());
							}
						}
					}
				}
			}
			objectDebuffs.erase(oIt);
		}
	}

	void Manager::ProcessShouldSkipCast(RE::Actor* a_actor, RE::MagicCaster* a_caster)
	{
		if (!a_caster || !a_caster->currentSpell) {
			return;
		}
		RestrictParams params{
			RESTRICT_ON::kCast,
			RESTRICT_TYPE::kRestrict,
			RESTRICT_REASON::kGeneric,
			a_actor,
			a_caster->currentSpell
		};
		RestrictResult result;
		if (result = ShouldSkip(params); result.shouldSkip) {
			if (a_actor->IsPlayerRef()) {
				const auto notification = Settings::GetSingleton()->GetNotification(params);
				if (!notification.empty()) {
					RE::SendHUDMessage::ShowHUDMessage(notification.c_str());
				}
			}
			a_caster->InterruptCast(true);
			RE::PlaySound("MAGFail");
		}
	}

	RE::BSEventNotifyControl Manager::ProcessEvent(RE::TESEquipEvent const* a_evn, RE::BSTEventSource<RE::TESEquipEvent>*)
	{
		if (!a_evn || !a_evn->actor) {
			return RE::BSEventNotifyControl::kContinue;
		}

		const auto actor = a_evn->actor->As<RE::Actor>();
		if (!actor || !actor->IsPlayerRef()) {
			return RE::BSEventNotifyControl::kContinue;
		}

		const auto item = RE::TESForm::LookupByID<RE::TESBoundObject>(a_evn->baseObject);
		if (!item) {
			return RE::BSEventNotifyControl::kContinue;
		}

		if (a_evn->equipped) {
			RestrictResult result;
			RestrictParams params{
				RESTRICT_ON::kEquip,
				RESTRICT_TYPE::kDebuff,
				RESTRICT_REASON::kGeneric,
				actor,
				item
			};

			if (result = ShouldSkip(params); result.shouldSkip && result.debuffForm) {
				AddDebuff(item, result.debuffForm, true);
				const auto notification = Settings::GetSingleton()->GetNotification(params);
				if (!notification.empty()) {
					RE::SendHUDMessage::ShowHUDMessage(notification.c_str());
				}
			}

			if (RestrictData::is_bow_or_crossbow(item)) {
				if (const auto ammo = actor->GetCurrentAmmo()) {
					params.restrictType = RESTRICT_TYPE::kRestrict;
					params.object = ammo;
					if (result = ShouldSkip(params); result.shouldSkip) {
						SKSE::GetTaskInterface()->AddTask([actor, ammo]() {
							RE::ActorEquipManager::GetSingleton()->UnequipObject(actor, ammo);
							RE::SendUIMessage::SendInventoryUpdateMessage(actor, nullptr);
						});
					}
				}
			}
		} else {
			RemoveDebuff(item, true);
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

		switch (REX::STR::CONST_HASH(a_evn->tag)) {
		case "BeginCastLeft"_h:
			ProcessShouldSkipCast(actor, actor->magicCasters[0]);
			break;
		case "BeginCastRight"_h:
			ProcessShouldSkipCast(actor, actor->magicCasters[1]);
			break;
		case "BeginCastVoice"_h:
			ProcessShouldSkipCast(actor, actor->magicCasters[3]);
			break;
		default:
			break;
		}

		return RE::BSEventNotifyControl::kContinue;
	}

	RE::BSEventNotifyControl Manager::ProcessEvent(SKSE::ModCallbackEvent const* a_evn, RE::BSTEventSource<SKSE::ModCallbackEvent>*)
	{
		if (a_evn && a_evn->eventName == "KID_KeywordDistributionDone"sv) {
			LoadKeywords();
		}
		return RE::BSEventNotifyControl::kContinue;
	}
}
