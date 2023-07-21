#pragma once
#include <Mebius.h>
#include "StateControllerExtension.h"
using namespace std;

static DWORD STATEID = 0x7FFFFFFF;

static auto SCtrlParseElemType = reinterpret_cast<bool (*)(TPFILE* tpf, STATE_INFO* sinfo, PLAYER_CACHE* pcache)>(0x46aa60);
static auto SCtrlRCElemFree = reinterpret_cast<void (*)(STATE_INFO* sinfo)>(0x4718d0);
static auto ConstExpI = reinterpret_cast<void (*)(EVAL_EXP* ptr, int value)>(0x406f20);
static auto ConstExpF = reinterpret_cast<void (*)(EVAL_EXP* ptr, float value)>(0x406fa0);
static auto EvalExpressionI = reinterpret_cast<int (*)(PLAYER* p, EVAL_EXP* ptr, int warnNo)>(0x4075e0);
static auto EvalExpressionF = reinterpret_cast<float (*)(PLAYER* p, EVAL_EXP* ptr)>(0x4076d0);

int findTargetStateByName(string);
int regModState(int);
int procModState(void);
void freeModState(void);
