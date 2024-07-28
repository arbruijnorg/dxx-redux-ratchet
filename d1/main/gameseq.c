/*
THE COMPUTER CODE CONTAINED HEREIN IS THE SOLE PROPERTY OF PARALLAX
SOFTWARE CORPORATION ("PARALLAX").  PARALLAX, IN DISTRIBUTING THE CODE TO
END-USERS, AND SUBJECT TO ALL OF THE TERMS AND CONDITIONS HEREIN, GRANTS A
ROYALTY-FREE, PERPETUAL LICENSE TO SUCH END-USERS FOR USE BY SUCH END-USERS
IN USING, DISPLAYING,  AND CREATING DERIVATIVE WORKS THEREOF, SO LONG AS
SUCH USE, DISPLAY OR CREATION IS FOR NON-COMMERCIAL, ROYALTY OR REVENUE
FREE PURPOSES.  IN NO EVENT SHALL THE END-USER USE THE COMPUTER CODE
CONTAINED HEREIN FOR REVENUE-BEARING PURPOSES.  THE END-USER UNDERSTANDS
AND AGREES TO THE TERMS HEREIN AND ACCEPTS THE SAME BY USE OF THIS FILE.
COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

/*
 *
 * Routines for EndGame, EndLevel, etc.
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <time.h>

#include "inferno.h"
#include "game.h"
#include "key.h"
#include "object.h"
#include "physics.h"
#include "dxxerror.h"
#include "joy.h"
#include "iff.h"
#include "pcx.h"
#include "timer.h"
#include "render.h"
#include "laser.h"
#include "screens.h"
#include "textures.h"
#include "slew.h"
#include "gauges.h"
#include "texmap.h"
#include "3d.h"
#include "effects.h"
#include "menu.h"
#include "gameseg.h"
#include "wall.h"
#include "ai.h"
#include "hostage.h"
#include "fuelcen.h"
#include "switch.h"
#include "digi.h"
#include "gamesave.h"
#include "scores.h"
#include "u_mem.h"
#include "palette.h"
#include "morph.h"
#include "lighting.h"
#include "newdemo.h"
#include "titles.h"
#include "collide.h"
#include "weapon.h"
#include "sounds.h"
#include "args.h"
#include "gameseq.h"
#include "gamefont.h"
#include "newmenu.h"
#include "endlevel.h"
#include "playsave.h"
#include "ctype.h"
#include "multi.h"
#include "fireball.h"
#include "kconfig.h"
#include "config.h"
#include "robot.h"
#include "automap.h"
#include "cntrlcen.h"
#include "powerup.h"
#include "text.h"
#include "piggy.h"
#include "texmerge.h"
#include "paging.h"
#include "mission.h"
#include "state.h"
#include "songs.h"
#ifdef NETWORK
#include "multi.h"
#endif
#include "strutil.h"
#ifdef EDITOR
#include "editor/editor.h"
#endif
#ifdef OGL
#include "ogl_init.h"
#endif
#include "custom.h"
#include "byteswap.h"
#include "segment.h"
#include "gameseg.h"
#include "multibot.h"
#include "player.h"
#include "math.h"

void init_player_stats_new_ship(ubyte pnum);
void copy_defaults_to_robot_all(void);
int AdvanceLevel(int secret_flag);
void StartLevel(int random);

//Current_level_num starts at 1 for the first level
//-1,-2,-3 are secret levels
//0 means not a real level loaded
int	Current_level_num=0,Next_level_num;
char	Current_level_name[LEVEL_NAME_LEN];

// #ifndef SHAREWARE
// int Last_level,Last_secret_level;
// #endif

// Global variables describing the player
int	N_players=1;	// Number of players ( >1 means a net game, eh?)
int 	Player_num=0;	// The player number who is on the console.
player			Players[MAX_PLAYERS];			// Misc player info
ranking Ranking; // Ranking system mod variables.
obj_position	Player_init[MAX_PLAYERS];

// Global variables telling what sort of game we have
int NumNetPlayerPositions = -1;

extern fix ThisLevelTime;

// Extern from game.c to fix a bug in the cockpit!

extern int last_drawn_cockpit;
extern int Last_level_path_created;

void HUD_clear_messages(); // From hud.c


void verify_console_object()
{
	Assert( Player_num > -1 );
	Assert( Players[Player_num].objnum > -1 );
	ConsoleObject = &Objects[Players[Player_num].objnum];
	Assert( ConsoleObject->id==Player_num );
}

int count_number_of_robots()
{
	int robot_count;
	int i;

	robot_count = 0;
	for (i=0;i<=Highest_object_index;i++) {
		if (Objects[i].type == OBJ_ROBOT)
			robot_count++;
	}

	return robot_count;
}


int count_number_of_hostages()
{
	int count;
	int i;

	count = 0;
	for (i=0;i<=Highest_object_index;i++) {
		if (Objects[i].type == OBJ_HOSTAGE)
			count++;
	}

	return count;
}


void
gameseq_init_network_players()
{
	int i,k,j;

	// Initialize network player start locations and object numbers

	ConsoleObject = &Objects[0];
	k = 0;
	j = 0;
	for (i=0;i<=Highest_object_index;i++) {

		if (( Objects[i].type==OBJ_PLAYER )	|| (Objects[i].type == OBJ_GHOST) || (Objects[i].type == OBJ_COOP))
		{
#ifndef SHAREWARE
			if ( (!(Game_mode & GM_MULTI_COOP) && ((Objects[i].type == OBJ_PLAYER)||(Objects[i].type==OBJ_GHOST))) ||
	           ((Game_mode & GM_MULTI_COOP) && ((j == 0) || ( Objects[i].type==OBJ_COOP ) )) )
			{

				Objects[i].type=OBJ_PLAYER;
#endif
				Player_init[k].pos = Objects[i].pos;
				Player_init[k].orient = Objects[i].orient;
				Player_init[k].segnum = Objects[i].segnum;
				Players[k].objnum = i;
				Objects[i].id = k;
				k++;
#ifndef SHAREWARE
			}
			else
				obj_delete(i);
			j++;
#endif
		}
	}

	NumNetPlayerPositions = k;

	if (Game_mode & GM_MULTI) {
		// Ensure we have 8 starting locations, even if there aren't 8 in the file.  This makes observer mode work in all levels.
		for (; k < MAX_PLAYERS; k++) {
			i = obj_allocate();

			Objects[i].type = OBJ_PLAYER;

			Player_init[k].pos = Objects[k % NumNetPlayerPositions].pos;
			Player_init[k].orient = Objects[k % NumNetPlayerPositions].orient;
			Player_init[k].segnum = Objects[k % NumNetPlayerPositions].segnum;
			Players[k].objnum = i;
			Objects[i].id = k;

			Objects[i].attached_obj = -1;
		}

		NumNetPlayerPositions = MAX_PLAYERS;
	}
}

void gameseq_remove_unused_players()
{
	int i;

	// 'Remove' the unused players

#ifdef NETWORK
	if (Game_mode & GM_MULTI)
	{
		for (i=0; i < NumNetPlayerPositions; i++)
		{
			if ((i == 0 && Netgame.host_is_obs) || (!Players[i].connected) || (i >= N_players))
			{
				multi_make_player_ghost(i);
			}
		}
	}
	else
#endif
	{		// Note link to above if!!!
		for (i=1; i < NumNetPlayerPositions; i++)
		{
			obj_delete(Players[i].objnum);
		}
	}
}

fix StartingShields=INITIAL_SHIELDS;

// Setup player for new game
void init_player_stats_game(ubyte pnum)
{
	Players[pnum].score = 0;
	Players[pnum].last_score = 0;
	Players[pnum].lives = INITIAL_LIVES;
	Players[pnum].level = 1;
	Players[pnum].time_level = 0;
	Players[pnum].time_total = 0;
	Players[pnum].hours_level = 0;
	Players[pnum].hours_total = 0;
	Players[pnum].killer_objnum = -1;
	Players[pnum].net_killed_total = 0;
	Players[pnum].net_kills_total = 0;
	Players[pnum].num_kills_level = 0;
	Players[pnum].num_kills_total = 0;
	Players[pnum].num_robots_level = 0;
	Players[pnum].num_robots_total = 0;
	Players[pnum].KillGoalCount = 0;
	Players[pnum].hostages_rescued_total = 0;
	Players[pnum].hostages_level = 0;
	Players[pnum].hostages_total = 0;
	Players[pnum].laser_level = 0;
	Players[pnum].flags = 0;
	Players[pnum].shields_delta = 0;
	Players[pnum].shields_time = 0;

	init_player_stats_new_ship(pnum);

	if (pnum == Player_num && Game_mode & GM_MULTI)
		multi_send_ship_status();
}

void init_ammo_and_energy(void)
{
	if (Players[Player_num].energy < INITIAL_ENERGY)
		Players[Player_num].energy = INITIAL_ENERGY;
	if (Players[Player_num].shields < StartingShields)
		Players[Player_num].shields = StartingShields;

//	for (i=0; i<MAX_PRIMARY_WEAPONS; i++)
//		if (Players[Player_num].primary_ammo[i] < Default_primary_ammo_level[i])
//			Players[Player_num].primary_ammo[i] = Default_primary_ammo_level[i];

//	for (i=0; i<MAX_SECONDARY_WEAPONS; i++)
//		if (Players[Player_num].secondary_ammo[i] < Default_secondary_ammo_level[i])
//			Players[Player_num].secondary_ammo[i] = Default_secondary_ammo_level[i];
	if (Players[Player_num].secondary_ammo[0] < 2 + NDL - Difficulty_level)
		Players[Player_num].secondary_ammo[0] = 2 + NDL - Difficulty_level;
}

// Setup player for new level (After completion of previous level)
void init_player_stats_level(int secret_flag)
{
	(void)secret_flag;
	// int	i;

	Players[Player_num].last_score = Players[Player_num].score;

	Players[Player_num].level = Current_level_num;

#ifdef NETWORK
	if (!Network_rejoined)
#endif
		Players[Player_num].time_level = 0;	//Note link to above if !!!!!!

	init_ammo_and_energy();

	Players[Player_num].killer_objnum = -1;

	Players[Player_num].num_kills_level = 0;
	Players[Player_num].num_robots_level = count_number_of_robots();
	Players[Player_num].num_robots_total += Players[Player_num].num_robots_level;

	Players[Player_num].hostages_level = count_number_of_hostages();
	Players[Player_num].hostages_total += Players[Player_num].hostages_level;
	Players[Player_num].hostages_on_board = 0;

		Players[Player_num].flags &= (~KEY_BLUE);
		Players[Player_num].flags &= (~KEY_RED);
		Players[Player_num].flags &= (~KEY_GOLD);

	Players[Player_num].flags &= (~PLAYER_FLAGS_INVULNERABLE);
	Players[Player_num].flags &= (~PLAYER_FLAGS_CLOAKED);

		Players[Player_num].cloak_time = 0;
		Players[Player_num].invulnerable_time = 0;

		if ((Game_mode & GM_MULTI) && !(Game_mode & GM_MULTI_COOP))
			Players[Player_num].flags |= (KEY_BLUE | KEY_RED | KEY_GOLD);

	Player_is_dead = 0; // Added by RH
	Dead_player_camera = NULL;
	Players[Player_num].homing_object_dist = -F1_0; // Added by RH

	// properly init these cursed globals
	Next_flare_fire_time = Last_laser_fired_time = Next_laser_fire_time = Next_missile_fire_time = GameTime64;

	init_gauges();

	if (Game_mode & GM_MULTI)
		multi_send_ship_status();
}

// Setup player for a brand-new ship
void init_player_stats_new_ship(ubyte pnum)
{
	int i;

	if (pnum == Player_num)
	{
		if (Newdemo_state == ND_STATE_RECORDING)
		{
			newdemo_record_laser_level(Players[Player_num].laser_level, 0);
			newdemo_record_player_weapon(0, 0);
			newdemo_record_player_weapon(1, 0);
		}
		Global_laser_firing_count=0;
		Players[Player_num].primary_weapon = 0;
		Players[Player_num].secondary_weapon = 0;
		dead_player_end(); //player no longer dead
		Player_is_dead = 0;
		Player_exploded = 0;
		Player_eggs_dropped = 0;

		int delete_camera = 1; 
#ifdef NETWORK	
		if ((Game_mode & GM_MULTI) && (Netgame.SpawnStyle == SPAWN_STYLE_PREVIEW)) {	
			delete_camera = 0; 
		}	
#endif
		if(delete_camera)
			Dead_player_camera = 0;
	}

	Players[pnum].energy = INITIAL_ENERGY;
	Players[pnum].shields = StartingShields;
    Players[pnum].shields_certain = 1;
	Players[pnum].laser_level = 0;
	Players[pnum].killer_objnum = -1;
	Players[pnum].hostages_on_board = 0;
	for (i=0; i<MAX_PRIMARY_WEAPONS; i++)
		Players[pnum].primary_ammo[i] = 0;
	for (i=1; i<MAX_SECONDARY_WEAPONS; i++)
		Players[pnum].secondary_ammo[i] = 0;
	Players[pnum].secondary_ammo[0] = 2 + NDL - Difficulty_level;
	Players[pnum].primary_weapon_flags = HAS_LASER_FLAG;
	Players[pnum].secondary_weapon_flags = HAS_CONCUSSION_FLAG;
	Players[pnum].flags &= ~(PLAYER_FLAGS_QUAD_LASERS | PLAYER_FLAGS_CLOAKED | PLAYER_FLAGS_INVULNERABLE);
	Players[pnum].cloak_time = 0;
	Players[pnum].invulnerable_time = 0;
	Players[pnum].homing_object_dist = -F1_0;

	RespawningConcussions[pnum] = 0; 
	VulcanAmmoBoxesOnBoard[pnum] = 0;
	VulcanBoxAmmo[pnum] = 0;

	digi_kill_sound_linked_to_object(Players[pnum].objnum);
	
	if (pnum == Player_num && Game_mode & GM_MULTI)
		multi_send_ship_status();
}

#ifdef EDITOR

extern int game_handler(window *wind, d_event *event, void *data);

//reset stuff so game is semi-normal when playing from editor
void editor_reset_stuff_on_level()
{
	gameseq_init_network_players();
	init_player_stats_level(0);
	Viewer = ConsoleObject;
	ConsoleObject = Viewer = &Objects[Players[Player_num].objnum];
	ConsoleObject->id=Player_num;
	ConsoleObject->control_type = CT_FLYING;
	ConsoleObject->movement_type = MT_PHYSICS;
	Game_suspended = 0;
	verify_console_object();
	Control_center_destroyed = 0;
	if (Newdemo_state != ND_STATE_PLAYBACK)
		gameseq_remove_unused_players();
	init_cockpit();
	init_robots_for_level();
	init_ai_objects();
	init_morphs();
	init_all_matcens();
	init_player_stats_new_ship(Player_num);
	if (!Game_wind)
		Game_wind = window_create(&grd_curscreen->sc_canvas, 0, 0, SWIDTH, SHEIGHT, game_handler, NULL);
}
#endif

void reset_player_object();


//do whatever needs to be done when a player dies in multiplayer

void DoGameOver()
{
#ifndef SHAREWARE
	if (PLAYING_BUILTIN_MISSION)
#endif
		scores_maybe_add_player(0);

	if (Game_wind)
		window_close(Game_wind);		// Exit out of game loop
}

//update various information about the player
void update_player_stats()
{
	Players[Player_num].time_level += FrameTime;	//the never-ending march of time...
	if ( Players[Player_num].time_level > i2f(3600) )	{
		Players[Player_num].time_level -= i2f(3600);
		Players[Player_num].hours_level++;
	}

	Players[Player_num].time_total += FrameTime;	//the never-ending march of time...
	if ( Players[Player_num].time_total > i2f(3600) )	{
		Players[Player_num].time_total -= i2f(3600);
		Players[Player_num].hours_total++;
	}
}

//go through this level and start any eclip sounds
void set_sound_sources()
{
	int segnum,sidenum;
	segment *seg;

	digi_init_sounds();		//clear old sounds

	for (seg=&Segments[0],segnum=0;segnum<=Highest_segment_index;seg++,segnum++)
		for (sidenum=0;sidenum<MAX_SIDES_PER_SEGMENT;sidenum++) {
			int tm,ec,sn;

			if ((tm=seg->sides[sidenum].tmap_num2) != 0)
				if ((ec=TmapInfo[tm&0x3fff].eclip_num)!=-1)
					if ((sn=Effects[ec].sound_num)!=-1) {
						vms_vector pnt;

						compute_center_point_on_side(&pnt,seg,sidenum);
						digi_link_sound_to_pos(sn,segnum,sidenum,&pnt,1, F1_0/2);

					}
		}

}


//fix flash_dist=i2f(1);
fix flash_dist=fl2f(.9);

//create flash for player appearance
void create_player_appearance_effect(object *player_obj)
{
	vms_vector pos;
	object *effect_obj;

#ifndef NDEBUG
	{
		int objnum = player_obj-Objects;
		if ( (objnum < 0) || (objnum > Highest_object_index) )
			Int3(); // See Rob, trying to track down weird network bug
	}
#endif

	if (player_obj == Viewer)
		vm_vec_scale_add(&pos, &player_obj->pos, &player_obj->orient.fvec, fixmul(player_obj->size,flash_dist));
	else
		pos = player_obj->pos;

	effect_obj = object_create_explosion(player_obj->segnum, &pos, player_obj->size, VCLIP_PLAYER_APPEARANCE );

	if (effect_obj) {
		effect_obj->orient = player_obj->orient;

		if ( Vclip[VCLIP_PLAYER_APPEARANCE].sound_num > -1 )
			digi_link_sound_to_object( Vclip[VCLIP_PLAYER_APPEARANCE].sound_num, effect_obj-Objects, 0, F1_0);
	}
}

//
// New Game sequencing functions
//

extern int descent_critical_error;

//get level filename. level numbers start at 1.  Secret levels are -1,-2,-3
char *get_level_file(int level_num)
{
#ifdef SHAREWARE
        {
                static char t[13];
                sprintf(t, "level%02d.sdl", level_num);
		return t;
        }
#else
        if (level_num<0)                //secret level
		return Secret_level_names[-level_num-1];
        else                                    //normal level
		return Level_names[level_num-1];
#endif
}

// routine to calculate the checksum of the segments.
void do_checksum_calc(ubyte *b, int len, unsigned int *s1, unsigned int *s2)
{

	while(len--) {
		*s1 += *b++;
		if (*s1 >= 255) *s1 -= 255;
		*s2 += *s1;
	}
}

ushort netmisc_calc_checksum()
{
	int i, j, k;
	unsigned int sum1,sum2;
	short s;
	int t;

	sum1 = sum2 = 0;
	for (i = 0; i < Highest_segment_index + 1; i++) {
		for (j = 0; j < MAX_SIDES_PER_SEGMENT; j++) {
			do_checksum_calc((unsigned char *)&(Segments[i].sides[j].type), 1, &sum1, &sum2);
			do_checksum_calc(&(Segments[i].sides[j].pad), 1, &sum1, &sum2);
			s = INTEL_SHORT(Segments[i].sides[j].wall_num);
			do_checksum_calc((ubyte *)&s, 2, &sum1, &sum2);
			s = INTEL_SHORT(Segments[i].sides[j].tmap_num);
			do_checksum_calc((ubyte *)&s, 2, &sum1, &sum2);
			s = INTEL_SHORT(Segments[i].sides[j].tmap_num2);
			do_checksum_calc((ubyte *)&s, 2, &sum1, &sum2);
			for (k = 0; k < 4; k++) {
				t = INTEL_INT(((int)Segments[i].sides[j].uvls[k].u));
				do_checksum_calc((ubyte *)&t, 4, &sum1, &sum2);
				t = INTEL_INT(((int)Segments[i].sides[j].uvls[k].v));
				do_checksum_calc((ubyte *)&t, 4, &sum1, &sum2);
				t = INTEL_INT(((int)Segments[i].sides[j].uvls[k].l));
				do_checksum_calc((ubyte *)&t, 4, &sum1, &sum2);
			}
			for (k = 0; k < 2; k++) {
				t = INTEL_INT(((int)Segments[i].sides[j].normals[k].x));
				do_checksum_calc((ubyte *)&t, 4, &sum1, &sum2);
				t = INTEL_INT(((int)Segments[i].sides[j].normals[k].y));
				do_checksum_calc((ubyte *)&t, 4, &sum1, &sum2);
				t = INTEL_INT(((int)Segments[i].sides[j].normals[k].z));
				do_checksum_calc((ubyte *)&t, 4, &sum1, &sum2);
			}
		}
		for (j = 0; j < MAX_SIDES_PER_SEGMENT; j++) {
			s = INTEL_SHORT(Segments[i].children[j]);
			do_checksum_calc((ubyte *)&s, 2, &sum1, &sum2);
		}
		for (j = 0; j < MAX_VERTICES_PER_SEGMENT; j++) {
			s = INTEL_SHORT(Segments[i].verts[j]);
			do_checksum_calc((ubyte *)&s, 2, &sum1, &sum2);
		}
		s = INTEL_SHORT(Segments[i].objects);
		do_checksum_calc((ubyte *)&s, 2, &sum1, &sum2);
		do_checksum_calc((unsigned char *)&(Segments[i].special), 1, &sum1, &sum2);
		do_checksum_calc((unsigned char *)&(Segments[i].matcen_num), 1, &sum1, &sum2);
		s = INTEL_SHORT(Segments[i].value);
		do_checksum_calc((ubyte *)&s, 2, &sum1, &sum2);
		t = INTEL_INT(((int)Segments[i].static_light));
		do_checksum_calc((ubyte *)&t, 4, &sum1, &sum2);
		s = INTEL_SHORT(0); // no matter if we need alignment on our platform, if we have editor we MUST consider this integer to get the same checksum as non-editor games calculate
		do_checksum_calc((ubyte *)&s, 2, &sum1, &sum2);
	}
	sum2 %= 255;
	return ((sum1<<8)+ sum2);
}

//load a level off disk. level numbers start at 1.  Secret levels are -1,-2,-3
void LoadLevel(int level_num,int page_in_textures)
{
	char *level_name;
	player save_player;

	save_player = Players[Player_num];

	Assert(level_num <= Last_level  && level_num >= Last_secret_level  && level_num != 0);

	level_name = get_level_file(level_num);

	if (!load_level(level_name))
		Current_level_num=level_num;

	gr_use_palette_table( "palette.256" );

	show_boxed_message(TXT_LOADING, 0);
#ifdef RELEASE
	timer_delay(F1_0);
#endif

#ifdef NETWORK
	my_segments_checksum = netmisc_calc_checksum();
#endif

	load_endlevel_data(level_num);
	
	load_custom_data(level_name);

#ifdef NETWORK
	reset_network_objects();
#endif

	Players[Player_num] = save_player;

	set_sound_sources();

	songs_play_level_song( Current_level_num, 0 );

	gr_palette_load(gr_palette);		//actually load the palette

	if ( page_in_textures )
		piggy_load_level_data();
}

//sets up Player_num & ConsoleObject
void InitPlayerObject()
{
	Assert(Player_num>=0 && Player_num<MAX_PLAYERS);

	if (Player_num != 0 )	{
		Players[0] = Players[Player_num];
		Player_num = 0;
	}

	Players[Player_num].objnum = 0;

	ConsoleObject = &Objects[Players[Player_num].objnum];

	ConsoleObject->type				= OBJ_PLAYER;
	ConsoleObject->id					= Player_num;
	ConsoleObject->control_type	= CT_FLYING;
	ConsoleObject->movement_type	= MT_PHYSICS;
}

void CalculateRank(int level_num)
{
	int levelHostages = 0;
	int levelPoints = 0;
	double parTime = 0;
	int playerPoints = 0;
	double secondsTaken = 0;
	int playerHostages = 0;
	int difficulty = 0;
	int deathCount = 0;
	double rankPoints2 = 0;
	char buffer[256];
	char filename[256];
	sprintf(filename, "ranks/%s/%s/level%d.hi", Players[Player_num].callsign, Current_mission->filename, level_num); // Find file for the requested level.
	if (level_num > Current_mission->last_level)
		sprintf(filename, "ranks/%s/%s/levelS%d.hi", Players[Player_num].callsign, Current_mission->filename, level_num - Current_mission->last_level);
	PHYSFS_file* fp = PHYSFS_openRead(filename);
	if (fp == NULL)
		rankPoints2 = -10; // If no data exists, just assume level never played and set rankPoints in the range that returns N/A.
	else {
		PHYSFSX_getsTerminated(fp, buffer); // Fetch level data starting here.
		levelHostages = atoi(buffer);
		if (levelHostages < 0) // If level is unplayed, we know because the first (and only) value in the file will be -1, which is impossible for the hostage count.
			rankPoints2 = -10;
		else {
			PHYSFSX_getsTerminated(fp, buffer);
			levelPoints = atoi(buffer);
			PHYSFSX_getsTerminated(fp, buffer);
			parTime = atof(buffer);
			PHYSFSX_getsTerminated(fp, buffer); // Fetch player data starting here.
			playerPoints = atoi(buffer);
			PHYSFSX_getsTerminated(fp, buffer);
			secondsTaken = atof(buffer);
			PHYSFSX_getsTerminated(fp, buffer);
			playerHostages = atoi(buffer);
			PHYSFSX_getsTerminated(fp, buffer);
			difficulty = atoi(buffer);
			PHYSFSX_getsTerminated(fp, buffer);
			deathCount = atoi(buffer);
		}
	}
	PHYSFS_close(fp);
	double maxScore = levelPoints * 3;
	maxScore = (int)maxScore;
	double skillPoints = 0;
	if (difficulty == 1)
		skillPoints = playerPoints * 0.25;
	if (difficulty == 2)
		skillPoints = playerPoints * 0.5;
	if (difficulty == 3)
		skillPoints = playerPoints * 0.75;
	if (difficulty == 4)
		skillPoints = playerPoints;
	double timePoints = (maxScore / 1.5) / pow(2, secondsTaken / parTime);
	if (secondsTaken < parTime)
		timePoints = (maxScore / 2.4) * (1 - (secondsTaken / parTime) * 0.2);
	double score = playerPoints + skillPoints + timePoints + playerHostages * (500 * (difficulty + 1));
	maxScore += levelHostages * 7500;
	double deathPoints = maxScore * 0.4 - maxScore * (0.4 / pow(2, deathCount));
	score -= deathPoints;
	if (playerHostages == levelHostages)
		score += levelHostages * (1000 * (difficulty + 1));
	score = (int)score;
	if (rankPoints2 > -5) {
		rankPoints2 = (score / maxScore) * 12;
	}
	if (rankPoints2 > -5 && maxScore == 0)
		rankPoints2 = 12;
	Ranking.calculatedScore = score;
	if (rankPoints2 < -5)
		Ranking.rank = 0;
	if (rankPoints2 > -5)
		Ranking.rank = 1;
	if (rankPoints2 >= 0)
		Ranking.rank = 2;
	if (rankPoints2 >= 1)
		Ranking.rank = 3;
	if (rankPoints2 >= 2)
		Ranking.rank = 4;
	if (rankPoints2 >= 3)
		Ranking.rank = 5;
	if (rankPoints2 >= 4)
		Ranking.rank = 6;
	if (rankPoints2 >= 5)
		Ranking.rank = 7;
	if (rankPoints2 >= 6)
		Ranking.rank = 8;
	if (rankPoints2 >= 7)
		Ranking.rank = 9;
	if (rankPoints2 >= 8)
		Ranking.rank = 10;
	if (rankPoints2 >= 9)
		Ranking.rank = 11;
	if (rankPoints2 >= 10)
		Ranking.rank = 12;
	if (rankPoints2 >= 11)
		Ranking.rank = 13;
	if (rankPoints2 >= 12)
		Ranking.rank = 14;
}

//starts a new game on the given level
void StartNewGame(int start_level)
{	
	PHYSFS_file* fp;
	char filename[256];
	int i = 1;
	char buffer[256];
	sprintf(buffer, "ranks/%s/%s", Players[Player_num].callsign, Current_mission->filename);
	PHYSFS_mkdir(buffer);
	while (i <= Current_mission->last_level - Current_mission->last_secret_level) {
		sprintf(filename, "ranks/%s/%s/level%d.hi", Players[Player_num].callsign, Current_mission->filename, i);
		if (i > Current_mission->last_level)
			sprintf(filename, "ranks/%s/%s/levelS%d.hi", Players[Player_num].callsign, Current_mission->filename, i - Current_mission->last_level);
		fp = PHYSFS_openRead(filename);
		if (fp == NULL) { // If this level's rank data file doesn't exist, create it now so it can be written to on the rank screen.
			fp = PHYSFS_openWrite(filename);
			PHYSFSX_printf(fp, "-1");
		}
		PHYSFS_close(fp);
		i++;
	}
	
	state_quick_item = -1;	// for first blind save, pick slot to save in

	Game_mode = GM_NORMAL;

	Next_level_num = 0;

	InitPlayerObject();					//make sure player's object set up

	init_player_stats_game(Player_num);		//clear all stats

	N_players = 1;

	StartNewLevel(start_level);

	Players[Player_num].starting_level = start_level;		// Mark where they started

	game_disable_cheats();
}

void DoEndLevelScoreGlitz(int network)
{
	if (Ranking.level_time == 0)
	Ranking.level_time = (Players[Player_num].hours_level * 3600) + ((double)Players[Player_num].time_level / 65536); // Failsafe for if this isn't updated.

	int level_points, skill_points, death_points, shield_points, energy_points, time_points, hostage_points;
	double skill_points2;
	int	all_hostage_points, endgame_points;
	char	all_hostage_text[64];
	char	endgame_text[64];
#define N_GLITZITEMS 12
	char				m_str[N_GLITZITEMS][31];
	newmenu_item	m[N_GLITZITEMS + 1];
	int				i, c;
	char				title[128];
	int				is_last_level;
	int				mine_level;

	//	Compute level player is on, deal with secret levels (negative numbers)
	mine_level = Players[Player_num].level;
	if (mine_level < 0)
		mine_level *= -(Last_level / N_secret_levels);

	level_points = Players[Player_num].score - Players[Player_num].last_score;
	Ranking.rankScore = level_points - Ranking.excludePoints;

	skill_points = 0, skill_points2 = 0;
	if (Difficulty_level == 1)
		skill_points2 = Ranking.rankScore / 4;
	if (Difficulty_level > 1) {
		skill_points = level_points * (Difficulty_level - 1) / 2;
		skill_points2 = Ranking.rankScore * ((double)Difficulty_level / 4);
	}
	skill_points -= skill_points % 100;
	skill_points2 = (int)skill_points2;

	shield_points = f2i(Players[Player_num].shields) * 10 * (Difficulty_level + 1);
	energy_points = f2i(Players[Player_num].energy) * 5 * (Difficulty_level + 1);
	time_points = (Ranking.maxScore / 1.5) / pow(2, Ranking.level_time / Ranking.parTime);
	if (Ranking.level_time < Ranking.parTime)
		time_points = (Ranking.maxScore / 2.4) * (1 - (Ranking.level_time / Ranking.parTime) * 0.2);
	Ranking.maxScore += Players[Player_num].hostages_level * 7500;
	hostage_points = Players[Player_num].hostages_on_board * 500 * (Difficulty_level + 1);

	all_hostage_text[0] = 0;
	endgame_text[0] = 0;

	if (Players[Player_num].hostages_on_board == Players[Player_num].hostages_level) {
		all_hostage_points = Players[Player_num].hostages_on_board * 1000 * (Difficulty_level + 1);
	}
	else
		all_hostage_points = 0;

	if (!cheats.enabled && !(Game_mode & GM_MULTI) && (Players[Player_num].lives) && (Current_level_num == Last_level)) {		//player has finished the game!
		endgame_points = Players[Player_num].lives * 10000;
		is_last_level = 1;
	}
	else
		endgame_points = is_last_level = 0;
	if (!cheats.enabled)
		add_bonus_points_to_score(shield_points + energy_points + skill_points + hostage_points + all_hostage_points + endgame_points);
	Ranking.rankScore += skill_points2 + time_points + hostage_points + all_hostage_points;
	death_points = Ranking.maxScore * 0.4 - Ranking.maxScore * (0.4 / pow(2, Ranking.deathCount));
	death_points *= -1;
	Ranking.rankScore += death_points;

	int minutes = Ranking.level_time / 60;
	double seconds = Ranking.level_time - minutes * 60;
	int parMinutes = Ranking.parTime / 60;
	double parSeconds = Ranking.parTime - parMinutes * 60;
	char* diffname = 0;
	char time[256];
	char parTime[256];
	c = 0;
	if (Difficulty_level == 0)
		diffname = "Trainee";
	if (Difficulty_level == 1)
		diffname = "Rookie";
	if (Difficulty_level == 2)
		diffname = "Hotshot";
	if (Difficulty_level == 3)
		diffname = "Ace";
	if (Difficulty_level == 4)
		diffname = "Insane";
	if (Ranking.quickload == 0) { // Only print the modded result screen if the player didn't quickload. Print the vanilla one otherwise, because quickloading into a level causes missing crucial mod info.
		if (seconds < 10 || seconds == 60)
			sprintf(time, "%i:0%.3f", minutes, seconds);
		else
			sprintf(time, "%i:%.3f", minutes, seconds);
		if (parSeconds < 10 || parSeconds == 60)
			sprintf(parTime, "%i:0%.0f", parMinutes, parSeconds);
		else
			sprintf(parTime, "%i:%.0f", parMinutes, parSeconds);
		sprintf(m_str[c++], "Level score:\t%.0f", level_points - Ranking.excludePoints);
		sprintf(m_str[c++], "Time: %s/%s\t%i", time, parTime, time_points);
		sprintf(m_str[c++], "Hostages: %i/%i\t%i", Players[Player_num].hostages_on_board, Players[Player_num].hostages_level, hostage_points);
		sprintf(m_str[c++], "Skill: %s\t%.0f", diffname, skill_points2);
		if (all_hostage_points > 0) {
			sprintf(m_str[c++], "Deaths: %.0f\t%i", Ranking.deathCount, death_points);
			sprintf(m_str[c++], "Full rescue bonus:\t%i\n", all_hostage_points);
		}
		else
			sprintf(m_str[c++], "Deaths: %.0f\t%i\n", Ranking.deathCount, death_points);
		sprintf(m_str[c++], "%s%0.0f", TXT_TOTAL_SCORE, Ranking.rankScore);

		double rankPoints = (Ranking.rankScore / Ranking.maxScore) * 12;
		if (Ranking.maxScore == 0)
			rankPoints = 12;
		char* rank = "E";
		if (rankPoints >= 0)
		rank = "D-";
		if (rankPoints >= 1)
		rank = "D";
		if (rankPoints >= 2)
		rank = "D+";
		if (rankPoints >= 3)
		rank = "C-";
		if (rankPoints >= 4)
		rank = "C";
		if (rankPoints >= 5)
		rank = "C+";
		if (rankPoints >= 6)
		rank = "B-";
		if (rankPoints >= 7)
		rank = "B";
		if (rankPoints >= 8)
		rank = "B+";
		if (rankPoints >= 9)
		rank = "A-";
		if (rankPoints >= 10)
		rank = "A";
		if (rankPoints >= 11)
		rank = "A+";
		if (rankPoints >= 12)
		rank = "S";

		if (cheats.enabled) {
			sprintf(m_str[c++], "Rank:\t %s (Cheated, no save)", rank);
		}
		else {
			sprintf(m_str[c++], "Rank:\t %s", rank);
			PHYSFS_File* fp;
			PHYSFS_File* temp;
			char filename[256];
			char temp_filename[256];
			sprintf(filename, "ranks/%s/%s/level%i.hi", Players[Player_num].callsign, Current_mission->filename, Current_level_num);
			if (Current_level_num < 0)
			sprintf(filename, "ranks/%s/%s/levelS%i.hi", Players[Player_num].callsign, Current_mission->filename, Current_level_num * -1);
			sprintf(temp_filename, "ranks/%s/%s/temp.hi", Players[Player_num].callsign, Current_mission->filename);
			fp = PHYSFS_openRead(filename);
			if (fp != NULL) {
				if (Current_level_num > 0) {
					CalculateRank(Current_level_num);
				}
				else {
					CalculateRank(Current_mission->last_level - Current_level_num);
				}
				if (Ranking.rankScore > Ranking.calculatedScore || Ranking.rank == 0) {
					temp = PHYSFS_openWrite(temp_filename);
					PHYSFSX_printf(temp, "%i\n", Players[Player_num].hostages_level);
					PHYSFSX_printf(temp, "%.0f\n", (Ranking.maxScore - Players[Player_num].hostages_level * 7500) / 3);
					PHYSFSX_printf(temp, "%f\n", Ranking.parTime);
					PHYSFSX_printf(temp, "%.0f\n", level_points - Ranking.excludePoints);
					PHYSFSX_printf(temp, "%f\n", Ranking.level_time);
					PHYSFSX_printf(temp, "%i\n", Players[Player_num].hostages_on_board);
					PHYSFSX_printf(temp, "%i\n", Difficulty_level);
					PHYSFSX_printf(temp, "%.0f\n", Ranking.deathCount);
					if (Ranking.rank > 0)
						sprintf(m_str[c++], "New record!");
					PHYSFS_close(temp);
					PHYSFS_close(fp);
					PHYSFS_delete(filename);
					PHYSFSX_rename(temp_filename, filename);
				}
				PHYSFS_close(fp);
			}
			else {
				sprintf(m_str[c++], "Saving error. Start new game.");
				PHYSFS_close(fp);
			}
		}
		int toRankS = Ranking.maxScore - Ranking.rankScore;
		if (rankPoints < 12) {
			strcpy(m_str[c++], "");
			sprintf(m_str[c++], "%i points to S rank", toRankS);
		}
	}
	else {
		sprintf(m_str[c++], "%s%i", TXT_SHIELD_BONUS, shield_points);
		sprintf(m_str[c++], "%s%i", TXT_ENERGY_BONUS, energy_points);
		sprintf(m_str[c++], "%s%i", TXT_HOSTAGE_BONUS, hostage_points);
		sprintf(m_str[c++], "%s%i\n", TXT_SKILL_BONUS, skill_points);
		if (!cheats.enabled && (Players[Player_num].hostages_on_board == Players[Player_num].hostages_level))
			sprintf(all_hostage_text, "%s%i\n", TXT_FULL_RESCUE_BONUS, all_hostage_points);
		if (!cheats.enabled && !(Game_mode & GM_MULTI) && (Players[Player_num].lives) && (Current_level_num == Last_level))		//player has finished the game!
			sprintf(endgame_text, "%s%i\n", TXT_SHIP_BONUS, endgame_points);
		sprintf(m_str[c++], "%s%i\n", TXT_TOTAL_BONUS, shield_points + energy_points + hostage_points + skill_points + all_hostage_points + endgame_points);
		sprintf(m_str[c++], "%s%i", TXT_TOTAL_SCORE, Players[Player_num].score);
	}

		for (i = 0; i < c; i++) {
			m[i].type = NM_TYPE_TEXT;
			m[i].text = m_str[i];
		}

		// m[c].type = NM_TYPE_MENU;	m[c++].text = "Ok";

		if (Current_level_num < 0)
			sprintf(title, "%s%s %d %s\n %s %s", is_last_level ? "\n\n\n" : "\n", TXT_SECRET_LEVEL, -Current_level_num, TXT_COMPLETE, Current_level_name, TXT_DESTROYED);
		else
			sprintf(title, "%s%s %d %s\n%s %s", is_last_level ? "\n\n\n" : "\n", TXT_LEVEL, Current_level_num, TXT_COMPLETE, Current_level_name, TXT_DESTROYED);

		Assert(c <= N_GLITZITEMS);

#ifdef NETWORK
		if (network && (Game_mode & GM_NETWORK))
			newmenu_do2(NULL, title, c, m, multi_endlevel_poll1, NULL, 0, Menu_pcx_name);
		else
#endif	// Note link!
			newmenu_do2(NULL, title, c, m, NULL, NULL, 0, Menu_pcx_name);
}

int draw_rock(newmenu *menu, d_event *event, grs_bitmap *background)
{
	menu = menu;

	switch (event->type)
	{
		case EVENT_WINDOW_DRAW:
			gr_set_current_canvas(NULL);
			show_fullscr(background);
			break;
			
		default:
			break;
	}
	
	return 0;
}

void do_screen_message(char *fmt, ...)
{
	va_list arglist;
	grs_bitmap background;
	char msg[1024];
	
	if (Game_mode & GM_MULTI)
		return;
	
	gr_init_bitmap_data(&background);
	if (pcx_read_bitmap(Menu_pcx_name, &background, BM_LINEAR, gr_palette) != PCX_ERROR_NONE)
		return;

	gr_palette_load(gr_palette);

	va_start(arglist, fmt);
	vsprintf(msg, fmt, arglist);
	va_end(arglist);
	
	nm_messagebox1(NULL, (int (*)(newmenu *, d_event *, void *))draw_rock, &background, 1, TXT_OK, msg);
	gr_free_bitmap_data(&background);
}

//called when the player has finished a level
void PlayerFinishedLevel(int secret_flag)
{
	int	rval;
	int 	was_multi = 0;

	if (Game_wind)
		window_set_visible(Game_wind, 0);

	//credit the player for hostages
	Players[Player_num].hostages_rescued_total += Players[Player_num].hostages_on_board;

#ifndef SHAREWARE
	if (!(Game_mode & GM_MULTI) && (secret_flag)) {
		newmenu_item	m[1];

		m[0].type = NM_TYPE_TEXT;
		m[0].text = " ";			//TXT_SECRET_EXIT;

		newmenu_do2(NULL, TXT_SECRET_EXIT, 1, m, NULL, NULL, 0, Menu_pcx_name);
	}
#endif

// -- mk mk mk -- used to be here -- mk mk mk --

#ifdef NETWORK
	if (Game_mode & GM_NETWORK)
	{
		if (secret_flag)
			Players[Player_num].connected = CONNECT_FOUND_SECRET; // Finished and went to secret level
		else
			Players[Player_num].connected = CONNECT_WAITING; // Finished but did not die
	}

#endif
	last_drawn_cockpit = -1;

	if (Current_level_num == Last_level) {
#ifdef NETWORK
		if ((Game_mode & GM_MULTI) && !(Game_mode & GM_MULTI_COOP))
		{
			was_multi = 1;
			multi_endlevel_score();
			rval = AdvanceLevel(secret_flag);				//now go on to the next one (if one)
		}
		else
#endif
		{	// Note link to above else!
			rval = AdvanceLevel(secret_flag);				//now go on to the next one (if one)
			DoEndLevelScoreGlitz(0);		//give bonuses
		}
	} else {
#ifdef NETWORK
		if (Game_mode & GM_MULTI)
			multi_endlevel_score();
		else
#endif	// Note link!!
			DoEndLevelScoreGlitz(0);		//give bonuses
		rval = AdvanceLevel(secret_flag);				//now go on to the next one (if one)
	}

	if (!was_multi && rval) {
#ifndef SHAREWARE
		if (PLAYING_BUILTIN_MISSION)
#endif
			scores_maybe_add_player(0);
		if (Game_wind)
			window_close(Game_wind);		// Exit out of game loop
	}
	else if (rval && Game_wind)
		window_close(Game_wind);

	if (Game_wind)
		window_set_visible(Game_wind, 1);
	reset_time();
}


//from which level each do you get to each secret level
// int Secret_level_table[MAX_SECRET_LEVELS_PER_MISSION];

//called to go to the next level (if there is one)
//if secret_flag is true, advance to secret level, else next normal one
//	Return true if game over.
int AdvanceLevel(int secret_flag)
{
	Control_center_destroyed = 0;

	#ifdef EDITOR
	if (Current_level_num == 0)
	{
		return 1;		//not a real level
	}
	#endif

	key_flush();

#ifdef NETWORK
	if (Game_mode & GM_MULTI)
	{
		int result;
		result = multi_endlevel(&secret_flag); // Wait for other players to reach this point
		if (result) // failed to sync
		{
			return (Current_level_num == Last_level);
		}
	}
#endif

	key_flush();

	if (Current_level_num == Last_level) {		//player has finished the game!

		if ((Newdemo_state == ND_STATE_RECORDING) || (Newdemo_state == ND_STATE_PAUSED))
			newdemo_stop_recording();

		do_end_briefing_screens(Ending_text_filename);

		return 1;

	} else {

		Next_level_num = Current_level_num+1;		//assume go to next normal level

		if (secret_flag) {			//go to secret level instead
			int i;

			for (i=0;i<-Last_secret_level;i++)
				if (Secret_level_table[i]==Current_level_num) {
					Next_level_num = -(i+1);
					break;
				}
			Assert(i<-Last_secret_level);		//couldn't find which secret level
		}

		if (Current_level_num < 0)	{			//on secret level, where to go?

			Assert(!secret_flag);				//shouldn't be going to secret level
			Assert(Current_level_num<=-1 && Current_level_num>=Last_secret_level);

			Next_level_num = Secret_level_table[(-Current_level_num)-1]+1;
		}

		StartNewLevel(Next_level_num);

	}

	key_flush();

	return 0;
}

//called when the player has died
void DoPlayerDead()
{
	int cycle_window_vis = 1;
#ifdef NETWORK
	if ( (Game_mode & GM_MULTI) && (Netgame.SpawnStyle == SPAWN_STYLE_PREVIEW))  {
		cycle_window_vis = 0; 
	}
#endif

	reset_auto_select();

	if (Game_wind && cycle_window_vis)
		window_set_visible(Game_wind, 0);

	reset_palette_add();

	gr_palette_load (gr_palette);

	dead_player_end();		//terminate death sequence (if playing)

	#ifdef EDITOR
	if (Game_mode == GM_EDITOR) {			//test mine, not real level
		object * player = &Objects[Players[Player_num].objnum];
		//nm_messagebox( "You're Dead!", 1, "Continue", "Not a real game, though." );
		if (Game_wind)
			window_set_visible(Game_wind, 1);
		load_level("gamesave.lvl");
		init_player_stats_new_ship(Player_num);
		player->flags &= ~OF_SHOULD_BE_DEAD;
		StartLevel(0);
		return;
	}
	#endif

#ifdef NETWORK
	if ( Game_mode&GM_MULTI )
	{
		multi_do_death(Players[Player_num].objnum);
	}
	else
#endif
	{				//Note link to above else!
		Players[Player_num].lives--;
		if (Players[Player_num].lives == 0)
		{
			DoGameOver();
			return;
		}
	}

	if ( Control_center_destroyed ) {
		int	rval;

		//clear out stuff so no bonus
		Players[Player_num].hostages_on_board = 0;
		Players[Player_num].energy = 0;
		Players[Player_num].shields = 0;
#ifdef NETWORK
		Players[Player_num].connected = CONNECT_DIED_IN_MINE;
#endif

		do_screen_message(TXT_DIED_IN_MINE); // Give them some indication of what happened

		if (Current_level_num == Last_level) {
#ifdef NETWORK
			if ((Game_mode & GM_MULTI) && !(Game_mode & GM_MULTI_COOP))
			{
				multi_endlevel_score();
				rval = AdvanceLevel(0);			//if finished, go on to next level
			}
			else
#endif
			{			// Note link to above else!
				rval = AdvanceLevel(0);			//if finished, go on to next level
				DoEndLevelScoreGlitz(0);
			}
			init_player_stats_new_ship(Player_num);
			last_drawn_cockpit = -1;
		} else {
#ifdef NETWORK
			if (Game_mode & GM_MULTI)
				multi_endlevel_score();
			else
#endif
				DoEndLevelScoreGlitz(0);		// Note above link!
			rval = AdvanceLevel(0);			//if finished, go on to next level
			init_player_stats_new_ship(Player_num);
			last_drawn_cockpit = -1;
		}

		if (rval) {
#ifndef SHAREWARE
			if (PLAYING_BUILTIN_MISSION)
#endif
				scores_maybe_add_player(0);
			if (Game_wind)
				window_close(Game_wind);		// Exit out of game loop
		}
	} else {
		init_player_stats_new_ship(Player_num);
		StartLevel(1);
	}

	if (Game_wind  && cycle_window_vis)
		window_set_visible(Game_wind, 1);
	reset_time();
}

//called when the player is starting a new level for normal game mode and restore state
void StartNewLevelSub(int level_num, int page_in_textures, int secret_flag)
{
	/*
	 * This flag is present for compatibility with D2X.  Set it to zero
	 * so the optimizer deletes all reference to it.
	 */
	secret_flag = 0;
	if (!(Game_mode & GM_MULTI)) {
		last_drawn_cockpit = -1;
	}

	if (Newdemo_state == ND_STATE_PAUSED)
		Newdemo_state = ND_STATE_RECORDING;

	if (Newdemo_state == ND_STATE_RECORDING) {
		newdemo_set_new_level(level_num);
		newdemo_record_start_frame(FrameTime );
	} 

	LoadLevel(level_num, page_in_textures);

	Assert(Current_level_num == level_num);	//make sure level set right

	gameseq_init_network_players(); // Initialize the Players array for
											  // this level

#ifdef NETWORK
	if (Game_mode & GM_NETWORK)
	{
		if(multi_level_sync()) // After calling this, Player_num is set
		{
			songs_play_song( SONG_TITLE, 1 ); // level song already plays but we fail to start level...
			return;
		}

		// Reset the timestamp when we can send another ship status packet (for observer mode)
		Next_ship_status_time = GameTime64;

		if (imulti_new_game)
		{
			for (int i=0;i<MAX_PLAYERS;i++)
			{
				init_player_stats_new_ship(i);
			}
		}
	}
#endif

	HUD_clear_messages();

	automap_clear_visited();

	init_player_stats_level(secret_flag);

	gr_use_palette_table( "palette.256" );
	gr_palette_load(gr_palette);

#ifndef SHAREWARE
#ifdef NETWORK
	if ((Game_mode & GM_MULTI_COOP) && Network_rejoined)
	{
		int i;
		for (i = 0; i < N_players; i++)
			Players[i].flags |= Netgame.player_flags[i];
	}
#endif
#endif

	Viewer = &Objects[Players[Player_num].objnum];

#ifdef NETWORK
	if (Game_mode & GM_MULTI)
	{
		multi_prep_level(); // Removes robots from level if necessary
	}
#endif

	gameseq_remove_unused_players();

	Game_suspended = 0;

	Control_center_destroyed = 0;

	init_cockpit();
	init_robots_for_level();
	init_ai_objects();
	init_morphs();
	init_all_matcens();
	reset_palette_add();

	if (!(Game_mode & GM_MULTI) && !cheats.enabled)
		set_highest_level(Current_level_num);

	reset_special_effects();

#ifdef OGL
	ogl_cache_level_textures();
#endif

#ifdef NETWORK
	if (Network_rejoined == 1)
	{
		Network_rejoined = 0;
		StartLevel(1);
	}
	else
#endif
		StartLevel(0);		// Note link to above if!

	copy_defaults_to_robot_all();
	init_controlcen_for_level();

	reset_respawnable_bots();

	set_homing_update_rate(Game_mode & GM_MULTI ? Netgame.HomingUpdateRate : 25);
	set_constant_homing_speed(Game_mode & GM_MULTI ? Netgame.ConstantHomingSpeed : 0);

	//	Say player can use FLASH cheat to mark path to exit.
	Last_level_path_created = -1;

	// Initialise for palette_restore()
	if (!((Game_mode & GM_MULTI) && (Newdemo_state != ND_STATE_PLAYBACK)))
		palette_save();

	if(Game_mode & GM_MULTI && PlayerCfg.AutoDemo && Newdemo_state != ND_STATE_RECORDING) {
		newdemo_start_recording();
	}

	if (!Game_wind)
		game();
}

#ifdef NETWORK
extern char PowerupsInMine[MAX_POWERUP_TYPES], MaxPowerupsAllowed[MAX_POWERUP_TYPES];
#endif
void bash_to_shield (int i,char *s)
{
#ifdef NETWORK
	int type=Objects[i].id;
#endif

#ifdef NETWORK
	PowerupsInMine[type]=MaxPowerupsAllowed[type]=0;
#endif

	Objects[i].id = POW_SHIELD_BOOST;
	Objects[i].rtype.vclip_info.vclip_num = Powerup_info[Objects[i].id].vclip_num;
	Objects[i].rtype.vclip_info.frametime = Vclip[Objects[i].rtype.vclip_info.vclip_num].frame_time;
	Objects[i].size = Powerup_info[POW_SHIELD_BOOST].size;
}

int find_connecting_side(point_seg* from, point_seg* to) // Sirius' function.
{
	for (int side = 0; side < MAX_SIDES_PER_SEGMENT; side++)
		if (Segments[from->segnum].children[side] == to->segnum)
			return side;
	// This shouldn't happen if consecutive nodes from a valid path were passed in
	Int3();
	return -1;
}

#define SHIP_MOVE_SPEED 3709337.6
#define LASER1_DPS (80 * F1_0)

#define OBJECTIVE_TYPE_INVALID 0
#define OBJECTIVE_TYPE_OBJECT 1
#define OBJECTIVE_TYPE_TRIGGER 2
#define OBJECTIVE_TYPE_ENERGY 3

typedef struct
{
	int type;
	int ID;
} partime_objective;

typedef struct
{
	int wallID;
	partime_objective unlockedBy;
} partime_locked_wall_info;

typedef struct
{
	partime_objective toDoList[MAX_OBJECTS + MAX_TRIGGERS];
	int toDoListSize;
	partime_objective doneList[MAX_OBJECTS + MAX_TRIGGERS];
	int doneListSize;
	// Remember the IDs of all walls in the level that are locked in some way, so we can see if any of our paths encounter them.
	partime_locked_wall_info lockedWalls[MAX_WALLS];
	int numLockedWalls;
	// We need to track how many times we trip matcens so we don't add their HP more than is actually possible.
	int matcenLives[MAX_ROBOT_CENTERS];
	// Time spent clearing matcens.
	fix matcenTime;
	// Track the locations of energy centers for when we need to make a pit stop...
	partime_objective energyCenters[MAX_NUM_FUELCENS];
	int numEnergyCenters;
	// Variable to tell it when to refill its energy. It is always equipped with Laser 1 and never uses anything else.
	fix simulatedEnergy;
	// How much robot HP we've had to destroy to this point.
	double levelHealth;
	fix energy_used_per_shield;
	fix energy_gained_per_pickup;
} partime_calc_state;

int getObjectiveSegnum(partime_objective objective)
{
	if (objective.type == OBJECTIVE_TYPE_OBJECT)
		return Objects[objective.ID].segnum;
	else if (objective.type == OBJECTIVE_TYPE_TRIGGER || objective.type == OBJECTIVE_TYPE_ENERGY)
		return objective.ID;
	return -1;
}

vms_vector getObjectivePosition(partime_objective objective)
{
	if (objective.type == OBJECTIVE_TYPE_OBJECT)
		return Objects[objective.ID].pos;
	vms_vector segmentCenter;
	compute_segment_center(&segmentCenter, &Segments[getObjectiveSegnum(objective)]);
	return segmentCenter;
}

int findKeyObjectID(int keyType)
{
	int powerupID;
	switch (keyType)
	{
	case KEY_BLUE:
		powerupID = POW_KEY_BLUE;
		break;
	case KEY_GOLD:
		powerupID = POW_KEY_GOLD;
		break;
	case KEY_RED:
		powerupID = POW_KEY_RED;
		break;
	default:
		// That's not a key!
		Int3();
		return -1;
	}

	for (int i = 0; i <= Highest_object_index; i++) {
		// Note that this doesn't handle objects whose robot_info causes them to drop keys. The base game has none of those.
		if ((Objects[i].type == OBJ_POWERUP && Objects[i].id == powerupID) ||
			(Objects[i].type == OBJ_ROBOT && Objects[i].contains_type == OBJ_POWERUP && Objects[i].contains_id == powerupID))
			return i;
	}

	return -1; // Not found
}

int findConnectedWallNum(int wall_num)
{
	wall* wall = &Walls[wall_num];
	segment* connectedSegment = &Segments[Segments[wall->segnum].children[wall->sidenum]];
	if (connectedSegment == NULL)
		return -1;

	int connectedSide = find_connect_side(&Segments[wall->segnum], connectedSegment);
	if (connectedSide >= 0)
		return connectedSegment->sides[connectedSide].wall_num;

	return -1;
}

int findTriggerWallForWall(int wallID)
{
	wall* thisWall = &Walls[wallID];
	wall* connectedWall = NULL;
	int connectedWallID = findConnectedWallNum(wallID);
	if (connectedWallID >= 0)
		connectedWall = &Walls[connectedWallID];

	// Keep in mind: This doesn't guarantee the NEAREST trigger for the locked wall, but that'll prooooobably be fine.
	for (int otherWallID = 0; otherWallID < Num_walls; otherWallID++) {
		if (otherWallID == wallID || Walls[otherWallID].trigger == -1)
			continue;
		trigger* t = &Triggers[Walls[otherWallID].trigger];
		for (short link_num = 0; link_num < t->num_links; link_num++) {
			// Does the trigger target point at this wall's segment/side?
			if (t->seg[link_num] == thisWall->segnum && t->side[link_num] == thisWall->sidenum) {
				return otherWallID;
			}
			// Or that of the other side of this wall, if any?
			if (connectedWall && t->seg[link_num] == connectedWall->segnum && t->side[link_num] == connectedWall->sidenum) {
				return otherWallID;
			}
		}
	}

	return -1;
}

int findReactorObjectID()
{
	for (int i = 0; i <= Highest_object_index; i++)
		if (Objects[i].type == OBJ_CNTRLCEN)
			return i;
	return -1;
}

void initLockedWalls(partime_calc_state* state)
{
	for (int i = 0; i < Num_walls; i++) {
		if ((Walls[i].type == WALL_DOOR && (Walls[i].keys == KEY_BLUE || Walls[i].keys == KEY_GOLD || Walls[i].keys == KEY_RED)) || Walls[i].flags == WALL_DOOR_LOCKED) {
			partime_locked_wall_info* wallInfo = &state->lockedWalls[state->numLockedWalls];
			state->numLockedWalls++;
			wallInfo->wallID = i;

			// Is it opened by a key?
			if (Walls[i].type == WALL_DOOR && (Walls[i].keys == KEY_BLUE || Walls[i].keys == KEY_GOLD || Walls[i].keys == KEY_RED)) {
				wallInfo->unlockedBy.type = OBJECTIVE_TYPE_OBJECT;
				wallInfo->unlockedBy.ID = findKeyObjectID(Walls[i].keys);
				continue;
			}

			// ...or is it opened by a trigger?
			if (Walls[i].flags == WALL_DOOR_LOCKED) {
				int unlockWall = findTriggerWallForWall(i);
				if (unlockWall != -1) {
					wallInfo->unlockedBy.type = OBJECTIVE_TYPE_TRIGGER;
					wallInfo->unlockedBy.ID = Walls[unlockWall].segnum;
					continue;
				}

				// So doors opened by the reactor exploding are handled correctly.
				for (short link_num = 0; link_num < ControlCenterTriggers.num_links; link_num++) {
					if (ControlCenterTriggers.seg[link_num] == Walls[i].segnum && ControlCenterTriggers.side[link_num] == Walls[i].sidenum) {
						wallInfo->unlockedBy.type = OBJECTIVE_TYPE_OBJECT;
						wallInfo->unlockedBy.ID = findReactorObjectID();
						break;
					}
				}
			}
		}
	}
}

int isWallUnlockCompleted(partime_calc_state* state, int lockedWallID)
{
	// Check if something in doneList unlocked this wall
	for (int doneItem = 0; doneItem < state->doneListSize; doneItem++) {
		if (state->doneList[doneItem].type == state->lockedWalls[lockedWallID].unlockedBy.type &&
			state->doneList[doneItem].ID == state->lockedWalls[lockedWallID].unlockedBy.ID)
			return 1;
	}

	// Check if something in doneList unlocked the other side of this wall
	int connectedWallNum = findConnectedWallNum(state->lockedWalls[lockedWallID].wallID);
	if (connectedWallNum >= 0) {
		for (int w = 0; w < state->numLockedWalls; w++)
			if (state->lockedWalls[w].wallID == connectedWallNum) {
				// Other side was locked; is it now unlocked?
				for (int doneItem = 0; doneItem < state->doneListSize; doneItem++)
					if (state->doneList[doneItem].type == state->lockedWalls[w].unlockedBy.type &&
						state->doneList[doneItem].ID == state->lockedWalls[w].unlockedBy.ID)
						return 1;

				// Nope.
				return 0;
			}
	}

	return 0;
}

void initEnergyStats(partime_calc_state* state)
{
	// We simulate killing robots with dual laser level 1. First we need to calculate its energy
	// consumption per point of damage here.
	fix energy_used = Weapon_info[Primary_weapon_to_weapon_info[LASER_INDEX]].energy_usage;
	// From do_laser_firing_player (laser.c). Trainee and Rookie halve energy consumption.
	// 2 bullets per shot.
	fix damage_output = 2 * Weapon_info[Primary_weapon_to_weapon_info[LASER_INDEX]].strength[4];
	state->energy_used_per_shield = fixdiv(energy_used, damage_output);

	// From pick_up_energy (powerup.c)
	state->energy_gained_per_pickup = 393216;
}

void addObjectiveToList(partime_objective* list, int* listSize, partime_objective objective)
{
	// First make sure it's not already in there
	for (int i = 0; i < *listSize; i++)
		if (list[i].type == objective.type && list[i].ID == objective.ID)
			return;

	if (*listSize >= MAX_OBJECTS + MAX_TRIGGERS)
		Int3(); // We're out of space in the list; this shouldn't happen.

	list[*listSize].type = objective.type;
	list[*listSize].ID = objective.ID;
	(*listSize)++;
}

void removeObjectiveFromList(partime_objective* list, int* listSize, partime_objective objective)
{
	if (*listSize == 0)
		return;

	int offset = 0;
	for (offset = 0; offset < *listSize; offset++)
		if (list[offset].type == objective.type && list[offset].ID == objective.ID)
			break;
	if (offset + 1 < *listSize)
		memmove(list + offset, list + offset + 1, (*listSize - offset - 1) * sizeof(partime_objective));
	(*listSize)--;
}

void addUnlockItemToToDoList(partime_calc_state* state, int lockedWallID)
{
	if (lockedWallID > state->numLockedWalls)
		Int3(); // ID out of range

	addObjectiveToList(state->toDoList, &state->toDoListSize, state->lockedWalls[lockedWallID].unlockedBy);
}

// Find a path from a start segment to an objective.
// A lot of this is copied from the mark_player_path_to_segment function in game.c.
int create_path_partime(int start_seg, int target_seg, point_seg** path_start, int* path_count)
{
	object* objp = ConsoleObject;
	short player_path_length = 0;
	Ranking.pathfinds++;
	ConsoleObject->segnum = start_seg; // We're gonna teleport the player to every one of the starting segments, then put him back at spawn in time for the level to start.

	if (create_path_points(objp, objp->segnum, target_seg, Point_segs_free_ptr, &player_path_length, 100, 0, 0, -1) == -1 ||
		Point_segs_free_ptr[player_path_length - 1].segnum != target_seg)
	{
		// Clear path_start and path_count, so it's clear we didn't find anything
		*path_start = NULL;
		*path_count = 0;
		return 0;
	}

	*path_start = Point_segs_free_ptr;
	*path_count = player_path_length;
	return 1;
}

int find_first_locked_wall_partime(partime_calc_state* state, point_seg* path, int path_count)
{
	for (int i = 0; i < path_count - 1; i++) {
		int sideNum = find_connecting_side(&path[i], &path[i + 1]);
		side* side = &Segments[path[i].segnum].sides[sideNum];
		if (side->wall_num < 0)
			// No wall here, so it's definitely not locked.
			continue;

		// Is this wall in the locked wall list?
		for (int w = 0; w < state->numLockedWalls; w++)
			if (side->wall_num == state->lockedWalls[w].wallID && !isWallUnlockCompleted(state, w))
				return w;
	}
	return -1;
}

double calculate_path_length_partime(point_seg* path, int path_count, partime_objective objective)
{
	// Find length of path in units and return it.
	// Note: technically we should be using f2fl on the result of vm_vec_dist, but since the
	// multipliers are baked into the constants in calculateParTime already, maybe it's better to
	// leave it for now.
	double pathLength = 0;
	if (path_count > 1) {
		int i, j;
		for (int i = 0; i < path_count - 1;) {
			for (j = path_count - 1; j > i; j--) {
				fvi_query fq;
				fvi_info hit_data;
				fq.p0 = &path[i].point;
				fq.startseg = path[i].segnum;
				fq.p1 = &path[j].point;
				fq.rad = 0x10;
				fq.thisobjnum = ConsoleObject;
				fq.ignore_obj_list = NULL;
				fq.flags = 0;
				// If the unshortened vector (i -> i + 1) still hits something, use it anyway.
				// Maybe it's a door.
				if (find_vector_intersection(&fq, &hit_data) == 0 || j == i + 1) {
					pathLength += vm_vec_dist(&path[i].point, &path[j].point);
					j--;
					break;
				}
			}
			// j decrements at the end of the search so we have to go one segment past it.
			i = j + 1;
		}
		// For objects, once we reach the target segment we move to the object to "pick it up".
		// Note: For now, this applies to robots, too.
		if (objective.type == OBJECTIVE_TYPE_OBJECT)
			pathLength += vm_vec_dist(&path[path_count - 1].point, &Objects[objective.ID].pos);
	}
	// Objective is in the same segment as the player. If it's an object, we still move to it.
	else if (objective.type == OBJECTIVE_TYPE_OBJECT)
		pathLength = vm_vec_dist(&Ranking.lastPosition, &Objects[objective.ID].pos);

	return pathLength;
}

partime_objective find_nearest_objective_partime(partime_calc_state* state, int addUnlocksToObjectiveList,
	int start_seg, partime_objective* objectiveList, int objectiveListSize, point_seg** path_start, int* path_count, double* path_length)
{
	// We track two path lengths here: the first for the nearest objective we can find a legal
	// path to, and the second including objectives that cannot be reached (not even with unlocks).
	// We use this for a degraded case where no objective is actually reachable, so that we still
	// return a *somewhat* meaningful value for weird levels that expect you to clip through walls
	// or something.
	double shortestLegalPathLength = -1;
	partime_objective nearestLegalObjective;
	double shortestDirectPathLength = -1;
	partime_objective nearestDirectObjective;

	vms_vector start;
	compute_segment_center(&start, &Segments[start_seg]);

	for (int i = 0; i < objectiveListSize; i++) {
		partime_objective objective = objectiveList[i];

		// In case we can't find a valid path to the target, draw a straight line to it and measure that to avoid softlocking.
		vms_vector end;
		compute_segment_center(&end, &Segments[getObjectiveSegnum(objective)]);
		double directPathLength = vm_vec_dist(&start, &end);
		if (directPathLength < shortestDirectPathLength || shortestDirectPathLength < 0) {
			shortestDirectPathLength = directPathLength;
			nearestDirectObjective = objective;
		}

		// Now figure out the real path, if there is one.
		int pathFound = create_path_partime(start_seg, getObjectiveSegnum(objective), path_start, path_count);
		if (pathFound) {
			int lockedWallID = find_first_locked_wall_partime(state, *path_start, *path_count);
			if (lockedWallID > -1) {
				if (addUnlocksToObjectiveList)
					addUnlockItemToToDoList(state, lockedWallID);
				// We can't reach this objective right now; find the next one.
				continue;
			}

			double pathLength = calculate_path_length_partime(*path_start, *path_count, objective);
			if (pathLength < shortestLegalPathLength || shortestLegalPathLength < 0) {
				shortestLegalPathLength = pathLength;
				nearestLegalObjective = objective;
			}
		}
	}

	// Did we find a legal objective? Return that
	if (shortestLegalPathLength >= 0) {
		// Regenerate the path since we may have checked something else in the meantime
		create_path_partime(start_seg, getObjectiveSegnum(nearestLegalObjective), path_start, path_count);
		*path_length = shortestLegalPathLength;
		return nearestLegalObjective;
	}
	// Otherwise check if *anything* was found
	else if (shortestDirectPathLength >= 0) {
		// There is no path for this case
		*path_start = NULL;
		*path_count = 0;
		*path_length = shortestDirectPathLength;
		return nearestDirectObjective;
	}
	else {
		// No objectives in list
		partime_objective emptyResult = { OBJECTIVE_TYPE_INVALID, 0 };
		return emptyResult;
	}
}

// Check if algo goes through fuelcen and regen its energy if so. Also check for energy pickups along the path. We need to do it in path order so simulatedEnergy stays accurate.
void update_energy_for_path_partime(partime_calc_state* state, point_seg* path, int path_count)
{
	// How much energy do we pick up while following this path?
	for (int i = 0; i < path_count; i++) {
		// If this segment is an energy center, recharge to 100
		if (Segments[path[i].segnum].special == SEGMENT_IS_FUELCEN && state->simulatedEnergy < 100 * F1_0)
			state->simulatedEnergy = 100 * F1_0;

		// If there are energy powerups in this segment, collect them
		for (int objNum = 0; objNum <= Highest_object_index; objNum++) {
			if (Objects[objNum].type == OBJ_POWERUP && Objects[objNum].id == POW_ENERGY && Objects[objNum].segnum == path[i].segnum) {
				// ...make sure we didn't already get this one
				int thisSourceCollected = 0;
				for (int j = 0; j < state->doneListSize; j++)
					if (state->doneList[j].type == OBJECTIVE_TYPE_OBJECT && state->doneList[j].ID == objNum) {
						thisSourceCollected = 1;
						break;
					}
				if (!thisSourceCollected) {
					state->simulatedEnergy += state->energy_gained_per_pickup;
					if (state->simulatedEnergy > MAX_ENERGY)
						state->simulatedEnergy = MAX_ENERGY;
					partime_objective energyObjective = { OBJECTIVE_TYPE_OBJECT, objNum };
					addObjectiveToList(state->doneList, &state->doneListSize, energyObjective);
				}
			}
		}
	}

	// How much energy does it take to handle the matcens along the way?
	// Check for matcens blocking the path, return the total HP of all the robots that could possibly come out in one round.
	fix matcenHealth = 0;
	for (int i = 0; i < path_count; i++) {
		segment* segp = &Segments[path[i].segnum];
		if (segp->special == SEGMENT_IS_ROBOTMAKER &&
			RobotCenters[segp->matcen_num].robot_flags[0] != 0 &&
			state->matcenLives[segp->matcen_num] > 0)
		{
			uint	flags;
			sbyte	legal_types[32];		//	32 bits in a word, the width of robot_flags.
			int	num_types, robot_index;
			robot_index = 0;
			num_types = 0;
			flags = RobotCenters[segp->matcen_num].robot_flags[0];
			while (flags) {
				if (flags & 1)
					legal_types[num_types++] = robot_index;
				flags >>= 1;
				robot_index++;
			}

			// Assume the worst case. Find the highest robot HP in this matcen and use that for all robots spawned.
			fix highestRobotHealth = 0;
			int n;
			fix evadeFactor;
			fix adjustedRobotHealth;
			fix offspringHealth;
			robot_info* highestRobInfo;
			for (n = 0; n < num_types; n++) {
				robot_info* robInfo = &Robot_info[legal_types[n]];
				double sizeFactor = (double)ConsoleObject->size / Polygon_models[robInfo->model_num].rad; // An enemy's size directly impacts how hard it is to hit. Anything bigger than your ship gives less time, and anything smaller gives more.
				evadeFactor = i2f(32 * robInfo->evade_speed[4]) > robInfo->max_speed[4] ?
				fixdiv(i2f(24 * robInfo->evade_speed[4]), (fix)SHIP_MOVE_SPEED) :
				fixdiv(i2f(32 * robInfo->evade_speed[4]), (fix)SHIP_MOVE_SPEED);
				adjustedRobotHealth = fixmul(robInfo->strength, F1_0 + evadeFactor * sizeFactor);
				//if (robot is still) // These behavior conditions may be removed, as it's possible that matcen enemies can't have non-normal behaviors.
					//adjustedRobotHealth = robInfo->strength;
				//if (robot snipes) // Don't add chase time for matcen enemies that drop bombs. They ALWAYS flee and aren't required. Sniping bots still need chased, or else they'll return to attack later.
					//adjustedRobotHealth += (adjustedRobotHealth * robInfo->max_speed[4]) / SHIP_MOVE_SPEED;
				if (robInfo->contains_type == OBJ_ROBOT) {
					robInfo = &Robot_info[legal_types[n]].contains_id;
					sizeFactor = 1; // (double)ConsoleObject->size / Polygon_models[robInfo->model_num].rad;
					evadeFactor = i2f(32 * robInfo->evade_speed[4]) > robInfo->max_speed[4] ?
						fixdiv(i2f(24 * robInfo->evade_speed[4]), (fix)SHIP_MOVE_SPEED) :
						fixdiv(i2f(32 * robInfo->evade_speed[4]), (fix)SHIP_MOVE_SPEED);
					offspringHealth = fixmul(robInfo->strength, F1_0 + evadeFactor * sizeFactor);
					//if (offspring is still) // Might get removed, it's possible that offspring can't have a non-normal behavior.
						//offspringHealth = robInfo->strength;
					//if (offspring drops bombs or snipes) // Might get removed, it's possible that offspring can't have a non-normal behavior.
						//offspringHealth += ((offspringHealth * robInfo->max_speed[4]) / SHIP_MOVE_SPEED) * obj->contains_count);
					offspringHealth * (robInfo->contains_count * (robInfo->contains_prob / 16));
					adjustedRobotHealth += offspringHealth;
				}
				if (adjustedRobotHealth > highestRobotHealth) {
					highestRobotHealth = adjustedRobotHealth;
					highestRobInfo = &Robot_info[legal_types[n]];
				}
			}
			if (highestRobInfo->contains_type == OBJ_POWERUP && highestRobInfo->contains_id == POW_ENERGY) { // If the robot is guaranteed to drop energy, give it to algo so it doesn't visit fuelcens more than needed.
				state->simulatedEnergy -= fixmul(adjustedRobotHealth, state->energy_used_per_shield);
				state->simulatedEnergy += state->energy_gained_per_pickup * highestRobInfo->contains_count;
				if (state->simulatedEnergy > MAX_ENERGY) // Cap algo's energy at 200, like the player's.
					state->simulatedEnergy = MAX_ENERGY;
			}
			matcenHealth += highestRobotHealth * 7; // 7 is the insane difficulty matcen capacity.
			state->matcenLives[segp->matcen_num]--;
		}
	}

	state->levelHealth += matcenHealth;
	state->simulatedEnergy -= fixmul(matcenHealth, state->energy_used_per_shield);
	state->matcenTime += fixdiv(matcenHealth, LASER1_DPS);
	if (matcenHealth > 0)
		printf("Fought matcens for %.3fs\n", f2fl(fixdiv(matcenHealth, LASER1_DPS)));
}

void update_energy_for_objective_partime(partime_calc_state* state, partime_objective objective)
{
	// How much energy does it take to complete this objective?
	if (objective.type == OBJECTIVE_TYPE_OBJECT) { // We don't fight triggers.
		object* obj = &Objects[objective.ID];
		fix offspringHealth; // So multipliers done to offspring don't bleed into their parents' values.
		if (obj->type == OBJ_ROBOT || obj->type == OBJ_CNTRLCEN) { // We don't fight keys or hostages.
			robot_info* robInfo = &Robot_info[obj->id];
			double sizeFactor = (double)ConsoleObject->size / obj->size; // An enemy's size directly impacts how hard it is to hit. Anything bigger than your ship gives less time, and anything smaller gives more.
			// Add the time it takes to kill this robot, but also give players more time to fight based on enemy speed, to account for robots evading shots. It's unrealistic to expect 100% accuracy on insane.
			fix evadeFactor = i2f(32 * robInfo->evade_speed[4]) > robInfo->max_speed[4] ? // Account for -25% evade speed to robots whose evade speed initially exceeds their max speed.
				fixdiv(i2f(24 * robInfo->evade_speed[4]), (fix)SHIP_MOVE_SPEED) :
				fixdiv(i2f(32 * robInfo->evade_speed[4]), (fix)SHIP_MOVE_SPEED);
			fix adjustedRobotHealth = fixmul(obj->shields, F1_0 + evadeFactor * sizeFactor);
			if (obj->ctype.ai_info.behavior == AIB_STILL) // If this robot can't move, remove all its evade bonuses.
				adjustedRobotHealth = obj->shields;
			if (obj->ctype.ai_info.behavior == AIB_HIDE || obj->ctype.ai_info.behavior == AIB_RUN_FROM) // Give the player even more time to dispatch a robot if it runs away, since they will have to persue it.
				adjustedRobotHealth += (adjustedRobotHealth * robInfo->max_speed[4]) / SHIP_MOVE_SPEED;
			robInfo = &Robot_info[obj->contains_id];
			sizeFactor = (double)ConsoleObject->size / Polygon_models[robInfo->model_num].rad; // This and the above line are probably in the wrong spot, or there needs to be another iteration of it somewhere else.
			if (obj->contains_type == OBJ_ROBOT) { // Now we account for robots guaranteed to drop from this robot, if any.
				evadeFactor = i2f(32 * robInfo->evade_speed[4]) > robInfo->max_speed[4] ?
					fixdiv(i2f(24 * robInfo->evade_speed[4]), (fix)SHIP_MOVE_SPEED) :
					fixdiv(i2f(32 * robInfo->evade_speed[4]), (fix)SHIP_MOVE_SPEED);
				offspringHealth = fixmul(obj->shields, F1_0 + evadeFactor * sizeFactor);
				//if (robInfo->ctype.ai_info.behavior == AIB_STILL) // Might get removed, it's possible that offspring can't have a non-normal behavior.
					//offspringHealth = robInfo->strength;
				//if (offspring drops bombs or snipes) // Might get removed, it's possible that offspring can't have a non-normal behavior.
					//offspringHealth += ((offspringHealth * robInfo->max_speed[4]) / SHIP_MOVE_SPEED) * obj->contains_count;
				adjustedRobotHealth += offspringHealth * obj->contains_count;
			}
			else if (robInfo->contains_type == OBJ_ROBOT) {
				evadeFactor = i2f(32 * robInfo->evade_speed[4]) > robInfo->max_speed[4] ?
					fixdiv(i2f(24 * robInfo->evade_speed[4]), (fix)SHIP_MOVE_SPEED) :
					fixdiv(i2f(32 * robInfo->evade_speed[4]), (fix)SHIP_MOVE_SPEED);
				offspringHealth = fixmul(obj->shields, F1_0 + evadeFactor * sizeFactor);
				//if (offspring is still) // Might get removed, it's possible that offspring can't have a non-normal behavior.
					//offspringHealth = robInfo->strength;
				//if (offspring drops bombs or snipes) // Might get removed, it's possible that offspring can't have a non-normal behavior.
					//offspringHealth += ((offspringHealth * robInfo->max_speed[4]) / SHIP_MOVE_SPEED) * obj->contains_count;
				adjustedRobotHealth += offspringHealth * (robInfo->contains_count * (robInfo->contains_prob / 16));
			}
			state->levelHealth += adjustedRobotHealth;
			state->simulatedEnergy -= fixmul(adjustedRobotHealth, state->energy_used_per_shield);
			if (obj->contains_type == OBJ_POWERUP && obj->contains_id == POW_ENERGY) {
				// If the robot is guaranteed to drop energy, give it to algo so it doesn't visit fuelcens more than needed.
				state->simulatedEnergy += state->energy_gained_per_pickup * obj->contains_count;
			}
			// Cap algo's energy at 200, like the player's.
			if (state->simulatedEnergy > MAX_ENERGY)
				state->simulatedEnergy = MAX_ENERGY;
		}
	}
}

int calculateParTime() // Here is where we have an algorithm run a simulated path through a level to determine how long the player should take. It always assumes slowest scenario to ensure the player can beat it.
{ // Honestly we're so lucky this algorithm even works remotely well to begin with.
	partime_calc_state state = { 0 };
	double levelDistance = 0; // Variable to track how much distance it's travelled.
	int segnum = ConsoleObject->segnum; // Start the algorithm off where the player spawns.
	int initialSegnum = ConsoleObject->segnum; // Version of segnum that stays at its initial value, to ensure the player is put in the right spot.
	int unlockID = -1; // ID of thing needed to unlock something. If negative, that means it's unlocked.
	int lockedWallID = -1; // ID of a wall in the locked wall list that currently blocks progress toward an objective.
	int i;
	int j;
	int loops = 0; // How many times the pathmaking process has repeated. This determines what toDoList is populated with, to make sure things are gone to in the right order.
	Ranking.pathfinds = 0; // If par time still not calculated after a million pathfinding operations, assume softlock and give up, returning current values. 1000000 should be mathematically impossible, even with 1000 objects.
	double pathLength; // Store create_path_partime's result in pathLength to compare to current shortest.
	double matcenTime = 0; // Debug variable to see how much time matcens are adding to the par time.
	point_seg* path_start; // The current path we are looking at (this is a pointer into somewhere in Point_segs).
	int path_count; // The number of segments in the path we're looking at.
	state.simulatedEnergy = 100 * F1_0; // Start with 100 energy.
	fix64 start_timer_value, end_timer_value; // For tracking how long this algorithm takes to run.

	// Calculate start time.
	timer_update();
	start_timer_value = timer_query();

	// Populate the locked walls list.
	initLockedWalls(&state);

	// Initialize all matcens to 3 lives.
	for (i = 0; i < Num_robot_centers; i++) {
		state.matcenLives[i] = 3;
	}

	// And energy stuff.
	initEnergyStats(&state);
	for (i = 0; i < Highest_segment_index; i++)
		if (Segments[i].special == SEGMENT_IS_FUELCEN) {
			state.energyCenters[state.numEnergyCenters].type = OBJECTIVE_TYPE_ENERGY;
			state.energyCenters[state.numEnergyCenters].ID = i;
			state.numEnergyCenters++;
		}

	while (loops < 3 && Ranking.pathfinds < 1000000) {
		// Collect our objectives at this stage...
		if (loops == 0) {
			for (i = 0; i <= Highest_object_index; i++) { // Populate the to-do list with all robots and hostages. Ignore robots not worth over zero, as the player isn't gonna go for those. This should never happen, but it's just a failsafe.
				if ((Objects[i].type == OBJ_ROBOT && Robot_info[Objects[i].id].score_value > 0 && Robot_info[Objects[i].id].boss_flag == 0) || Objects[i].type == OBJ_HOSTAGE) {
					partime_objective objective = { OBJECTIVE_TYPE_OBJECT, i };
					addObjectiveToList(state.toDoList, &state.toDoListSize, objective);
				}
			}
		}
		if (loops == 1) {
			for (i = 0; i <= Highest_object_index; i++) { // Populate the to-do list with all reactors and bosses.
				if ((Objects[i].type == OBJ_ROBOT && Robot_info[Objects[i].id].boss_flag != 0) || Objects[i].type == OBJ_CNTRLCEN) {
					partime_objective objective = { OBJECTIVE_TYPE_OBJECT, i };
					addObjectiveToList(state.toDoList, &state.toDoListSize, objective);
				}
			}
		}
		if (loops == 2) { // Put the nearest exit on the list. It's possible that the exit leading where the player wants to go could be further, but they can always return to this level for the time bonus.
			for (i = 0; i <= Num_triggers && state.toDoListSize == 0; i++) {
				if (Triggers[i].flags == TRIGGER_EXIT) {
					for (j = 0; j <= Num_walls; j++) {
						if (Walls[j].trigger == i) {
							partime_objective objective = { OBJECTIVE_TYPE_TRIGGER, Walls[j].segnum };
							addObjectiveToList(state.toDoList, &state.toDoListSize, objective);
						}
					}
				}
			}
		}

		while (state.toDoListSize > 0) {
			// Find which object on the to-do list is the closest, ignoring the reactor/boss if it's not the only thing left.
			partime_objective nearestObjective =
				find_nearest_objective_partime(&state, 1, segnum, state.toDoList, state.toDoListSize, &path_start, &path_count, &pathLength);

			if (nearestObjective.type == OBJECTIVE_TYPE_INVALID) {
				// This should only happen if there are no objectives in the list.
				// If that happens, we're done with this phase.
				break;
			}

			// Mark this objective as done.
			removeObjectiveFromList(state.toDoList, &state.toDoListSize, nearestObjective);
			addObjectiveToList(state.doneList, &state.doneListSize, nearestObjective);

			// Track resource consumption and robot HP destroyed.
			// If there's no path and we're doing straight line distance, we have no idea what we'd
			// be crossing through, so tracking resources for the path would be meaningless.
			// We can still check the objective itself, though.
			if (path_start != NULL) {
				update_energy_for_path_partime(&state, path_start, path_count); // Do energy stuff.
			}
			update_energy_for_objective_partime(&state, nearestObjective);
			printf("Now at %.3f energy\n", f2fl(state.simulatedEnergy));

			int nearestObjectiveSegnum = getObjectiveSegnum(nearestObjective);
			printf("Path from segment %i to %i: %.3fs\n", segnum, nearestObjectiveSegnum, pathLength / SHIP_MOVE_SPEED);
			// Now move ourselves to the objective for the next pathfinding iteration
			segnum = nearestObjectiveSegnum;
			Ranking.lastPosition = getObjectivePosition(nearestObjective);
			levelDistance += pathLength;

			if (state.simulatedEnergy < 0 && !((loops == 1 && state.toDoListSize == 0) || loops == 2)) { // Algo's energy's out. If not running for exit, search for nearest fuelcen, go to it and recharge.
				partime_objective nearestEnergyCenter =
					find_nearest_objective_partime(&state, 0, segnum, state.energyCenters, state.numEnergyCenters, &path_start, &path_count, &pathLength);

				if (nearestEnergyCenter.type != OBJECTIVE_TYPE_INVALID) { // Only update algo's location and increment level distance if a fuelcen was found.
					// I know we're out of energy here but it'd be unfair not to account for matcens in the way of the fuelcen. In practice, players will come to these at low energy, not zero.
					if (path_start != NULL) {
						update_energy_for_path_partime(&state, path_start, path_count);
					}

					int energyCenterSegnum = getObjectiveSegnum(nearestEnergyCenter);
					printf("Path from segment %i to %i: %0.3fs (FUELCEN)\n", segnum, energyCenterSegnum, pathLength / SHIP_MOVE_SPEED);
					segnum = energyCenterSegnum;
					Ranking.lastPosition = getObjectivePosition(nearestEnergyCenter);
					levelDistance += pathLength;
				}

				if (state.simulatedEnergy < 100 * F1_0)
					state.simulatedEnergy = 100 * F1_0; // We always reset energy to avoid a softlock, even if a valid path to a fuelcen couldn't be found.
			}
		}
		loops++;
	}
	ConsoleObject->segnum = initialSegnum;

	// Calculate end time.
	timer_update();
	end_timer_value = timer_query();

	printf("Par time: %.3fs (%.3f movement, %.3f combat) Matcen time: %.3fs+\nCalculation time: %.3fs\n",
		levelDistance / SHIP_MOVE_SPEED + state.levelHealth / LASER1_DPS,
		levelDistance / SHIP_MOVE_SPEED,
		state.levelHealth / LASER1_DPS,
		f2fl(state.matcenTime),
		f2fl(end_timer_value - start_timer_value));
	return (levelDistance / SHIP_MOVE_SPEED) + (state.levelHealth / LASER1_DPS);
}

//called when the player is starting a new level for normal game model
void StartNewLevel(int level_num)
{
	hide_menus();

	GameTime64 = 0;
	ThisLevelTime=0;
	
	Ranking.deathCount = 0;

	Ranking.excludePoints = 0;

	Ranking.rankScore = 0;

	Ranking.maxScore = 0;
	
	Ranking.quickload = 0;

	Ranking.level_time = 0; // Set this to 0 despite it going unused until set to time_level, so we can save a variable when telling the in-game timer which time variable to display.

	StartNewLevelSub(level_num, 1, 0);

	int i;
	for (i = 0; i <= Highest_object_index; i++) {
		if (Objects[i].type == OBJ_ROBOT) {
			Ranking.maxScore += Robot_info[Objects[i].id].score_value;
			if (Objects[i].contains_type == OBJ_ROBOT)
				Ranking.maxScore += Robot_info[Objects[i].contains_id].score_value * Objects[i].contains_count;
		}
		if (Objects[i].type == OBJ_CNTRLCEN)
			Ranking.maxScore += CONTROL_CEN_SCORE;
		if (Objects[i].type == OBJ_HOSTAGE)
			Ranking.maxScore += HOSTAGE_SCORE;
	}
	Ranking.maxScore *= 3; // This is not the final max score. Max score is still 7500 higher per hostage, but that's added last second so the time bonus can be weighted properly.
	Ranking.maxScore = (int)Ranking.maxScore;
	Ranking.parTime = calculateParTime();
	Ranking.parTime = (int)Ranking.parTime; // Truncate the par time so it looks better/legible on the result screen, and leaves room for the time bonus.
}

int previewed_spawn_point = 0; 

//initialize the player object position & orientation (at start of game, or new ship)
void InitPlayerPosition(int random)
{
	int NewPlayer=0;

	if (is_observer())
		NewPlayer = 0;
	else if (! ((Game_mode & GM_MULTI) && !(Game_mode&GM_MULTI_COOP)) ) // If not deathmatch
		NewPlayer = Player_num;
#ifdef NETWORK	
	else if ((Game_mode & GM_MULTI) && (Netgame.SpawnStyle == SPAWN_STYLE_PREVIEW)  && Dead_player_camera != NULL)
		NewPlayer = previewed_spawn_point; 
#endif
	else if (random == 1)
	{
		int i, trys=0;
		fix closest_dist = 0x7ffffff, dist;

		timer_update();
		d_srand((fix)timer_query());
		do {
			trys++;
			NewPlayer = d_rand() % NumNetPlayerPositions;

			closest_dist = 0x7fffffff;

			for (i=0; i<N_players; i++ )	{
				if ( (i!=Player_num) && (Objects[Players[i].objnum].type == OBJ_PLAYER) )	{
					dist = find_connected_distance(&Objects[Players[i].objnum].pos, Objects[Players[i].objnum].segnum, &Player_init[NewPlayer].pos, Player_init[NewPlayer].segnum, 15, WID_FLY_FLAG ); // Used to be 5, search up to 15 segments
					if ( (dist < closest_dist) && (dist >= 0) )	{
						closest_dist = dist;
					}
				}
			}
		} while ( (closest_dist<i2f(15*20)) && (trys<MAX_PLAYERS*2) );
	}
	else
	{
		// If deathmatch and not random, positions were already determined by sync packet
		reset_player_object();
		reset_cruise();
		return;
	}

	Assert(NewPlayer >= 0);
	Assert(NewPlayer < NumNetPlayerPositions);
	ConsoleObject->pos = Player_init[NewPlayer].pos;
	ConsoleObject->orient = Player_init[NewPlayer].orient;
#ifdef NETWORK	
	if ((Game_mode & GM_MULTI) && (Netgame.SpawnStyle == SPAWN_STYLE_PREVIEW) && Dead_player_camera != NULL) {
		ConsoleObject->orient = Dead_player_camera->orient;  
		Dead_player_camera = NULL; 
	}

	if (is_observer()) {
		ConsoleObject->pos = Objects[Players[Current_obs_player].objnum].pos;
		ConsoleObject->orient = Objects[Players[Current_obs_player].objnum].orient;
		obj_relink_all();
	} else
#endif	
		obj_relink(ConsoleObject-Objects,Player_init[NewPlayer].segnum);
	reset_player_object();
	reset_cruise();
}

//	-----------------------------------------------------------------------------------------------------
//	Initialize default parameters for one robot, copying from Robot_info to *objp.
//	What about setting size!?  Where does that come from?
void copy_defaults_to_robot(object *objp)
{
	robot_info	*robptr;
	int			objid;

	Assert(objp->type == OBJ_ROBOT);
	objid = objp->id;
	Assert(objid < N_robot_types);

	robptr = &Robot_info[objid];

	objp->shields = robptr->strength;

}

//	-----------------------------------------------------------------------------------------------------
//	Copy all values from the robot info structure to all instances of robots.
//	This allows us to change bitmaps.tbl and have these changes manifested in existing robots.
//	This function should be called at level load time.
void copy_defaults_to_robot_all(void)
{
	int	i;

	for (i=0; i<=Highest_object_index; i++)
		if (Objects[i].type == OBJ_ROBOT)
			copy_defaults_to_robot(&Objects[i]);
}

int	Do_appearance_effect=0;

//	-----------------------------------------------------------------------------------------------------
//called when the player is starting a level (new game or new ship)
void StartLevel(int random)
{
	Assert(!Player_is_dead);

	InitPlayerPosition(random);

	verify_console_object();

	ConsoleObject->control_type	= CT_FLYING;
	ConsoleObject->movement_type	= MT_PHYSICS;

	// create_player_appearance_effect(ConsoleObject);
	Do_appearance_effect = 1;

	if (Game_mode & GM_MULTI)
	{
		if ((Game_mode & GM_MULTI_COOP) || (Game_mode & GM_MULTI_ROBOTS))
			multi_send_score();
	 	multi_send_reappear();
		multi_do_protocol_frame(1, 1);
	}
	else // in Singleplayer, after we died ...
	{
		disable_matcens(); // ... disable matcens and ...
		clear_transient_objects(0); // ... clear all transient objects.
	}

	ai_reset_all_paths();
	ai_init_boss_for_ship();

	reset_rear_view();
	Auto_fire_fusion_cannon_time = 0;
	Fusion_charge = 0;

	if (!(Game_mode & GM_MULTI)) // stuff for Singleplayer only
	{

	}
}


