#ifndef _F1SPIRIT_PLAYERCAR
#define _F1SPIRIT_PLAYERCAR

#include "AIDebug.h"

class PlayerCCar: public RacingCCar
{

	public:
		PlayerCCar();
		~PlayerCCar();
    
        void PlayerAICS480(bool *accelerate_action, bool *brake_action, int *turn_action, int *gear_action, KEYBOARDSTATE *k, List<RacingCCar> *cars, List<RacingCCar> **car_grid, int car_grid_sx, int car_grid_sy);
        void PlayerAICS480_keyboard(bool *accelerate_action, bool *brake_action, int *turn_action, int *gear_action, KEYBOARDSTATE *k, List<RacingCCar> *cars, List<RacingCCar> **car_grid, int car_grid_sx, int car_grid_sy);
    
        void addAIDebugline(float x1,float y1,float x2,float y2,int r,int g,int b,int a);
        void clearAIDebugLines();
        void drawAIDebugLines(PlayerCCar *v);
    
		int vx, vy;
		int vdx, vdy;
		int up, down, left, right, accelerate, brake;

		float c_old_x, c_old_y, c_old_a, c_old_z;
		float c_x, c_y, c_a, c_z, c_a_speed;

		bool wrong_way;

		Uint32 lap_time[MAX_NLAPS];
		int show_lap_time;

		int show_damage_timmer, show_damage_cycle;

		bool inside_pit;
		int pit_time;
		int show_pit_stop;

		int last_sign_showing;
		int times_sign_showed;

        // These variables are to draw lines for debugging purposes:
        List<AIDebugLine> debug_lines;
};


#endif
