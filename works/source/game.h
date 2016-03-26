#ifndef _GAME_H_
#define _GAME_H_	


#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>          
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <math.h>

#define BUFFER_LENGTH 1024
#define SEND_LENGTH 128
#define RECV_LENGTH 1 //1 is ok
#define CHECK_LENGTH 8 //8 is ok, in case of game-over not got
#define MAX_ROUND 600
#define REST_ROUND(num) (MAX_ROUND - num)
#define GAMEOVER -1
#define PROTOCOL_NOTFOUND 0
#define PROTOCOL_FOUND 1
/* 行动 */
#define ACTION_BLIND 0
#define ACTION_FOLD 1
#define ACTION_CALL 2
#define ACTION_CHECK 3
#define ACTION_RAISE 4
#define ACTION_ALLIN 5
/* 盲注数量 */
#define DOUBLE_BIGBLIND 80
#define THREE_BIGBLIND	120
#define FOUR_BIGBLIND	160
#define FIVE_BIGBLIND	200
#define EIGHT_BIGBLIND 300
/* 当前进度 */
#define PRGS_PREFLOP 0
#define PRGS_FLOP 1
#define PRGS_TURN 2
#define PRGS_RIVER 3
/* 花色 */
#define COLOR_SPADES 1
#define COLOR_HEARTS 2
#define COLOR_CLUBS 3
#define COLOR_DIAMONDS 4
/* 对手模型取样数 */
#define PLAYERWP_SAMPLENUM 30

#define CARDS_N 52
#define DECK_N 8192
#define TOTALPLAYER_N 8
#define OTHERPLAYER_N 7
#define RANDNUM (rand() % 100)
#define SMALLRATE_REDUCE 1.1
#define SMALL_BETPERCENT 0.05
#define MEDIUM_BETPERCENT 0.1
#define BIG_BETPERCENT 0.2

#define DECK(index) (deckTable[index])
#define DECK_SPADES(deck)   ((deck).suits.spades)
#define DECK_HEARTS(deck)   ((deck).suits.hearts)
#define DECK_CLUBS(deck)    ((deck).suits.clubs)
#define DECK_DIAMONDS(deck) ((deck).suits.diamonds)

#define DECK_RESET(deck) do { (deck).cards = 0; } while (0)
#define DECK_ISREPEAT(deck1, deck2) (((deck1).cards & (deck2).cards) != 0)
#define DECK_OP(result, op1, op2, OP) \
do { (result).cards = ((op1).cards)OP((op2).cards); } while (0)
#define DECK_OR(result, op1, op2) DECK_OP(result, op1, op2, | )
#define DECK_AND(result, op1, op2) DECK_OP(result, op1, op2, &)
#define DECK_XOR(result, op1, op2) DECK_OP(result, op1, op2, ^)
#define DECK_SET(deck, index)			\
do {\
Deck _deck = DECK(index);			\
DECK_OR((deck), (deck), _deck);		\
} while (0)

#define CARDTYPE_HIGHCARD  0
#define CARDTYPE_ONEPAIR   1
#define CARDTYPE_TWOPAIR   2
#define CARDTYPE_TREEKIND  3
#define CARDTYPE_STRAIGHT  4
#define CARDTYPE_FLUSH     5
#define CARDTYPE_FULLHOUSE 6
#define CARDTYPE_FOURKIND  7
#define CARDTYPE_STFLUSH   8

#define CARDTYPE_MOVE    24
#define CARDTYPE_MAP     0x0F000000
#define CARDS_MOVE       0
#define CARDS_MAP        0x000FFFFF
#define FIRSTCARD_MOVE   16
#define FIRSTCARD_MAP    0x000F0000
#define SECONDCARD_MOVE  12
#define SECONDCARD_MAP   0x0000F000
#define THIRDCARD_MOVE   8
#define THIRDCARD_MAP    0x00000F00
#define FOURTHCARD_MOVE  4
#define FOURTHCARD_MAP   0x000000F0
#define FIFTHCARD_MOVE   0
#define FIFTHCARD_MAP    0x0000000F
#define CARD_WIDTH       4
#define CARD_MAP         0x0F

#define CARDTYPE_VALUE(c)   ((c) << CARDTYPE_MOVE)
#define FIRSTCARD_VALUE(c)  ((c) << FIRSTCARD_MOVE)
#define SECONDCARD_VALUE(c) ((c) << SECONDCARD_MOVE)
#define THIRDCARD_VALUE(c)  ((c) << THIRDCARD_MOVE)
#define FOURTHCARD_VALUE(c) ((c) << FOURTHCARD_MOVE)
#define FIFTHCARD_VALUE(c)  ((c) << FIFTHCARD_MOVE)

typedef unsigned char  uint8;
typedef signed char   int8;
typedef unsigned short uint16;
typedef unsigned int   uint32;
typedef unsigned long long uint64;

typedef union {
	uint64  cards;
	struct {
		uint32 spades : 13;   //黑桃
	uint32: 3;
		uint32 hearts : 13;   //梅花
	uint32: 3;
		uint32 clubs : 13;   //方块
	uint32: 3;
		uint32 diamonds : 13;   //红桃
	uint32: 3;
	} suits;
} Deck;

/* 座次信息 */
typedef struct
{
	int pid; // player id
	int jetton; // jetton left
	int money; //money left
} seat;
/* 卡牌 */
typedef struct
{
	int color; // 1-4 => S/H/C/D
	int point; // 1-13 => 2/3/4/5/6/7/8/9/10/J/Q/K/A
	int index; // 0-51
} card;
/* 询问信息 */
typedef struct
{
	int pid;
	int jetton;
	int money;
	int bet; // total bet
	int action; // 0:blind 1:check 2:call 3:raise 4:all_in 5:fold
} inquireInfo;
/* 最紧对手信息 */
/*typedef struct
{
int pid;
int action;//最紧对手的当前行动
double avgWp;
double sigmaWp;
double upperWp;//最紧对手当前阶段的胜率上限
double lowerWp;//最紧对手当前阶段的胜率下限
} oppoPlayer;*/
/* 对手胜率取样 */
/*typedef struct
{
int pid;//对手id
card handCard[2];//对手当前手牌
double winPercent[4][PLAYERWP_SAMPLENUM]; //胜率存储数组，分为4阶段（preflop阶段暂时不需要），每阶段取样PLAYERWP_SAMPLENUM个
double wpAvg[4];//4阶段的胜率平均值
double wpSigma[4];//4阶段的胜率标准差
int wpCurNum;//当前对手的取样数，可大于PLAYERWP_SAMPLENUM，多余按照 wpCurNum%PLAYERWP_SAMPLENUM覆盖存储
} playerWinPercent;*/

typedef struct
{
	int pid;//对手id
	int seatNum;//座次
	int isFold;//是否弃牌
	int isDead;//是否out
	int playNumAfterRule;//规则后局数
	int foldNumAfterRule;//规则后弃牌数
	int preRaiseNum;//preflop阶段raise次数
	int postRaiseNum;//postflop阶段raise次数
	int postPlayNum;//postflop阶段局数
	int winNum;//showdown后胜利次数
	int showDownNum;//showdown次数
}oppoInfo;

typedef struct
{
	int progress; // 0: pre-flop 1: flop 2: turn 3: river 4: fold 5: all_in
	int handNum; //当前局数
	card publicCard[5]; //flop: 0-2 turn: 3 river: 4
	Deck flopDeck;//flop公牌
	Deck turnDeck;//turn公牌
	Deck riverDeck;//river公牌
	Deck publicDeck;//所有公牌
	seat seatInfo[TOTALPLAYER_N]; //座次信息
	int smallBlind; //small blind
	int bigBlind; // big blind
	int totalSeatNum; //本局座位数
	inquireInfo inquire[TOTALPLAYER_N];//询问信息
	inquireInfo notify[TOTALPLAYER_N];//通知信息，用于已allin或已fold
	int totalPot; //池底
	int lastTotalPot; //上一次池底，用于计算allin后的边池
	int nowCallPot; //当前需要下注数
	int foldPlayerNum;//已fold的人数
	int foldPlayer[TOTALPLAYER_N];//已fold的pid
	int preFlopRoundNum;//pre flop 阶段的轮数
	int flopRoundNum;//flop 阶段轮数
	int turnRoundNum;//turn 阶段轮数
	int riverRoundNum;//river 阶段轮数
	double lastCalPostResult;//上一轮postflop计算胜率结果存储，如果未进入下一阶段，可用于重复利用
	int beforePlayerNum;//坐在我前面的玩家数
	int afterRaisePlayerNum;//在某玩家raise后没有弃牌的玩家数
	int callPlayerNum;//call的玩家数
	int curPlayerNum;//当前等效玩家数
	int isShowDown;//是否showdown
	oppoInfo opInfo[OTHERPLAYER_N];//对手信息
	int curRaiseId;//当前加注选手id
	int preFlopNum;//未加注preflop阶段数
	int oppoActionList[200][2];
	int oppoActionNum;
} publicInfo;

typedef struct
{
	int mySeat; //我的座次号 0（庄）-7
	card handCard[2];//我的手牌
	Deck handDeck;
	int myJetton;//我的筹码
	int myMoney;//我的金币
	int myBet;//我的投注
	int myRoundBet;//本轮投注总额
	int goldGap;//和其他所有玩家金币差
	int isWin;//是否已经胜利
	int isFold;//已弃牌
	int raiseBet;//加注金额
	int isRuleActived;//是否发动
	int isBluffed;//是否已经bluff过
} privateInfo;

extern privateInfo myInfo;
extern publicInfo pubInfo;
extern FILE *file;
extern double preFlopTable[7][13][13];//起手牌概率表
extern Deck deckTable[CARDS_N];//整副牌
extern uint8 nRankTable[DECK_N];//点数表
extern uint8 straightTable[DECK_N];//顺子表
extern uint8 topCardTable[DECK_N];//顶牌表
extern uint32 topFiveCardsTable[DECK_N];//五张顶牌表
#endif