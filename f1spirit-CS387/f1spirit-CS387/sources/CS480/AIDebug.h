#ifndef __AI_DEBUG
#define __AI_DEBUG

class AIDebugLine
{
public:
    AIDebugLine(float a_x1, float a_y1, float a_x2, float a_y2, int a_r, int a_g, int a_b, int a_a) {
        x1 = a_x1;
        y1 = a_y1;
        x2 = a_x2;
        y2 = a_y2;
        r = a_r;
        g = a_g;
        b = a_b;
        a = a_a;
    }
    
    float x1,y1;
    float x2,y2;
    int r,g,b,a;
};

#endif