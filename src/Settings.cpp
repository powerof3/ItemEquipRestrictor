#include "Settings.h"

bool Settings::LoadSettings()
{
	constexpr auto path = L"Data/SKSE/Plugins/po3_ItemEquipRestrictor.ini";

	CSimpleIniA ini;
	ini.SetUnicode();

	ini.LoadFile(path);

	ini::get_value(ini, notificationRestrict.show, "Restrict", "bShowNotification", nullptr);
	ini::get_value(ini, notificationRestrict.generic, "Restrict", "sNotificationGeneric", ";You cannot equip {item} -> You cannot equip Ebony Armor");
	ini::get_value(ini, notificationSpellRestrict.generic, "Restrict", "sNotificationSpellGeneric", nullptr);
	ini::get_value(ini, notificationRestrict.skill, "Restrict", "sNotificationSkill", nullptr);
	ini::get_value(ini, notificationSpellRestrict.skill, "Restrict", "sNotificationSpellSkill", nullptr);
	ini::get_value(ini, notificationRestrict.level, "Restrict", "sNotificationLevel", nullptr);
	ini::get_value(ini, notificationSpellRestrict.level, "Restrict", "sNotificationSpellLevel", nullptr);

	ini::get_value(ini, notificationRestrict.show, "Debuff", "bShowNotification", "");
	ini::get_value(ini, notificationDebuff.generic, "Debuff", "sNotificationGeneric", nullptr);
	ini::get_value(ini, notificationSpellDebuff.generic, "Debuff", "sNotificationSpellGeneric", nullptr);
	ini::get_value(ini, notificationDebuff.skill, "Debuff", "sNotificationSkill", nullptr);
	ini::get_value(ini, notificationSpellDebuff.skill, "Debuff", "sNotificationSpellSkill", nullptr);
	ini::get_value(ini, notificationDebuff.level, "Debuff", "sNotificationLevel", nullptr);
	ini::get_value(ini, notificationSpellDebuff.level, "Debuff", "sNotificationSpellLevel", nullptr);

	(void)ini.SaveFile(path);

	return true;
}

std::string Settings::GetNotification(const RestrictParams& a_params) const
{
	std::string finalNotification;

	Notification notification;
	if (a_params.restrictOn == RESTRICT_ON::kEquip) {
		notification = a_params.restrictType == RESTRICT_TYPE::kDebuff ? notificationDebuff : notificationRestrict;
	} else {
		notification = a_params.restrictType == RESTRICT_TYPE::kDebuff ? notificationSpellDebuff : notificationSpellRestrict;
		notification.show = a_params.restrictType == RESTRICT_TYPE::kDebuff ? notificationDebuff.show : notificationRestrict.show;
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
