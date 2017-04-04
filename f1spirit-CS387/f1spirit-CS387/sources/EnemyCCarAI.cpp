//
//  EnemyCCarAI.cpp
//  F-1 Spirit
//
//  Created by Santiago Ontanon on 4/5/12.
//  Copyright (c) 2012 Drexel University. All rights reserved.
//

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



void EnemyCCar::EnemyAI(bool *accelerate, bool *brake, int *turn, float *offset, int terrain, List<RacingCCar> *cars, List<RacingCCar> **car_grid, int car_grid_sx, int car_grid_sy)
{
	CRoadPiece *rp = road_position.GetObj();
	
	bool ahead_offset_computed = false, ahead2_offset_computed = false;
	// float ahead_offset=0,ahead2_offset=0;
	float ahead_adif;
	float cangle = car->get_a() - 90;
	float max_speed = car->get_max_speed();
	float current_speed = car->get_speed();
	float fwx;
	float fwy;
	float vx = car->get_speed_x();
	float vy = car->get_speed_y();
	float vn = float(sqrt(vx * vx + vy * vy));
	int road_wide_to_ignore = 160;
	int position_range = 4;
    bool offset_computed = false;
    
	if (vn == 0) {
		vx = 1;
		vy = 0;
	} else {
		vx /= vn;
		vy /= vn;
	} 
    
	*accelerate = true;
	*brake = false;
	*turn = 0;
	rangle = 0;
	ahead_rangle = 0;
	ahead2_rangle = 0;
    
	if (track->road[0]->get_w1() >= 256) {
		road_wide_to_ignore = 260;
		position_range = 12;
	} 
    
	/* Position in which the car will be if continuing straight for 96 pixels: */
	if (rp->get_w1() == rp->get_w2() && rp->get_w1() < road_wide_to_ignore) {
		/* Compute ahead: */
		{
			int i;
			bool tmp, tmp2;
			float t_fwx, t_fwy, t_fwz, t_fwa, t_fww, min_d;
			float t_fwx2, t_fwy2, t_fwz2, t_fwa2, t_fww2, min_d2;
			List<CRoadPiece> road_p2;
			CRoadPiece *rp2 = rp, *rp2n;
            
			road_p2.Instance(road_position);
			road_p2.Synchronize(&road_position);
			road_p2.Next();
            
			if (road_p2.EndP())
				road_p2.Rewind();
            
			rp2n = road_p2.GetObj();
            
			fwx = car->get_x();
            
			fwy = car->get_y();
            
			tmp = rp2->get_path_position(fwx, fwy, &t_fwx, &t_fwy, &t_fwz, &t_fwa, &t_fww, &min_d);
            
			fwx = t_fwx;
            
			fwy = t_fwy;
            
			ahead_offset_computed = false;
            
			ahead2_offset_computed = false;
            
			for (i = 0;i < 8 && tmp;i++) {
				fwx = float(fwx + 64 * cos((t_fwa * M_PI) / 180.0F));
				fwy = float(fwy + 64 * sin((t_fwa * M_PI) / 180.0F));
				tmp = rp2->get_path_position(fwx, fwy, &t_fwx, &t_fwy, &t_fwz, &t_fwa, &t_fww, &min_d);
				{
					tmp2 = rp2n->get_path_position(fwx, fwy, &t_fwx2, &t_fwy2, &t_fwz2, &t_fwa2, &t_fww2, &min_d2);
                    
					if (min_d2 < min_d) {
						t_fwx = t_fwx2;
						t_fwy = t_fwy2;
						t_fwz = t_fwz2;
						t_fwa = t_fwa2;
						tmp = tmp2;
						rp2 = rp2n;
						road_p2.Next();
                        
						if (road_p2.EndP())
							road_p2.Rewind();
                        
						rp2n = road_p2.GetObj();
					} 
				}
                
				fwx = t_fwx;
				fwy = t_fwy;
                
				if (i == 3) {
					ahead_offset_computed = tmp;
					ahead_rangle = t_fwa;
				} 
                
				if (i == 7) {
					ahead2_offset_computed = tmp;
					ahead2_rangle = t_fwa;
				} 
			} 
		}
        
		/* Current position: */
        offset_computed = rp->offset_from_road_center(car->get_x(), car->get_y(), offset, &rangle);
        
		if (!offset_computed) {
			int i = road_position.PositionRef(rp);
			i--;
            
			if (i < 0)
				i = road_position.Length() - 1;
            
			offset_computed = road_position[i]->offset_from_road_center(car->get_x(), car->get_y(), offset, &rangle);
		} 
        
		/* Test if inside PIT: */
		{
			bool inside_pit = false;
			{
				float cx = fwx, cy = fwy;
                
				if (track->pit_angle != 0) {
					float tx = car->get_x() - track->pit_x[0];
					float ty = car->get_y() - track->pit_y[0];
					float a = track->pit_angle * M_PI / 180.0F;
					cx = float(tx * cos(a) + ty * sin(a)) + track->pit_x[0];
					cy = float( -tx * sin(a) + ty * cos(a)) + track->pit_y[0];
				} 
                
				if (cx >= track->pit_x[0] &&
                    cx < track->pit_x[1] &&
                    cy >= track->pit_y[0] &&
                    cy < track->pit_y[1]) {
					inside_pit = true;
				} 
			}
            
			if (inside_pit) {
				road_offset = float(1 + (rg->IRandom(0, 3)) * 8);
				road_offset_change = 500;
			} 
		}
	} 
    
	if (road_offset_change < 0) {
		if (offset_computed) {
			road_offset = *offset;
			road_offset_change = 250 + (rg->IRandom(0, 500));
		} 
	} else {
		if (road_offset_change == 0) {
			road_offset = float(( -position_range + (rg->IRandom(0, position_range * 2))) * 8);
			road_offset_change = 250 + (rg->IRandom(0, 500));
		} else {
			road_offset_change--;
		} 
        
		if (offset_computed) {
			float adif = cangle - rangle;
			float allowed_adif = float(fabs(road_offset - *offset) / 3);
            
			if (allowed_adif < 10)
				allowed_adif = 10;
            
			while (adif > 180)
				adif -= 360;
            
			while (adif < -180)
				adif += 360;
            
			if (adif > 2)
				*turn = -1;
            
			if (adif < -2)
				*turn = 1;
            
			if (adif <= allowed_adif && adif >= -allowed_adif) {
				if (*offset > road_offset + 8) {
					*turn = -1;
				} 
                
				if (*offset < road_offset - 8) {
					*turn = 1;
				} 
			} 
		} 
	} 
    
	// If there is a curve in front and the car goes too fast, brake:
	if (offset_computed && ahead2_offset_computed) {
		float f = 1;
        
		ahead_adif = rangle - ahead2_rangle;
        
		while (ahead_adif > 180)
			ahead_adif -= 360;
        
		while (ahead_adif < -180)
			ahead_adif += 360;
        
		offset_dif = float(fabs(ahead_adif));
        
		if (AI_type == 3) {
			if (ahead_adif > 5) {
				road_offset = float(-position_range * 8);
			} 
            
			if (ahead_adif < -5) {
				road_offset = float(position_range * 8);
			} 
		} 
        
        int offset_car_type[6] = {60,80,65,60,60,50};
        int offset_car_type_max = offset_car_type[min(track->get_track_number(),6)];
        
        f = min(offset_dif, offset_car_type_max) / offset_car_type_max;
                
        if (AI_type == 3 && f < 0.15) f = 0;
        f *= f;                
        f = 1 - f * 0.5F;
        
		if (current_speed > max_speed*(f*1.05))
			*brake = true;
        
		if (current_speed > max_speed*f)
			*accelerate = false;
	}
    
	if (terrain != 0 && current_speed > 100 && track->get_track_number() != 1)
		*brake = true;
    
	// If obstacle in front: brake (and increase the speed of offset_change)
	{
		float closest_col = 0;
		RacingCCar *collided = 0;
		bool col = false;
		int i;
        
		for (i = 0;!col && i < 8;i++) {
			fwx = car->get_x() + vx * (32 + i * 16);
			fwy = car->get_y() + vy * (32 + i * 16);
			collided = car->cars_collision(fwx, fwy, car->get_z(), cars, car_grid, car_grid_sx, car_grid_sy);
            
			if (collided != 0) {
				closest_col = float(32 + i * 16);
				col = true;
			} 
		} 
        
		if (col) {
			/* compute the relative speed of the collidee: */
			float rsx = collided->car->get_speed_x() - car->get_speed_x();
			float rsy = collided->car->get_speed_y() - car->get_speed_y();
			float rs = rsx * vx + rsy * vy;
            
			if (closest_col < 40) {
				if (rs < 0) {
					*accelerate = false;
					*brake = true;
				} 
			} else if (closest_col < 80) {
				if (rs < 0) {
					*accelerate = false;
				} 
                
				if (rs < -20) {
					*brake = true;
				} 
			} else {
				if (rs < -20) {
					*accelerate = false;
				} 
			} 
            
			if (rs < 0 && car->get_speed() > 100 && road_offset_change > 32)
				road_offset_change = 32;
            
			if (rs < -10) {
				if (*offset > 0 && *offset < 40)
					*turn = 1;
                
				if (*offset < 0 && *offset > -40)
					*turn = -1;
                
				if (*offset > 40)
					*turn = -1;
                
				if (*offset < 40)
					*turn = 1;
			} 
		} 
	}
    
	// If lateral obstacle when turning, stop turning!
	if (*turn == 1) {
		float fwx;
		float fwy;
		float vx = car->get_speed_x();
		float vy = car->get_speed_y();
		float vn = float(sqrt(vx * vx + vy * vy));
		vx /= vn;
		vy /= vn;
        
		fwx = car->get_x() - vy * 16;
		fwy = car->get_y() + vx * 16;
        
		if (car->cars_collision(fwx, fwy, car->get_z(), cars, car_grid, car_grid_sx, car_grid_sy)) {
			*turn = 0;
		} 
	} 
    
	if (*turn == -1) {
		float fwx;
		float fwy;
		float vx = car->get_speed_x();
		float vy = car->get_speed_y();
		float vn = float(sqrt(vx * vx + vy * vy));
		vx /= vn;
		vy /= vn;
        
		fwx = car->get_x() + vy * 16;
		fwy = car->get_y() - vx * 16;
        
		if (car->cars_collision(fwx, fwy, car->get_z(), cars, car_grid, car_grid_sx, car_grid_sy)) {
			*turn = 0;
		} 
	} 
} 