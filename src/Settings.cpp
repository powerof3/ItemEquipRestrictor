#include "Settings.h"

bool Settings::LoadSettings()
{
	const auto store = REX::FIniSettingStore::GetSingleton();
	store->Init(path.data(), "");

	store->Load();
	store->Save();

	return true;
}

std::string Settings::GetNotification(const RestrictParams& a_params) const
{
	std::string finalNotification;

	const bool          isDebuff = a_params.restrictType == RESTRICT_TYPE::kDebuff;
	const Notification* notification;

	if (a_params.restrictOn == RESTRICT_ON::kEquip) {
		switch (a_params.object->GetFormType()) {
		case RE::FormType::Spell:
			notification = isDebuff ? &restrictEquipSpellDebuff : &restrictEquipSpell;
			break;
		case RE::FormType::Shout:
			notification = isDebuff ? &restrictEquipShoutDebuff : &restrictEquipShout;
			break;
		default:
			notification = isDebuff ? &restrictEquipDebuff : &restrictEquip;
			break;
		}
	} else if (a_params.restrictOn == RESTRICT_ON::kCast) {
		notification = isDebuff ? &restrictCastDebuff : &restrictCast;
	} else {  // kPickUp
		notification = isDebuff ? &restrictPickUpDebuff : &restrictPickUp;
	}

	if (!notification->show) {
		return finalNotification;
	}

	switch (a_params.restrictReason) {
	case RESTRICT_REASON::kGeneric:
		finalNotification = notification->generic;
		break;
	case RESTRICT_REASON::kSkill:
		finalNotification = notification->skill;
		break;
	case RESTRICT_REASON::kLevel:
		finalNotification = notification->level;
		break;
	}

	std::string_view placeholder;
	if (a_params.restrictOn == RESTRICT_ON::kCast) {
		placeholder = "{magicItem}"sv;
	} else {
		switch (a_params.object->GetFormType()) {
		case RE::FormType::Spell:
			placeholder = "{spell}"sv;
			break;
		case RE::FormType::Shout:
			placeholder = "{shout}"sv;
			break;
		default:
			placeholder = "{item}"sv;
			break;
		}
	}

	REX::STR::REPLACE_ALL(finalNotification, placeholder, a_params.object->GetName());
	return finalNotification;
}
