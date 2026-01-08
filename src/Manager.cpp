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

	RestrictResult Manager::ShouldSkip(const std::string& a_keywordEDID, const RestrictData& a_data, RestrictParams& a_params)
	{
		RestrictResult result;

		if (_rejectedKeywords.contains(a_keywordEDID)) {
			return result;
		}

		auto it = _restrictKeywords.find(a_keywordEDID);
		if (it == _restrictKeywords.end()) {
			auto restrictType = RestrictFilter::GetRestrictType(a_keywordEDID);
			if (restrictType == RESTRICT_ON::kInvalid) {
				_rejectedKeywords.emplace(a_keywordEDID);
				return result;
			}
			it = _restrictKeywords.try_emplace(a_keywordEDID, a_keywordEDID, restrictType).first;
		}

		return it->second.MatchFilter(a_data, a_params);
	}

	RestrictResult Manager::ShouldSkip(RestrictParams& a_params)
	{
		if (a_params.object->Is(RE::FormType::Shout)) {
			RestrictResult result;
			
			const auto shout = a_params.object->As<RE::TESShout>();
			for (std::uint32_t i = 0; i < 3; ++i) {
				const auto& shoutWord = shout->variations[i];
				if (shoutWord.spell) {
					result = ShouldSkip(shoutWord.spell->As<RE::BGSKeywordForm>(), a_params);
					if (result.shouldSkip) {
						break;
					}
				}
			}

			return result;
		} else {
			return ShouldSkip(a_params.object->As<RE::BGSKeywordForm>(), a_params);	
		}
	}

	RestrictResult Manager::ShouldSkip(RE::BGSKeywordForm* a_keywordForm, RestrictParams& a_params)
	{
		RestrictResult result;

		if (!a_keywordForm || a_keywordForm->GetNumKeywords() == 0) {
			return result;
		}
		
		RestrictData restrictData(a_params);
		if (!restrictData.valid) {
			return result;
		}

		a_keywordForm->ForEachKeyword([&](const RE::BGSKeyword* a_keyword) {
			if (const auto edid = a_keyword->GetFormEditorID(); !string::is_empty(edid)) {
				if (result = ShouldSkip(edid, restrictData, a_params); result.shouldSkip) {
					return RE::BSContainer::ForEachResult::kStop;
				}
			}
			return RE::BSContainer::ForEachResult::kContinue;
		});

		return result;
	}

	void Manager::AddDebuff(const RE::TESBoundObject* a_item, RE::TESForm* a_debuffForm)
	{
		if (a_debuffForm->Is(RE::FormType::Perk)) {
			RE::PlayerCharacter::GetSingleton()->AddPerk(a_debuffForm->As<RE::BGSPerk>());
		} else {
			RE::PlayerCharacter::GetSingleton()->AddSpell(a_debuffForm->As<RE::SpellItem>());
		}

		_objectDebuffs[a_item->GetFormID()].insert(a_debuffForm->GetFormID());
		_debuffObjects[a_debuffForm->GetFormID()].insert(a_item->GetFormID());
	}

	void Manager::RemoveDebuff(const RE::TESBoundObject* a_item)
	{
		const auto itemID = a_item->GetFormID();

		if (const auto oIt = _objectDebuffs.find(itemID); oIt != _objectDebuffs.end()) {
			for (const auto& debuffID : oIt->second) {
				if (const auto dIt = _debuffObjects.find(debuffID); dIt != _debuffObjects.end()) {
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
			_objectDebuffs.erase(oIt);
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
		if (!actor || !a_evn->actor->IsPlayerRef()) {
			return RE::BSEventNotifyControl::kContinue;
		}

		const auto item = RE::TESForm::LookupByID<RE::TESBoundObject>(a_evn->baseObject);
		if (!item) {
			return RE::BSEventNotifyControl::kContinue;
		}

		if (a_evn->equipped) {
			RestrictParams params{
				RESTRICT_ON::kEquip,
				RESTRICT_TYPE::kDebuff,
				RESTRICT_REASON::kGeneric,
				actor,
				item
			};
			RestrictResult result;
			if (result = ShouldSkip(params); result.shouldSkip && result.debuffForm) {
				AddDebuff(item, result.debuffForm);
				const auto notification = Settings::GetSingleton()->GetNotification(params);
				if (!notification.empty()) {
					RE::SendHUDMessage::ShowHUDMessage(notification.c_str());
				}
			} else if (RestrictData::is_bow_or_crossbow(item)) {
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
			RemoveDebuff(item);
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
			ProcessShouldSkipCast(actor, actor->magicCasters[3]);
			break;
		default:
			break;
		}

		return RE::BSEventNotifyControl::kContinue;
	}
}
