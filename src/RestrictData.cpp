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
					ShortOrGlobal level;
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
					std::pair<FactionOrAV, FloatOrGlobal> factionOrAV;
					const auto&                           first = matches[1].str();
					const auto&                           second = matches[2].str();
					if (string::is_only_digit(first)) {
						factionOrAV.first = string::to_num<RE::ActorValue>(first);
					} else if (const auto faction = RE::TESForm::LookupByEditorID<RE::TESFaction>(first)) {
						factionOrAV.first = faction;
					} else {
						factionOrAV.first = RE::ActorValueList::GetSingleton()->LookupActorValueByName(first.c_str());
					}
					if (string::is_only_digit(second)) {
						factionOrAV.second = string::to_num<float>(second);
					} else {
						factionOrAV.second = RE::TESForm::LookupByEditorID<RE::TESGlobal>(second);
					}
					filter = factionOrAV;
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
	bool match = false;

	std::visit(overload{
				   [&](FLAGS a_flags) {
					   switch (a_flags) {
					   case FLAGS::kIsMale:
						   match = (a_data.sex == RE::SEX::kMale);
						   break;
					   case FLAGS::kIsFemale:
						   match = (a_data.sex == RE::SEX::kFemale);
						   break;
					   case FLAGS::kIsPlayer:
						   match = a_data.isPlayer;
						   break;
					   case FLAGS::kIsNPC:
						   match = !a_data.isPlayer;
						   break;
					   case FLAGS::kIsInCombat:
						   match = a_data.isInCombat;
						   break;
					   }
				   },
				   [&](const ShortOrGlobal& a_level) {
					   std::uint16_t level = 0;
					   std::visit(overload{
									  [&](std::uint16_t b_level) {
										  level = b_level;
									  },
									  [&](RE::TESGlobal* global) {
										  level = static_cast<std::uint16_t>(global ? global->value : 0.0f);
									  },
								  },
						   a_level);
					   match = a_data.actorLevel >= level;
					   a_params.restrictReason = RESTRICT_REASON::kLevel;
				   },
				   [&](const std::pair<FactionOrAV, FloatOrGlobal>& a_pair) {
					   float level = 0.0f;
					   std::visit(overload{
									  [&](float val) {
										  level = val;
									  },
									  [&](RE::TESGlobal* global) {
										  level = global ? global->value : 0.0f;
									  },
								  },
						   a_pair.second);

					   std::visit(overload{
									  [&](RE::ActorValue av) {
										  match = a_data.actor->GetActorValue(av) >= level;
										  a_params.restrictReason = RESTRICT_REASON::kSkill;
									  },
									  [&](RE::TESFaction* faction) {
										  float factionRank = faction ? static_cast<float>(a_data.actor->GetFactionRank(faction, a_data.isPlayer)) : -1.0f;
										  match = factionRank >= level;
									  },
								  },
						   a_pair.first);
				   },
				   [&](RE::TESForm* a_form) {
					   switch (a_form->GetFormType()) {
					   case RE::FormType::Faction:
						   match = a_data.actor->IsInFaction(a_form->As<RE::TESFaction>());
						   break;
					   case RE::FormType::Perk:
						   match = a_data.actor->HasPerk(a_form->As<RE::BGSPerk>());
						   break;
					   case RE::FormType::Race:
						   match = a_data.actor->GetRace() == a_form;
						   break;
					   case RE::FormType::Spell:
						   match = a_data.actor->HasSpell(a_form->As<RE::SpellItem>());
						   break;
					   case RE::FormType::MagicEffect:
						   match = a_data.actor->HasMagicEffect(a_form->As<RE::EffectSetting>());
						   break;
					   case RE::FormType::Weapon:
					   case RE::FormType::Armor:
						   match = a_data.has_worn_object(a_form->As<RE::TESBoundObject>());
						   break;
					   default:
						   break;
					   }
				   },
				   [&](const std::string& a_keyword) {
					   match = a_data.match_keyword(a_keyword);
				   },
			   },
		filter);

	return invertFilter ? !match : match;
}

RESTRICT_ON RestrictFilter::GetRestrictType(const std::string& a_keywordEDID)
{
	if (a_keywordEDID.starts_with("RestrictEquip:")) {
		return RESTRICT_ON::kEquip;
	}

	if (a_keywordEDID.starts_with("RestrictCast:")) {
		return RESTRICT_ON::kCast;
	}

	if (a_keywordEDID.starts_with("RestrictPickUp:")) {
		return RESTRICT_ON::kPickUp;
	}

	return RESTRICT_ON::kInvalid;
}

bool RestrictFilter::FilterGroup::MatchFilter(const RestrictData& a_data, RestrictParams& a_params) const
{
	return std::ranges::all_of(filters, [&](const auto& filter) {
		return filter.MatchFilter(a_data, a_params);
	});
}

RestrictResult RestrictFilter::MatchFilter(const RestrictData& a_data, RestrictParams& a_params)
{
	RestrictResult result{};

	if (a_params.restrictOn != restrictOn || (a_params.restrictType == RESTRICT_TYPE::kRestrict && debuffForm && a_data.isPlayer)) {
		return result;
	}

	if (filtersALL.empty() && filtersANY.empty()) {
		return result;
	}

	bool matchesGroups = std::ranges::any_of(filtersALL, [&](const auto& filterGroup) {
		return filterGroup.MatchFilter(a_data, a_params);
	});

	bool matchesAny = std::ranges::any_of(filtersANY, [&](const auto& filter) {
		return filter.MatchFilter(a_data, a_params);
	});

	result.shouldSkip = !(matchesGroups || matchesAny);
	result.debuffForm = debuffForm;

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
	object(a_baseParams.object),
	wornObjectsInitialized(false),
	actorKeywordsInitialized(false),
	wornObjectKeywordsInitialized(false)
{
	auto npc = a_baseParams.actor->GetActorBase();
	if (npc) {
		sex = npc->GetSex();
		actorLevel = npc->GetLevel();
		isPlayer = actor->IsPlayerRef();
		isInCombat = actor->IsInCombat();
	} else {
		actor = nullptr;
		object = nullptr;
	}
}

bool RestrictData::has_worn_object(RE::TESForm* a_form) const
{
	cache_worn_objects();
	return wornObjects.contains(a_form->As<RE::TESBoundObject>());
}

bool RestrictData::match_keyword(const std::string& a_filter) const
{
	cache_actor_keywords();
	if (actorKeywords.contains(a_filter)) {
		return true;
	}

	cache_worn_object_keywords();
	return wornObjectKeywords.contains(a_filter);
}

void RestrictData::collectKeywords(StringSet& a_set, RE::BGSKeywordForm* a_form) const
{
	if (a_form) {
		a_form->ForEachKeyword([&](RE::BGSKeyword* a_kw) {
			if (auto edid = a_kw->GetFormEditorID(); edid && *edid)
				a_set.emplace(edid);
			return RE::BSContainer::ForEachResult::kContinue;
		});
	}
}

void RestrictData::cache_actor_keywords() const
{
	if (actorKeywordsInitialized) {
		return;
	}

	collectKeywords(actorKeywords, actor->GetActorBase());
	collectKeywords(actorKeywords, actor->GetRace());

	actorKeywordsInitialized = true;
}

void RestrictData::cache_worn_objects() const
{
	if (wornObjectsInitialized) {
		return;
	}

	for (auto& [item, data] : actor->GetInventory()) {
		auto& [count, entry] = data;
		if (item && item != object && entry->IsWorn() && count > 0) {
			wornObjects.emplace(item);
		}
	}

	wornObjectsInitialized = true;
}

void RestrictData::cache_worn_object_keywords() const
{
	if (wornObjectKeywordsInitialized) {
		return;
	}

	cache_worn_objects();
	for (auto& item : wornObjects) {
		collectKeywords(wornObjectKeywords, item->As<RE::BGSKeywordForm>());
	}
	wornObjectKeywordsInitialized = true;
}

bool RestrictData::is_bow_or_crossbow(RE::TESForm* a_object)
{
	if (const auto weap = a_object->As<RE::TESObjectWEAP>(); weap && (weap->IsBow() || weap->IsCrossbow())) {
		return true;
	}
	return false;
}
