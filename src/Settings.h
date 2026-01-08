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

	Notification notificationRestrict { true, "You cannot equip this item", "You lack the necessary skill to equip this item", "You lack the necessary level to equip this item" };
	Notification notificationSpellRestrict{ true, "You cannot cast this spell", "You lack the necessary skill to cast this spell", "You lack the necessary level to cast this spell" };

    Notification notificationDebuff{ true, "You cannot equip this item", "You lack the necessary skill to properly equip this item", "You lack the necessary level to properly equip this item" };
	Notification notificationSpellDebuff{ true, "You cannot cast this spell", "You lack the necessary skill to properly cast this spell", "You lack the necessary level to properly cast this spell" };
};
