#include "Hooks.h"

#include "Manager.h"
#include "Settings.h"

namespace Hooks
{
	struct DoEquip
	{
		static void thunk(RE::ActorEquipManager* a_manager, RE::Actor* a_actor, RE::TESBoundObject* a_object, const RE::ObjectEquipParams& a_objectEquipParams)
		{
			if (a_actor && a_object && !a_objectEquipParams.forceEquip) {
				if (!a_objectEquipParams.extraDataList || !a_objectEquipParams.extraDataList->HasQuestObjectAlias()) {
					RestrictParams params{
						RESTRICT_ON::kEquip,
						RESTRICT_TYPE::kRestrict,
						RESTRICT_REASON::kGeneric,
						a_actor,
						a_object
					};
					RestrictResult result;
					if (result = ItemRestrictor::Manager::GetSingleton()->ShouldSkip(params); result.shouldSkip) {
						if (a_actor->IsPlayerRef()) {
							const auto notification = Settings::GetSingleton()->GetNotification(params);
							if (a_objectEquipParams.showMessage && !notification.empty()) {
								RE::SendHUDMessage::ShowHUDMessage(notification.c_str());
							}
						}
						return;
					}
				}
			}

			return func(a_manager, a_actor, a_object, a_objectEquipParams);
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	struct MagicEquipParams
	{
		RE::BGSEquipSlot* equipSlot;   // 00
		bool              queueEquip;  // 01
	};

	struct DoEquipMagic
	{
		static void thunk(RE::ActorEquipManager* a_manager, RE::Actor* a_actor, RE::TESBoundObject* a_object, const MagicEquipParams& a_magicEquipParams)
		{
			if (a_actor && a_object) {
				RestrictParams params{
					RESTRICT_ON::kEquip,
					RESTRICT_TYPE::kRestrict,
					RESTRICT_REASON::kGeneric,
					a_actor,
					a_object
				};
				RestrictResult result;
				if (result = ItemRestrictor::Manager::GetSingleton()->ShouldSkip(params); result.shouldSkip) {
					if (a_actor->IsPlayerRef()) {
						const auto notification = Settings::GetSingleton()->GetNotification(params);
						if (!notification.empty()) {
							RE::SendHUDMessage::ShowHUDMessage(notification.c_str());
						}
					}
					return;
				}
			}

			return func(a_manager, a_actor, a_object, a_magicEquipParams);
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	struct DoEquipShout
	{
		static void thunk(RE::ActorEquipManager* a_manager, RE::Actor* a_actor, RE::TESBoundObject* a_object, const MagicEquipParams& a_magicEquipParams)
		{
			if (a_actor && a_object) {
				RestrictParams params{
					RESTRICT_ON::kEquip,
					RESTRICT_TYPE::kRestrict,
					RESTRICT_REASON::kGeneric,
					a_actor,
					a_object
				};
				RestrictResult result;
				if (result = ItemRestrictor::Manager::GetSingleton()->ShouldSkip(params); result.shouldSkip) {
					if (a_actor->IsPlayerRef()) {
						const auto notification = Settings::GetSingleton()->GetNotification(params);
						if (!notification.empty()) {
							RE::SendHUDMessage::ShowHUDMessage(notification.c_str());
						}
					}
					return;
				}
			}

			return func(a_manager, a_actor, a_object, a_magicEquipParams);
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	struct PickUpObject
	{
		static void thunk(RE::Actor* a_actor, RE::TESObjectREFR* a_object, std::int32_t a_count, bool a_arg3, bool a_playSound)
		{
			if (a_object) {
				auto baseObject = a_object->GetObjectReference();
				if (baseObject) {
					RestrictResult result;
					RestrictParams params{
						RESTRICT_ON::kPickUp,
						RESTRICT_TYPE::kRestrict,
						RESTRICT_REASON::kGeneric,
						a_actor,
						baseObject
					};
					if (!a_object->extraList.HasQuestObjectAlias()) {
						if (result = ItemRestrictor::Manager::GetSingleton()->ShouldSkip(params); result.shouldSkip) {
							if (a_actor->IsPlayerRef()) {
								const auto notification = Settings::GetSingleton()->GetNotification(params);
								if (!notification.empty()) {
									RE::SendHUDMessage::ShowHUDMessage(notification.c_str());
								}
							}
							return;
						}
					}
					if (a_actor->IsPlayerRef()) {
						params.restrictType = RESTRICT_TYPE::kDebuff;
						if (result = ItemRestrictor::Manager::GetSingleton()->ShouldSkip(params); result.shouldSkip && result.debuffForm) {
							const auto notification = Settings::GetSingleton()->GetNotification(params);
							if (!notification.empty()) {
								RE::SendHUDMessage::ShowHUDMessage(notification.c_str());
							}
							ItemRestrictor::Manager::GetSingleton()->AddDebuff(baseObject, result.debuffForm, false);
						}
					}
				}
			}

			return func(a_actor, a_object, a_count, a_arg3, a_playSound);
		}
		static inline REL::Relocation<decltype(thunk)> func;
		static inline constexpr std::size_t            idx = 0x0CC;
	};

	struct DropObject
	{
		static void thunk(RE::Actor* a_actor, const RE::TESBoundObject* a_object, RE::ExtraDataList* a_extraList, std::int32_t a_count, const RE::NiPoint3* a_dropLoc, const RE::NiPoint3* a_rotate)
		{
			if (a_object) {
				ItemRestrictor::Manager::GetSingleton()->RemoveDebuff(a_object, false);
			}

			return func(a_actor, a_object, a_extraList, a_count, a_dropLoc, a_rotate);
		}
		static inline REL::Relocation<decltype(thunk)> func;
		static inline constexpr std::size_t            idx = 0x0CB;
	};

	void Install()
	{
		SKSE::AllocTrampoline(128);

		std::array targets{
			std::make_pair(RELOCATION_ID(37938, 38894), OFFSET(0xE5, 0x170)),  //ActorEquipManager::EquipObject
			std::make_pair(RELOCATION_ID(37937, 38893), 0xBC),                 //ActorEquipManager::EquipImpl?
		};

		for (const auto& [id, offset] : targets) {
			REL::Relocation<std::uintptr_t> target{ id, offset };
			stl::write_thunk_call<DoEquip>(target.address());
		}

		REL::Relocation<std::uintptr_t> targetMagic{ RELOCATION_ID(37973, 38928) };
		stl::hook_function_prologue<DoEquipMagic, OFFSET(5, 6)>(targetMagic.address());

		REL::Relocation<std::uintptr_t> targetShout{ RELOCATION_ID(37975, 38930) };
		stl::hook_function_prologue<DoEquipShout, 5>(targetShout.address());

		stl::write_vfunc<RE::Character, PickUpObject>();
		stl::write_vfunc<RE::PlayerCharacter, PickUpObject>();
		stl::write_vfunc<RE::PlayerCharacter, DropObject>();
	}
}
