#include "game.h"

typedef struct SampleResult{
	uint32 winCnt;
	uint32 tieCnt;
	uint32 failCnt;
	uint32 totalCnt;
}SampleResult;

int getCardCnt(Deck deck);
double calHandStrength(Deck handDeck, Deck publicDeck, int otherPlayerNum);
int isWin();
int stackProtect(double winRate);
int chooseAction(double winRate, double betRate);
