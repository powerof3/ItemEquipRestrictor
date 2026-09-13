#pragma once

#include "RestrictData.h"

class Settings : public REX::TSingleton<Settings>
{
public:
	bool        LoadSettings();
	std::string GetNotification(const RestrictParams& a_params) const;

private:
	struct Notification
	{
		Notification(std::string_view a_section, std::string a_generic, std::string a_skill, std::string a_level) :
			show(a_section, "bShowNotification", true),
			generic(a_section, "sNotificationGeneric", std::move(a_generic)),
			skill(a_section, "sNotificationSkill", std::move(a_skill)),
			level(a_section, "sNotificationLevel", std::move(a_level))
		{}

		Notification(const Notification&) = delete;
		Notification(Notification&&) = delete;
		Notification& operator=(const Notification&) = delete;
		Notification& operator=(Notification&&) = delete;

		[[nodiscard]] const std::string& GetGeneric() const { return stl::get_setting_ref(generic); }
		[[nodiscard]] const std::string& GetSkill() const { return stl::get_setting_ref(skill); }
		[[nodiscard]] const std::string& GetLevel() const { return stl::get_setting_ref(level); }

		// members
		REX::TIniSetting<bool>        show;
		REX::TIniSetting<std::string> generic;
		REX::TIniSetting<std::string> skill;
		REX::TIniSetting<std::string> level;
	};

	// members
	static constexpr auto path = R"(Data\SKSE\Plugins\po3_ItemEquipRestrictor.ini)"sv;

	Notification restrictEquip{ "RestrictEquipItem"sv,
		"You cannot equip this item",
		"You lack the necessary skill to equip this item",
		"You lack the necessary level to equip this item" };
	Notification restrictEquipDebuff{ "RestrictEquipItemDebuff"sv,
		"You cannot properly equip this item",
		"You lack the necessary skill to properly equip this item",
		"You lack the necessary level to properly equip this item" };

	Notification restrictEquipSpell{ "RestrictEquipSpell"sv,
		"You cannot equip this spell",
		"You lack the necessary skill to equip this spell",
		"You lack the necessary level to equip this spell" };
	Notification restrictEquipSpellDebuff{ "RestrictEquipSpellDebuff"sv,
		"You cannot properly equip this spell",
		"You lack the necessary skill to properly equip this spell",
		"You lack the necessary level to properly equip this spell" };

	Notification restrictEquipShout{ "RestrictEquipShout"sv,
		"You cannot equip this shout",
		"You lack the necessary skill to equip this shout",
		"You lack the necessary level to equip this shout" };
	Notification restrictEquipShoutDebuff{ "RestrictEquipShoutDebuff"sv,
		"You cannot properly equip this shout",
		"You lack the necessary skill to properly equip this shout",
		"You lack the necessary level to properly equip this shout" };

	Notification restrictCast{ "RestrictCast"sv,
		"You cannot cast this spell",
		"You lack the necessary skill to cast this spell",
		"You lack the necessary level to cast this spell" };
	Notification restrictCastDebuff{ "RestrictCastDebuff"sv,
		"You cannot properly cast this spell",
		"You lack the necessary skill to properly cast this spell",
		"You lack the necessary level to properly cast this spell" };

	Notification restrictPickUp{ "RestrictPickUp"sv,
		"You cannot pick up this item",
		"You lack the necessary skill to pick up this item",
		"You lack the necessary level to pick up this item" };
	Notification restrictPickUpDebuff{ "RestrictPickUpDebuff"sv,
		"You cannot properly pick up this item",
		"You lack the necessary skill to properly pick up this item",
		"You lack the necessary level to properly pick up this item" };
};
