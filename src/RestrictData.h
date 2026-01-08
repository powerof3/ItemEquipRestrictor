#pragma once

enum class RESTRICT_ON
{
	kInvalid,
	kEquip,
	kCast
};

enum class RESTRICT_TYPE
{
	kRestrict,
	kDebuff
};

enum class RESTRICT_REASON
{
	kGeneric,
	kSkill,
	kLevel
};

struct RestrictParams
{
	RESTRICT_ON     restrictOn;
	RESTRICT_TYPE   restrictType;
	RESTRICT_REASON restrictReason;
	RE::Actor*      actor;
	RE::TESForm*    object;
};

struct RestrictResult
{
	bool         shouldSkip{ false };
	RE::TESForm* debuffForm{ nullptr };
};

struct RestrictData
{
	RestrictData() = default;
	RestrictData(const RestrictParams& a_baseParams);

	bool        match_keyword(const std::string& a_filter, RE::TESForm* a_object) const;
	static bool is_bow_or_crossbow(RE::TESForm* a_object);

	RE::Actor*    actor;
	RE::TESForm*  object;
	RE::SEX       sex;
	std::uint16_t actorLevel;
	bool          valid;
};

struct RestrictFilter
{
	// RestrictEquip:Female -> men cannot wear this
	// RestrictEquip:!Female -> only men can wear this

	// RestrictEquip:ActorTypeVampire -> only vampires can wear this
	// RestrictEquip:!ActorTypeVampire -> vampires cannot wear this

	using ShortOrGlobal = std::variant<std::uint16_t, RE::TESGlobal*>;
	using FactionOrAV = std::variant<RE::ActorValue, RE::TESFaction*>;
	using FloatOrGlobal = std::variant<float, RE::TESGlobal*>;

	struct Filter
	{
		enum class FLAGS
		{
			kNone = 0,
			kIsMale = 1 << 0,
			kIsFemale = 1 << 1,
			kIsPlayer = 1 << 2,
			kIsNPC = 1 << 3,
			kIsInCombat = 1 << 4
		};

		Filter() = default;
		Filter(const std::string& filter);

		bool MatchFilter(const RestrictData& a_data, RestrictParams& a_baseParams) const;

		// members
		std::variant<
			FLAGS,                                  // flags
			ShortOrGlobal,                          // level
			std::pair<FactionOrAV, FloatOrGlobal>,  // factionRank/actorValue
			RE::TESForm*,                           // form
			std::string>                            // keyword
			 filter{};
		bool invertFilter{ false };
	};

	struct FilterGroup
	{
		FilterGroup() = default;
		FilterGroup(const std::string& a_filter);

		bool MatchFilter(const RestrictData& a_data, RestrictParams& a_baseParams) const;

		// members
		std::vector<Filter> filters;
	};

	RestrictFilter() = default;
	RestrictFilter(const std::string& a_keywordEDID, RESTRICT_ON a_restrictOn);

	static RESTRICT_ON GetRestrictType(const std::string& a_keywordEDID);
	RestrictResult     MatchFilter(const RestrictData& a_data, RestrictParams& a_baseParams);

	// members
	RESTRICT_ON              restrictOn{ RESTRICT_ON::kInvalid };
	std::vector<FilterGroup> filtersALL;
	std::vector<Filter>      filtersANY;
	RE::TESForm*             debuffForm{};
};
