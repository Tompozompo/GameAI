#ifdef _WIN32
#include "windows.h"
#endif

#include "math.h"
#include "stdlib.h"
#include "string.h"

#include "GL/gl.h"
#include "GL/glu.h"
#include "SDL.h"
#include "SDL_image.h"
#include "SDL_mixer.h"
#include "SDL_net.h"

#include "F1Spirit.h"
#include "sound.h"
#include "List.h"
#include "2DCMC.h"
#include "auxiliar.h"
#include "GLTile.h"
#include "PlacedGLTile.h"
#include "RotatedGLTile.h"
#include "keyboardstate.h"
#include "CPlayer.h"
#include "RoadPiece.h"
#include "CCar.h"
#include "2DCMC.h"
#include "track.h"
#include "RacingCCar.h"
#include "PlayerCCar.h"
#include "EnemyCCar.h"
#include "GameParameters.h"
#include "ReplayInfo.h"
#include "F1SpiritGame.h"
#include "F1SpiritApp.h"
#include "F1Spirit-auxiliar.h"
#include "randomc.h"

#include "debug.h"

#ifdef KITSCHY_DEBUG_MEMORY
#include "debug_memorymanager.h"
#endif

extern TRanrotBGenerator *rg;

EnemyCCar::EnemyCCar() : RacingCCar()
{
	car = 0;
	road_offset_change = 0;
	road_offset = 0;
	offset_dif = 0;
	AI_type = 0;
} 


EnemyCCar::~EnemyCCar()
{
	delete car;
	car = 0;
} 


bool EnemyCCar::cycle(CRoadPiece *current_piece, CTrack *track, List<RacingCCar> *cars, List<F1S_SFX> *sfx, int sfx_volume, List<RacingCCar> **car_grid, int car_grid_sx, int car_grid_sy)
{
	int turn = 0;
	bool accelerate = false;
	bool brake = false;
	float offset;
	int terrain = -1;

	/* Follow the car's position && determine terrain: */

	if (!road_position.EmptyP()) {
		LLink<CRoadPiece> *old_pos, *pos;
		CRoadPiece *rp;
		float t1, t2, t3, t4, t5;
		float d, min_d;
		int i;

		old_pos = pos = road_position.GetPos();
		rp = road_position.GetObj();
		rp->get_path_position(car->get_x(), car->get_y(), &t1, &t2, &t3, &t4, &t5, &min_d);

		if (rp->type != 0 && rp->inside_test(car->get_x(), car->get_y())) {
			terrain = 0;
		} 

		for (i = 0;i < 4;i++) {
			if (road_position.LastP()) {
				road_position.Rewind();
			} else {
				road_position.Next();
			} 

			rp = road_position.GetObj();

			rp->get_path_position(car->get_x(), car->get_y(), &t1, &t2, &t3, &t4, &t5, &d);

			if (d < min_d) {
				min_d = d;
				pos = road_position.GetPos();

				if (terrain == -1 && rp->type != 0 && rp->inside_test(car->get_x(), car->get_y())) {
					terrain = 0;
				} 
			} 
		} 

		if (min_d < 160) {
			int p1 = 0, p2 = 0;
			road_position.SetPos(old_pos);
			p1 = road_position.PositionRef(road_position.GetObj());
			road_position.SetPos(pos);
			p2 = road_position.PositionRef(road_position.GetObj());
			/* Passed by the start line: */

			if (p2 < p1) {
				if (first_lap)
					laps++;
				else
					first_lap = true;

				//#ifdef F1SPIRIT_DEBUG_MESSAGES
				//    output_debug_message("Enemy player car completed %i laps\n",laps);
				//#endif
			} 
		} else {
			road_position.SetPos(old_pos);
		} 

		if (terrain == -1) {
			CRoadPiece *rp;
			int i = road_position.PositionRef(road_position.GetObj());

			if (i == 0) {
				rp = road_position[road_position.Length() - 1];
			} else {
				rp = road_position[i - 1];
			} 

			if (rp->type != 0 &&
			        rp->inside_test(car->get_x(), car->get_y())) {
				terrain = 0;
			} 
		} 

		if (terrain == -1) {
			if (track->get_background_type() == 1)
				terrain = 1;

			if (track->get_background_type() == 2)
				terrain = 2;
		} 

	} 

	/* Compute position: */
	{
		List<CRoadPiece> lr;
		CRoadPiece *rp;
		int effective_laps;

		if (!first_lap) {
			effective_laps = -1;
		} else {
			effective_laps = laps;
		} 

		/* laps: */
		position = effective_laps * track->get_length();

		/* pieces: */
		lr.Instance(*track->get_road());

		lr.Rewind();

		while (lr.Iterate(rp) && rp != road_position.GetObj())
			position += rp->get_length();

		/* offset inside current piece: */
		/* simplification: inverse of the distance to end of piece = distance */
		{
			position += rp->get_length();
			rp = road_position.GetObj();
			float d = float(sqrt((car->get_x() - rp->get_x2()) * (car->get_x() - rp->get_x2()) +
			                     (car->get_y() - rp->get_y2()) * (car->get_y() - rp->get_y2())));
			position -= d;
		}

	}

	/* Enemy AI: */
	if (car->get_state() == 1 || car->get_state() == 3) {
        EnemyAI(&accelerate, &brake, &turn, &offset, terrain, cars, car_grid, car_grid_sx, car_grid_sy);		
	} 

	/* Drive the enemy car: */
    CRoadPiece *rp = road_position.GetObj();
    bool offset_computed = rp->offset_from_road_center(car->get_x(), car->get_y(), &offset, &rangle);
	if (offset_computed) {
		if (offset > -32 &&
		        offset < 32)
			return car->cycle(accelerate, brake, 0, turn, current_piece, track, cars, sfx, sfx_volume, false, terrain, position, car_grid, car_grid_sx, car_grid_sy);
		else
			return car->cycle(accelerate, brake, 0, turn, current_piece, track, cars, sfx, sfx_volume, true, terrain, position, car_grid, car_grid_sx, car_grid_sy);
	} else {
		    return car->cycle(accelerate, brake, 0, turn, current_piece, track, cars, sfx, sfx_volume, true, terrain, position, car_grid, car_grid_sx, car_grid_sy);
	} 
} 


bool EnemyCCar::load_status(FILE *fp)
{
	road_offset_change = fgetc(fp);
	road_offset_change += fgetc(fp) * 256;

	if (road_offset_change == 65535)
		road_offset_change = -1;

	load_float(fp, &road_offset);

	return car->load_status(fp);
} 


bool EnemyCCar::save_status(FILE *fp)
{
	fputc(road_offset_change&0x0ff, fp);
	fputc(road_offset_change >> 8, fp);

	save_float(fp, road_offset);

	return car->save_status(fp);
} 





