#include "Settings.h"

bool Settings::LoadSettings()
{
	constexpr auto path = L"Data/SKSE/Plugins/po3_ItemEquipRestrictor.ini";

	CSimpleIniA ini;
	ini.SetUnicode();

	ini.LoadFile(path);

	ini::get_value(ini, restrictEquip.show, "RestrictEquipItem", "bShowNotification", nullptr);
	ini::get_value(ini, restrictEquip.generic, "RestrictEquipItem", "sNotificationGeneric", ";You cannot equip {item} -> You cannot equip Ebony Armor");
	ini::get_value(ini, restrictEquip.skill, "RestrictEquipItem", "sNotificationSkill", nullptr);
	ini::get_value(ini, restrictEquip.level, "RestrictEquipItem", "sNotificationLevel", nullptr);

	ini::get_value(ini, restrictEquipDebuff.show, "RestrictEquipItemDebuff", "bShowNotification", nullptr);
	ini::get_value(ini, restrictEquipDebuff.generic, "RestrictEquipItemDebuff", "sNotificationGeneric", nullptr);
	ini::get_value(ini, restrictEquipDebuff.skill, "RestrictEquipItemDebuff", "sNotificationSkill", nullptr);
	ini::get_value(ini, restrictEquipDebuff.level, "RestrictEquipItemDebuff", "sNotificationLevel", nullptr);

	ini::get_value(ini, restrictEquipSpell.show, "RestrictEquipSpell", "bShowNotification", nullptr);
	ini::get_value(ini, restrictEquipSpell.generic, "RestrictEquipSpell", "sNotificationGeneric", ";You cannot equip {spell} -> You cannot equip Flames");
	ini::get_value(ini, restrictEquipSpell.skill, "RestrictEquipSpell", "sNotificationSkill", nullptr);
	ini::get_value(ini, restrictEquipSpell.level, "RestrictEquipSpell", "sNotificationLevel", nullptr);

	ini::get_value(ini, restrictEquipSpellDebuff.show, "RestrictEquipSpellDebuff", "bShowNotification", nullptr);
	ini::get_value(ini, restrictEquipSpellDebuff.generic, "RestrictEquipSpellDebuff", "sNotificationGeneric", nullptr);
	ini::get_value(ini, restrictEquipSpellDebuff.skill, "RestrictEquipSpellDebuff", "sNotificationSkill", nullptr);
	ini::get_value(ini, restrictEquipSpellDebuff.level, "RestrictEquipSpellDebuff", "sNotificationLevel", nullptr);

	ini::get_value(ini, restrictEquipShout.show, "RestrictEquipShout", "bShowNotification", nullptr);
	ini::get_value(ini, restrictEquipShout.generic, "RestrictEquipShout", "sNotificationGeneric",";You cannot equip {shout} -> You cannot equip Unrelenting Force");
	ini::get_value(ini, restrictEquipShout.skill, "RestrictEquipShout", "sNotificationSkill", nullptr);
	ini::get_value(ini, restrictEquipShout.level, "RestrictEquipShout", "sNotificationLevel", nullptr);

	ini::get_value(ini, restrictEquipShoutDebuff.show, "RestrictEquipShoutDebuff", "bShowNotification", nullptr);
	ini::get_value(ini, restrictEquipShoutDebuff.generic, "RestrictEquipShoutDebuff", "sNotificationGeneric", nullptr);
	ini::get_value(ini, restrictEquipShoutDebuff.skill, "RestrictEquipShoutDebuff", "sNotificationSkill", nullptr);
	ini::get_value(ini, restrictEquipShoutDebuff.level, "RestrictEquipShoutDebuff", "sNotificationLevel", nullptr);

	ini::get_value(ini, restrictCast.show, "RestrictCast", "bShowNotification", nullptr);
	ini::get_value(ini, restrictCast.generic, "RestrictCast", "sNotificationGeneric", ";You cannot cast {magicItem} -> You cannot cast Flames");
	ini::get_value(ini, restrictCast.skill, "RestrictCast", "sNotificationSkill", nullptr);
	ini::get_value(ini, restrictCast.level, "RestrictCast", "sNotificationLevel", nullptr);

	ini::get_value(ini, restrictCastDebuff.show, "RestrictCastDebuff", "bShowNotification", nullptr);
	ini::get_value(ini, restrictCastDebuff.generic, "RestrictCastDebuff", "sNotificationGeneric", nullptr);
	ini::get_value(ini, restrictCastDebuff.skill, "RestrictCastDebuff", "sNotificationSkill", nullptr);
	ini::get_value(ini, restrictCastDebuff.level, "RestrictCastDebuff", "sNotificationLevel", nullptr);

	(void)ini.SaveFile(path);

	return true;
}

std::string Settings::GetNotification(const RestrictParams& a_params) const
{
	std::string finalNotification;

	Notification notification;
	if (a_params.restrictOn == RESTRICT_ON::kEquip) {
		if (a_params.object->Is(RE::FormType::Spell)) {
			notification = a_params.restrictType == RESTRICT_TYPE::kDebuff ? restrictEquipSpellDebuff : restrictEquipSpell;
		} else if (a_params.object->Is(RE::FormType::Shout)) {
			notification = a_params.restrictType == RESTRICT_TYPE::kDebuff ? restrictEquipShoutDebuff : restrictEquipShout;
		} else {
			notification = a_params.restrictType == RESTRICT_TYPE::kDebuff ? restrictEquipDebuff : restrictEquip;
		}
	} else {
		notification = a_params.restrictType == RESTRICT_TYPE::kDebuff ? restrictCastDebuff : restrictCast;
	}

	if (!notification.show) {
		return finalNotification;
	}

	switch (a_params.restrictReason) {
	case RESTRICT_REASON::kGeneric:
		finalNotification = notification.generic;
		break;
	case RESTRICT_REASON::kSkill:
		finalNotification = notification.skill;
		break;
	case RESTRICT_REASON::kLevel:
		finalNotification = notification.level;
		break;
	}

	if (a_params.restrictOn == RESTRICT_ON::kCast) {
		string::replace_all(finalNotification, "{magicItem}", a_params.object->GetName());
	} else {
		if (a_params.object->Is(RE::FormType::Spell)) {
			string::replace_all(finalNotification, "{spell}", a_params.object->GetName());
		} else if (a_params.object->Is(RE::FormType::Shout)) {
			string::replace_all(finalNotification, "{shout}", a_params.object->GetName());
		} else {
			string::replace_all(finalNotification, "{item}", a_params.object->GetName());
		}
	}

	return finalNotification;
}
