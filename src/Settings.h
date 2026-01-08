#pragma once

#include "RestrictData.h"

class Settings : public REX::Singleton<Settings>
{
public:
    bool        LoadSettings();
	std::string GetNotification(const RestrictParams& a_params) const;

private:
	// members
	struct Notification
	{
		bool        show;
	    std::string generic;
		std::string skill;
		std::string level;
	};

	Notification restrictEquip { true, "You cannot equip this item", "You lack the necessary skill to equip this item", "You lack the necessary level to equip this item" };
	Notification restrictEquipDebuff{ true, "You cannot properly equip this item", "You lack the necessary skill to properly equip this item", "You lack the necessary level to properly equip this item" };

	Notification restrictEquipSpell{ true, "You cannot equip this spell", "You lack the necessary skill to equip this spell", "You lack the necessary level to equip this spell" };
	Notification restrictEquipSpellDebuff{ true, "You cannot properly equip this spell", "You lack the necessary skill to properly equip this spell", "You lack the necessary level to properly equip this spell" };

	Notification restrictEquipShout{ true, "You cannot equip this shout", "You lack the necessary skill to equip this shout", "You lack the necessary level to equip this shout" };
	Notification restrictEquipShoutDebuff{ true, "You cannot properly equip this shout", "You lack the necessary skill to properly equip this shout", "You lack the necessary level to properly equip this shout" };

	Notification restrictCast{ true, "You cannot cast this spell", "You lack the necessary skill to cast this spell", "You lack the necessary level to cast this spell" };
	Notification restrictCastDebuff{ true, "You cannot properly cast this spell", "You lack the necessary skill to properly cast this spell", "You lack the necessary level to properly cast this spell" };

	Notification restrictPickUp{ true, "You cannot pick up this item", "You lack the necessary skill to pick up this item", "You lack the necessary level to pick up this item" };
	Notification restrictPickUpDebuff{ true, "You cannot properly pick up this item", "You lack the necessary skill to properly pick up this item", "You lack the necessary level to properly pick up this item" };
};
