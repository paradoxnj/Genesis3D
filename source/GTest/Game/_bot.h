#ifndef _bot_h
#define _bot_h

///////////////////////////////////////////////////////////////////////
// _Bot.h - misc junk I want - either out of ignorance or true need
///////////////////////////////////////////////////////////////////////

extern int32 randomseed;
int32 krand(void);
int32 RandomRange(int32 range);

void VectorRotateY(geVec3d *vec, float delta_ang, geVec3d *result);

#define M_PI (3.14159f)
#define PI_2 (M_PI*2.0f)
#define M_PI2 (PI_2)

//#define MAX_STACK_SIZE MAX_TRACKS
typedef struct Stack
    { int32 TOS, Size, *Data;} 
Stack;

void StackInit(Stack *s);
void StackReset(Stack *s);
void StackPush(Stack *s, int32 data);
int32 StackPop(Stack *s);
int32 StackTop(Stack *s);
geBoolean StackIsEmpty(Stack *s);
void StackSetup(Stack *s);

#endif