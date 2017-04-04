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


void PlayerCCar::addAIDebugline(float x1,float y1,float x2,float y2,int r,int g,int b,int a)
{
    debug_lines.Add(new AIDebugLine(x1,y1,x2,y2,r,g,b,a));
}
                
void PlayerCCar::clearAIDebugLines() {
    debug_lines.Delete();
}

void PlayerCCar::drawAIDebugLines(PlayerCCar *v)
{
    AIDebugLine *line;
    debug_lines.Rewind();
	glPushMatrix();
	glScalef(v->c_z, v->c_z, v->c_z);
    glNormal3f(0.0, 0.0, 1.0);
    glBegin(GL_LINES);
    while(debug_lines.Iterate(line)) {
        glColor4f(line->r/255.0,line->g/255.0,line->b/255.0,line->a/255.0);
        glVertex3f(line->x1 - v->c_x, line->y1 - v->c_y, 0);
        glVertex3f(line->x2 - v->c_x, line->y2 - v->c_y, 0);        
    }
    glEnd();
    glPopMatrix();
}


