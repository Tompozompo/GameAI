#ifdef _WIN32
#include "windows.h"
#else
#include <dirent.h>
#endif

#include "stdio.h"
#include "math.h"
#include "stdlib.h"
#include "string.h"

#include "GL/gl.h"
#include "GL/glu.h"
#include "SDL.h"
#include "SDL_mixer.h"
#include "SDL_image.h"
#include "SDL_net.h"

#include "F1Spirit.h"
#include "sound.h"
#include "auxiliar.h"
#include "List.h"
#include "2DCMC.h"
#include "GLTile.h"
#include "PlacedGLTile.h"
#include "RotatedGLTile.h"
#include "keyboardstate.h"
#include "CPlayer.h"
#include "CCar.h"
#include "2DCMC.h"
#include "RoadPiece.h"
#include "track.h"
#include "RacingCCar.h"
#include "PlayerCCar.h"
#include "EnemyCCar.h"
#include "GameParameters.h"
#include "ReplayInfo.h"
#include "F1SpiritGame.h"
#include "F1SpiritTrackViewer.h"
#include "F1SpiritApp.h"
#include "F1Spirit-auxiliar.h"
#include "filehandling.h"

#include "debug.h"

#ifdef KITSCHY_DEBUG_MEMORY
#include "debug_memorymanager.h"
#endif

extern int application_version;
extern int LISTENING_TIME;
extern int N_SFX_CHANNELS;
extern bool sound;
extern int SCREEN_X;
extern int SCREEN_Y;
extern bool network;
extern int network_tcp_port;
extern int network_udp_port;

extern char *f1spirit_replay_version;

#define MENU_CONSTANT 20
#define TRACKPREVIEW_CONSTANT 25
#define CLIENT_REFRESH_TIMMER  250
#define MAX_RETRIES 5

int F1SpiritApp::menu_cycle(KEYBOARDSTATE *k)
{
	if (race_game != 0) {
		delete race_game;
		race_game = 0;
	} 

	if (state_cycle == 0) {
		int i;

		if (menu_current_menu == 0 ||
		        menu_current_menu == 4) {
			/*
			#ifdef _WIN32
			            if (sound) {
			                Stop_playback();
			                N_SFX_CHANNELS = Resume_playback(N_SFX_CHANNELS, 0);
			            } // if
			#endif
			*/
			Sound_create_music("sound/game_menu", -1);
		} 

		menu_fading = -1;

		menu_fading_ctnt = MENU_CONSTANT;

		menu_showing_points = 0;

		menu_showing_track = -1;

		menu_showing_track_timmer = 0;

		menu_showing_track_timmer2 = 0;

		menu_showing_car_type = -1;

		menu_showing_car = -1;

		menu_showing_car_timmer = 0;

		menu_showing_piece_type = -1;

		menu_showing_piece = -1;

		menu_showing_piece_timmer = 0;

		delete menu_track;

		menu_track = 0;

		delete menu_track_frame;

		menu_track_frame = 0;

		delete menu_points_frame;

		menu_points_frame = 0;

		for (i = 0;i < 3;i++) {
			delete menu_car_top[i];
			menu_car_top[i] = 0;
			delete menu_car_side[i];
			menu_car_side[i] = 0;
			delete menu_car_frame[i];
			menu_car_frame[i] = 0;

			delete menu_piece[i];
			menu_piece[i] = 0;
			delete menu_piece_frame[i];
			menu_piece_frame[i] = 0;
		} 

		delete menu_info_frame;

		menu_info_frame = 0;

		delete menu_design;

		menu_design = 0;

	} 

	if (menu_fading == 0) {
		Sound_music_volume(MIX_MAX_VOLUME);
	} else {
		if (menu_fading > 0)
			menu_fading_ctnt++;
		else
			menu_fading_ctnt--;

		if (menu_fading_ctnt <= 0 && menu_fading == -1) {
			Sound_music_volume(MIX_MAX_VOLUME - 5*menu_fading_ctnt);
			menu_fading = 0;
		} 

		if (menu_fading != 2 && menu_fading != 4) {
			Sound_music_volume(MIX_MAX_VOLUME - (MIX_MAX_VOLUME / MENU_CONSTANT)*menu_fading_ctnt);
		} 

		if (menu_fading_ctnt >= MENU_CONSTANT && menu_fading == 1) {
			menu_fading = 0;
			Sound_release_music();
			return APP_STATE_KONAMI;
		} 

		if (menu_fading_ctnt >= MENU_CONSTANT && menu_fading == 2) {
			menu_fading = 0;
			return APP_STATE_HISCORE;
		} 

		if (menu_fading_ctnt >= MENU_CONSTANT && menu_fading == 3) {
			menu_fading = 0;
			Sound_release_music();
			return APP_STATE_TRACKLOAD;
		} 

		if (menu_fading_ctnt >= MENU_CONSTANT && menu_fading == 4) {
			menu_fading = 0;
			return APP_STATE_REPLAYMANAGER;
		} 
	} 

	if (k->keyboard[SDLK_m] && k->keyboard[SDLK_a] && k->keyboard[SDLK_x]) {
		cheat_max_point = true;
	} 

	if (k->keyboard[SDLK_e] && k->keyboard[SDLK_n] && k->keyboard[SDLK_d]) {
		endsequence_state = 0;
		endsequence_timmer = 0;
		endsequence_returnstate = APP_STATE_MENU;
		return APP_STATE_ENDSEQUENCE;
	} 

	if (state_cycle == 0 || menu_state == MENU_CONSTANT*2 || menu_redefining_key || menu_force_rebuild_menu) {
		int browsing = 0;

		if (menu_options[1] != 0)
			browsing = 1;

		if (state_cycle != 0 && !menu_force_rebuild_menu) {

#ifdef F1SPIRIT_DEBUG_MESSAGES
			output_debug_message("Executing menu action: %i(%i)\n", menu_option_type[browsing][menu_selected[browsing]], menu_option_parameter[browsing][menu_selected[browsing]]);
#endif

			switch (menu_option_type[browsing][menu_selected[browsing]]) {

				case 0:  /* QUIT: */
					menu_fading = 1;
					menu_fading_ctnt = 0;
					break;

				case 1:  /* MENU CHANGE: */
					menu_current_menu = menu_option_parameter[browsing][menu_selected[browsing]];

					if (menu_current_menu == 23) {
						menu_readme_start_y = 0;
						menu_readme_move_y = 0;
					} 

					break;

				case 2:  /* EDITABLE OPTION FOR PLAYER NAME: */
					menu_current_menu = menu_option_parameter[browsing][menu_selected[browsing]];

					strcpy((char *)menu_stored_player_name, (char *)menu_editing);

					break;

				case 3:  /* SELECT PLAYER: */
					menu_current_menu = menu_option_parameter[browsing][menu_selected[browsing]];

					{
						FILE *fp;
						char filename[80];
						current_player = new CPlayerInfo();

						sprintf(filename, "players/player%i.dat", menu_selected[0]);
						fp = f1open(filename, "rb", USERDATA);

						if (fp != 0) {
							current_player->load(fp, N_TRACKS, f1spirit_replay_version);
							strcpy(player_filename, filename);
							fclose(fp);
						} 
					}

					break;

				case 4:  /* DELETE PLAYER: */
					menu_current_menu = menu_option_parameter[browsing][menu_selected[browsing]];
					{
						/* Load all the players: */
						List<CPlayerInfo> players;
						List<char> names;
						int i, nplayers;
						char filename[80];
						FILE *fp;
						CPlayerInfo *p;

						nplayers = get_player_list(&names);

						for (i = 0;i < nplayers;i++) {
							sprintf(filename, "players/player%i.dat", i);
							fp = f1open(filename, "rb", USERDATA);
							p = new CPlayerInfo();
							p->load(fp, N_TRACKS, f1spirit_replay_version);
							fclose(fp);
							remove(filename);
							players.Add(p);
						} 

						players.DeletePosition(menu_selected[0]);

						i = 0;

						players.Rewind();

						while (players.Iterate(p)) {
							sprintf(filename, "players/player%i.dat", i);
							fp = f1open(filename, "wb", USERDATA);
							p->save(fp);
							fclose(fp);
							i++;
						} 
					}

					break;

				case 5:  /* ADD ONE PLAYER TO THE CURRENT CONFIGURATION: */
					menu_current_menu = menu_option_parameter[browsing][menu_selected[browsing]];
					current_player->add_player();
					{
						FILE *fp;

						fp = f1open(player_filename, "wb", USERDATA);

						if (fp != 0) {
							current_player->save(fp);
							fclose(fp);
						} 
					}

					break;

				case 6:  /* SUB ONE PLAYER TO THE CURRENT CONFIGURATION: */
					menu_current_menu = menu_option_parameter[browsing][menu_selected[browsing]];
					current_player->sub_player();
					{
						FILE *fp;

						fp = f1open(player_filename, "wb", USERDATA);

						if (fp != 0) {
							current_player->save(fp);
							fclose(fp);
						} 
					}

					break;

				case 7:  /* SET A KEY: */
					break;

				case 8:  /* SET A JOYSTICK: */
					menu_current_menu = menu_option_parameter[browsing][menu_selected[browsing]];
					{
						FILE *fp;
						current_player->set_joystick(menu_selected[0] - 3, menu_selected[1] - 1);

						fp = f1open(player_filename, "wb", USERDATA);

						if (fp != 0) {
							current_player->save(fp);
							fclose(fp);
						} 
					}

					break;

				case 9:  /* GO TO THE HIGHSCORES/REPLAYS: */

					if (menu_option_parameter[browsing][menu_selected[browsing]] == 0) {
						menu_fading = 2;
						menu_fading_ctnt = 0;
						hiscore_showing = 0;
						hiscore_timmer = 25;
						hiscore_action = 0;
						hiscore_table_regen = true;
					} else {
						menu_fading = 4;
						menu_fading_ctnt = 0;
						replaymanager_timmer = 25;
						replaymanager_state = 0;
					} 

					break;

				case 10: { /* SELECT RACE: */
						F1S_GParameters parameters;
						parameters.load_ascii("f1spirit2.cfg");

						menu_selected_track = menu_option_parameter[browsing][menu_selected[browsing]];

						if (menu_selected_nplayers == 1) {
							menu_multiplayer_n_enemycars = parameters.race_cars[menu_selected_track];
							menu_multiplayer_enemy_speed = 1;
							menu_current_menu = 11;
						} else {
							menu_multiplayer_n_enemycars = parameters.race_cars[menu_selected_track];
							menu_multiplayer_enemy_speed = 1;
							menu_current_menu = 46;
						} 

						menu_selecting_player = 0;
					}

					break;

				case 11:  /* MENU CHANGE TO SUBMENU: */
					menu_current_menu = menu_option_parameter[browsing][menu_selected[browsing]];
					break;

				case 12:  /* CHANGE CAR PIECE: */
					menu_selected_part[menu_selecting_player][menu_option_parameter[browsing][menu_selected[browsing]]] = menu_selected[browsing];
					menu_current_menu = 28;
					break;

				case 13: { /* START GAME: */
						/* Create the player(s) car(s): */
						if (menu_selecting_player >= menu_selected_nplayers - 1) {
							menu_fading = 3;
							menu_fading_ctnt = 0;
							menu_selected_car[menu_selecting_player] = menu_option_parameter[browsing][menu_selected[browsing]];
						} else {
							menu_current_menu = 11;
							menu_selected_car[menu_selecting_player] = menu_option_parameter[browsing][menu_selected[browsing]];
						} 

						menu_selecting_player++;
					}

					break;

				case 15: { /* CHANGE MUSIC VOLUME: */
					}

					break;

				case 16: { /* CHANGE SFX VOLUME: */
					}

					break;

				case 18:  /* SET THE NUMBER OF PLAYERS: */
					menu_current_menu = menu_option_parameter[browsing][menu_selected[browsing]];
					{
						menu_selected_nplayers = menu_selected[browsing] + 2;
					}

					break;

				case 19:
					break;

				case 20:
					break;

				case 21:
					break;

				case 22:  /* EDITABLE OPTION FOR DESIGN NAME: */
					menu_current_menu = menu_option_parameter[browsing][menu_selected[browsing]];
					{
						/* Create and save the design: */
						{
							int type;
							char *folders[6] = {"designs/stock",
							                    "designs/rally",
							                    "designs/f3",
							                    "designs/f3000",
							                    "designs/endurance",
							                    "designs/f1"
							                   };
							char filename[256];
							FILE *fp;

							type = min(menu_selected_track, 5);
							sprintf(filename, "%s/%s.car", folders[type], menu_editing);
#ifdef F1SPIRIT_DEBUG_MESSAGES
							output_debug_message("Creating design file: %s\n", filename);
#endif

							fp = f1open(filename, "wb", USERDATA);

							if (fp != 0) {
#ifdef F1SPIRIT_DEBUG_MESSAGES
								output_debug_message("Success\n", filename);
#endif

								fputc(menu_selected_part[menu_selecting_player][0], fp);
								fputc(menu_selected_part[menu_selecting_player][1], fp);
								fputc(menu_selected_part[menu_selecting_player][2], fp);
								fputc(menu_selected_part[menu_selecting_player][3], fp);
								fputc(menu_selected_part[menu_selecting_player][4], fp);
								fclose(fp);
							} else {
#ifdef F1SPIRIT_DEBUG_MESSAGES
								output_debug_message("Failure\n", filename);
#endif

							} 
						}

					}

					break;

				case 23:  /* LOAD CAR DESIGN: */
					menu_current_menu = menu_option_parameter[browsing][menu_selected[browsing]];
					{
						/* load the design: */
						char design_name[256];
						int type;
						char *folders[6] = {"designs/stock",
						                    "designs/rally",
						                    "designs/f3",
						                    "designs/f3000",
						                    "designs/endurance",
						                    "designs/f1"
						                   };
						char filename[256];
						FILE *fp;
						int i, start = 0, end = 0, current = -1;

						type = min(menu_selected_track, 5);

						for (i = 0;menu_options[browsing][i] != 0 && current < menu_selected[browsing];i++) {
							if (menu_options[browsing][i] == '\n' || menu_options[browsing][i] == 0) {
								start = end;
								end = i + 1;
								current++;
							} 
						} 

						for (i = start;i < end - 1 && menu_options[browsing][i] != '\n';i++)
							design_name[i - start] = menu_options[browsing][i];

						design_name[i - start] = 0;

						sprintf(filename, "%s/%s.car", folders[type], design_name);

#ifdef F1SPIRIT_DEBUG_MESSAGES

						output_debug_message("Opening design file: %s\n", filename);

#endif

						fp = f1open(filename, "rb", USERDATA);

						if (fp != 0) {
#ifdef F1SPIRIT_DEBUG_MESSAGES
							output_debug_message("Success\n", filename);
#endif

							menu_selected_part[menu_selecting_player][0] = fgetc(fp);
							menu_selected_part[menu_selecting_player][1] = fgetc(fp);
							menu_selected_part[menu_selecting_player][2] = fgetc(fp);
							menu_selected_part[menu_selecting_player][3] = fgetc(fp);
							menu_selected_part[menu_selecting_player][4] = fgetc(fp);
							fclose(fp);
						} else {
#ifdef F1SPIRIT_DEBUG_MESSAGES
							output_debug_message("Failure\n", filename);
#endif

						} 
					}

					break;

				case 24: { /* EDITABLE OPTION PLAYER PASSWORD: */
						/* Create the player: */
						CPlayerInfo *pi = new CPlayerInfo(menu_stored_player_name, (char *)menu_editing);
						FILE *fp;
						int i;
						List<char> names;
						char filename[80];

						if (pi != 0) {
							i = get_player_list(&names);

							sprintf(filename, "players/player%i.dat", i);
							fp = f1open(filename, "wb", USERDATA);

							if (fp != 0) {
								pi->save(fp);
								strcpy(player_filename, filename);
								fclose(fp);
							} 

							current_player = pi;
						} else {
							menu_current_menu = 0;
						} 

						menu_current_menu = menu_option_parameter[browsing][menu_selected[browsing]];
					}

					break;

				case 25:  /* CANCEL HTTP ACCESS: */
					menu_current_menu = menu_option_parameter[browsing][menu_selected[browsing]];
					break;

				case 26: { /* DOWNLOAD HISCORES: */
						menu_current_menu = menu_option_parameter[browsing][menu_selected[browsing]];
					}

					break;

				case 27: { /* REGISTER PLAYER: */
						menu_current_menu = menu_option_parameter[browsing][menu_selected[browsing]];
					}

					break;

				case 28: { /* UPLOAD HIGHSCORES: */
						menu_current_menu = menu_option_parameter[browsing][menu_selected[browsing]];
					}

					break;

				case 29: { /* REGISTER SERVER: */
                    }
					break;

				case 30: { /* UNREGISTER SERVER: */
                    }
					break;

				case 32: { /* CHAT MESSAGE: */
                    }
					break;

				case 34: { /* RETRIEVE SERVER LIST + CLIENT NAME: */
                    }
					break;

				case 35: { /* ENTERED A SERVER IP: */
                    }
					break;

				case 36: { /* CONNECT TO A LOCAL/GLOBAL SERVER: */
                    }
					break;

				case 37: { /* CLIENT: UNREGISTER FROM SERVER: */
                    }
					break;

				case 38: { /* CHANGE Nº of ENEMY CARS: */
					}

					break;

				case 39: { /* CHANGE ENEMY SPEED: */
					}

					break;

			} 
		} 

		delete []menu_title[0];

		menu_title[0] = 0;

		delete []menu_options[0];

		menu_options[0] = 0;

		delete []menu_title[1];

		menu_title[1] = 0;

		delete []menu_options[1];

		menu_options[1] = 0;

		menu_create_menu();

		menu_force_rebuild_menu = false;

	} else {
		if (menu_fading == 0) {
			if (menu_state < MENU_CONSTANT)
				menu_state++;

			if (menu_state == MENU_CONSTANT) {
				menu_prev_nmenus = 0;

				if (menu_options[0] != 0)
					menu_prev_nmenus++;

				if (menu_options[1] != 0)
					menu_prev_nmenus++;
			} 

			if (menu_state > MENU_CONSTANT)
				menu_state++;
		} 
	} 


	if (menu_readme_move_y != 0) {
		menu_readme_start_y += menu_readme_move_y;

		if (menu_readme_move_y > 0)
			menu_readme_move_y--;

		if (menu_readme_move_y < 0)
			menu_readme_move_y++;

		if (menu_readme_start_y < 0)
			menu_readme_start_y = 0;
	} 

	if (menu_state == MENU_CONSTANT) {
		int browsing = 0;

		if (menu_options[1] != 0)
			browsing = 1;

		if (!menu_redefining_key) {
			if (menu_current_menu == 23 &&
			        (k->keyboard[SDLK_DOWN] ||
			         k->keyboard[SDLK_RIGHT] ||
			         k->keyboard[SDLK_PAGEDOWN])) {
				menu_readme_move_y = 8;
			} 

			if (menu_current_menu == 23 &&
			        (k->keyboard[SDLK_UP] ||
			         k->keyboard[SDLK_LEFT] ||
			         k->keyboard[SDLK_PAGEUP])) {
				menu_readme_move_y = -8;
			} 

			if ((k->keyboard[SDLK_DOWN] && !k->old_keyboard[SDLK_DOWN]) ||
			        (k->keyboard[SDLK_RIGHT] && !k->old_keyboard[SDLK_RIGHT])) {
				if ((k->keyboard[SDLK_RIGHT] && !k->old_keyboard[SDLK_RIGHT]) &&
				        (menu_option_type[browsing][menu_selected[browsing]] == 15 ||
				         menu_option_type[browsing][menu_selected[browsing]] == 16 ||
				         menu_option_type[browsing][menu_selected[browsing]] == 38 ||
				         menu_option_type[browsing][menu_selected[browsing]] == 39)) {
					if (menu_option_type[browsing][menu_selected[browsing]] == 15) {
						int v = current_player->get_music_volume();

						if (v >= MIX_MAX_VOLUME)
							current_player->set_music_volume(0);

						if (v == 96)
							current_player->set_music_volume(MIX_MAX_VOLUME);

						if (v == 64)
							current_player->set_music_volume(96);

						if (v == 32)
							current_player->set_music_volume(64);

						if (v == 0)
							current_player->set_music_volume(32);
					} 

					if (menu_option_type[browsing][menu_selected[browsing]] == 16) {
						int v = current_player->get_sfx_volume();

						if (v >= MIX_MAX_VOLUME)
							current_player->set_sfx_volume(0);

						if (v == 96)
							current_player->set_sfx_volume(MIX_MAX_VOLUME);

						if (v == 64)
							current_player->set_sfx_volume(96);

						if (v == 32)
							current_player->set_sfx_volume(64);

						if (v == 0)
							current_player->set_sfx_volume(32);
					} 

					if (menu_option_type[browsing][menu_selected[browsing]] == 38) {
						if (menu_multiplayer_n_enemycars < 500)
							menu_multiplayer_n_enemycars += 5;
					} 

					if (menu_option_type[browsing][menu_selected[browsing]] == 39) {
						if (menu_multiplayer_enemy_speed < 2)
							menu_multiplayer_enemy_speed++;
					} 

					menu_force_rebuild_menu = true;

					Sound_play(S_menu_move, MIX_MAX_VOLUME);

					{
						FILE *fp;

						fp = f1open(player_filename, "wb", USERDATA);

						if (fp != 0) {
							current_player->save(fp);
							fclose(fp);
						} 
					}
				} else {
					if (menu_noptions[browsing] > 1) {
						menu_selected[browsing]++;
						Sound_play(S_menu_move, MIX_MAX_VOLUME);

						if (menu_selected[browsing] >= menu_noptions[browsing])
							menu_selected[browsing] -= menu_noptions[browsing];

						menu_selected_timmer[browsing] = 0;
					} 
				} 
			} 

			if ((k->keyboard[SDLK_UP] && !k->old_keyboard[SDLK_UP]) ||
			        (k->keyboard[SDLK_LEFT] && !k->old_keyboard[SDLK_LEFT])) {

				if ((k->keyboard[SDLK_LEFT] && !k->old_keyboard[SDLK_LEFT]) &&
				        (menu_option_type[browsing][menu_selected[browsing]] == 15 ||
				         menu_option_type[browsing][menu_selected[browsing]] == 16 ||
				         menu_option_type[browsing][menu_selected[browsing]] == 38 ||
				         menu_option_type[browsing][menu_selected[browsing]] == 39)) {
					if (menu_option_type[browsing][menu_selected[browsing]] == 15) {
						int v = current_player->get_music_volume();

						if (v >= MIX_MAX_VOLUME)
							current_player->set_music_volume(96);

						if (v == 96)
							current_player->set_music_volume(64);

						if (v == 64)
							current_player->set_music_volume(32);

						if (v == 32)
							current_player->set_music_volume(0);

						if (v == 0)
							current_player->set_music_volume(MIX_MAX_VOLUME);
					} 

					if (menu_option_type[browsing][menu_selected[browsing]] == 16) {
						int v = current_player->get_sfx_volume();

						if (v == MIX_MAX_VOLUME)
							current_player->set_sfx_volume(96);

						if (v == 96)
							current_player->set_sfx_volume(64);

						if (v == 64)
							current_player->set_sfx_volume(32);

						if (v == 32)
							current_player->set_sfx_volume(0);

						if (v == 0)
							current_player->set_sfx_volume(MIX_MAX_VOLUME);
					} 

					if (menu_option_type[browsing][menu_selected[browsing]] == 38) {
						if (menu_multiplayer_n_enemycars > 5)
							menu_multiplayer_n_enemycars -= 5;
					} 

					if (menu_option_type[browsing][menu_selected[browsing]] == 39) {
						if (menu_multiplayer_enemy_speed > 0)
							menu_multiplayer_enemy_speed--;
					} 

					menu_force_rebuild_menu = true;

					Sound_play(S_menu_move, MIX_MAX_VOLUME);

					{
						FILE *fp;

						fp = f1open(player_filename, "wb", USERDATA);

						if (fp != 0) {
							current_player->save(fp);
							fclose(fp);
						} 
					}
				} else {
					if (menu_noptions[browsing] > 1) {
						menu_selected[browsing]--;
						Sound_play(S_menu_move, MIX_MAX_VOLUME);

						if (menu_selected[browsing] < 0)
							menu_selected[browsing] += menu_noptions[browsing];

						menu_selected_timmer[browsing] = 0;
					} 
				} 
			} 

			if (k->keyboard[SDLK_ESCAPE] && !k->old_keyboard[SDLK_ESCAPE]) {
				menu_selected[browsing] = menu_noptions[browsing] - 1;
				Sound_play(S_menu_select, MIX_MAX_VOLUME);
				menu_state++;
			} 

			if (menu_option_type[browsing][menu_selected[browsing]] != 2 &&
			        menu_option_type[browsing][menu_selected[browsing]] != 22 &&
			        menu_option_type[browsing][menu_selected[browsing]] != 24 &&
			        menu_option_type[browsing][menu_selected[browsing]] != 29 &&
			        menu_option_type[browsing][menu_selected[browsing]] != 31 &&
			        menu_option_type[browsing][menu_selected[browsing]] != 32 &&
			        menu_option_type[browsing][menu_selected[browsing]] != 34 &&
			        k->keyboard[SDLK_SPACE] && !k->old_keyboard[SDLK_SPACE]) {
				if (menu_option_type[browsing][menu_selected[browsing]] != 19 &&
				        menu_option_type[browsing][menu_selected[browsing]] != 20)
					Sound_play(S_menu_select, MIX_MAX_VOLUME);

				if (menu_option_type[browsing][menu_selected[browsing]] == 7) {
					/* redefining a key: */
					menu_redefining_key = true;
				} else {
					if (menu_option_type[browsing][menu_selected[browsing]] == 15 ||
					        menu_option_type[browsing][menu_selected[browsing]] == 16 ||
					        menu_option_type[browsing][menu_selected[browsing]] == 19 ||
					        menu_option_type[browsing][menu_selected[browsing]] == 20) {
						if (menu_option_type[browsing][menu_selected[browsing]] == 15) {
							int v = current_player->get_music_volume();

							if (v >= MIX_MAX_VOLUME)
								current_player->set_music_volume(0);

							if (v == 96)
								current_player->set_music_volume(MIX_MAX_VOLUME);

							if (v == 64)
								current_player->set_music_volume(96);

							if (v == 32)
								current_player->set_music_volume(64);

							if (v == 0)
								current_player->set_music_volume(32);
						} 

						if (menu_option_type[browsing][menu_selected[browsing]] == 16) {
							int v = current_player->get_sfx_volume();

							if (v == MIX_MAX_VOLUME)
								current_player->set_sfx_volume(0);

							if (v == 96)
								current_player->set_sfx_volume(MIX_MAX_VOLUME);

							if (v == 64)
								current_player->set_sfx_volume(96);

							if (v == 32)
								current_player->set_sfx_volume(64);

							if (v == 0)
								current_player->set_sfx_volume(32);
						} 

						menu_force_rebuild_menu = true;
					} else {
						if (menu_option_type[browsing][menu_selected[browsing]] >= 0)
							menu_state++;
					} 
				} 
			} 

			if (k->keyboard[SDLK_RETURN] && !k->old_keyboard[SDLK_RETURN]) {
				if (menu_option_type[browsing][menu_selected[browsing]] == 2 ||
				        menu_option_type[browsing][menu_selected[browsing]] == 22 ||
				        menu_option_type[browsing][menu_selected[browsing]] == 24 ||
				        menu_option_type[browsing][menu_selected[browsing]] == 29 ||
				        menu_option_type[browsing][menu_selected[browsing]] == 31 ||
				        menu_option_type[browsing][menu_selected[browsing]] == 32 ||
				        menu_option_type[browsing][menu_selected[browsing]] == 34 ||
				        menu_option_type[browsing][menu_selected[browsing]] == 35) {
					if (menu_editing_valid) {
						Sound_play(S_menu_select, MIX_MAX_VOLUME);
						menu_state++;
					} else {
						Sound_play(S_wrong, MIX_MAX_VOLUME);
					} 
				} else {
					Sound_play(S_menu_select, MIX_MAX_VOLUME);

					if (menu_option_type[browsing][menu_selected[browsing]] >= 0)
						menu_state++;
				} 
			} 

			if (menu_option_type[browsing][menu_selected[browsing]] == 2 ||
			        menu_option_type[browsing][menu_selected[browsing]] == 22 ||
			        menu_option_type[browsing][menu_selected[browsing]] == 24 ||
			        menu_option_type[browsing][menu_selected[browsing]] == 29 ||
			        menu_option_type[browsing][menu_selected[browsing]] == 31 ||
			        menu_option_type[browsing][menu_selected[browsing]] == 32 ||
			        menu_option_type[browsing][menu_selected[browsing]] == 34 ||
			        menu_option_type[browsing][menu_selected[browsing]] == 35) {
				bool retest = false;

				List<SDL_keysym> l;
				SDL_keysym *ks;

				l.Instance(k->keyevents);
				l.Rewind();

				while (l.Iterate(ks)) {
#ifdef F1SPIRIT_DEBUG_MESSAGES
					output_debug_message("User has pressed a key in the EDIT window: %i\n", ks->unicode);
#endif

					if ((ks->unicode & 0xFF00) == 0) {
						int c, first_return;
						char tmp[100], tmp2[100];
						int l = strlen((char *)menu_editing);

						if (menu_option_type[browsing][menu_selected[browsing]] == 32) {
							if (l > 24)
								l = 24;
						} else {
							if (l > 16)
								l = 16;
						} 

						c = ks->unicode & 0xFF;

						first_return = 0;

						while (menu_options[browsing][first_return] != '\n')
							first_return++;

						strcpy(tmp2, (char *)menu_options[browsing] + first_return);


#ifdef F1SPIRIT_DEBUG_MESSAGES

						output_debug_message("The Unicode value is: %i\n", c);

#endif

						if (c >= 32 && c < 256) {
							menu_editing[l + 1] = 0;
							menu_editing[l] = c;

							if (menu_option_type[browsing][menu_selected[browsing]] == 35) {
								sprintf(tmp, "IP: %s_%s", menu_editing, tmp2);
							} else {
								sprintf(tmp, "%s_%s", menu_editing, tmp2);
							} 

							if (menu_option_type[browsing][menu_selected[browsing]] == 24) {
								unsigned int i;

								for (i = 0;i < strlen((char *)menu_editing);i++)
									tmp[i] = '*';
							} 

							delete menu_options[browsing];

							menu_options[browsing] = 0;

							menu_options[browsing] = new char[strlen(tmp) + 1];

							strcpy(menu_options[browsing], tmp);

							Sound_play(S_menu_move, MIX_MAX_VOLUME);

							retest = true;

#ifdef F1SPIRIT_DEBUG_MESSAGES

							output_debug_message("The string being edited is now: %s\n", menu_editing);

#endif

						} 
					} 
				} 

				if (k->keyboard[SDLK_BACKSPACE] && !k->old_keyboard[SDLK_BACKSPACE]) {
					char tmp[100], tmp2[100];
					int first_return;

					first_return = 0;

					while (menu_options[browsing][first_return] != '\n')
						first_return++;

					strcpy(tmp2, (char *)menu_options[browsing] + first_return);

					if (menu_editing[0] != 0)
						menu_editing[strlen((char *)menu_editing) - 1] = 0;

					if (menu_option_type[browsing][menu_selected[browsing]] == 35) {
						sprintf(tmp, "IP: %s_%s", menu_editing, tmp2);
					} else {
						sprintf(tmp, "%s_%s", menu_editing, tmp2);
					} 

					if (menu_option_type[browsing][menu_selected[browsing]] == 24) {
						unsigned int i;

						for (i = 0;i < strlen((char *)menu_editing);i++)
							tmp[i] = '*';
					} 

					delete menu_options[browsing];

					menu_options[browsing] = 0;

					menu_options[browsing] = new char[strlen(tmp) + 1];

					strcpy(menu_options[browsing], tmp);

					Sound_play(S_menu_move, MIX_MAX_VOLUME);

					retest = true;
				} 


				if (retest) {
					if (menu_option_type[browsing][menu_selected[browsing]] == 2) {
						List<char> names;
						char *name;
						int nplayers;
						bool found = false;

#ifdef F1SPIRIT_DEBUG_MESSAGES

						output_debug_message("Testing whether the player name is already taken...\n", menu_editing);
#endif

						nplayers = get_player_list(&names);

						names.Rewind();
						menu_editing_valid = true;

						while (names.Iterate(name)) {
							if (strcmp((char *)menu_editing, name) == 0) {
#ifdef F1SPIRIT_DEBUG_MESSAGES
								output_debug_message("Yes, the player name already exists!\n", menu_editing);
#endif

								found = true;
							} 
						} 

#ifdef F1SPIRIT_DEBUG_MESSAGES
						output_debug_message("End of test\n", menu_editing);

#endif

						if (found)
							menu_editing_valid = false;

						if (menu_editing[0] == 0)
							menu_editing_valid = false;

						{
							int i;
							bool found = false;

							for (i = 0;menu_editing[i] != 0;i++) {
								if (menu_editing[i] >= 'a' && menu_editing[i] <= 'z')
									found = true;

								if (menu_editing[i] >= 'A' && menu_editing[i] <= 'Z')
									found = true;

								if (menu_editing[i] >= '0' && menu_editing[i] <= '9')
									found = true;
							} 

							if (!found)
								menu_editing_valid = false;
						}
					} 

					if (menu_option_type[browsing][menu_selected[browsing]] == 22) {
						bool found = false;

#ifdef F1SPIRIT_DEBUG_MESSAGES

						output_debug_message("Testing whether the design name is already taken...\n", menu_editing);
#endif

						menu_editing_valid = true;
						{
							int type;
							char *folders[6] = {"designs/stock",
							                    "designs/rally",
							                    "designs/f3",
							                    "designs/f3000",
							                    "designs/endurance",
							                    "designs/f1"
							                   };
							char filename[256];
							FILE *fp;

							type = min(menu_selected_track, 5);
							sprintf(filename, "%s/%s.car", folders[type], menu_editing);
							fp = f1open(filename, "r", USERDATA);

							if (fp != 0) {
								fclose(fp);
								found = true;
#ifdef F1SPIRIT_DEBUG_MESSAGES

								output_debug_message("Yes, the design name already exists!\n", menu_editing);
#endif

							} 
						}


#ifdef F1SPIRIT_DEBUG_MESSAGES
						output_debug_message("End of test\n", menu_editing);
#endif

						if (found)
							menu_editing_valid = false;

						if (menu_editing[0] == 0)
							menu_editing_valid = false;

						{
							int i;
							bool found = false;

							for (i = 0;menu_editing[i] != 0;i++) {
								if (menu_editing[i] >= 'a' && menu_editing[i] <= 'z')
									found = true;

								if (menu_editing[i] >= 'A' && menu_editing[i] <= 'Z')
									found = true;

								if (menu_editing[i] >= '0' && menu_editing[i] <= '9')
									found = true;
							} 

							if (!found)
								menu_editing_valid = false;
						}
					} 

					if (menu_option_type[browsing][menu_selected[browsing]] == 24 ||
					        menu_option_type[browsing][menu_selected[browsing]] == 29 ||
					        menu_option_type[browsing][menu_selected[browsing]] == 31 ||
					        menu_option_type[browsing][menu_selected[browsing]] == 32 ||
					        menu_option_type[browsing][menu_selected[browsing]] == 34) {

						menu_editing_valid = true;

						if (menu_editing[0] == 0)
							menu_editing_valid = false;

						{
							int i;
							bool found = false;

							for (i = 0;menu_editing[i] != 0;i++) {
								if (menu_editing[i] >= 'a' && menu_editing[i] <= 'z')
									found = true;

								if (menu_editing[i] >= 'A' && menu_editing[i] <= 'Z')
									found = true;

								if (menu_editing[i] >= '0' && menu_editing[i] <= '9')
									found = true;
							} 

							if (!found)
								menu_editing_valid = false;
						}
					} 

					/* Check whether the entered text is a valid IP: */
					if (menu_option_type[browsing][menu_selected[browsing]] == 35) {
						int ip[4] = {0, 0, 0, 0};
						int i, j;

						i = 0;
						j = 3;

						while (menu_editing[i] != 0 && j >= 0) {
							if (menu_editing[i] == '.')
								j--;

							if (menu_editing[i] >= '0' && menu_editing[i] <= '9')
								ip[j] = ip[j] * 10 + int(menu_editing[i] - '0');

							i++;
						} 

						if (menu_editing[i] == 0 && j == 0 &&
						        ip[0] <= 255 &&
						        ip[1] <= 255 &&
						        ip[2] <= 255 &&
						        ip[3] <= 255)
							menu_editing_valid = true;
						else
							menu_editing_valid = false;
					} 

				} 
			} 

			/* Selecting TRACK: */
			if (menu_current_menu == 9 || menu_current_menu == 10) {
				int track = -1;

				if (menu_current_menu == 9) {
					if (menu_selected[browsing] < menu_noptions[browsing] - 1)
						track = menu_selected[browsing];

					if (track >= 5)
						track = -1;
				} 

				if (menu_current_menu == 10) {
					if (menu_selected[browsing] < menu_noptions[browsing] - 1)
						track = menu_selected[browsing] + 5;
				} 

				if (menu_showing_track != track) {
					char *names[N_TRACKS] = {"graphics/track-previews/stock.png",
					                         "graphics/track-previews/rally.png",
					                         "graphics/track-previews/f3.png",
					                         "graphics/track-previews/f3000.png",
					                         "graphics/track-previews/endurance.png",
					                         "graphics/track-previews/f1-1.png",
					                         "graphics/track-previews/f1-2.png",
					                         "graphics/track-previews/f1-3.png",
					                         "graphics/track-previews/f1-4.png",
					                         "graphics/track-previews/f1-5.png",
					                         "graphics/track-previews/f1-6.png",
					                         "graphics/track-previews/f1-7.png",
					                         "graphics/track-previews/f1-8.png",
					                         "graphics/track-previews/f1-9.png",
					                         "graphics/track-previews/f1-10.png",
					                         "graphics/track-previews/f1-11.png",
					                         "graphics/track-previews/f1-12.png",
					                         "graphics/track-previews/f1-13.png",
					                         "graphics/track-previews/f1-14.png",
					                         "graphics/track-previews/f1-15.png",
					                         "graphics/track-previews/f1-16.png",
					                         "graphics/track-previews/oval.png",
					                         "graphics/track-previews/oval.png",
					                         "graphics/track-previews/micro.png",
					                         "graphics/track-previews/micro.png"
					                        };


					if (track >= 0 && track < N_TRACKS) {
						if (menu_track == 0) {
							menu_showing_track_timmer = 0;
							menu_showing_track_timmer2 = 0;
						} 

						if (menu_track != 0)
							delete menu_track;

						menu_track = new GLTile(names[track]);

						if (menu_track_viewer != 0)
							delete menu_track_viewer;

						menu_track_viewer = 0;

						menu_showing_track_timmer2 = 0;

						if (menu_showing_track == -1)
							menu_showing_track_timmer = 1;

						menu_showing_track = track;

						delete menu_points_frame;

						menu_points_frame = 0;

						if (menu_option_type[browsing][menu_selected[browsing]] >= 0) {
							menu_track_locked = false;
						} else {
							menu_track_locked = true;
						} 
					} else {
						if (menu_showing_track_timmer >= 0)
							menu_showing_track_timmer = -1;
					} 
				} 

			} else {
				if (menu_showing_track != -1 && menu_showing_track_timmer >= 0) {
					menu_showing_track_timmer = -1;
				} 
			} 


			/* Selecting PREVIOUS MADE CAR: */
			if (menu_current_menu == 12) {
				int car_type = menu_selected_track;

				if (car_type > 5)
					car_type = 5;

				if (menu_selected_track == 21)
					car_type = 0;

				if (menu_selected_track == 22)
					car_type = 5;

				if (menu_selected_track == 23)
					car_type = 0;

				if (menu_selected_track == 24)
					car_type = 5;

				if (menu_selected[browsing] < menu_noptions[browsing] - 1)
					menu_showing_car = menu_selected[browsing];
				else
					menu_showing_car = -1;

				if (menu_showing_car_type != car_type) {
					char *names1[18] = {"graphics/car-stock-1-top.png",
					                    "graphics/car-stock-2-top.png",
					                    "graphics/car-stock-3-top.png",

					                    "graphics/car-rally-1-top.png",
					                    "graphics/car-rally-2-top.png",
					                    "graphics/car-rally-3-top.png",

					                    "graphics/car-f3-1-top.png",
					                    "graphics/car-f3-2-top.png",
					                    "graphics/car-f3-3-top.png",

					                    "graphics/car-f3000-1-top.png",
					                    "graphics/car-f3000-2-top.png",
					                    "graphics/car-f3000-3-top.png",

					                    "graphics/car-endurance-1-top.png",
					                    "graphics/car-endurance-2-top.png",
					                    "graphics/car-endurance-3-top.png",

					                    "graphics/car-f1-1-top.png",
					                    "graphics/car-f1-2-top.png",
					                    "graphics/car-f1-3-top.png"
					                   };

					char *names2[18] = {"graphics/car-stock-1-side.png",
					                    "graphics/car-stock-2-side.png",
					                    "graphics/car-stock-3-side.png",

					                    "graphics/car-rally-1-side.png",
					                    "graphics/car-rally-2-side.png",
					                    "graphics/car-rally-3-side.png",

					                    "graphics/car-f3-1-side.png",
					                    "graphics/car-f3-2-side.png",
					                    "graphics/car-f3-3-side.png",

					                    "graphics/car-f3000-1-side.png",
					                    "graphics/car-f3000-2-side.png",
					                    "graphics/car-f3000-3-side.png",

					                    "graphics/car-endurance-1-side.png",
					                    "graphics/car-endurance-2-side.png",
					                    "graphics/car-endurance-3-side.png",

					                    "graphics/car-f1-1-side.png",
					                    "graphics/car-f1-2-side.png",
					                    "graphics/car-f1-3-side.png"
					                   };

					if (car_type >= 0 && car_type < 6) {
						int i;

						if (menu_car_top[0] == 0)
							menu_showing_car_timmer = 0;

						for (i = 0;i < 3;i++) {
							if (menu_car_top[i] != 0)
								delete menu_car_top[i];

							if (menu_car_side[i] != 0)
								delete menu_car_side[i];

							menu_car_top[i] = new GLTile(names1[car_type * 3 + i]);

							menu_car_side[i] = new GLTile(names2[car_type * 3 + i]);
						} 

						if (menu_showing_car_type == -1)
							menu_showing_car_timmer = 1;

						menu_showing_car_type = car_type;
					} else {
						if (menu_showing_car_timmer >= 0)
							menu_showing_car_timmer = -1;
					} 
				} 

			} else {
				if (menu_showing_car_type != -1 && menu_showing_car_timmer >= 0)
					menu_showing_car_timmer = -1;
			} 


			/* Selecting PERSONAL DESIGN CAR PIECES: */
			if (menu_current_menu == 14 ||
			        menu_current_menu == 15 ||
			        menu_current_menu == 16 ||
			        menu_current_menu == 17 ||
			        menu_current_menu == 18) {
				int piece_type = (menu_current_menu - 14);

				if (menu_selected[browsing] < menu_noptions[browsing] - 1)
					menu_showing_piece = menu_selected[browsing];
				else
					menu_showing_piece = -1;

				if (menu_showing_piece_type != piece_type) {
					char *names[9] = { "graphics/brake-1.png",
					                   "graphics/brake-2.png",
					                   "graphics/brake-3.png",

					                   "graphics/suspension-1.png",
					                   "graphics/suspension-2.png",
					                   "graphics/suspension-3.png",

					                   "graphics/gear-1.png",
					                   "graphics/gear-2.png",
					                   "graphics/gear-3.png"
					                 };
					char *engines[6] = {"graphics/engine-1.png",
					                    "graphics/engine-2.png",
					                    "graphics/engine-3.png",
					                    "graphics/engine-4.png",
					                    "graphics/engine-5.png",
					                    "graphics/engine-6.png"
					                   };

					if (piece_type >= 0 && piece_type < 6) {
						int i;

						if (menu_piece[0] == 0)
							menu_showing_piece_timmer = 0;

						if (piece_type == 0) {
							int car_type = menu_selected_track;

							if (car_type > 5)
								car_type = 5;

							char *bnames[18] = {"graphics/car-stock-1-side.png",
							                    "graphics/car-stock-2-side.png",
							                    "graphics/car-stock-3-side.png",

							                    "graphics/car-rally-1-side.png",
							                    "graphics/car-rally-2-side.png",
							                    "graphics/car-rally-3-side.png",

							                    "graphics/car-f3-1-side.png",
							                    "graphics/car-f3-2-side.png",
							                    "graphics/car-f3-3-side.png",

							                    "graphics/car-f3000-1-side.png",
							                    "graphics/car-f3000-2-side.png",
							                    "graphics/car-f3000-3-side.png",

							                    "graphics/car-endurance-1-side.png",
							                    "graphics/car-endurance-2-side.png",
							                    "graphics/car-endurance-3-side.png",

							                    "graphics/car-f1-1-side.png",
							                    "graphics/car-f1-2-side.png",
							                    "graphics/car-f1-3-side.png"
							                   };

							for (i = 0;i < 3;i++) {
								if (menu_piece[i] != 0)
									delete menu_piece[i];

								menu_piece[i] = new GLTile(bnames[car_type * 3 + i]);
							} 

						} else {
							if (piece_type == 1) {
								for (i = 0;i < 6;i++) {
									if (menu_piece[i] != 0)
										delete menu_piece[i];

									menu_piece[i] = new GLTile(engines[i]);
								} 
							} else {
								for (i = 0;i < 3;i++) {
									if (menu_piece[i] != 0)
										delete menu_piece[i];

									menu_piece[i] = new GLTile(names[(piece_type - 2) * 3 + i]);
								} 
							} 
						} 

						if (menu_showing_piece_type == -1)
							menu_showing_piece_timmer = 1;

						menu_showing_piece_type = piece_type;
					} else {
						if (menu_showing_piece_timmer >= 0)
							menu_showing_piece_timmer = -1;
					} 
				} 

			} else {
				if (menu_showing_piece_type != -1 && menu_showing_piece_timmer >= 0)
					menu_showing_piece_timmer = -1;
			} 


			/* Selecting PERSONAL DESIGN CAR: */
			if (menu_current_menu == 13 && menu_design == 0) {

				char *bnames[18] = {"graphics/car-stock-1-side.png",
				                    "graphics/car-stock-2-side.png",
				                    "graphics/car-stock-3-side.png",

				                    "graphics/car-rally-1-side.png",
				                    "graphics/car-rally-2-side.png",
				                    "graphics/car-rally-3-side.png",

				                    "graphics/car-f3-1-side.png",
				                    "graphics/car-f3-2-side.png",
				                    "graphics/car-f3-3-side.png",

				                    "graphics/car-f3000-1-side.png",
				                    "graphics/car-f3000-2-side.png",
				                    "graphics/car-f3000-3-side.png",

				                    "graphics/car-endurance-1-side.png",
				                    "graphics/car-endurance-2-side.png",
				                    "graphics/car-endurance-3-side.png",

				                    "graphics/car-f1-1-side.png",
				                    "graphics/car-f1-2-side.png",
				                    "graphics/car-f1-3-side.png"
				                   };
				char *names[9] = { "graphics/brake-1.png",
				                   "graphics/brake-2.png",
				                   "graphics/brake-3.png",

				                   "graphics/suspension-1.png",
				                   "graphics/suspension-2.png",
				                   "graphics/suspension-3.png",

				                   "graphics/gear-1.png",
				                   "graphics/gear-2.png",
				                   "graphics/gear-3.png"
				                 };
				char *engines[6] = {"graphics/engine-1.png",
				                    "graphics/engine-2.png",
				                    "graphics/engine-3.png",
				                    "graphics/engine-4.png",
				                    "graphics/engine-5.png",
				                    "graphics/engine-6.png"
				                   };


				int w = 0, h = 0;
				SDL_Surface *sfc;
				SDL_Surface *parts[5] = {0, 0, 0, 0, 0};
				int car_type = menu_selected_track;

				if (car_type > 5)
					car_type = 5;

				parts[0] = IMG_Load(bnames[car_type * 3 + menu_selected_part[menu_selecting_player][0]]);

				parts[1] = IMG_Load(engines[menu_selected_part[menu_selecting_player][1]]);

				parts[2] = IMG_Load(names[menu_selected_part[menu_selecting_player][2]]);

				parts[3] = IMG_Load(names[1 * 3 + menu_selected_part[menu_selecting_player][3]]);

				parts[4] = IMG_Load(names[2 * 3 + menu_selected_part[menu_selecting_player][4]]);

				w = max(parts[1]->w, parts[2]->w) + 32 +
				    parts[0]->w + 32 +
				    max(parts[3]->w, parts[4]->w);

				h = max(parts[1]->h, parts[2]->h) + 16 +
				    max(parts[3]->h, parts[4]->h);

				sfc = SDL_CreateRGBSurface(SDL_SWSURFACE, w + 32, h + 32, 32, RMASK, GMASK, BMASK, AMASK);

				SDL_FillRect(sfc, 0, 0);

				draw_menu_frame(sfc, 0, 0, w + 32, h + 32);

				{
					SDL_Rect r;

					r.x = 16 +
					      max(parts[1]->w, parts[2]->w) + 32;
					r.y = 16 + h / 2 - parts[0]->h / 2;
					SDL_BlitSurface(parts[0], 0, sfc, &r);

					r.x = 16;
					r.y = 16;
					SDL_BlitSurface(parts[1], 0, sfc, &r);
					r.x = 16;
					r.y = 32 + max(parts[1]->h, parts[2]->h);
					SDL_BlitSurface(parts[2], 0, sfc, &r);

					r.x = 16 +
					      max(parts[1]->w, parts[2]->w) + 32 +
					      parts[0]->w + 32;
					r.y = 16;
					SDL_BlitSurface(parts[3], 0, sfc, &r);
					r.x = 16 +
					      max(parts[1]->w, parts[2]->w) + 32 +
					      parts[0]->w + 32;
					r.y = 32 + max(parts[1]->h, parts[2]->h);
					SDL_BlitSurface(parts[4], 0, sfc, &r);
				}

				SDL_FreeSurface(parts[0]);

				SDL_FreeSurface(parts[1]);
				SDL_FreeSurface(parts[2]);
				SDL_FreeSurface(parts[3]);
				SDL_FreeSurface(parts[4]);

				menu_design = new GLTile(sfc);
				menu_design->set_hotspot(w / 2 + 16, 0);
				//    SDL_FreeSurface(sfc);

				menu_showing_design_timmer = 1;

			} else {
				if (menu_current_menu != 13 && menu_showing_design_timmer >= 0)
					menu_showing_design_timmer = -1;
			} 


		} else {
			int i;

			for (i = 0;i < SDLK_LAST;i++) {
				if (k->keyboard[i] && !k->old_keyboard[i]) {
					current_player->set_key(menu_selected[0] - 2, menu_selected[1], i);
					menu_redefining_key = false;
					menu_force_rebuild_menu = true;
					Sound_play(S_menu_select, MIX_MAX_VOLUME);
					{
						FILE *fp;

						fp = f1open(player_filename, "wb", USERDATA);

						if (fp != 0) {
							current_player->save(fp);
							fclose(fp);
						} 
					}
				} 
			} 
		} 
	} 


	if (menu_track != 0) {
		menu_showing_track_timmer2++;

		if (menu_track_viewer != 0) {
			menu_track_viewer->cycle();

			if (menu_track_viewer->get_nlaps() > 0 && menu_showing_track_timmer2 < 10000)
				menu_showing_track_timmer2 = 10000;
		} 

		if (menu_showing_track_timmer2 > 10000 + TRACKPREVIEW_CONSTANT) {
			menu_showing_track_timmer2 = 0;
			delete menu_track_viewer;
			menu_track_viewer = 0;
		} 
	} 

	if (menu_showing_track_timmer > 0 &&
	        menu_showing_track_timmer < MENU_CONSTANT)
		menu_showing_track_timmer++;

	if (menu_showing_track_timmer < 0 &&
	        menu_showing_track_timmer > -MENU_CONSTANT)
		menu_showing_track_timmer--;

	if (menu_showing_track_timmer == -MENU_CONSTANT) {
		delete menu_track;
		menu_track = 0;
		menu_showing_track = -1;
		menu_showing_track_timmer = 0;
	} 

	if (menu_showing_track_timmer == MENU_CONSTANT) {
		menu_showing_track_timmer = 0;
	} 

	if (menu_showing_car_timmer > 0 &&
	        menu_showing_car_timmer < MENU_CONSTANT)
		menu_showing_car_timmer++;

	if (menu_showing_car_timmer < 0 &&
	        menu_showing_car_timmer > -MENU_CONSTANT)
		menu_showing_car_timmer--;

	if (menu_showing_car_timmer == -MENU_CONSTANT) {
		int i;

		for (i = 0;i < 3;i++) {
			delete menu_car_top[i];
			menu_car_top[i] = 0;
			delete menu_car_side[i];
			menu_car_side[i] = 0;
			delete menu_car_frame[i];
			menu_car_frame[i] = 0;
		} 

		menu_showing_car_type = -1;

		menu_showing_car_timmer = 0;
	} 

	if (menu_showing_car_timmer == MENU_CONSTANT) {
		menu_showing_car_timmer = 0;
	} 

	if (menu_showing_piece_timmer > 0 &&
	        menu_showing_piece_timmer < MENU_CONSTANT)
		menu_showing_piece_timmer++;

	if (menu_showing_piece_timmer < 0 &&
	        menu_showing_piece_timmer > -MENU_CONSTANT)
		menu_showing_piece_timmer--;

	if (menu_showing_piece_timmer == -MENU_CONSTANT) {
		int i;

		for (i = 0;i < 6;i++) {
			delete menu_piece[i];
			menu_piece[i] = 0;
			delete menu_piece_frame[i];
			menu_piece_frame[i] = 0;
		} 

		menu_showing_piece_type = -1;

		menu_showing_piece_timmer = 0;
	} 

	if (menu_showing_piece_timmer == MENU_CONSTANT) {
		menu_showing_piece_timmer = 0;
	} 

	if (menu_showing_design_timmer > 0 &&
	        menu_showing_design_timmer < MENU_CONSTANT)
		menu_showing_design_timmer++;

	if (menu_showing_design_timmer < 0 &&
	        menu_showing_design_timmer > -MENU_CONSTANT)
		menu_showing_design_timmer--;

	if (menu_showing_design_timmer == -MENU_CONSTANT) {
		delete menu_design;
		menu_design = 0;
		menu_showing_design_timmer = 0;
	} 

	if (menu_showing_design_timmer == MENU_CONSTANT) {
		menu_showing_design_timmer = 0;
	} 

	menu_showing_piece_x = (menu_desired_showing_piece_x + 3 * menu_showing_piece_x) / 4;

	menu_selected_timmer[0]++;
	menu_selected_timmer[1]++;

	return APP_STATE_MENU;
} 

