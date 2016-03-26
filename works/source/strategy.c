#include <stdio.h>
#include "strategy.h"

static uint32 _calDeckVal(Deck cards, int cardCnt)
{
	uint32 val = 0, ranks, four, three, two, dupCnt, rankCnt, t, top, second;
	uint32 s, h, c, d;

	s = DECK_SPADES(cards);
	h = DECK_HEARTS(cards);
	c = DECK_CLUBS(cards);
	d = DECK_DIAMONDS(cards);
	ranks = s | h | c | d;
	rankCnt = nRankTable[ranks];
	dupCnt = cardCnt - rankCnt;

	/* 如果是顺子，同花，同花顺，立即返回
	*/
	if (rankCnt >= 5) {
		if (nRankTable[s] >= 5) {
			if (straightTable[s])
				return CARDTYPE_VALUE(CARDTYPE_STFLUSH) + FIRSTCARD_VALUE(straightTable[s]);
			else
				val = CARDTYPE_VALUE(CARDTYPE_FLUSH) + topFiveCardsTable[s];
		}
		else if (nRankTable[h] >= 5) {
			if (straightTable[h])
				return CARDTYPE_VALUE(CARDTYPE_STFLUSH) + FIRSTCARD_VALUE(straightTable[h]);
			else
				val = CARDTYPE_VALUE(CARDTYPE_FLUSH) + topFiveCardsTable[h];
		}
		else if (nRankTable[c] >= 5) {
			if (straightTable[c])
				return CARDTYPE_VALUE(CARDTYPE_STFLUSH) + FIRSTCARD_VALUE(straightTable[c]);
			else
				val = CARDTYPE_VALUE(CARDTYPE_FLUSH) + topFiveCardsTable[c];
		}
		else if (nRankTable[d] >= 5) {
			if (straightTable[d])
				return CARDTYPE_VALUE(CARDTYPE_STFLUSH) + FIRSTCARD_VALUE(straightTable[d]);
			else
				val = CARDTYPE_VALUE(CARDTYPE_FLUSH) + topFiveCardsTable[d];
		}
		else {
			int st;
			st = straightTable[ranks];
			if (st)
				val = CARDTYPE_VALUE(CARDTYPE_STRAIGHT) + FIRSTCARD_VALUE(st);
		};
		if (val && dupCnt < 3)
			return val;
	};

	/*
	* 有两种情况:
	1) 没有顺子或者同花
	2) 有顺子或者同花，还可能有葫芦和四条
	*/
	switch (dupCnt)
	{
	case 0:
		/* It's a high card */
		return CARDTYPE_VALUE(CARDTYPE_HIGHCARD) + topFiveCardsTable[ranks];
		break;

	case 1:
		/* It's a one pair */
		two = ranks ^ (s ^ h ^ c ^ d);
		val = CARDTYPE_VALUE(CARDTYPE_ONEPAIR) + FIRSTCARD_VALUE(topCardTable[two]);
		t = ranks ^ two;
		top = (topFiveCardsTable[t] >> CARD_WIDTH) & ~FIFTHCARD_MAP;
		val += top;
		return val;
		break;
	case 2:
		/* it's a two pair */
		two = ranks ^ (s ^ h ^ c ^ d);
		if (two)
		{
			t = ranks ^ two;
			val = CARDTYPE_VALUE(CARDTYPE_TWOPAIR)
				+ (topFiveCardsTable[two]
				& (FIRSTCARD_MAP | SECONDCARD_MAP))
				+ THIRDCARD_VALUE(topCardTable[t]);
			return val;
		}
		else
		{
			three = ((s&h) | (c&d)) & ((s&c) | (h&d));
			val = CARDTYPE_VALUE(CARDTYPE_TREEKIND)
				+ FIRSTCARD_VALUE(topCardTable[three]);
			t = ranks ^ three;
			second = topCardTable[t];
			val += SECONDCARD_VALUE(second);
			t ^= (1 << second);
			val += THIRDCARD_VALUE(topCardTable[t]);
			return val;
		}
		break;
	default:
		four = s & h & c & d;
		if (four) {
			t = topCardTable[four];
			val = CARDTYPE_VALUE(CARDTYPE_FOURKIND)
				+ FIRSTCARD_VALUE(t)
				+ SECONDCARD_VALUE(topCardTable[ranks ^ (1 << t)]);
			return val;
		};

		two = ranks ^ (s ^ h ^ c ^ d);
		if (nRankTable[two] != dupCnt) {
			three = ((s&h) | (c&d)) & ((s&c) | (h&d));
			val = CARDTYPE_VALUE(CARDTYPE_FULLHOUSE);
			top = topCardTable[three];
			val += FIRSTCARD_VALUE(top);
			t = (two | three) ^ (1 << top);
			val += SECONDCARD_VALUE(topCardTable[t]);
			return val;
		};

		if (val) /* flus and straight */
			return val;
		else {
			/* Must be two pair */
			val = CARDTYPE_VALUE(CARDTYPE_TWOPAIR);
			top = topCardTable[two];
			val += FIRSTCARD_VALUE(top);
			second = topCardTable[two ^ (1 << top)];
			val += SECONDCARD_VALUE(second);
			val += THIRDCARD_VALUE(topCardTable[ranks ^ (1 << top)
				^ (1 << second)]);
			return val;
		};
		break;
	};
	return 0;
}

int getCardCnt(Deck deck)
{
	uint32 s = DECK_SPADES(deck);
	uint32 h = DECK_HEARTS(deck);
	uint32 c = DECK_CLUBS(deck);
	uint32 d = DECK_DIAMONDS(deck);
	return  nRankTable[s] + nRankTable[h] + nRankTable[c] + nRankTable[d];
}

static SampleResult _calTwoPlayerSampleResult(int cardCnt, Deck deadDeck, Deck p1, Deck p2)
{
	SampleResult sr = { 0 };
	uint32 val1, val2;
	int i1, i2, i3, i4, i5;
	Deck deck1, deck2, c1, c2, c3, c4, c5, d1, d2, d3, d4, d5;

	i1 = i2 = i3 = i4 = i5 = 0;
	DECK_RESET(d5);
	deck1 = deck2 = c1 = c2 = c3 = c4 = c5 = d1 = d2 = d3 = d4 = d5;

	switch (cardCnt) {
	default:
	case 5:
	case 0:
		break;
	case 4:
		i2 = CARDS_N - 1;
		break;
	case 3:
		i3 = CARDS_N - 1;
		break;
	case 2:
		i4 = CARDS_N - 1;
		break;
	case 1:
		i5 = CARDS_N - 1;
		break;
	}
	switch (cardCnt) {
	default:
		//printf("calc_card_rate: cards's number error!\n");
		break;

	case 5:
		for (i1 = CARDS_N - 1; i1 >= 0; i1--) {
			c1 = DECK(i1);
			if (DECK_ISREPEAT(deadDeck, c1))
				continue;
			d1 = c1;
			for (i2 = i1 - 1; i2 >= 0; i2--) {
	case 4:
		c2 = DECK(i2);
		if (DECK_ISREPEAT(deadDeck, c2))
			continue;
		DECK_OR(d2, d1, c2);
		for (i3 = i2 - 1; i3 >= 0; i3--) {
	case 3:
		c3 = DECK(i3);
		if (DECK_ISREPEAT(deadDeck, c3))
			continue;
		DECK_OR(d3, d2, c3);
		for (i4 = i3 - 1; i4 >= 0; i4--) {
	case 2:
		c4 = DECK(i4);
		if (DECK_ISREPEAT(deadDeck, c4))
			continue;
		DECK_OR(d4, d3, c4);
		for (i5 = i4 - 1; i5 >= 0; i5--) {
	case 1:
		c5 = DECK(i5);
		if (DECK_ISREPEAT(deadDeck, c5))
			continue;
		DECK_OR(d5, d4, c5);
	case 0:
	{
			  ++sr.totalCnt;
			  DECK_OR(deck1, p1, d5);
			  val1 = _calDeckVal(deck1, 7);
			  DECK_OR(deck2, p2, d5);
			  val2 = _calDeckVal(deck2, 7);
			  if (val1 > val2)
				  ++sr.winCnt;
			  else if (val1 < val2)
				  ++sr.failCnt;
			  else
				  ++sr.tieCnt;
	}
		}
		}
		}
			}
		}
		return sr;
	}
}

static SampleResult _calPlayerSampleResult(Deck handDeck, Deck publicDeck)
{
	SampleResult sr = { 0 };
	Deck d1, d2, dDead;
	uint32 cardCnt = getCardCnt(publicDeck);
	if (cardCnt < 3)
	{
		return sr;
	}
	DECK_OR(d1, handDeck, publicDeck);

	int i1, i2;
	Deck c1, c2, deadDeck;
	DECK_OR(deadDeck, handDeck, publicDeck);
	DECK_RESET(c2);
	c1 = c2;
	for (i1 = CARDS_N - 1; i1 >= 0; i1--)
	{
		c1 = DECK(i1);
		if (DECK_ISREPEAT(deadDeck, c1))
			continue;

		for (i2 = i1 - 1; i2 >= 0; i2--)
		{
			c2 = DECK(i2);
			if (DECK_ISREPEAT(deadDeck, c2))
				continue;
			DECK_OR(c2, c1, c2);
			DECK_OR(d2, c2, publicDeck);
			DECK_OR(dDead, d2, deadDeck);
			SampleResult tmp = _calTwoPlayerSampleResult(5 - cardCnt, dDead, d1, d2);
			sr.winCnt += tmp.winCnt;
			sr.tieCnt += tmp.tieCnt;
			sr.failCnt += tmp.failCnt;
			sr.totalCnt += tmp.totalCnt;
		}
	}
	return sr;
}

static double _calMultiHandStrength(int totalCnt, int winCntt, int tieCnt, int otherPlayerNum)
{
	int i;
	double handStrength = 1;
	int isFirst = 1;
	int win = winCntt + tieCnt / (otherPlayerNum + 1);
	for (i = 0; i < otherPlayerNum; ++i)
	{
		if (isFirst)
		{
			handStrength = (double)(win - i) / (totalCnt - i);
			isFirst = 0;
		}
		else
		{
			handStrength *= (double)(win - i) / (totalCnt - i);
		}
	}
	return handStrength;
}

double calHandStrength(Deck handDeck, Deck publicDeck, int otherPlayerNum)
{
	SampleResult sr = _calPlayerSampleResult(handDeck, publicDeck);
	fprintf(file, "win count:%d, tie count:%d, fail count:%d.\n", sr.winCnt, sr.tieCnt, sr.failCnt);
	double handStrength = _calMultiHandStrength(sr.totalCnt, sr.winCnt, sr.tieCnt, otherPlayerNum);
	pubInfo.lastCalPostResult = handStrength;
	return handStrength;
}

//获取自己和其他玩家金币和的差距(金币包括金币和筹码)
//返回自己金币-其他玩家金币
static int _getGoldGap()
{
	int i;
	int myGold = myInfo.myJetton + myInfo.myMoney;
	int othersGold = 0, goldGap;
	for (i = 0; i < 7; i++)
	{
		if (i != myInfo.mySeat)
		{
			othersGold += pubInfo.seatInfo[i].jetton + pubInfo.seatInfo[i].money;
		}
	}
	goldGap = myGold - othersGold;
	myInfo.goldGap = goldGap;
	return goldGap;
}

static int _getRaiseBet(int min, int max, double ratio)
{
	int bet = (int)(myInfo.myJetton * ratio / 10) * 10;
	if (bet < min)
	{
		return min;
	}
	else if (bet > max)
	{
		return max;
	}
	else
	{
		return bet;
	}
}

int isWin()
{
	int foldGold = 0;
	if (pubInfo.totalSeatNum == 2)
	{
		foldGold = ((REST_ROUND(pubInfo.handNum) / 2) + 1) * pubInfo.smallBlind;
	}
	else
	{
		foldGold = ((REST_ROUND(pubInfo.handNum) / pubInfo.totalSeatNum) + 1) * (pubInfo.smallBlind + pubInfo.bigBlind);
	}
	//if (goldGap >= foldGold * 2)
	//{
	//	myInfo.isWin = 1;
	//	return 1;
	//}
	if (myInfo.myJetton + myInfo.myMoney - foldGold > 16000)
	{
		myInfo.isWin = 1;
		return 1;
	}
	return 0;
}

/*
筹码保护
*/
int stackProtect(double winPercent){
	//stack protection
	int stack = myInfo.myJetton - pubInfo.nowCallPot;
	int stackProtect;
	// fprintf(file, "money %d curpn %d winp %f\n", myInfo.myMoney, pubInfo.curPlayerNum, winPercent);
	/* 按照金币数有无进行分类 */
	if (0 == myInfo.myMoney){
		stackProtect = stack < 10 * pubInfo.bigBlind && winPercent < 0.8;
	}
	else{
		stackProtect = stack < 4 * pubInfo.bigBlind && myInfo.myJetton > 8 * pubInfo.bigBlind && winPercent < 0.8;
	}
	return stackProtect;
}

static int _freeCall(double betRate)
{
	if (betRate == 0)
	{
		return ACTION_CHECK;
	}
	else
	{
		return ACTION_FOLD;
	}
}

static double _findRaiseRatio(int progress)
{
	int i;
	double ratio, min = -1;
	for (i = 0; i < OTHERPLAYER_N; i++)
	{
		if (!pubInfo.opInfo[i].isFold && !pubInfo.opInfo[i].isDead)
		{
			if (progress == PRGS_PREFLOP)
			{
				ratio = (double)pubInfo.opInfo[i].preRaiseNum / pubInfo.preFlopNum;
			}
			else
			{
				if (pubInfo.opInfo[i].postPlayNum < 20)
				{
					ratio = 0;
				}
				else
				{
					ratio = (double)pubInfo.opInfo[i].postRaiseNum / pubInfo.opInfo[i].postPlayNum;
				}
			}
			if (min < ratio)
			{
				min = ratio;
			}
		}
	}
	if (min == -1)
	{
		return 0;
	}
	return min;
}

static int _isPreRaise()
{
	int i;
	double ratio = 1;
	for (i = 0; i < OTHERPLAYER_N; i++)
	{
		if (pubInfo.opInfo[i].pid == pubInfo.curRaiseId)
		{
			ratio = (double)pubInfo.opInfo[i].preRaiseNum / pubInfo.preFlopNum;
			if (ratio > 0.5)
			{
				fprintf(file, "%d is a preraise dog! %d/%d\n", pubInfo.curRaiseId, pubInfo.opInfo[i].preRaiseNum, pubInfo.preFlopNum);
				return 1;
			}
			fprintf(file, "%d is not a preraise dog! %d/%d\n", pubInfo.curRaiseId, pubInfo.opInfo[i].preRaiseNum, pubInfo.preFlopNum);
			break;
		}
	}
	return 0;
}

static int _getBeforePlayerNum()
{
	int i, cnt = 0;
	for (i = 0; i < OTHERPLAYER_N; i++)
	{
		if (!pubInfo.opInfo[i].isFold && !pubInfo.opInfo[i].isDead)
		{
			if (myInfo.mySeat == 0)
			{
					cnt++;
			}
			else if (myInfo.mySeat > 0 && pubInfo.opInfo[i].seatNum > 0 && pubInfo.opInfo[i].seatNum < myInfo.mySeat)
			{
				cnt++;
			}
		}
	}
	return cnt;
}

static int _isActiveRule()
{
	int i, j, index = -1, foldNum = 1, playNum = 1;
	double ratio = 1, temp;
	fprintf(file, "_getBeforePlayerNum : %d", _getBeforePlayerNum());
	if (myInfo.myRoundBet == 0 && pubInfo.curPlayerNum < 4 || pubInfo.nowCallPot == 0 && _getBeforePlayerNum() > pubInfo.curPlayerNum / 2)
	{
		for (i = 0; i < OTHERPLAYER_N; i++)
		{
			if (pubInfo.opInfo[i].isFold || pubInfo.opInfo[i].isDead || pubInfo.opInfo[i].playNumAfterRule < 6)
			{
				continue;
			}
			temp = (double)pubInfo.opInfo[i].foldNumAfterRule / pubInfo.opInfo[i].playNumAfterRule;
			if (ratio > temp)
			{
				ratio = temp;
			}
		}
		if (ratio > 0.5)
		{
			fprintf(file, "fold ratio : %f, rule actived!\n", ratio);
			return 1;
		}
		else
		{
			fprintf(file, "fold ratio : %f, rule not actived!\n", ratio);
			return 0;
		}
	}
	else
	{
		return 0;
	}
}

static int _preFlopStrategy(double winRate, double betRate, double betPercent)
{
	int act;
	int randNum = RANDNUM;
	double raiseRatio;

	fprintf(file, "cur raise id : %d\n", pubInfo.curRaiseId);
	fprintf(file, "preFlopNum : %d\n", pubInfo.preFlopNum);
	fprintf(file, "afterRaisePlayerNum : %d\n", pubInfo.afterRaisePlayerNum);
	fprintf(file, "isPreRaise : %d\n", _isPreRaise());
	fprintf(file, "isBluffed : %d\n", myInfo.isBluffed);
	if (pubInfo.handNum < 30)
	{
		if (!myInfo.isBluffed)
		{
			if (winRate >(double)1 / pubInfo.curPlayerNum * 1.5 && pubInfo.curPlayerNum < 4)
			{
				act = ACTION_RAISE;
				myInfo.raiseBet = _getRaiseBet(300, 500, MEDIUM_BETPERCENT);
				myInfo.isBluffed = 1;
				pubInfo.preFlopNum--;
			}
			else
			{
				act = _freeCall(betRate);
			}
		}
		else
		{
			act = ACTION_CALL;
		}
	}
	else
	{
		raiseRatio = _findRaiseRatio(pubInfo.progress);
		fprintf(file, "preRaiseRatio : %f\n", raiseRatio);
		fprintf(file, "getBeforePlayerNum : %d\n", _getBeforePlayerNum());
		fprintf(file, "callPlayerNum : %d\n", pubInfo.callPlayerNum);
		if (myInfo.myRoundBet <= pubInfo.bigBlind && pubInfo.curPlayerNum < 4
			&& winRate < (double)1 / pubInfo.curPlayerNum * 1.3 && raiseRatio < 0.5)
		{
			fprintf(file, "i am pre raise dog!\n");
			act = ACTION_RAISE;
			myInfo.raiseBet = _getRaiseBet(THREE_BIGBLIND, FIVE_BIGBLIND, SMALL_BETPERCENT);
			myInfo.isRuleActived = 1;
			pubInfo.preFlopNum--;
		}
		else if (myInfo.myRoundBet > pubInfo.bigBlind && _isPreRaise() && pubInfo.afterRaisePlayerNum == 0)
		{
			fprintf(file, "i beat pre raise dog!\n");
			if (winRate < (double)1 / pubInfo.curPlayerNum)
			{
				if (betPercent < SMALL_BETPERCENT && myInfo.myRoundBet <= DOUBLE_BIGBLIND)
				{
					act = ACTION_CALL;
				}
				else
				{
					act = _freeCall(betRate);
				}
			}
			else if (winRate < (double)1 / pubInfo.curPlayerNum * 1.3)
			{
				if (betPercent < MEDIUM_BETPERCENT && myInfo.myRoundBet <= THREE_BIGBLIND)
				{
					act = ACTION_CALL;
				}
				else
				{
					act = _freeCall(betRate);
				}
			}
			else if (winRate < (double)1 / pubInfo.curPlayerNum * 1.5)
			{
				if (betPercent < SMALL_BETPERCENT && myInfo.myRoundBet <= DOUBLE_BIGBLIND)
				{
					act = ACTION_RAISE;
					myInfo.raiseBet = DOUBLE_BIGBLIND;
				}
				else if (betPercent < MEDIUM_BETPERCENT)
				{
					act = ACTION_CALL;
				}
				else
				{
					act = _freeCall(betRate);
				}
			}
			else
			{
				act = ACTION_RAISE;
				myInfo.raiseBet = FIVE_BIGBLIND;
			}
		}
		else
		{
			if (winRate > 0.8)
			{
				if (betPercent < MEDIUM_BETPERCENT)
				{
					if (randNum < 30)
					{
						act = ACTION_RAISE;
						myInfo.raiseBet = THREE_BIGBLIND;
					}
					else if (randNum < 70)
					{
						act = ACTION_RAISE;
						myInfo.raiseBet = DOUBLE_BIGBLIND;
					}
					else
					{
						act = ACTION_RAISE;
						myInfo.raiseBet = pubInfo.bigBlind;
					}
				}
				else
				{
					act = ACTION_CALL;
				}

			}
			else if (winRate > 0.7)
			{
				if (betPercent < MEDIUM_BETPERCENT)
				{
					if (randNum < 30)
					{
						act = ACTION_RAISE;
						myInfo.raiseBet = THREE_BIGBLIND;
					}
					else if (randNum < 70)
					{
						act = ACTION_RAISE;
						myInfo.raiseBet = DOUBLE_BIGBLIND;
					}
					else
					{
						act = ACTION_RAISE;
						myInfo.raiseBet = pubInfo.bigBlind;
					}
				}
				else if (betPercent < BIG_BETPERCENT)
				{
					act = ACTION_CALL;
				}
				else
				{
					act = _freeCall(betRate);
				}
			}
			else
			{
				if (winRate < (double)1 / pubInfo.curPlayerNum)
				{
					act = _freeCall(betRate);
				}
				else if (winRate < (double)1 / pubInfo.curPlayerNum * 1.15)
				{
					if (betPercent < SMALL_BETPERCENT)
					{
						if (randNum < 80)
						{
							act = ACTION_CALL;
						}
						else
						{
							act = _freeCall(betRate);
						}
					}
					else
					{
						act = _freeCall(betRate);
					}
				}
				else if (winRate < (double)1 / pubInfo.curPlayerNum * 1.3)
				{

					if (betPercent < MEDIUM_BETPERCENT)
					{
						act = ACTION_CALL;
					}
					else
					{
						act = _freeCall(betRate);
					}
				}
				else if (winRate < (double)1 / pubInfo.curPlayerNum * 1.5)
				{
					if (betPercent < SMALL_BETPERCENT)
					{
						if (randNum < 50)
						{
							act = ACTION_RAISE;
							myInfo.raiseBet = pubInfo.bigBlind;
						}
						else
						{
							act = ACTION_CALL;
						}
					}
					else if (betPercent < MEDIUM_BETPERCENT)
					{
						act = ACTION_CALL;
					}
					else
					{
						act = _freeCall(betRate);
					}
				}
				else
				{
					if (betPercent < SMALL_BETPERCENT)
					{
						act = ACTION_RAISE;
						myInfo.raiseBet = pubInfo.bigBlind;
					}
					else if (betPercent < BIG_BETPERCENT)
					{
						act = ACTION_CALL;
					}
					else
					{
						act = _freeCall(betRate);
					}
				}
			}
		}
	}
	return act;
}

static int _postFlopStrategy(double winRate, double betRate, double betPercent)
{
	int act;
	int randNum = RANDNUM;
	double rr = winRate / ((double)myInfo.myBet / pubInfo.totalPot);
	double raiseRatio;
	if (_isActiveRule())
	{
		myInfo.isRuleActived = 1;
		act = ACTION_RAISE;
		myInfo.raiseBet = _getRaiseBet(THREE_BIGBLIND, FIVE_BIGBLIND, 0.03);
	}
	else
	{
		raiseRatio = _findRaiseRatio(pubInfo.progress);
		fprintf(file, "raise ratio : %f\n", raiseRatio);
		if (raiseRatio > 0.3)
		{
			fprintf(file, "i beat raise dog!\n");
			if (winRate < 0.65)
			{
				if (betPercent < SMALL_BETPERCENT && myInfo.myRoundBet <= DOUBLE_BIGBLIND)
				{
					act = ACTION_CALL;
				}
				else
				{
					act = _freeCall(betRate);
				}
			}
			else if (winRate < 0.82)
			{
				if (betPercent < MEDIUM_BETPERCENT && myInfo.myRoundBet <= THREE_BIGBLIND)
				{
					act = ACTION_CALL;
				}
				else
				{
					act = _freeCall(betRate);
				}
			}
			else if (winRate < 0.95)
			{
				if (betPercent < BIG_BETPERCENT && myInfo.myRoundBet <= FIVE_BIGBLIND)
				{
					act = ACTION_CALL;
				}
				else
				{
					act = _freeCall(betRate);
				}
			}
			else
			{
				act = ACTION_RAISE;
				myInfo.raiseBet = FIVE_BIGBLIND;
			}
		}
		else
		{
			if (winRate < 0.5)
			{
				if (betPercent < SMALL_BETPERCENT && myInfo.myRoundBet <= DOUBLE_BIGBLIND)
				{
					act = ACTION_CALL;
				}
				else
				{
					act = _freeCall(betRate);
				}
			}
			else if (winRate < 0.65)
			{
				if (betPercent < SMALL_BETPERCENT && myInfo.myRoundBet <= FOUR_BIGBLIND)
				{
					act = ACTION_CALL;
				}
				else
				{
					act = _freeCall(betRate);
				}
			}
			else if (winRate < 0.82)
			{
				if (pubInfo.progress == PRGS_FLOP || PRGS_TURN == PRGS_TURN)
				{
					if (betPercent < SMALL_BETPERCENT && myInfo.myRoundBet <= DOUBLE_BIGBLIND)
					{
						act = ACTION_RAISE;
						if (randNum < 30)
						{
							myInfo.raiseBet = pubInfo.bigBlind;
						}
						else if (randNum < 80)
						{
							myInfo.raiseBet = DOUBLE_BIGBLIND;
						}
						else
						{
							myInfo.raiseBet = THREE_BIGBLIND;
						}
					}
					else if (betPercent < MEDIUM_BETPERCENT && myInfo.myRoundBet <= FOUR_BIGBLIND)
					{
						act = ACTION_CALL;
					}
					else
					{
						act = _freeCall(betRate);
					}
				}
				else if (pubInfo.progress == PRGS_RIVER)
				{
					if (betPercent < MEDIUM_BETPERCENT && myInfo.myRoundBet <= THREE_BIGBLIND)
					{
						act = ACTION_CALL;
					}
					else
					{
						act = _freeCall(betRate);
					}
				}
			}
			else if (winRate < 0.92)
			{
				if (pubInfo.progress == PRGS_FLOP || PRGS_TURN == PRGS_TURN)
				{
					if (betPercent < MEDIUM_BETPERCENT)
					{
						act = ACTION_RAISE;
						if (randNum < 10)
						{
							myInfo.raiseBet = THREE_BIGBLIND;
						}
						else if (randNum < 50)
						{
							myInfo.raiseBet = FOUR_BIGBLIND;
						}
						else
						{
							myInfo.raiseBet = FIVE_BIGBLIND;
						}
					}
					else if (betPercent < BIG_BETPERCENT)
					{
						act = ACTION_CALL;
					}
					else
					{
						act = _freeCall(betRate);
					}
				}
				else if (pubInfo.progress == PRGS_RIVER)
				{
					if (betPercent < MEDIUM_BETPERCENT && myInfo.myRoundBet <= DOUBLE_BIGBLIND)
					{
						act = ACTION_RAISE;
						if (randNum < 50)
						{
							myInfo.raiseBet = pubInfo.bigBlind;
						}
						else
						{
							myInfo.raiseBet = DOUBLE_BIGBLIND;
						}
					}
					else if (betPercent < BIG_BETPERCENT && myInfo.myRoundBet <= FOUR_BIGBLIND)
					{
						act = ACTION_CALL;
					}
					else
					{
						act = _freeCall(betRate);
					}
				}
			}
			else
			{
				if (pubInfo.progress == PRGS_FLOP || PRGS_TURN == PRGS_TURN)
				{
					act = ACTION_RAISE;
					if (randNum < 40)
					{
						myInfo.raiseBet = DOUBLE_BIGBLIND;
					}
					else if (randNum < 70)
					{
						myInfo.raiseBet = THREE_BIGBLIND;
					}
					else if (randNum < 90)
					{
						myInfo.raiseBet = FOUR_BIGBLIND;
					}
					else
					{
						myInfo.raiseBet = FIVE_BIGBLIND;
					}
				}
				else if (pubInfo.progress == PRGS_RIVER)
				{
					act = ACTION_RAISE;
					if (randNum < 30)
					{
						myInfo.raiseBet = FOUR_BIGBLIND;
					}
					else
					{
						myInfo.raiseBet = FIVE_BIGBLIND;
					}
				}
			}
		}
	}
	return act;
}

int chooseAction(double winRate, double betRate)
{
	int act;
	double betPercent;

	if (winRate >= 0.999)
	{
		fprintf(file, "must win!\n");
		return ACTION_ALLIN;
	}

	if (isWin())
	{
		fprintf(file, "already win!\n");
		return _freeCall(betRate);
	}

	if (stackProtect(winRate))
	{
		fprintf(file, "stack protect!\n");
		return _freeCall(betRate);
	}

	betPercent = (double)myInfo.myRoundBet / (myInfo.myJetton > 2000 ? myInfo.myJetton : 2000);
	//preflop策略
	if (pubInfo.progress == PRGS_PREFLOP)
	{
		act = _preFlopStrategy(winRate, betRate, betPercent);
	}
	else
	{
		if (pubInfo.progress == PRGS_RIVER && pubInfo.curPlayerNum > 3)
		{
			fprintf(file, "player num protect!\n");
			winRate /= (1 + (double)(pubInfo.curPlayerNum - 3) / 10);
		}
		act = _postFlopStrategy(winRate, betRate, betPercent);
	}
	return act;
}