#include "player_movement.h"
#include "Zeal.h"
#include "EqAddresses.h"
#include "StringUtil.h"

static void CloseSpellbook(void)
{
	Zeal::EqGame::get_self()->ChangeStance(Stance::Stand);
	Zeal::EqGame::Windows->SpellBook->IsVisible = false;
}

void PlayerMovement::handle_movement_binds(int cmd, bool key_down)
{
	if (!Zeal::EqGame::game_wants_input() && key_down)
	{
		if (!Zeal::EqGame::KeyMods->Alt && !Zeal::EqGame::KeyMods->Shift && !Zeal::EqGame::KeyMods->Ctrl)
		{

			if (Zeal::EqGame::is_new_ui())
			{
				if (Zeal::EqGame::Windows->Loot && Zeal::EqGame::Windows->Loot->IsOpen && Zeal::EqGame::Windows->Loot->IsVisible)
				{
					Zeal::EqGame::EqGameInternal::CLootWndDeactivate((int)Zeal::EqGame::Windows->Loot, 0);
					return;
				}
				else if (Zeal::EqGame::Windows->SpellBook && Zeal::EqGame::Windows->SpellBook->IsVisible)
				{
					switch (cmd) {
						case 3:
							CloseSpellbook();
							break;
						case 5: // default should be turn page left (not implemented)
							if (!spellbook_right_autostand) { return; }
							CloseSpellbook();
							break;
						case 6: // default to turn page right (not implemented)
							if (!spellbook_left_autostand) { return; }
							CloseSpellbook();
							break;
						case 211:
							if (!spellbook_left_strafe_autostand) { return; }
							CloseSpellbook();
							break;
						case 212:
							if (!spellbook_right_strafe_autostand) { return; }
							CloseSpellbook();
							break;
						default: { return; }
					}
				}
			}
			else
			{
				if (Zeal::EqGame::get_active_corpse())
				{
					// how do we close corpse without closing the window?
					return;
				}
				else if (Zeal::EqGame::OldUI::spellbook_window_open())
				{
					// left and right arrows dont turn pages on oldui by default
					// so I'm not sure if we'll add support for other keys
					if (cmd == 4) { return; }
					else if (cmd == 5 && !spellbook_left_autostand)  { return; }
					else if (cmd == 6 && !spellbook_right_autostand) { return; }
				}
			}

			switch (Zeal::EqGame::get_controlled()->StandingState)
			{
				case Zeal::EqEnums::Stance::Sitting:
					Zeal::EqGame::get_controlled()->ChangeStance(Stance::Stand);
					break;
				default: { return; }
			}
		}
	}
}

void PlayerMovement::handle_spellcast_binds(int cmd)
{
	if (!Zeal::EqGame::game_wants_input())
	{
		if (Zeal::EqGame::is_new_ui())
		{
			if (Zeal::EqGame::Windows->Loot && Zeal::EqGame::Windows->Loot->IsOpen && Zeal::EqGame::Windows->Loot->IsVisible)
			{
				return;
			}
			else if (Zeal::EqGame::Windows->SpellBook && Zeal::EqGame::Windows->SpellBook->IsVisible)
			{
				return;
			}
		}
		else
		{
			if (Zeal::EqGame::get_active_corpse())
			{
				// how do we close corpse without closing the window?
				return;
			}
			else if (Zeal::EqGame::OldUI::spellbook_window_open())
			{
				return;
			}
		}

		// not in a window, handle things normally
		switch (Zeal::EqGame::get_self()->StandingState)
		{
			case Zeal::EqEnums::Stance::Sitting:
				Zeal::EqGame::get_self()->ChangeStance(Stance::Stand);
				break;
			default: { return; }
		}
	}
}

void PlayerMovement::set_strafe(strafe_direction dir)
{
	//slightly revised so the game properly sets speed based on encumber and handles the stance checks
	if (dir == strafe_direction::None || !Zeal::EqGame::can_move())
	{
		current_strafe = strafe_direction::None;
		*Zeal::EqGame::strafe_direction = 0;
		*Zeal::EqGame::strafe_speed = 0;
		if (orig_reset_strafe[0] != 0)
			mem::copy(0x53f424, orig_reset_strafe, 7);
	}
	else
	{
		if (orig_reset_strafe[0] == 0)
			mem::set(0x53f424, 0x90, 7, orig_reset_strafe);
		else if (*(BYTE*)0x53f424 != 0x90)
			mem::set(0x53f424, 0x90, 7);


		if (dir == strafe_direction::Right)
		{
			current_strafe = strafe_direction::Right;
			*Zeal::EqGame::strafe_direction = 0x2;
		}
		else
		{
			current_strafe = strafe_direction::Left;
			*Zeal::EqGame::strafe_direction = 0x1;
		}
	}
}

void PlayerMovement::callback_main()
{
	if (current_strafe != strafe_direction::None)
	{
		Zeal::EqStructures::Entity* controlled_player = Zeal::EqGame::get_controlled();
		float encumberance = Zeal::EqGame::encum_factor();
		*Zeal::EqGame::strafe_speed = encumberance + encumberance;
		if (controlled_player->IsSneaking || controlled_player->StandingState == Stance::Duck)
			*Zeal::EqGame::strafe_speed *= .5f;
		if (controlled_player->ActorInfo && controlled_player->ActorInfo->MovementSpeedModifier < 0)
			*Zeal::EqGame::strafe_speed *= .5f;
		if (controlled_player->ActorInfo && controlled_player->ActorInfo->Unsure_Strafe_Calc != 0)
			*Zeal::EqGame::strafe_speed *= .25f;
		if (controlled_player->ActorInfo && controlled_player->ActorInfo->MovementSpeedModifier < -1000.0f)
		{
			*Zeal::EqGame::strafe_direction = 0;
			*Zeal::EqGame::strafe_speed = 0;
		}
		if (controlled_player->StandingState != Stance::Duck && controlled_player->StandingState != Stance::Stand)
		{
			*Zeal::EqGame::strafe_direction = 0;
			*Zeal::EqGame::strafe_speed = 0;
		}
	}
}

void PlayerMovement::load_settings()
{
	if (!ini_handle->exists("Zeal", "LeftTurnSpellbookAutostand"))
		ini_handle->setValue<bool>("Zeal", "LeftTurnSpellbookAutostand", false);
	if (!ini_handle->exists("Zeal", "RightTurnSpellbookAutostand"))
		ini_handle->setValue<bool>("Zeal", "RightTurnSpellbookAutostand", false);
	if (!ini_handle->exists("Zeal", "LeftStrafeSpellbookAutostand"))
		ini_handle->setValue<bool>("Zeal", "LeftStrafeSpellbookAutostand", true);
	if (!ini_handle->exists("Zeal", "RightStrafeSpellbookAutostand"))
		ini_handle->setValue<bool>("Zeal", "RightStrafeSpellbookAutostand", true);

	spellbook_left_autostand = ini_handle->getValue<bool>("Zeal", "LeftTurnSpellbookAutostand");
	spellbook_right_autostand = ini_handle->getValue<bool>("Zeal", "RightTurnSpellbookAutostand");
	spellbook_left_strafe_autostand = ini_handle->getValue<bool>("Zeal", "LeftStrafeSpellbookAutostand");
	spellbook_right_strafe_autostand = ini_handle->getValue<bool>("Zeal", "RightStrafeSpellbookAutostand");
}

PlayerMovement::PlayerMovement(ZealService* zeal, class Binds* binds, class IO_ini* ini)
{
	ini_handle = ini;
	load_settings();

	// ISSUE: Mapping LEFT/RIGHT arrow keys to strafe on TAKP2.1 client fails to function.
	binds->replace_bind(211, [this](int state) {
		if (!state && current_strafe == strafe_direction::Left)
		{
			set_strafe(strafe_direction::None);
		}
		else if (state)
		{
			handle_movement_binds(211, state);
			set_strafe(strafe_direction::Left);
		}
		return false;
	}); // strafe left
	binds->replace_bind(212, [this](int state) {
		if (!state && current_strafe == strafe_direction::Right)
		{
			set_strafe(strafe_direction::None);
		}
		else if (state)
		{
			handle_movement_binds(212, state);
			set_strafe(strafe_direction::Right);
		}
		return false;
	}); // strafe right

	zeal->commands_hook->add("/autostand", {},
		[this](std::vector<std::string>& args) {
			if (args.size() == 1 || args.size() > 2)
			{
				Zeal::EqGame::print_chat("usage: /autostand [LeftTurn | RightTurn | LeftStrafe | RightStrafe]");
				return true;
			}
			if (args.size() > 1) {
				std::ostringstream oss;
				if (StringUtil::caseInsensitive(args[1], "LeftTurn"))
				{
					spellbook_left_autostand = !spellbook_left_autostand;
					ini_handle->setValue<bool>("Zeal", "LeftTurnSpellbookAutostand", spellbook_left_autostand);
					std::string is_enabled = spellbook_left_autostand ? "enabled" : "disabled";
					oss << "[Autostand] Left turn spellbook autostand has been " << is_enabled << "." << std::endl;
					Zeal::EqGame::print_chat(oss.str());
					return true;
				}
				else if (StringUtil::caseInsensitive(args[1], "RightTurn"))
				{
					spellbook_right_autostand = !spellbook_right_autostand;
					ini_handle->setValue<bool>("Zeal", "RightTurnSpellbookAutostand", spellbook_right_autostand);
					std::string is_enabled = spellbook_right_autostand ? "enabled" : "disabled";
					oss << "[Autostand] Right turn spellbook autostand has been " << is_enabled << "." << std::endl;
					Zeal::EqGame::print_chat(oss.str());
					return true;
				}
				else if (StringUtil::caseInsensitive(args[1], "LeftStrafe"))
				{
					spellbook_left_strafe_autostand = !spellbook_left_strafe_autostand;
					ini_handle->setValue<bool>("Zeal", "LeftStrafeSpellbookAutostand", spellbook_left_strafe_autostand);
					std::string is_enabled = spellbook_left_strafe_autostand ? "enabled" : "disabled";
					oss << "[Autostand] Left strafe spellbook autostand has been " << is_enabled << "." << std::endl;
					Zeal::EqGame::print_chat(oss.str());
					return true;
				}
				else if (StringUtil::caseInsensitive(args[1], "RightStrafe"))
				{
					spellbook_right_strafe_autostand = !spellbook_right_strafe_autostand;
					ini_handle->setValue<bool>("Zeal", "RightStrafeSpellbookAutostand", spellbook_right_strafe_autostand);
					std::string is_enabled = spellbook_right_strafe_autostand ? "enabled" : "disabled";
					oss << "[Autostand] Right strafe spellbook autostand has been " << is_enabled << "." << std::endl;
					Zeal::EqGame::print_chat(oss.str());
					return true;
				}
			}
			return false;
		}
	);
	zeal->main_loop_hook->add_callback([this]() { callback_main(); }, callback_fn::MainLoop);
}