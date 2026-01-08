#include "Settings.h"

bool Settings::LoadSettings()
{
	constexpr auto path = L"Data/SKSE/Plugins/po3_ItemEquipRestrictor.ini";

	CSimpleIniA ini;
	ini.SetUnicode();

	ini.LoadFile(path);

	ini::get_value(ini, restrictEquip.show, "RestrictEquip", "bShowNotification", nullptr);
	ini::get_value(ini, restrictEquip.generic, "RestrictEquip", "sNotificationGeneric", ";You cannot equip {item} -> You cannot equip Ebony Armor");
	ini::get_value(ini, restrictEquip.skill, "RestrictEquip", "sNotificationSkill", nullptr);
	ini::get_value(ini, restrictEquip.level, "RestrictEquip", "sNotificationLevel", nullptr);

	ini::get_value(ini, restrictEquipDebuff.show, "RestrictEquipDebuff", "bShowNotification", nullptr);
	ini::get_value(ini, restrictEquipDebuff.generic, "RestrictEquipDebuff", "sNotificationGeneric", nullptr);
	ini::get_value(ini, restrictEquipDebuff.skill, "RestrictEquipDebuff", "sNotificationSkill", nullptr);
	ini::get_value(ini, restrictEquipDebuff.level, "RestrictEquipDebuff", "sNotificationLevel", nullptr);

	ini::get_value(ini, restrictEquipSpell.show, "RestrictEquipSpell", "bShowNotification", nullptr);
	ini::get_value(ini, restrictEquipSpell.generic, "RestrictEquipSpell", "sNotificationGeneric", nullptr);
	ini::get_value(ini, restrictEquipSpell.skill, "RestrictEquipSpell", "sNotificationSkill", nullptr);
	ini::get_value(ini, restrictEquipSpell.level, "RestrictEquipSpell", "sNotificationLevel", nullptr);

	ini::get_value(ini, restrictEquipSpellDebuff.show, "RestrictEquipSpellDebuff", "bShowNotification", nullptr);
	ini::get_value(ini, restrictEquipSpellDebuff.generic, "RestrictEquipSpellDebuff", "sNotificationGeneric", nullptr);
	ini::get_value(ini, restrictEquipSpellDebuff.skill, "RestrictEquipSpellDebuff", "sNotificationSkill", nullptr);
	ini::get_value(ini, restrictEquipSpellDebuff.level, "RestrictEquipSpellDebuff", "sNotificationLevel", nullptr);

	ini::get_value(ini, restrictCast.show, "RestrictCast", "bShowNotification", nullptr);
	ini::get_value(ini, restrictCast.generic, "RestrictCast", "sNotificationGeneric", nullptr);
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

	string::replace_all(finalNotification, "{item}", a_params.object->GetName());
	return finalNotification;
}
