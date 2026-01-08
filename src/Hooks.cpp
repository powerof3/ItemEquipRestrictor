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
	}
}
