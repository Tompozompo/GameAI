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
	float Dx = car->get_x() - Ex;
	float Dy = car->get_y() - Ey;
	float Dmag = get_magnitude(Dx, Dy);
	*Ax = -Dx / Dmag;
	*Ay = -Dy / Dmag;
}


void angular_acceleration(CCar* car, float Ax, float Ay, int* turn_action)
{
	float rightx = car->get_speed_y();
	float righty = -car->get_speed_x();
	float rightmag = get_magnitude(rightx, righty);
	float Amag = get_magnitude(Ax, Ay);

	float dotp = dot(rightx, righty, Ax, Ay);
    if (dotp>2) *turn_action = 1;
    if (dotp<-2) *turn_action = -1;
}


void PlayerCCar::PlayerAICS480(bool *accelerate_action, bool *brake_action, int *turn_action, int *gear_action, KEYBOARDSTATE *k, List<RacingCCar> *cars, List<RacingCCar> **car_grid, int car_grid_sx, int car_grid_sy)
{
    clearAIDebugLines();
    
    // Find the movement speed:
    float vx = car->get_speed_x();
    float vy = car->get_speed_y();
    float v = sqrt(vx*vx+vy*vy);
    
    // Draw a green line in the direction of movement:
    addAIDebugline(car->get_x(), car->get_y(), car->get_x() + vx*64/v, car->get_y()+ vy*64/v,0,255,0,255);
    

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

    // Compute a position in the road that is 128 pixels ahead of the current position:
    {
        float next_pathx2 = 0, next_pathy2 = 0, next_pathz2 = 0, next_patha2 = 0, next_pathw2 = 0, next_distance2 = 0;
        float fwx = float(pathx + 128 * cos((patha * M_PI) / 180.0F));
        float fwy = float(pathy + 128 * sin((patha * M_PI) / 180.0F));
        rp->get_path_position(fwx, fwy, &next_pathx, &next_pathy, &next_pathz, &next_patha, &next_pathw, &next_distance);
        next_rp->get_path_position(fwx, fwy, &next_pathx2, &next_pathy2, &next_pathz2, &next_patha2, &next_pathw2, &next_distance2);
        if (next_distance2<next_distance) {
            next_pathx = next_pathx2;
            next_pathy = next_pathy2;
        } 
    }

    // Draw a white line from the car to the closest position of the road:
    addAIDebugline(car->get_x(), car->get_y(), pathx,pathy,255,255,255,255);

    // Draw a blue line from the car to the position in the road that is 128 pixels ahead:
    addAIDebugline(car->get_x(), car->get_y(), next_pathx,next_pathy,0,0,255,255);

	float a_x, a_y;
	seek(car, next_pathx, next_pathy, &a_x, &a_y);
	angular_acceleration(car, a_x, a_y, turn_action);

	// Draw a green line
	addAIDebugline(car->get_x(), car->get_y(), car->get_x() + a_x*54, car->get_y() + a_y*64, 255, 255, 0, 255);
    
    if (v>0) {
		float theta = M_PI / 4;
		float e = theta / 8;
		float maxlenx = car->get_x() + (vx / v) * 64;
		float maxleny = car->get_y() + (vy / v) * 64;
		float bound1x, bound1y, bound2x, bound2y;
		rotate(maxlenx, maxleny, car->get_x(), car->get_y(), theta, &bound1x, &bound1y);
		rotate(maxlenx, maxleny, car->get_x(), car->get_y(), -theta, &bound2x, &bound2y);

		// Draw 2 red lines for the cone to check for collision
		addAIDebugline(car->get_x(), car->get_y(), bound1x, bound1y, 255,0,0,255);
		addAIDebugline(car->get_x(), car->get_y(), bound2x, bound2y, 255,0,0,255);
		int count = 1;
		for (int i = 8; i < 64; i += 8) {
			float fwx = car->get_x() + (vx / v) * i;
			float fwy = car->get_y() + (vy / v) * i;
			for (float t = -theta; t <= theta; t += e) {
				float fwxr, fwyr;
				rotate(fwx, fwy, car->get_x(), car->get_y(), t, &fwxr, &fwyr);
				//addAIDebugline(car->get_x(), car->get_y(), fwxr, fwyr, 255, 0, 0, 255);
				RacingCCar* collided = 0;
				collided = car->cars_collision(fwxr, fwyr, car->get_z(), cars, car_grid, car_grid_sx, car_grid_sy);
				if (collided != 0) {
					float a2_x, a2_y;
					flee(car, collided->car->get_x(), collided->car->get_x(), &a2_x, &a2_y);
					if(turn_action == 0)
						angular_acceleration(car, a2_x, a2_y, turn_action);
					a_x += a2_x;
					a_y += a2_y;
					count++;
				}
			}
		}
    } 
	if (get_magnitude(a_x, a_y) > 0)
		*accelerate_action = true;
	else
		*brake_action = true;

    // If need be, shift gears:
    if (car->get_rpm() > car->get_engine()->max_rpm*0.9 && car->get_currentgear() < 3) *gear_action = 1;
    if (car->get_rpm() < car->get_engine()->max_rpm*0.5 && car->get_currentgear() > 0) *gear_action = -1;
}


/*
void PlayerCCar::PlayerAICS480(bool *accelerate_action, bool *brake_action, int *turn_action, int *gear_action, KEYBOARDSTATE *k, List<RacingCCar> *cars, List<RacingCCar> **car_grid, int car_grid_sx, int car_grid_sy)
{
    clearAIDebugLines();
    
    // Find the movement speed:
    float vx = car->get_speed_x();
    float vy = car->get_speed_y();
    float v = sqrt(vx*vx+vy*vy);
    
    // Draw a green line in the direction of movement:
    addAIDebugline(car->get_x(), car->get_y(), car->get_x() + vx*32/v, car->get_y()+ vy*32/v,0,255,0,255);
    

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

    // Compute a position in the road that is 128 pixels ahead of the current position:
    {
        float next_pathx2 = 0, next_pathy2 = 0, next_pathz2 = 0, next_patha2 = 0, next_pathw2 = 0, next_distance2 = 0;
        float fwx = float(pathx + 64 * cos((patha * M_PI) / 180.0F));
        float fwy = float(pathy + 64 * sin((patha * M_PI) / 180.0F));
        rp->get_path_position(fwx, fwy, &next_pathx, &next_pathy, &next_pathz, &next_patha, &next_pathw, &next_distance);
        next_rp->get_path_position(fwx, fwy, &next_pathx2, &next_pathy2, &next_pathz2, &next_patha2, &next_pathw2, &next_distance2);
        if (next_distance2<next_distance) {
            next_pathx = next_pathx2;
            next_pathy = next_pathy2;
        } 
    }

    // Draw a white line from the car to the closest position of the road:
    addAIDebugline(car->get_x(), car->get_y(), pathx,pathy,255,255,255,255);

    // Draw a blue line from the car to the position in the road that is 128 pixels ahead:
    addAIDebugline(car->get_x(), car->get_y(), next_pathx,next_pathy,0,0,255,255);
    
    // Determine whether we have turn or not:
    if (v>0) {
        float v2x = next_pathx - car->get_x();
        float v2y = next_pathy - car->get_y();
        float v2n = sqrt(v2x*v2x + v2y*v2y);
        
        // This determines the angle:
        float angle_cos = (v2x/v2n)*(vx/v) + (v2y/v2n)*(vy/v);
        float angle = acos(angle_cos);
        
        // This determines whether we have to turn left or right: 
        float tmp = (vx*(next_pathy - car->get_y()) - vy*(next_pathx - car->get_x()));
        
        if (angle>0.1 && tmp>0) *turn_action = 1;
        if (angle>0.1 && tmp<0) *turn_action = -1;
    }
    
    // Accelerate unless there is a car ahead:
    if (v>0) {
        RacingCCar *collided = 0;

        // check if there is a car in the space from 8 to 64 pixels ahead of our car:
        for(int i = 8;i<64;i+=8) {
            float fwx = car->get_x() + (vx/v) * i;
            float fwy = car->get_y() + (vy/v) * i;
            collided = car->cars_collision(fwx, fwy, car->get_z(), cars, car_grid, car_grid_sx, car_grid_sy);
            if (collided!=0) break;
        }
            
        if (collided == 0) {
            *accelerate_action = true;   
        } else {
            *brake_action = true;
        }
    } else {
        *accelerate_action = true;
    }
    
    
    // If need be, shift gears:
    if (car->get_rpm() > car->get_engine()->max_rpm*0.9 && car->get_currentgear() < 3) *gear_action = 1;
    if (car->get_rpm() < car->get_engine()->max_rpm*0.5 && car->get_currentgear() > 0) *gear_action = -1;
} 
*/

