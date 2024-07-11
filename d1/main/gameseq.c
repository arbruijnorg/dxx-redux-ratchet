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
			if (!fp == NULL) {
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

int create_path_partime(int start_seg, int segnum, int check_id, int simulatedEnergy, int* doneList, int* doneListLength) // A lot of this is copied from the mark_player_path_to_segment function in game.c.
{
	int i;
	int j;
	int n;
	int c;
	object* objp = ConsoleObject;
	short		player_path_length = 0;
	int		player_hide_index = -1;
	double pathLength = 0; // player_path_length is just the number of segments it goes through. This is the units, which is what's important for par time, as segments can be all sorts of sizes.
	int lockSegs = 0; // How many times the same locked wall appears in the same path. If twice, that means we've passed through it, so return the unlock ID. I sure hope no levels abuse this logic's hole...
	int matcenHealth = 0; // How beefy are the all the matcens we're passing through combined?
	int lockedWallIDs[MAX_WALLS] = {0}; // Remember the IDs of all walls in the level that are locked in some way, so we can see if any of our paths encounter them.
	int lockedWallsLength = 0; // How many locked walls there are so far, for incrementing the list.
	int highestHP = -1;
	Ranking.pathfinds++;
	ConsoleObject->segnum = start_seg; // We're gonna teleport the player to every one of the starting segments, then put him back at spawn in time for the level to start.
	for (i = 0; i < MAX_WALLS; i++) {
		if ((Walls[i].type == WALL_DOOR && (Walls[i].keys == KEY_BLUE || Walls[i].keys == KEY_GOLD || Walls[i].keys == KEY_RED)) || Walls[i].flags == WALL_DOOR_LOCKED) {
			lockedWallIDs[lockedWallsLength] = i;
			lockedWallsLength++;
		}
	}
	if (create_path_points(objp, objp->segnum, segnum, Point_segs_free_ptr, &player_path_length, 100, 0, 0, -1) == -1) {
		if (check_id == 0)
			return vm_vec_dist(&Point_segs[0].point, &Point_segs[player_path_length].point); // If we can't find a valid path to the target, just draw a straight line to it and measure that to avoid softlocking.
		if (check_id == 1)
			return -1;
		if (check_id == 2)
			return 0;
		if (check_id == 3)
			return simulatedEnergy;
	}
	player_hide_index = Point_segs_free_ptr - Point_segs;
	if (check_id == 0) { // Find length of path in units and return it.
		for (i = 0; i < player_path_length - 1; i++) {
			pathLength += vm_vec_dist(&Point_segs[i].point, &Point_segs[i + 1].point);
		}
		return pathLength;
	}
	if (check_id == 1) { // Check for locked doors blocking the path, then return either -1, or the ID of what unlocks it. Thanks to Sirius for help with this part.
		for (i = 0; i < player_path_length - 1; i++) {
			for (j = 0; j < lockedWallsLength; j++) {
				int side = find_connecting_side(&Point_segs[i], &Point_segs[i + 1]);
				if (Segments[Point_segs[i].segnum].sides[side].wall_num == lockedWallIDs[j]) {
					if (Walls[lockedWallIDs[j]].type == WALL_DOOR) {
						if (Walls[lockedWallIDs[j]].keys == KEY_BLUE) {
							for (n = 0; n <= Highest_object_index; n++) {
								if ((Objects[n].type == OBJ_POWERUP && Objects[n].id == POW_KEY_BLUE) || (Objects[n].type == OBJ_ROBOT && Objects[n].contains_type == OBJ_POWERUP && Objects[n].contains_id == POW_KEY_BLUE))
									return n;
							}
						}
						if (Walls[lockedWallIDs[j]].keys == KEY_GOLD) {
							for (n = 0; n <= Highest_object_index; n++) {
								if ((Objects[n].type == OBJ_POWERUP && Objects[n].id == POW_KEY_GOLD) || (Objects[n].type == OBJ_ROBOT && Objects[n].contains_type == OBJ_POWERUP && Objects[n].contains_id == POW_KEY_GOLD))
									return n;
							}
						}
						if (Walls[lockedWallIDs[j]].keys == KEY_RED) {
							for (n = 0; n <= Highest_object_index; n++) {
								if ((Objects[n].type == OBJ_POWERUP && Objects[n].id == POW_KEY_RED) || (Objects[n].type == OBJ_ROBOT && Objects[n].contains_type == OBJ_POWERUP && Objects[n].contains_id == POW_KEY_RED))
									return n;
							}
						}
					}
					if (Walls[lockedWallIDs[j]].flags == WALL_DOOR_LOCKED) {
						for (n = 0; n < Num_walls; n++) {
							if (n == lockedWallIDs[j] || Walls[n].trigger == -1)
								continue;
							trigger* t = &Triggers[Walls[n].trigger];
							for (short link_num = 0; link_num < t->num_links; link_num++) {
								if (t->seg[link_num] == Walls[lockedWallIDs[j]].segnum && t->side[link_num] == Walls[lockedWallIDs[j]].sidenum)
									return Walls[n].segnum + MAX_OBJECTS + 1; // Return a value out of bounds for objects as a way to have the to-do list differentiate between them and triggers.
							}
						} // Keep in mind: This doesn't guarantee the NEAREST trigger for the locked wall, but that'll prooooobably be fine.
						for (short n = 0; n < ControlCenterTriggers.num_links; n++) { // So doors opened by the reactor exploding are handled correctly.
							if (ControlCenterTriggers.seg[n] == Walls[lockedWallIDs[j]].segnum && ControlCenterTriggers.side[n] == Walls[lockedWallIDs[j]].sidenum) {
								for (c = 0; c <= Highest_object_index; c++) {
									if (Objects[c].type == OBJ_CNTRLCEN)
										return c;
								}
							}
						}
					}
				}
			}
		}
		return -1;
	}
	if (check_id == 2) { // Check for matcens blocking the path, return the total HP of all the robots that could possibly come out in one round.
		for (i = 0; i <= Highest_segment_index; i++) {
			if (Segments[i].special == SEGMENT_IS_ROBOTMAKER) {
				for (j = 0; j < player_path_length; j++) {
					if (i == Point_segs[j].segnum) {
						if (RobotCenters[Segments[i].matcen_num].robot_flags[0] != 0) {
							uint	flags;
							sbyte	legal_types[32];		//	32 bits in a word, the width of robot_flags.
							int	num_types, robot_index;
							robot_index = 0;
							num_types = 0;
							flags = RobotCenters[Segments[i].matcen_num].robot_flags[0];
							while (flags) {
								if (flags & 1)
									legal_types[num_types++] = robot_index;
								flags >>= 1;
								robot_index++;
							}
							for (n = 0; n < num_types; n++) {
								if (Robot_info[legal_types[n]].strength > highestHP || highestHP < 0)
									highestHP = Robot_info[legal_types[n]].strength;
							}
							matcenHealth += highestHP; // Find the highest robot HP in this matcen and increment by that. 0 is a placeholder.
						}
					}
				}
			}
		}
		return matcenHealth;
	}
	if (check_id == 3) { // Check if algo goes through fuelcen and regen its energy if so. Also check for energy pickups along the path. We need to do it in path order so simulatedEnergy stays accurate.
		int energySources[MAX_OBJECTS + MAX_SEGMENTS] = { 0 }; // We should never need anywhere near this many slots, but just to be safe.
		int numSources = 0;
		for (i = 0; i <= Highest_object_index; i++) {
			if (Objects[i].type == OBJ_POWERUP && Objects[i].id == POW_ENERGY) {
				energySources[numSources] = i;
				numSources++;
			}
		}
		for (i = 0; i <= Highest_segment_index; i++) {
			if (Segments[i].special == SEGMENT_IS_FUELCEN) {
				energySources[numSources] = i + MAX_OBJECTS;
				numSources++;
			}
		}
		for (i = 0; i < player_path_length; i++) {
			for (j = 0; j < numSources; j++) {
				if (Point_segs[i].segnum == energySources[j]) {
					if (energySources[j] < MAX_OBJECTS + 1) {
						int thisSourceCollected = 0;
						for (n = 0; n < *doneListLength; n++) {
							if (doneList[n] = energySources[j])
								thisSourceCollected = 1;
						}
						if (thisSourceCollected == 0) {
							simulatedEnergy += 15728640;
							if (simulatedEnergy > 524288000)
								simulatedEnergy = 524288000;
							doneList[*doneListLength] = energySources[j];
							doneListLength++;
						}
					}
					else if (simulatedEnergy < 262144000)
						simulatedEnergy = 262144000;
				}
			}
		}
		return simulatedEnergy;
	}
}

void addItemToToDoList(int* toDoList, int* toDoListLength, int itemID)
{
	// First make sure it's not already in there
	for (int i = 0; i < *toDoListLength; i++)
		if (toDoList[i] == itemID)
			return;
	// Next let's make sure it's not the reactor, because we don't wanna go there until we have loops.
	if (Objects[itemID].type == OBJ_CNTRLCEN)
			return;

	toDoList[*toDoListLength] = itemID;
	(*toDoListLength)++;
}

void removeItemFromToDoList(int* toDoList, int* toDoListLength, int itemID)
{
	if (*toDoListLength == 0)
		return;

	int offset = 0;
	for (offset = 0; offset < *toDoListLength; offset++)
		if (toDoList[offset] == itemID)
			break;
	if (offset + 1 < *toDoListLength)
		memmove(toDoList + offset, toDoList + offset + 1, (*toDoListLength - offset - 1) * sizeof(int));
	(*toDoListLength)--;
}

int calculateParTime() // Here is where we have an algorithm run a simulated path through a level to determine how long the player should take. It always assumes slowest scenario to ensure the player can beat it.
{ // Honestly we're so lucky this algorithm even works remotely well to begin with. "Spaghetti code" is an understatement, as it spans several different Ratchet IQ scores. It works though, so I'll take it.
	double levelDistance = 0; // Variable to track how much distance it's travelled.
	double levelHealth = 0; // Variable to track how much damage it's had to do.
	int simulatedEnergy = 262144000; // Variable to tell it when to refill its energy. It is always equipped with Laser 1 and never uses anything else.
	int toDoList[MAX_OBJECTS + MAX_TRIGGERS] = { 0 }; // List of remaining objects the algorithm has to travel to.
	int doneList[MAX_OBJECTS + MAX_TRIGGERS] = { 0 }; // List of objects the algorithm has already travelled to.
	int blacklist[MAX_OBJECTS + MAX_TRIGGERS] = { 0 }; // List of objects the algorithm is currently not allowed to travel to.
	int nearestID = 0; // ID of the nearest relevant thing, so we can get its position and health.
	int segnum = ConsoleObject->segnum; // Start the algorithm off where the player spawns.
	int initialSegnum = ConsoleObject->segnum; // Version of segnum that stays at its initial value, to ensure the player is put in the right spot.
	int toDoListLength = 0; // Keeps track of how many items are in toDoList so the algorithm knows everything is done when = 0.
	int doneListLength = 0; // Keeps track of how many items are in doneList so it knows what index to add newly done objects to.
	int blacklistLength = 0; // Keeps track of how many items are in... okay I think you get the point.
	int unlockID = -1; // ID of thing needed to unlock something. If negative, that means it's unlocked.
	int i;
	int j;
	int c;
	int id; // ID we're currently pathing to.
	int highestHP; // The shields of the strongest enemy in a relevant matcen.
	int loops = 0; // How many times the pathmaking process has repeated. This determines what toDoList is populated with, to make sure things are gone to in the right order.
	Ranking.pathfinds = 0; // If par time still not calculated after a million pathfinding operations, assume softlock and give up, returning current values. 1000000 should be mathematically impossible, even with 1000 objects.
	double pathLength; // Store create_path_partime's result in pathLength to compare to current shortest.
	double shortestPathLength = -1; // For storing the shortest path found so far to determine which object to draw to first.
	while (loops < 3 && Ranking.pathfinds < 1000000) {
		if (loops == 0) {
			for (i = 0; i <= Highest_object_index; i++) { // Populate the to-do list with all robots and hostages. Ignore robots not worth over zero, as the player isn't gonna go for those. This should never happen, but it's just a failsafe.
				if ((Objects[i].type == OBJ_ROBOT && Robot_info[Objects[i].id].score_value > 0 && Robot_info[Objects[i].id].boss_flag == 0) || Objects[i].type == OBJ_HOSTAGE) {
					toDoList[toDoListLength] = i;
					toDoListLength++;;
					unlockID = create_path_partime(segnum, Objects[i].segnum, 1, simulatedEnergy, doneList, &doneListLength);
					if (unlockID > -1) {
						addItemToToDoList(toDoList, &toDoListLength, unlockID);
					}
				}
			}
		}
		if (loops == 1) {
			for (i = 0; i <= Highest_object_index; i++) { // Populate the to-do list with all reactors and bosses.
				if ((Objects[i].type == OBJ_ROBOT && Robot_info[Objects[i].id].boss_flag != 0) || Objects[i].type == OBJ_CNTRLCEN) {
					toDoList[toDoListLength] = i;
					toDoListLength++;;
					unlockID = create_path_partime(segnum, Objects[i].segnum, 1, simulatedEnergy, doneList, &doneListLength);
					if (unlockID > -1) {
						addItemToToDoList(toDoList, &toDoListLength, unlockID);
					}
				}
			}
		}
		if (loops == 2) { // Put the exit on the list.
			for (i = 0; i <= Num_triggers && toDoListLength == 0; i++) {
				if (Triggers[i].flags == TRIGGER_EXIT) {
					for (j = 0; j <= Num_walls; j++) {
						if (Walls[j].trigger == i) {
							toDoList[toDoListLength] = Walls[j].segnum + MAX_OBJECTS + 1;
							toDoListLength++;
							unlockID = create_path_partime(segnum, Walls[j].segnum, 1, simulatedEnergy, doneList, &doneListLength);
							if (unlockID > -1) {
								addItemToToDoList(toDoList, &toDoListLength, unlockID);
							}
						}
					}
				}
			}
		}
		while (toDoListLength > 0) {
			shortestPathLength = -1;
			for (i = 0; i < toDoListLength; i++) { // Find which object on the to-do list is the closest, ignoring the reactor/boss if it's not the only thing left.
				id = toDoList[i];
				if (id > MAX_OBJECTS) {
					pathLength = create_path_partime(segnum, id -  MAX_OBJECTS - 1, 0, simulatedEnergy, doneList, &doneListLength);
					unlockID = create_path_partime(segnum, id - MAX_OBJECTS - 1, 1, simulatedEnergy, doneList, &doneListLength); // Just in case there are multiple layers of triggers unlocking doors (D1 level 21 red room).
					if (unlockID > -1) {
						for (c = 0; c < doneListLength; c++) { // Check everything the algorithm has done to see if we have what unlocks the obstacle. If so, mark it as unlocked so it can pass through.
							if (doneList[c] == unlockID) {
								unlockID = -1;
							}
						}
					}
					if (unlockID > -1) {
						addItemToToDoList(toDoList, &toDoListLength, unlockID);
						removeItemFromToDoList(toDoList, &toDoListLength, id);
						blacklist[blacklistLength] = id;
						blacklistLength++;
					}
				}
				else
					pathLength = create_path_partime(segnum, Objects[id].segnum, 0, simulatedEnergy, doneList, &doneListLength);
				if (pathLength < shortestPathLength || shortestPathLength < 0) {
					shortestPathLength = pathLength;
					nearestID = id;
				}
			}
			int nearestTargetSegnum = (nearestID > MAX_OBJECTS) ? (nearestID - MAX_OBJECTS - 1) : Objects[nearestID].segnum;
			unlockID = create_path_partime(segnum, nearestTargetSegnum, 1, simulatedEnergy, doneList, &doneListLength);
			if (unlockID > -1) {
				for (i = 0; i < doneListLength; i++) { // Check everything the algorithm has done to see if we have what unlocks the obstacle. If so, mark it as unlocked so it can pass through.
					if (doneList[i] == unlockID) {
						unlockID = -1;
					}
				}
			}
			if (unlockID > -1) {
				removeItemFromToDoList(toDoList, &toDoListLength, nearestID);
				blacklist[blacklistLength] = nearestID;
				blacklistLength++;
			}
			else {
				removeItemFromToDoList(toDoList, &toDoListLength, nearestID);
				doneList[doneListLength] = nearestID;
				doneListLength++;
				if (nearestID > MAX_OBJECTS || Objects[nearestID].type == OBJ_CNTRLCEN || (Objects[i].type == OBJ_ROBOT && Robot_info[Objects[i].id].boss_flag != 0) || Objects[nearestID].type == OBJ_POWERUP) { // We don't have to specify what powerup since keys are the only ones the algorithm looks for.
					for (i = 0; i < blacklistLength; i++) { // Clear the blacklist, because we just unlocked something.
						toDoList[toDoListLength + i] = blacklist[i];
					}
					toDoListLength += blacklistLength;
					blacklistLength = 0;
				}
				simulatedEnergy = create_path_partime(segnum, nearestTargetSegnum, 3, simulatedEnergy, doneList, &doneListLength); // Do energy stuff.
				if (nearestID < MAX_OBJECTS) { // We don't fight triggers.
					if (Objects[nearestID].type == OBJ_ROBOT || Objects[nearestID].type == OBJ_CNTRLCEN) { // We don't fight keys or hostages.
						levelHealth += Objects[nearestID].shields;
						simulatedEnergy -= Objects[nearestID].shields; // Subtract how much energy killing this robot would cost with laser 1. This variable's max value is scaled to be 1:1 with damage.
						if (Objects[nearestID].contains_type == OBJ_ROBOT) { // Now we account for robots guaranteed to drop from this robot, if any.
							levelHealth += Robot_info[Objects[nearestID].contains_id].strength * Objects[nearestID].contains_count;
							simulatedEnergy -= Robot_info[Objects[nearestID].contains_id].strength * Objects[nearestID].contains_count;
						}
						if (Objects[nearestID].contains_type == OBJ_POWERUP && Objects[nearestID].contains_id == POW_ENERGY) {
							simulatedEnergy += 15728640 * Objects[nearestID].contains_count; // If the robot is guaranteed to drop energy, give it to algo so it doesn't visit fuelcens more than needed.
							if (simulatedEnergy > 524288000)
								simulatedEnergy = 524288000; // Cap algo's energy at 200, like the player's.
						}
					}
				}
				highestHP = 0; //create_path_partime(segnum, nearestTargetSegnum, 2); // For matcen check. Disabled because Boli's par time was massively inflated.
				if (highestHP > 0) {
					levelHealth += highestHP * 7;
					simulatedEnergy -= highestHP * 7;
				}
				printf("Path from segment %i to %i: %.3fs\n", segnum, nearestTargetSegnum, shortestPathLength / 3709337.6);
				segnum = nearestTargetSegnum;
				levelDistance += shortestPathLength;
				shortestPathLength = -1;
				if (simulatedEnergy < 0 && !((loops == 1 && toDoListLength == 0) || loops == 2)) { // Algo's energy's out. If not running for exit, search for nearest fuelcen, go to it and recharge.
					for (i = 0; i <= Highest_segment_index; i++) {
						if (Segments[i].special == SEGMENT_IS_FUELCEN) {
							pathLength = create_path_partime(segnum, i, 0, simulatedEnergy, doneList, &doneListLength);
							unlockID = create_path_partime(segnum, i, 1, simulatedEnergy, doneList, &doneListLength);
							if (unlockID > -1) { // If nearest fuelcen is locked behind an inaccessible door, pick another one if available.
								for (j = 0; j < doneListLength; j++) {
									if (doneList[j] == unlockID) {
										unlockID = -1;
									}
								}
							}
							if (unlockID < 0 && (pathLength < shortestPathLength || shortestPathLength < 0)) {
								shortestPathLength = pathLength;
								nearestID = i;
								highestHP = 0; //create_path_partime(segnum, nearestTargetSegnum, 2); // For matcen check. Disabled because Boli's par time was massively inflated.
								if (highestHP > 0) {
									levelHealth += highestHP * 7;
									simulatedEnergy -= highestHP * 7;
								} // I know we're out of energy here but it'd be unfair not to account for matcens in the way of the fuelcen. In practice, players will come to these at low energy, not zero.
							}
						}
					}
					simulatedEnergy = 262144000; // We always reset energy to avoid a softlock, even if a valid path to a fuelcen couldn't be found.
					if (shortestPathLength > -1) { // Only update algo's location and increment level distance if a fuelcen was found.
						printf("Path from segment %i to %i: %0.3fs (FUELCEN)\n", segnum, nearestID, shortestPathLength / 3709337.6);
						segnum = nearestID;
						levelDistance += shortestPathLength;
					}
				}
			}
		}
		loops++;
	}
	ConsoleObject->segnum = initialSegnum;
	printf("Par time: %.3fs (%.3f movement, %.3f combat)\n", levelDistance / 3709337.6 + levelHealth / 5242880, levelDistance / 3709337.6, levelHealth / 5242880);
	return (levelDistance / 3709337.6) + (levelHealth / 5242880); // levelDistance divisor is ship's movement speed and levelHealth divisor is laser 1's DPS.
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
	double levelHealth = 0; // For testing purposes.
	for (i = 0; i <= Highest_object_index; i++) {
		if (Objects[i].type == OBJ_ROBOT) {
			Ranking.maxScore += Robot_info[Objects[i].id].score_value;
			levelHealth += Objects[i].shields;
			if (Objects[i].contains_type == OBJ_ROBOT && Objects[i].contains_count > 0) {
				Ranking.maxScore += Robot_info[Objects[i].contains_id].score_value * Objects[i].contains_count;
				levelHealth += Robot_info[Objects[i].contains_id].strength * Objects[i].contains_count;
			}
		}
		if (Objects[i].type == OBJ_CNTRLCEN) {
			Ranking.maxScore += CONTROL_CEN_SCORE;
			levelHealth += Objects[i].shields;
		}
		if (Objects[i].type == OBJ_HOSTAGE) {
			Ranking.maxScore += HOSTAGE_SCORE;
		}
	}
	Ranking.maxScore *= 3; // This is not the final max score. Max score is still 7500 higher per hostage, but that's added last second so the time bonus can be weighted properly.
	Ranking.maxScore = (int)Ranking.maxScore;
	Ranking.parTime = calculateParTime();
	Ranking.parTime = (int)Ranking.parTime; // Truncate the par time so it looks better/legible on the result screen, and leaves room for the time bonus.
	printf("Expected combat time: %.3fs\n", levelHealth / 5242880);
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


