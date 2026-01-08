#include "RestrictData.h"

RestrictFilter::Filter::Filter(const std::string& a_filter)
{
	std::string filter_copy = a_filter;
	if (filter_copy.starts_with('!')) {
		invertFilter = true;
		filter_copy.erase(0, 1);
	} else {
		invertFilter = false;
	}

	switch (string::const_hash(filter_copy)) {
	case "Male"_h:
		filter = FLAGS::kIsMale;
		break;
	case "Female"_h:
		filter = FLAGS::kIsFemale;
		break;
	case "Player"_h:
		filter = FLAGS::kIsPlayer;
		break;
	case "NPC"_h:
		filter = FLAGS::kIsNPC;
		break;
	case "Combat"_h:
		filter = FLAGS::kIsInCombat;
		break;
	default:
		{
			if (filter_copy.starts_with("Level(")) {
				static srell::regex pattern(R"(\(([^)]+)\))");
				if (srell::smatch matches; srell::regex_search(filter_copy, matches, pattern)) {
					Level level;
					if (string::is_only_digit(matches[1].str())) {
						level = string::to_num<std::uint16_t>(matches[1].str());
					} else {
						level = RE::TESForm::LookupByEditorID<RE::TESGlobal>(matches[1].str());
					}
					filter = level;
				}
				return;
			} else if (filter_copy.contains("(")) {
				static srell::regex pattern(R"(([^\(]*)\(([^)]+)\))");
				if (srell::smatch matches; srell::regex_search(filter_copy, matches, pattern)) {
					std::pair<RE::ActorValue, ActorValueLevel> avLevel;
					if (string::is_only_digit(matches[1].str())) {
						avLevel.first = string::to_num<RE::ActorValue>(matches[1].str());
					} else {
						avLevel.first = RE::ActorValueList::GetSingleton()->LookupActorValueByName(matches[1].str().c_str());
					}
					if (string::is_only_digit(matches[2].str())) {
						avLevel.second = string::to_num<float>(matches[2].str());
					} else {
						avLevel.second = RE::TESForm::LookupByEditorID<RE::TESGlobal>(matches[2].str());
					}
					filter = avLevel;
				}
			} else if (const auto filterForm = RE::TESForm::LookupByEditorID(filter_copy); filterForm && filterForm->IsNot(RE::FormType::Keyword)) {
				filter = filterForm;
			} else {
				filter = filter_copy;
			}
			break;
		}
	}
}

bool RestrictFilter::Filter::MatchFilter(const RestrictData& a_data, RestrictParams& a_params) const
{
	bool result = false;
	std::visit(overload{
				   [&](FLAGS a_flags) {
					   switch (a_flags) {
					   case Filter::FLAGS::kIsMale:
						   result = invertFilter ? a_data.sex == RE::SEX::kFemale : a_data.sex == RE::SEX::kMale;
						   break;
					   case Filter::FLAGS::kIsFemale:
						   result = invertFilter ? a_data.sex == RE::SEX::kMale : a_data.sex == RE::SEX::kFemale;
						   break;
					   case Filter::FLAGS::kIsPlayer:
						   result = invertFilter ? !a_data.actor->IsPlayerRef() : a_data.actor->IsPlayerRef();
						   break;
					   case Filter::FLAGS::kIsNPC:
						   result = invertFilter ? a_data.actor->IsPlayerRef() : !a_data.actor->IsPlayerRef();
						   break;
					   case Filter::FLAGS::kIsInCombat:
						   result = invertFilter ? !a_data.actor->IsInCombat() : a_data.actor->IsInCombat();
						   break;
					   default:
						   break;
					   }
				   },
				   [&](const Level& a_level) {
					   std::uint16_t level;
					   std::visit(overload{
									  [&](std::uint16_t b_level) {
										  level = b_level;
									  },
									  [&](RE::TESGlobal* global) {
										  level = static_cast<std::uint16_t>(global ? global->value : 0.0f);
									  },
								  },
						   a_level);
					   if (bool match = a_data.actorLevel >= level; invertFilter ? !match : match) {
						   result = true;
					   }
					   a_params.restrictReason = RESTRICT_REASON::kLevel;
				   },
				   [&](const std::pair<RE::ActorValue, ActorValueLevel>& a_level) {
					   float minLevel = 0;
					   std::visit(overload{
									  [&](float b_level) {
										  minLevel = b_level;
									  },
									  [&](RE::TESGlobal* global) {
										  minLevel = global ? global->value : 0.0f;
									  },
								  },
						   a_level.second);
					   float currentValue = a_data.actor->GetActorValue(a_level.first);
					   if (const bool match = currentValue >= minLevel; invertFilter ? !match : match) {
						   result = true;
					   }
					   a_params.restrictReason = RESTRICT_REASON::kSkill;
				   },
				   [&](RE::TESForm* a_form) {
					   switch (a_form->GetFormType()) {
					   case RE::FormType::Faction:
						   {
							   bool match = a_data.actor->IsInFaction(a_form->As<RE::TESFaction>());
							   result = invertFilter ? !match : match;
						   }
						   break;
					   case RE::FormType::Perk:
						   {
							   bool match = a_data.actor->HasPerk(a_form->As<RE::BGSPerk>());
							   result = invertFilter ? !match : match;
						   }
						   break;
					   case RE::FormType::Race:
						   {
							   bool match = a_data.actor->GetRace() == a_form;
							   result = invertFilter ? !match : match;
						   }
						   break;
					   case RE::FormType::Spell:
						   {
							   bool match = a_data.actor->HasSpell(a_form->As<RE::SpellItem>());
							   result = invertFilter ? !match : match;
						   }
						   break;
					   case RE::FormType::MagicEffect:
						   {
							   bool match = a_data.actor->HasMagicEffect(a_form->As<RE::EffectSetting>());
							   result = invertFilter ? !match : match;
						   }
						   break;
					   default:
						   break;
					   }
				   },
				   [&](const std::string& a_keyword) {
					   bool match = a_data.match_keyword(a_keyword);
					   result = invertFilter ? !match : match;
				   },
			   },
		filter);

	return result;
}

bool RestrictFilter::FilterGroup::MatchFilter(const RestrictData& a_data, RestrictParams& a_params) const
{
	return std::ranges::all_of(filters, [&](const auto& filter) {
		return filter.MatchFilter(a_data, a_params);
	});
}

RESTRICT_ON RestrictFilter::GetRestrictType(const std::string& a_keywordEDID)
{
	if (a_keywordEDID.starts_with("RestrictEquip:")) {
		return RESTRICT_ON::kEquip;
	}

	if (a_keywordEDID.starts_with("RestrictCast:")) {
		return RESTRICT_ON::kCast;
	}

	return RESTRICT_ON::kInvalid;
}

RestrictResult RestrictFilter::MatchFilter(const RestrictData& a_data, RestrictParams& a_params)
{
	RestrictResult result{};

	if (a_params.restrictOn != restrictOn) {
		return result;
	}

	if (a_params.restrictOn == RESTRICT_ON::kEquip && a_params.restrictType == RESTRICT_TYPE::kRestrict && debuffForm && a_data.actor->IsPlayerRef()) {
		return result;
	}

	result.debuffForm = debuffForm;

	if (!filtersALL.empty()) {
		result.shouldSkip = std::ranges::none_of(filtersALL, [&](const auto& filterGroup) {
			return filterGroup.MatchFilter(a_data, a_params);
		});
	}
	if (!result.shouldSkip && !filtersANY.empty()) {
		result.shouldSkip = std::ranges::none_of(filtersANY, [&](const auto& filter) {
			return filter.MatchFilter(a_data, a_params);
		});
	}

	return result;
}

RestrictFilter::FilterGroup::FilterGroup(const std::string& a_filter)
{
	const auto split_filters = string::split(a_filter, "+");
	filters.reserve(split_filters.size());
	for (auto& filter : split_filters) {
		filters.emplace_back(filter);
	}
}

RestrictFilter::RestrictFilter(const std::string& a_keywordEDID, RESTRICT_ON a_restrictOn) :
	restrictOn(a_restrictOn)
{
	const auto restrict_kywd = string::split(a_keywordEDID, ":");

	// RestrictEquip:<Filters>:<Debuff>

	if (restrict_kywd.size() > 1) {
		const auto split_filters = string::split(restrict_kywd[1], ",");
		for (const auto& filterStr : split_filters) {
			if (filterStr.contains('+')) {
				filtersALL.emplace_back(filterStr);
			} else {
				filtersANY.emplace_back(filterStr);
			}
		}
		if (restrict_kywd.size() > 2) {
			debuffForm = RE::TESForm::LookupByEditorID(restrict_kywd[2]);
		}
	}
}

RestrictData::RestrictData(const RestrictParams& a_baseParams) :
	actor(a_baseParams.actor),
	object(a_baseParams.object)
{
	auto npc = a_baseParams.actor->GetActorBase();
	if (npc) {
		sex = npc->GetSex();
		actorLevel = npc->GetLevel();
		lHand = actor->GetEquippedObject(true);
		rHand = actor->GetEquippedObject(false);
		isObjectAmmo = object->IsAmmo();
		equippedLHandBow = isObjectAmmo && lHand && is_bow_or_crossbow(lHand);
		equippedRHandBow = isObjectAmmo && rHand && is_bow_or_crossbow(rHand);
		valid = true;
	} else {
		valid = false;
	}
}

bool RestrictData::match_keyword(const std::string& a_filter) const
{
	if (actor->HasKeywordString(a_filter)) {
		return true;
	}
	if (isObjectAmmo) {
		if (equippedRHandBow && rHand->HasKeywordByEditorID(a_filter) || equippedLHandBow && lHand->HasKeywordByEditorID(a_filter)) {
			return true;
		}
	}
	for (auto& [item, data] : actor->GetInventory()) {
		auto& [count, entry] = data;
		if (entry->IsWorn() && count > 0 && item->HasKeywordByEditorID(a_filter)) {
			return true;
		}
	}
	return false;
}

bool RestrictData::is_bow_or_crossbow(RE::TESForm* a_object)
{
	if (const auto weap = a_object->As<RE::TESObjectWEAP>(); weap && (weap->IsBow() || weap->IsCrossbow())) {
		return true;
	}
	return false;
}
