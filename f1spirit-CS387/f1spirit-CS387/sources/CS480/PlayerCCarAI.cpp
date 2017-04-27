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



void PlayerCCar::PlayerAICS480_keyboard(bool *accelerate_action, bool *brake_action, int *turn_action, int *gear_action, KEYBOARDSTATE *k, List<RacingCCar> *cars, List<RacingCCar> **car_grid, int car_grid_sx, int car_grid_sy)
{
    if (k->keyboard[left] && !k->keyboard[right]) *turn_action = -1;
    if (k->keyboard[right] && !k->keyboard[left]) *turn_action = 1;
    if (k->keyboard[up] && !k->old_keyboard[up] && !k->keyboard[down]) *gear_action = 1;
    if (k->keyboard[down] && !k->old_keyboard[down] && !k->keyboard[up]) *gear_action = -1;    
    if (k->keyboard[accelerate]) *accelerate_action = true;
    if (k->keyboard[brake]) *brake_action = true;
} 


float get_magnitude(float x, float y)
{
	return sqrt(x*x + y*y);
}


float dot(float x1, float y1, float x2, float y2)
{
	return x1 * x2 + y1 * y2;
}


void rotate(float x, float y, float centerx, float centery, float theta, float* xr, float* yr)
{
	*xr = (x-centerx)*cos(theta) - (y-centery)*sin(theta) + centerx;
	*yr = (x-centerx)*sin(theta) + (y-centery)*cos(theta) + centery;
}


void seek(CCar* car, float Ex, float Ey, float* Ax, float* Ay)
{
	float Dx = car->get_x() - Ex;
	float Dy = car->get_y() - Ey;
	float Dmag = get_magnitude(Dx, Dy);
	*Ax = Dx / Dmag;
	*Ay = Dy / Dmag;
}


void flee(CCar* car, float Ex, float Ey, float* Ax, float* Ay)
{
	float Dx = Ex - car->get_x();
	float Dy = Ey - car->get_y();
	float Dmag = get_magnitude(Dx, Dy);
	*Ax = -Dx / Dmag;
	*Ay = -Dy / Dmag;
}


void angular_acceleration(CCar* car, float Ax, float Ay, float* angle, bool *accelerate_action, bool *brake_action, int *turn_action)
{
	float vx = car->get_speed_x();
	float vy = car->get_speed_y();
	float rightx = vy;
	float righty = -vx;
	float v = get_magnitude(vx, vy);
	float right = get_magnitude(rightx, righty);
	float A = get_magnitude(Ax, Ay);
	float proj = dot(rightx, righty, Ax, Ay);

	float angle_cos = dot(Ax, Ay, vx, vy) / (v*A);
	*angle = acos(angle_cos);
	
	*accelerate_action = true;
	*brake_action = false;
	if (*angle>0.1 && proj>0.5) *turn_action = 1;
	if (*angle>0.1 && proj<-0.5) *turn_action = -1;
}


void PlayerCCar::PlayerAICS480(bool *accelerate_action, bool *brake_action, int *turn_action, int *gear_action, KEYBOARDSTATE *k, List<RacingCCar> *cars, List<RacingCCar> **car_grid, int car_grid_sx, int car_grid_sy)
{
    clearAIDebugLines();
    
    // Find the movement speed:
    float vx = car->get_speed_x();
    float vy = car->get_speed_y();
    float v = sqrt(vx*vx+vy*vy);
    
    // The road is made out of a collection of pieces, get the piece over which the car is on right now:
    CRoadPiece *rp = road_position.GetObj();
    // Get the road piece following the current road piece:
    CRoadPiece *next_rp;
    {
        List<CRoadPiece> road_p2;
        road_p2.Instance(road_position);
        road_p2.Synchronize(&road_position);
        road_p2.Next();    
        if (road_p2.EndP()) road_p2.Rewind();
        next_rp = road_p2.GetObj();
    } 

    // Find closest position from the car in the road:
    float pathx = 0, pathy = 0, pathz = 0, patha = 0, pathw = 0, distance = 0;
    float next_pathx = 0, next_pathy = 0, next_pathz = 0, next_patha = 0, next_pathw = 0, next_distance = 0;
    rp->get_path_position(car->get_x(),car->get_y(),&pathx,&pathy,&pathz,&patha,&pathw,&distance);
	
	int seek_ahead = 128;
    // Compute a position in the road that is seek_ahead pixels ahead of the current position:
    {
        float next_pathx2 = 0, next_pathy2 = 0, next_pathz2 = 0, next_patha2 = 0, next_pathw2 = 0, next_distance2 = 0;
        float fwx = float(pathx + seek_ahead * cos((patha * M_PI) / 180.0F));
        float fwy = float(pathy + seek_ahead * sin((patha * M_PI) / 180.0F));
        rp->get_path_position(fwx, fwy, &next_pathx, &next_pathy, &next_pathz, &next_patha, &next_pathw, &next_distance);
        next_rp->get_path_position(fwx, fwy, &next_pathx2, &next_pathy2, &next_pathz2, &next_patha2, &next_pathw2, &next_distance2);
        if (next_distance2<next_distance) {
			rp = next_rp;
            next_pathx = next_pathx2;
            next_pathy = next_pathy2;
			next_pathw = next_pathw2;
        } 
    }

    // Draw a white line from the car to the closest position of the road:
    addAIDebugline(car->get_x(), car->get_y(), pathx,pathy,255,255,255,255);

	//the default seek target, which is the area 128 pixels up the road with the same offset from the center
	float offset_x = pathx - car->get_x();
	float offset_y = pathy - car->get_y();
	float seek_target_x = next_pathx - offset_x;
	float seek_target_y = next_pathy - offset_y;
    
	// Draw a blue line from the car to the position of the default seek target
    addAIDebugline(car->get_x(), car->get_y(), seek_target_x, seek_target_y,0,0,255,255);

	float a_x, a_y, angle;
	int look_ahead = 64;
	if (v > 0) {
		float delta = M_PI / 4;
		float e = M_PI / 32;
		bool safe_r = true, safe_l = true, open_l = true, open_r = true;
		// Shoot 2 rays in front of the car to check for cars we need to pass
		for (int i = 8; i < look_ahead; i += 8) {
			float fwx = car->get_x() + (vx / v) * i;
			float fwy = car->get_y() + (vy / v) * i;
			float r1_rightx, r1_righty, r1_leftx, r1_lefty;
			rotate(fwx, fwy, car->get_x(), car->get_y(), e, &r1_rightx, &r1_righty);
			rotate(fwx, fwy, car->get_x(), car->get_y(), -e, &r1_leftx, &r1_lefty);
			addAIDebugline(car->get_x(), car->get_y(), r1_rightx, r1_righty, 255, 0, 0, 255);
			addAIDebugline(car->get_x(), car->get_y(), r1_leftx, r1_lefty, 255, 0, 0, 255);
			RacingCCar *collided_car_r = 0, *collided_car_l = 0;
			collided_car_r = car->cars_collision(r1_rightx, r1_righty, car->get_z(), cars, car_grid, car_grid_sx, car_grid_sy);
			collided_car_l = car->cars_collision(r1_leftx, r1_lefty, car->get_z(), cars, car_grid, car_grid_sx, car_grid_sy);
			if (collided_car_r != 0) {
				safe_r = false;
				break;
			}
			else if (collided_car_l != 0) {
				safe_l = false;
				break;
			}
		}

		// Shoot 2 short rays to the side of the car to make sure we don't turn off the road
		float look_side = 20;
		float fwx = car->get_x() + (vx / v) * look_side;
		float fwy = car->get_y() + (vy / v) * look_side;
		float r2x, r2y, r2offsetx, r2offsety, r3x, r3y, r3offsetx, r3offsety;
		float junk1, junk2, junk3, junk4;
		rotate(fwx, fwy, car->get_x(), car->get_y(), delta, &r2x, &r2y);
		rotate(fwx, fwy, car->get_x(), car->get_y(), -delta, &r3x, &r3y);
		rp->get_path_position(r2x, r2y, &r2offsetx, &r2offsety, &junk1, &junk2, &junk3, &junk4);
		rp->get_path_position(r3x, r3y, &r3offsetx, &r3offsety, &junk1, &junk2, &junk3, &junk4);
		addAIDebugline(car->get_x(), car->get_y(), r2x, r2y, 255, 0, 127, 255);
		addAIDebugline(car->get_x(), car->get_y(), r3x, r3y, 255, 0, 127, 255);
		if (get_magnitude(r2x -r2offsetx, r2y -r2offsety) > next_pathw / 2)
			open_r = false;
		if (get_magnitude(r3x -r3offsetx, r3y -r3offsety) > next_pathw / 2)
			open_l = false;

		// Use the 4 rays to rotate the default seek target to a better location
		if(open_r && !open_l) {
			float new_target_x, new_target_y;
			rotate(seek_target_x, seek_target_y, car->get_x(), car->get_y(), 2*e, &new_target_x, &new_target_y);
			seek_target_x = new_target_x;
			seek_target_y = new_target_y;
		}
		else if(!open_r && open_l) {
			float new_target_x, new_target_y;
			rotate(seek_target_x, seek_target_y, car->get_x(), car->get_y(), -2*e, &new_target_x, &new_target_y);
			seek_target_x = new_target_x;
			seek_target_y = new_target_y;
		}
		else {
			if (safe_r && !safe_l) {
				float new_target_x, new_target_y;
				rotate(seek_target_x, seek_target_y, car->get_x(), car->get_y(), 2*e, &new_target_x, &new_target_y);
				seek_target_x = new_target_x;
				seek_target_y = new_target_y;
			}
			else if (safe_l && !safe_r) {
				float new_target_x, new_target_y;
				rotate(seek_target_x, seek_target_y, car->get_x(), car->get_y(), -2*e, &new_target_x, &new_target_y);
				seek_target_x = new_target_x;
				seek_target_y = new_target_y;
			}
		}

		// Draw a green line from the car to the position of the chosen seek target
		addAIDebugline(car->get_x(), car->get_y(), seek_target_x, seek_target_y, 0, 255, 0, 255);
		
		// Use seek to go to the chosen place
		seek(car, seek_target_x, seek_target_y, &a_x, &a_y);
		angular_acceleration(car, a_x, a_y, &angle, accelerate_action, brake_action, turn_action);
    } else {
		*accelerate_action = true;
	}

    // If need be, shift gears:
    if (car->get_rpm() > car->get_engine()->max_rpm*0.9 && car->get_currentgear() < 3) *gear_action = 1;
    if (car->get_rpm() < car->get_engine()->max_rpm*0.5 && car->get_currentgear() > 0) *gear_action = -1;
}
