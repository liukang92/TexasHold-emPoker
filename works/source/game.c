#include "game.h"
#include "strategy.h"

char *serverIp, *clientIp;
unsigned int serverPort, clientPort, pid;
int mySocket;
/* 用于分割消息的协议 */
char protocolBegin[10][20] = { "seat/ \n", "blind/ \n", "hold/ \n", "inquire/ \n", "flop/ \n", "turn/ \n", "river/ \n", "showdown/ \n", "pot-win/ \n", "notify/ \n" };
char protocolEnd[10][20] = { "/seat \n", "/blind \n", "/hold \n", "/inquire \n", "/flop \n", "/turn \n", "/river \n", "/showdown \n", "/pot-win \n", "/notify \n" };
int protocolNum = 10;//协议总数
char totalBuf[BUFFER_LENGTH];//存放消息缓冲区
int totalBufLen = 0;//存放消息缓冲区长度
char tmpInfo[BUFFER_LENGTH] = { 0 }, newInqInfo[BUFFER_LENGTH] = {0},lastInqInfo[BUFFER_LENGTH] = { 0 };//当前协议分割消息的临时存放缓冲区
privateInfo myInfo;
publicInfo pubInfo;
FILE *file;
/*
	分割函数，按delim进行分割，返回p和num
	*/
void splitInfoByDelim(char *buf, char *delim, char *p[], int *num){
	int pIndex = 0;
	do{
		if (0 == pIndex){
			p[pIndex] = strtok(buf, delim);
		}
		else{
			p[pIndex] = strtok(NULL, delim);
		}
		pIndex++;
	} while (NULL != p[pIndex - 1]);
	*num = pIndex - 1;
}

/*
	获取座次信息 0(庄) 1(小盲) ...
	*/
void splitSeatInfo(char *buf){
	int qNum, i, j, k, isDead;
	for (i = 0; i < TOTALPLAYER_N; i++){
		memset(&pubInfo.seatInfo[i], 0, sizeof(seat));
	}
	char *p[10], *q[10];
	splitInfoByDelim(buf, "\n", p, &pubInfo.totalSeatNum);

	for (i = 0; i < pubInfo.totalSeatNum; i++){
		splitInfoByDelim(p[i], " ", q, &qNum);
		if (0 == i){
			pubInfo.seatInfo[i].pid = atoi(q[1]);
			pubInfo.seatInfo[i].jetton = atoi(q[2]);
			pubInfo.seatInfo[i].money = atoi(q[3]);
		}
		else if (i <= 2){
			pubInfo.seatInfo[i].pid = atoi(q[2]);
			pubInfo.seatInfo[i].jetton = atoi(q[3]);
			pubInfo.seatInfo[i].money = atoi(q[4]);
		}
		else{
			pubInfo.seatInfo[i].pid = atoi(q[0]);
			pubInfo.seatInfo[i].jetton = atoi(q[1]);
			pubInfo.seatInfo[i].money = atoi(q[2]);
		}

	}
	/* 获取我的座次信息 */
	for (i = 0; i < pubInfo.totalSeatNum; i++){
		if (pubInfo.seatInfo[i].pid == pid){
			myInfo.mySeat = i;
			myInfo.myJetton = pubInfo.seatInfo[i].jetton;
			myInfo.myMoney = pubInfo.seatInfo[i].money;
			break;
		}
	}
	/* 给oppoinfo赋值 */
	if (pubInfo.handNum == 1)
	{
		k = 0;
		for (i = 0; i < pubInfo.totalSeatNum; i++){
			if (pubInfo.seatInfo[i].pid == pid)
			{
				continue;
			}
			pubInfo.opInfo[k].pid = pubInfo.seatInfo[i].pid;
			pubInfo.opInfo[k].seatNum = i;
			k++;
		}
	}
	else
	{
		for (j = 0; j < OTHERPLAYER_N; j++){
			isDead = 1;
			for (i = 0; i < pubInfo.totalSeatNum; i++){
				if (pubInfo.seatInfo[i].pid == pid){
					continue;
				}
				if (pubInfo.seatInfo[i].pid == pubInfo.opInfo[j].pid){
					pubInfo.opInfo[j].seatNum = i;
					isDead = 0;
					break;
				}
			}
			if (isDead){
				pubInfo.opInfo[j].isDead = 1;
			}
		}
	}
	return;
}
/*
	获取盲注信息，返回sb（小盲注）和bb（大盲注）
	*/
void splitBlindInfo(char *buf, int *sb, int *bb){
	char *p[10], *q[10];
	int i, num, qNum;
	splitInfoByDelim(buf, "\n", p, &num);
	for (i = 0; i < num; i++){
		splitInfoByDelim(p[i], " ", q, &qNum);
		if (0 == i){
			*sb = atoi(q[1]);
		}
		else{
			*bb = atoi(q[1]);
		}
	}
	return;
}
/*
	将花色字符串转换为int
	1:SPADES 2:HEARTS 3:CLUBS 4:DIAMONDS
	*/
int color2int(char *color){
	int c;
	if (0 == strcmp(color, "SPADES")) c = COLOR_SPADES;
	if (0 == strcmp(color, "HEARTS")) c = COLOR_HEARTS;
	if (0 == strcmp(color, "CLUBS")) c = COLOR_CLUBS;
	if (0 == strcmp(color, "DIAMONDS")) c = COLOR_DIAMONDS;
	return c;
}
/*
	将行动字符串转换为int
	0:blind 1:check 2:call 3:raise 4:all_in 5:fold
	*/
int action2int(char *action){
	int a;
	if (0 == strcmp(action, "blind")) a = ACTION_BLIND;
	if (0 == strcmp(action, "check")) a = ACTION_CHECK;
	if (0 == strcmp(action, "call")) a = ACTION_CALL;
	if (0 == strcmp(action, "raise")) a = ACTION_RAISE;
	if (0 == strcmp(action, "all_in")) a = ACTION_ALLIN;
	if (0 == strcmp(action, "fold")) a = ACTION_FOLD;
	return a;
}
/*
	将点数字符串转换为int
	1-13 => 2/3/4/5/6/7/8/9/10/J/Q/K/A
	*/
int point2int(char *point){
	int p = -1;
	unsigned char c = point[0];
	switch (c){
	case 'A': p = 13; break;
	case '2': case '3': case '4': case '5':
	case '6': case '7': case '8': case '9':
		p = c - '1';
		break;
	case 'J': p = 10; break;
	case 'Q': p = 11; break;
	case 'K': p = 12; break;
	default:
		break;
	}
	// compare if "10"
	if (0 == strcmp(point, "10")) p = 9;
	return p;
}
/*
	获取手牌信息
	*/
void splitHandCardInfo(char *buf, card *hc){
	char *p[10], *q[10];
	int i, num, qNum;
	DECK_RESET(myInfo.handDeck);
	pubInfo.flopDeck = pubInfo.turnDeck = pubInfo.riverDeck = myInfo.handDeck;
	splitInfoByDelim(buf, "\n", p, &num);
	for (i = 0; i < num; i++){
		splitInfoByDelim(p[i], " ", q, &qNum);
		(hc[i]).color = color2int(q[0]);
		(hc[i]).point = point2int(q[1]);
		(hc[i]).index = ((hc[i]).color - 1) * 13 + ((hc[i]).point - 1);
		DECK_SET(myInfo.handDeck, (hc[i]).index);
	}
	return;
}
/*
	更新当前跟注额及已弃牌对手信息
*/
int updateNowCallPotAndFoldPlayer(int inqPlayerNum, inquireInfo *inq){
	int i, j, foldNum=0;
	for(i=0; i<=inqPlayerNum; i++){
		pubInfo.nowCallPot = inq[i].bet > pubInfo.nowCallPot ? inq[i].bet : pubInfo.nowCallPot;//更新池底
		/* 更新我的金币、筹码、下注 */
		if (inq[i].pid == pid){
			myInfo.myBet = inq[i].bet;
			myInfo.myJetton = inq[i].jetton;
			myInfo.myMoney = inq[i].money;
		}
		/* 更新已fold玩家数目及pid */
		int isSame = 0;
		if (ACTION_FOLD == inq[i].action){
			foldNum++;
			for (j = 0; j < TOTALPLAYER_N; j++){
				if (pubInfo.foldPlayer[j] == inq[i].pid){
					isSame = 1;
					break;
				}
			}
			if (!isSame){
				pubInfo.foldPlayer[pubInfo.foldPlayerNum] = inq[i].pid;
				pubInfo.foldPlayerNum++;
			}
		}
	}
	return foldNum;
			
}
/*
	更新玩家行动序列信息
*/
void updateOppoActionList(int inqPlayerNum, inquireInfo *inq, char *flag){
	if(0 == memcmp(lastInqInfo, newInqInfo, strlen(newInqInfo))){
		// fprintf(file, "-------lastInqInfo-------\n%s\n--------------\n", lastInqInfo);
		// fprintf(file, "-------newInqInfo-------\n%s\n--------------\n", newInqInfo);
		fprintf(file, "msg repeat, ignore it!\n");
		return ;
	}
	int i, j, tmpIndex, lower, upper;
	//根据inquire和notify来分开处理
	if('i' == flag[0]){// inquire信息
		if(/*!myInfo.isFold && */PRGS_FLOP==pubInfo.progress && 0==pubInfo.flopRoundNum){
			for(i=inqPlayerNum; i>=0; i--){
				if(inq[i].pid == pubInfo.seatInfo[1].pid){//small blind 对应的位置
					upper = i;
				}
				if(inq[i].pid == pid){// 我上一家的对手位置
					lower = 0;
				}
			}
		}else{
			upper = inqPlayerNum;
			lower = 0;
		}
	}else if('n' == flag[0]){// notify信息
		if(/*!myInfo.isFold && */PRGS_PREFLOP==pubInfo.progress){
			for(i=inqPlayerNum; i>=0; i--){
				if(inq[i].pid == pubInfo.seatInfo[2].pid){//big blind 对应的位置
					lower = i;
				}
				if(inq[i].pid == pid){// 我的位置
					upper = i;
				}
			}
		}else{
			upper = inqPlayerNum;
			lower = 0;
		}
	}
	/*
		提取lower和upper之间的notify/inquire信息，因为可能会部分重复，需要去除无效范围
	*/
	for(i=upper; i>=lower; i--){
		if(pid == inq[i].pid){//我的行为
			if(!myInfo.isFold){
				if(inq[i].action == ACTION_FOLD) myInfo.isFold = 1;
				tmpIndex = pubInfo.oppoActionNum;
				pubInfo.oppoActionList[tmpIndex][0] = inq[i].pid;
				/*如果是raise，则直接记录raise值，后期用>10判断此玩家加注额 */
				pubInfo.oppoActionList[tmpIndex][1] = inq[i].action==ACTION_RAISE? inq[i].bet: inq[i].action;
				pubInfo.oppoActionNum ++;
			}
		}
		for (j = 0; j < OTHERPLAYER_N; j++){
			if (pubInfo.opInfo[j].pid == inq[i].pid){//对手行为
				if(!pubInfo.opInfo[j].isFold){
					tmpIndex = pubInfo.oppoActionNum;
					pubInfo.oppoActionList[tmpIndex][0] = inq[i].pid;
					/*如果是raise，则直接记录raise值，后期用>10判断此玩家加注额 */
					pubInfo.oppoActionList[tmpIndex][1] = inq[i].action==ACTION_RAISE? inq[i].bet: inq[i].action;
					pubInfo.oppoActionNum ++;
				}
			}
		}
	}
	
}
/*
	更新对手统计信息
*/
void updateOppoInfo(int inqPlayerNum, inquireInfo *inq){
	int i, j;
	for(i=0; i<=inqPlayerNum; i++){
		/* update opinfo isfold*/
		for (j = 0; j < OTHERPLAYER_N; j++){
			if (pubInfo.opInfo[j].pid == inq[i].pid){
				if (!pubInfo.opInfo[j].isFold){
					if (myInfo.isRuleActived){
						pubInfo.opInfo[j].playNumAfterRule++;
					}
					switch (inq[i].action){
					case ACTION_FOLD:
						pubInfo.opInfo[j].isFold = 1;
						if (myInfo.isRuleActived){
							pubInfo.opInfo[j].foldNumAfterRule++;
						}
						break;
					case ACTION_RAISE: case ACTION_ALLIN:
						if (pubInfo.curRaiseId == -1){
							pubInfo.curRaiseId = pubInfo.opInfo[j].pid;
						}
						if (PRGS_PREFLOP == pubInfo.progress){
							if (!myInfo.isRuleActived && !myInfo.isBluffed){
								pubInfo.opInfo[j].preRaiseNum++;
							}
						}
						break;
					case ACTION_CALL:
						if (pubInfo.curRaiseId == -1){
							pubInfo.afterRaisePlayerNum++;
						}
						pubInfo.callPlayerNum++;
						break;
					default:
						break;
					}
				}
				break;
			}
		}
	}
	for (i = inqPlayerNum; i >= 0; i--){
		if (inq[i].pid != pid){
			if (inq[i].action == ACTION_RAISE || inq[i].action == ACTION_ALLIN){
				for (j = 0; j < OTHERPLAYER_N; j++){
					if (pubInfo.opInfo[j].pid == inq[i].pid){
						pubInfo.opInfo[j].postRaiseNum++;
						pubInfo.opInfo[j].postPlayNum++;
					}
				}
				break;
			}else{
				for (j = 0; j < OTHERPLAYER_N; j++){
					if (pubInfo.opInfo[j].pid == inq[i].pid){
						pubInfo.opInfo[j].postPlayNum++;
					}
				}
			}

		}
	}
}
/*
	获取询问信息
	*/
void splitNotifyInfo(char *buf, inquireInfo *inq){
	int i, num, qNum, j, k;
	for (i = 0; i < TOTALPLAYER_N; i++){
		memset(&pubInfo.notify[i], 0, sizeof(inquireInfo));
	}
	char *p[10], *q[10];

	splitInfoByDelim(buf, "\n", p, &num);
	pubInfo.curRaiseId = -1;
	pubInfo.afterRaisePlayerNum = 0;
	pubInfo.callPlayerNum = 0;
	for (i = 0; i < num; i++){
		splitInfoByDelim(p[i], " ", q, &qNum);
		if ((num - 1) == i){// total pot

		}else{
			/* 顺序：pid jetton money bet action  */
			inq[i].pid = atoi(q[0]);
			inq[i].jetton = atoi(q[1]);
			inq[i].money = atoi(q[2]);
			inq[i].bet = atoi(q[3]);
			inq[i].action = action2int(q[4]);
		}
	}
	// updateNowCallPotAndFoldPlayer(num-2, inq);
	updateOppoActionList(num-2, inq, "notify");
	updateOppoInfo(num-2, inq);
			
	myInfo.isRuleActived = 0;
	return;
}
/*
	获取询问信息
	*/
void splitInquireInfo(char *buf, inquireInfo *inq){
	int i, num, qNum, j, k;
	for (i = 0; i < TOTALPLAYER_N; i++){
		memset(&pubInfo.inquire[i], 0, sizeof(inquireInfo));
	}
	char *p[10], *q[10];

	splitInfoByDelim(buf, "\n", p, &num);
	pubInfo.nowCallPot = 0;
	int foldNum = 0;
	pubInfo.curRaiseId = -1;
	pubInfo.afterRaisePlayerNum = 0;
	pubInfo.callPlayerNum = 0;
	for (i = 0; i < num; i++){
		splitInfoByDelim(p[i], " ", q, &qNum);
		if ((num - 1) == i){// total pot
			pubInfo.lastTotalPot = pubInfo.totalPot;
			pubInfo.totalPot = atoi(q[2]);
		}
		else{
			/* 顺序：pid jetton money bet action  */
			inq[i].pid = atoi(q[0]);
			inq[i].jetton = atoi(q[1]);
			inq[i].money = atoi(q[2]);
			inq[i].bet = atoi(q[3]);
			inq[i].action = action2int(q[4]);
		}
	}
	foldNum = updateNowCallPotAndFoldPlayer(num-2, inq);
	updateOppoActionList(num-2, inq, "inquire");
	updateOppoInfo(num-2, inq);
	
	myInfo.isRuleActived = 0;
	pubInfo.nowCallPot -= myInfo.myBet;//更新当前下注数，需减掉已下注
	myInfo.myRoundBet += pubInfo.nowCallPot;
	fprintf(file, "myRoundBet : %d\n", myInfo.myRoundBet);
	/* 更新当前玩家数 */
	pubInfo.curPlayerNum = pubInfo.totalSeatNum - foldNum;
	/* 针对需要allin的情况，更新当前下注数和池底 */
	if (pubInfo.nowCallPot > myInfo.myJetton){// need allin
		pubInfo.nowCallPot = myInfo.myJetton;
		pubInfo.totalPot = pubInfo.lastTotalPot + (pubInfo.curPlayerNum - 1) * myInfo.myJetton;
		fprintf(file, "need allin, last total pot:%d side pot:%d\n", pubInfo.lastTotalPot, pubInfo.totalPot);
	}
	/* 更新各阶段（preflop flop turn river）轮数 */
	if (PRGS_PREFLOP == pubInfo.progress) pubInfo.preFlopRoundNum++;
	if (PRGS_FLOP == pubInfo.progress) pubInfo.flopRoundNum++;
	if (PRGS_TURN == pubInfo.progress) pubInfo.turnRoundNum++;
	if (PRGS_RIVER == pubInfo.progress) pubInfo.riverRoundNum++;

	return;
}
/*
	获取公共牌信息，flag进行标记
	flag: flop | turn | river
	*/
void splitPublicCardInfo(char *buf, card *pc, char *flag){
	char *p[10], *q[10];
	int i, num, qNum, j, index;
	splitInfoByDelim(buf, "\n", p, &num);
	for (i = 0; i < num; i++){
		splitInfoByDelim(p[i], " ", q, &qNum);
		if ('f' == flag[0]) j = i; // flop
		if ('t' == flag[0]) j = i + 3; // turn
		if ('r' == flag[0]) j = i + 4;  // river
		(pc[j]).color = color2int(q[0]);
		(pc[j]).point = point2int(q[1]);
		(pc[j]).index = ((pc[j]).color - 1) * 13 + ((pc[j]).point - 1);
	}
	if ('f' == flag[0])
	{
		for (i = 0; i < 3; i++)
		{
			DECK_SET(pubInfo.flopDeck, (pc[i]).index);
		}
		pubInfo.riverDeck = pubInfo.turnDeck = pubInfo.flopDeck;
	}
	else if ('t' == flag[0])
	{
		DECK_SET(pubInfo.turnDeck, (pc[3]).index);
		pubInfo.riverDeck = pubInfo.turnDeck;
	}
	else if ('r' == flag[0])
	{
		DECK_SET(pubInfo.riverDeck, (pc[4]).index);
	}
	return;
}
/*
	计算对手模型胜率及均值标准差，用于建模
	*/
//void reCalPlayerWinPercent(int i){
//	int index = pubInfo.playerWp[i].wpCurNum % PLAYERWP_SAMPLENUM;
//	double wp1, wp2, wp3;
//	wp1 = postFlopWinRate(pubInfo.playerWp[i].handCard, pubInfo.publicCard, PRGS_FLOP);
//	wp2 = postFlopWinRate(pubInfo.playerWp[i].handCard, pubInfo.publicCard, PRGS_TURN);
//	wp3 = postFlopWinRate(pubInfo.playerWp[i].handCard, pubInfo.publicCard, PRGS_RIVER);
//	pubInfo.playerWp[i].winPercent[PRGS_PREFLOP][index] = 0.0;
//	pubInfo.playerWp[i].winPercent[PRGS_FLOP][index] = wp1;
//	pubInfo.playerWp[i].winPercent[PRGS_TURN][index] = wp2;
//	pubInfo.playerWp[i].winPercent[PRGS_RIVER][index] = wp3;
//
//	fprintf(file, "pid:%d\t%f\t%f\t%f\n", pubInfo.playerWp[i].pid, wp1, wp2, wp3);
//	/* 更新对手模型中的均值、方差等 */
//	int p, k, num, dd;
//	double sum, tmp;
//	dd = pubInfo.playerWp[i].wpCurNum > PLAYERWP_SAMPLENUM ? PLAYERWP_SAMPLENUM : (pubInfo.playerWp[i].wpCurNum + 1);
//	for (p = 0; p <= 3; p++){
//		num = 0;
//		sum = 0.0;
//		for (k = 0; k < dd; k++){
//			sum += pubInfo.playerWp[i].winPercent[p][k];
//			num++;
//		}
//		pubInfo.playerWp[i].wpAvg[p] = sum / num;
//		sum = 0.0;
//		for (k = 0; k < num; k++){
//			tmp = pubInfo.playerWp[i].winPercent[p][k] - pubInfo.playerWp[i].wpAvg[p];
//			sum += pow(tmp, 2);
//		}
//		pubInfo.playerWp[i].wpSigma[p] = sqrt(sum / num);
//	}
//
//	pubInfo.playerWp[i].wpCurNum++;
//}
/*
	获取showdown信息
	*/
void splitShowDownInfo(char *buf){
	fprintf(file, "\n*****cal player winpercent*****\n");
	char *p[20], *q[10];
	int i, num, qNum, j, k, index;
	int isExist;
	splitInfoByDelim(buf, "\n", p, &num);
	/* 忽略公共牌信息（已经获取过）从第8行开始，获取showdown信息 */
	for (i = 7; i < num; i++){
		splitInfoByDelim(p[i], " ", q, &qNum);
		/* update opinfo showdown*/
		for (j = 0; j < OTHERPLAYER_N; j++){
			if (pubInfo.opInfo[j].pid == atoi(q[1])){
				pubInfo.opInfo[j].showDownNum++;
				pubInfo.isShowDown = 1;
				break;
			}
		}
		// if (pid == atoi(q[1])) break; //排除自己
		// isExist = 0;
		// for (j = 0; j < 7; j++){
		// 	if (pubInfo.playerWp[j].pid == atoi(q[1])){
		// 		index = j;
		// 		isExist = 1;
		// 		break;
		// 	}
		// }
		// if (!isExist){
		// 	pubInfo.playerWp[pubInfo.playerWpNum].pid = atoi(q[1]);
		// 	index = pubInfo.playerWpNum;
		// 	pubInfo.playerWpNum++;
		// }
		// fprintf(file, "cal index: %d playerWpNum:%d\n", index, pubInfo.playerWpNum);
		/* 更新对手模型的手牌 */
		//pubInfo.playerWp[index].handCard[0].color = color2int(q[2]);
		//pubInfo.playerWp[index].handCard[0].point = point2int(q[3]);
		//pubInfo.playerWp[index].handCard[1].color = color2int(q[4]);
		//pubInfo.playerWp[index].handCard[1].point = point2int(q[5]);
		//fprintf(file, "%d %d %d %d \n", pubInfo.playerWp[index].handCard[0].color, pubInfo.playerWp[index].handCard[0].point, pubInfo.playerWp[index].handCard[1].color, pubInfo.playerWp[index].handCard[1].point);
		//reCalPlayerWinPercent(index);//更新对手模型胜率及均值标准差
	}

	//for (i = 0; i < pubInfo.playerWpNum; i++){
	//	fprintf(file, "pid: %d\n", pubInfo.playerWp[i].pid);
	//	fprintf(file, "wpavg: %f\t%f\t%f\t%f wpsigma: %f\t%f\t%f\t%f\n", pubInfo.playerWp[i].wpAvg[0], pubInfo.playerWp[i].wpAvg[1], pubInfo.playerWp[i].wpAvg[2], pubInfo.playerWp[i].wpAvg[3], pubInfo.playerWp[i].wpSigma[0], pubInfo.playerWp[i].wpSigma[1], pubInfo.playerWp[i].wpSigma[2], pubInfo.playerWp[i].wpSigma[3]);
	//}
	return;
}

/*
	获取彩池划分信息
	*/
void splitPotWinInfo(char *buf){
	char *p[10], *q[10];
	int i, num, qNum, j;
	splitInfoByDelim(buf, "\n", p, &num);
	// fprintf(file, "pot win:\n");
	if (pubInfo.isShowDown){
		for (i = 0; i < num; i++){
			splitInfoByDelim(p[i], ": ", q, &qNum);
			/* update opinfo potwin*/
			for (j = 0; j < OTHERPLAYER_N; j++){
				if (pubInfo.opInfo[j].pid == atoi(q[0])){
					pubInfo.opInfo[j].winNum++;
					break;
				}
			}
		}
	}
	fprintf(file, "action list:\n");
	for(i=0; i<pubInfo.oppoActionNum; i++){
		fprintf(file, "(%d %d) ", pubInfo.oppoActionList[i][0], pubInfo.oppoActionList[i][1]);
	}
	fprintf(file, "\n");
	return;
}

/*
	转int为花色字符串
	*/
void int2color(int i, char *c){
	switch (i){
	case 1:
		sprintf(c, "♠");
		break;
	case 2:
		sprintf(c, "♥");
		break;
	case 3:
		sprintf(c, "♣");
		break;
	case 4:
		sprintf(c, "♦");
		break;
	default:
		break;
	}
}
/*
	转int为点数字符串
	*/
void int2point(int i, char *p){
	switch (i){
	case 1:case 2:case 3:case 4:case 5:case 6:case 7:case 8:case 9:
		sprintf(p, "%d", i + 1);
		break;
	case 10:
		sprintf(p, "J");
		break;
	case 11:
		sprintf(p, "Q");
		break;
	case 12:
		sprintf(p, "K");
		break;
	case 13:
		sprintf(p, "A");
		break;
	default:
		break;
	}
}

/*
	获取对手模型中最紧的对手
	*/
// void getMostTightPlayer(){
// 	int i, k, ind;
// 	double mostTight = -1000, ttt, sigma = 0;
// 	int mostTightPid = -1, inqInd, action = -1;
// 	for (i = 0; i < TOTALPLAYER_N; i++){
// 		if (0 == pubInfo.inquire[i].pid || pid == pubInfo.inquire[i].pid || ACTION_FOLD == pubInfo.inquire[i].action){//排除已弃牌的对手和我自己
// 			continue;
// 		}
// 		else{
// 			ind = -1;
// 			for (k = 0; k<OTHERPLAYER_N; k++){
// 				if (pubInfo.playerWp[k].pid == pubInfo.inquire[i].pid){
// 					ind = k;
// 					inqInd = i;
// 					break;
// 				}
// 			}
// 			if (-1 != ind && pubInfo.playerWp[ind].wpCurNum>0){
// 				ttt = pubInfo.playerWp[ind].wpAvg[pubInfo.progress];
// 				if (mostTight < ttt){
// 					mostTightPid = pubInfo.playerWp[ind].pid;
// 					mostTight = ttt;
// 					action = pubInfo.inquire[inqInd].action;
// 					sigma = pubInfo.playerWp[ind].wpSigma[pubInfo.progress];
// 				}
// 				else{

// 				}
// 			}
// 		}
// 	}
// 	/* 更新最紧对手信息 */
// 	pubInfo.mostTightPlayer.pid = mostTightPid;
// 	pubInfo.mostTightPlayer.action = action;
// 	pubInfo.mostTightPlayer.upperWp = mostTight + sigma;
// 	pubInfo.mostTightPlayer.lowerWp = mostTight - sigma;
// 	pubInfo.mostTightPlayer.avgWp = mostTight;
// 	pubInfo.mostTightPlayer.sigmaWp = sigma;

// 	fprintf(file, "most tight player: %d %f %f %d\n", pubInfo.mostTightPlayer.pid, pubInfo.mostTightPlayer.lowerWp, pubInfo.mostTightPlayer.upperWp, pubInfo.mostTightPlayer.action);
// }
/*
	pre flop 阶段策略
	*/
int preFlopStrategy(){
	int point1 = myInfo.handCard[0].point;
	int color1 = myInfo.handCard[0].color;
	int point2 = myInfo.handCard[1].point;
	int color2 = myInfo.handCard[1].color;
	int tmp, act;
	if (point1 > point2){// make sure like this : 3 4
		tmp = point1;
		point1 = point2;
		point2 = tmp;
		tmp = color1;
		color1 = color2;
		color2 = tmp;
	}
	if (pubInfo.curPlayerNum == 1) return ACTION_CALL;
	/* 由表查得胜率，上三角为同色 */
	double winPercent;
	if (color1 == color2){//s, upper matrix
		winPercent = preFlopTable[pubInfo.curPlayerNum - 2][point1 - 1][point2 - 1] / 100.0;
	}
	else{//o, lower matrix
		winPercent = preFlopTable[pubInfo.curPlayerNum - 2][point2 - 1][point1 - 1] / 100.0;
	}

	double betPercent = ((double)pubInfo.nowCallPot) / (pubInfo.nowCallPot + pubInfo.totalPot);
	fprintf(file, "\n*****cal action to take*****\n");
	fprintf(file, "my jetton: %d my money: %d call pot: %d, total pot: %d\n", myInfo.myJetton, myInfo.myMoney, pubInfo.nowCallPot, pubInfo.totalPot);
	fprintf(file, "win percent is %f bet percent is %f \n", winPercent, betPercent);
	act = chooseAction(winPercent, betPercent);
	return act;
}

/*
	post flop阶段策略
	*/
int postFlopStrategy(){
	int act;
	if (pubInfo.curPlayerNum == 1) return ACTION_CALL;
	double winRate;
	double betRate = ((double)pubInfo.nowCallPot) / (pubInfo.nowCallPot + pubInfo.totalPot);
	fprintf(file, "flop round: %d turn round: %d river round: %d\n", pubInfo.flopRoundNum, pubInfo.turnRoundNum, pubInfo.riverRoundNum);
	if (PRGS_FLOP == pubInfo.progress && 1 == pubInfo.flopRoundNum){
		winRate = calHandStrength(myInfo.handDeck, pubInfo.flopDeck, pubInfo.curPlayerNum - 1);
	}
	else if (PRGS_TURN == pubInfo.progress && 1 == pubInfo.turnRoundNum){
		winRate = calHandStrength(myInfo.handDeck, pubInfo.turnDeck, pubInfo.curPlayerNum - 1);
	}
	else if (PRGS_RIVER == pubInfo.progress && 1 == pubInfo.riverRoundNum){
		winRate = calHandStrength(myInfo.handDeck, pubInfo.riverDeck, pubInfo.curPlayerNum - 1);
	}
	else{
		winRate = pubInfo.lastCalPostResult;
	}
	act = chooseAction(winRate, betRate);

	fprintf(file, "my jetton: %d my money: %d call pot: %d, total pot: %d\n", myInfo.myJetton, myInfo.myMoney, pubInfo.nowCallPot, pubInfo.totalPot);
	fprintf(file, "win percent is %f, bet percent is %f \n", winRate, betRate);
	/* 获取最紧对手与之比较 */
	//getMostTightPlayer();
	//if (pubInfo.mostTightPlayer.pid != -1){
	//	if (winRate > pubInfo.mostTightPlayer.upperWp){
	//		act = ACTION_CALL;
	//	}
	//	else if (winRate < pubInfo.mostTightPlayer.lowerWp){
	//		act = ACTION_FOLD;
	//	}
	//	else{
	//		double zz = (winRate - pubInfo.mostTightPlayer.avgWp) / pubInfo.mostTightPlayer.sigmaWp;//z=(x-u)/sigma
	//		if (zz >= 1.28){// 1.28为0.8的z值
	//			act = ACTION_CALL;
	//		}
	//		else{
	//			act = ACTION_CALL;
	//		}
	//	}
	//}
	return act;
}
/*
	行动
	*/
void takeAction(){
	fprintf(file, "total player: %d fold player: %d cur player: %d\n", pubInfo.totalSeatNum, pubInfo.foldPlayerNum, pubInfo.curPlayerNum);
	int i;
	for (i = 0; i < OTHERPLAYER_N; i++){
		fprintf(file, "op pid: %d seatnum: %d isDead: %d isFold: %d win/showDown: %d/%d ", pubInfo.opInfo[i].pid, pubInfo.opInfo[i].seatNum, pubInfo.opInfo[i].isDead, pubInfo.opInfo[i].isFold, pubInfo.opInfo[i].winNum, pubInfo.opInfo[i].showDownNum);
		fprintf(file, "fold/play afterRule: %d/%d ", pubInfo.opInfo[i].foldNumAfterRule, pubInfo.opInfo[i].playNumAfterRule);
		fprintf(file, "preRaise/postRaise/postPlay: %d/%d/%d\n", pubInfo.opInfo[i].preRaiseNum, pubInfo.opInfo[i].postRaiseNum, pubInfo.opInfo[i].postPlayNum);
	}

	int raiseNum;
	char action[30];
	int act;
	if (PRGS_PREFLOP == pubInfo.progress){ // pre flop
		act = preFlopStrategy();
	}
	else{
		act = postFlopStrategy();
	}

	switch (act){
	case ACTION_FOLD:
		sprintf(action, "fold \n");
		break;
	case ACTION_CALL:
		sprintf(action, "call \n");
		break;
	case ACTION_CHECK:
		sprintf(action, "check \n");
		break;
	case ACTION_RAISE:
		sprintf(action, "raise %d \n", myInfo.raiseBet);
		break;
	case ACTION_ALLIN:
		sprintf(action, "all_in \n");
		break;
	default:
		sprintf(action, "fold \n");
		break;
	}
	int sendSize = send(mySocket, action, strlen(action), 0);
	fprintf(file, "action: %s\n", action);
	return;
}
/*
	打印卡牌信息
	*/
void printCard(char *flag){
	char s1[3], s2[3], s3[3], s4[3];
	int i;
	if (flag[0] == 'h'){
		fprintf(file, "hand card : (");
		for (i = 0; i < 2; i++){
			int2color(myInfo.handCard[i].color, s1);
			int2point(myInfo.handCard[i].point, s2);
			fprintf(file, "%s%s ", s1, s2);
		}

	}
	else if (flag[0] == 'f'){
		fprintf(file, "flop : (");
		for (i = 0; i < 3; i++){
			int2color(pubInfo.publicCard[i].color, s1);
			int2point(pubInfo.publicCard[i].point, s2);
			fprintf(file, "%s%s ", s1, s2);
		}

	}
	else if (flag[0] == 't'){
		fprintf(file, "turn: (");
		for (i = 3; i < 4; i++){
			int2color(pubInfo.publicCard[i].color, s1);
			int2point(pubInfo.publicCard[i].point, s2);
			fprintf(file, "%s%s ", s1, s2);
		}

	}
	else if (flag[0] == 'r'){
		fprintf(file, "river : (");
		for (i = 4; i < 5; i++){
			int2color(pubInfo.publicCard[i].color, s1);
			int2point(pubInfo.publicCard[i].point, s2);
			fprintf(file, "%s%s ", s1, s2);
		}

	}
	fprintf(file, ")\n");

}
/*
	每局开始之前需要初始化的参数
	*/
void initEveryHandParams(){
	pubInfo.foldPlayerNum = 0;
	memset(pubInfo.foldPlayer, 0, sizeof(pubInfo.foldPlayer));
	myInfo.myJetton = 0;
	myInfo.myMoney = 0;
	myInfo.myBet = 0;
	myInfo.isBluffed = 0;
	myInfo.isFold = 0;
	pubInfo.preFlopRoundNum = pubInfo.flopRoundNum = pubInfo.turnRoundNum = pubInfo.riverRoundNum = 0;
	pubInfo.handNum++;
	pubInfo.preFlopNum++;

	int iii;
	for (iii = 0; iii < OTHERPLAYER_N; iii++){
		pubInfo.opInfo[iii].isFold = 0;
		pubInfo.opInfo[iii].seatNum = -1;
	}
	myInfo.isRuleActived = 0;
	pubInfo.isShowDown = 0;
	pubInfo.oppoActionNum = 0;
	memset(pubInfo.oppoActionList, 0, sizeof(pubInfo.oppoActionList));
}
/*
	处理各类信息
	*/
void handleInfo(int type){
	switch (type){
	case 0: // seat info
		initEveryHandParams();
		splitSeatInfo(tmpInfo);
		fprintf(file, "-----------------hand %d------------------\n\n", pubInfo.handNum);
		fprintf(file, "my seat: %d \n", myInfo.mySeat);
		break;
	case 1: // blind info
		splitBlindInfo(tmpInfo, &(pubInfo.smallBlind), &(pubInfo.bigBlind));
		break;
	case 2: // hand card
		splitHandCardInfo(tmpInfo, myInfo.handCard);
		printCard("handcard");
		pubInfo.progress = PRGS_PREFLOP; //pre flop
		myInfo.myRoundBet = 0;
		break;
	case 3: // inqure and take action !!!!
		splitInquireInfo(tmpInfo, pubInfo.inquire);
		takeAction();
		break;
	case 4: // flop
		splitPublicCardInfo(tmpInfo, pubInfo.publicCard, "flop");
		printCard("flop");
		pubInfo.progress = PRGS_FLOP; // flop
		myInfo.myRoundBet = 0;
		break;
	case 5: // turn
		splitPublicCardInfo(tmpInfo, pubInfo.publicCard, "turn");
		printCard("turn");
		pubInfo.progress = PRGS_TURN; //turn
		myInfo.myRoundBet = 0;
		break;
	case 6: // river
		splitPublicCardInfo(tmpInfo, pubInfo.publicCard, "river");
		printCard("river");
		pubInfo.progress = PRGS_RIVER; //river
		myInfo.myRoundBet = 0;
		break;
	case 7: //show down info
		splitShowDownInfo(tmpInfo);
		break;
	case 8: // pot win
		splitPotWinInfo(tmpInfo);
		break;
	case 9: // notify
		splitNotifyInfo(tmpInfo, pubInfo.notify);
		break;
	default:
		break;
	}
	return;
}

/*
	读取从gameserver接收的信息
	判断协议是否出现，出现则剪切到tmpInfo中
	example: seat/ eol ...(info we want)... /seat eol
	*/
int getInfoFromBuffer(){
	//find the protocolBegin and the protocolEnd
	int j;
	int copyLen, moveLen;
	char *pBegin, *pEnd;
	int isFound = 0;
	/* 遍历每种协议 */
	for (j = 0; j < protocolNum; j++){
		pBegin = strstr(totalBuf, protocolBegin[j]);
		if (NULL == pBegin) continue;
		pEnd = strstr(pBegin, protocolEnd[j]);
		if (NULL != pEnd){// 发现协议
			/* 拷贝协议包裹的内容到tmpInfo中 */
			copyLen = strlen(pBegin) - strlen(pEnd) - strlen(protocolBegin[j]);
			memset(tmpInfo, '\0', BUFFER_LENGTH);
			memcpy(tmpInfo, pBegin + strlen(protocolBegin[j]), copyLen);
			/* 为了比较inquire信息是否重发 */
			if(3==j || 9==j){
				memset(lastInqInfo, '\0', BUFFER_LENGTH);
				memcpy(lastInqInfo, newInqInfo, strlen(newInqInfo));
				memset(newInqInfo, '\0', BUFFER_LENGTH);
				memcpy(newInqInfo, pBegin + strlen(protocolBegin[j]), copyLen);
			}
			/* 将总缓冲区向前移动 */
			moveLen = strlen(pEnd) - strlen(protocolEnd[j]);
			memmove(totalBuf, pEnd + strlen(protocolEnd[j]), moveLen); //remove the info got from totalBuf
			memset(totalBuf + moveLen, '\0', BUFFER_LENGTH - moveLen);
			totalBufLen = moveLen;
			isFound = 1;
			handleInfo(j);
			
			break;
		}
	}
	/* 找到game-over协议 */
	if (NULL != (pBegin = strstr(totalBuf, "game-over \n"))){
		return GAMEOVER;
	}
	return 1 == isFound ? PROTOCOL_FOUND : PROTOCOL_NOTFOUND;
}


int main(int argc, char *argv[]){
	/*
		接收 5 参数
		*/
	serverIp = argv[1];
	serverPort = atoi(argv[2]);
	clientIp = argv[3];
	clientPort = atoi(argv[4]);
	pid = atoi(argv[5]);
	char pname[30];
	sprintf(pname, "Whyjjc");

	int srd = (unsigned)time(NULL) + pid;
	srand(srd);

	int cnt;
	/* 初始化局数、对手建模信息 */
	myInfo.isWin = 0;
	pubInfo.handNum = 0;
	// pubInfo.playerWpNum = 0;
	int iii;
	for (iii = 0; iii < OTHERPLAYER_N; iii++){
		pubInfo.opInfo[iii].foldNumAfterRule = 0;
		pubInfo.opInfo[iii].playNumAfterRule = 0;
		pubInfo.opInfo[iii].preRaiseNum = 0;
		pubInfo.opInfo[iii].postRaiseNum = 0;
		pubInfo.opInfo[iii].postPlayNum = 0;
		pubInfo.opInfo[iii].winNum = 0;
		pubInfo.opInfo[iii].showDownNum = 0;
		pubInfo.opInfo[iii].isDead = 0;
		pubInfo.opInfo[iii].pid = 0;
	}

	char fname[30];
	sprintf(fname, "log_%s.dat", clientIp);
	file = fopen(fname, "a+");

	/*fprintf(file, "progName: %s\n", progName);
	fprintf(file, "serverIp: %s\n", serverIp);
	fprintf(file, "serverPort: %d\n", serverPort);
	fprintf(file, "clientIp: %s\n", clientIp);
	fprintf(file, "clientPort: %d\n", clientPort);
	fprintf(file, "pid: %d\n", pid);
	fprintf(file, "pname: %s\n", pname);*/

	/*
		create socket
		*/

	mySocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (-1 == mySocket){
		return 0;
	}

	/*
		bind clientIp and clientPort to mySocket
		*/
	int reuse = 1;
	setsockopt(mySocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse));
	struct sockaddr_in client_addr;
	bzero((char*)&client_addr, sizeof(client_addr));
	client_addr.sin_addr.s_addr = inet_addr(clientIp);
	client_addr.sin_family = AF_INET;
	client_addr.sin_port = htons(clientPort);
	int bindResult = bind(mySocket, (struct sockaddr*) &client_addr, sizeof(client_addr));
	if (-1 == bindResult){
		return 0;
	}

	/*
		connect to server, if failed, retry
		*/
	struct sockaddr_in server_addr;
	bzero((char*)&server_addr, sizeof(server_addr));
	server_addr.sin_addr.s_addr = inet_addr(serverIp);
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(serverPort);
	struct sockaddr *srvAddr = (struct sockaddr*) &server_addr;
	int cnntResult;
	while (1){//if connect failed,retry
		cnntResult = connect(mySocket, srvAddr, sizeof(server_addr));
		if (cnntResult >= 0){
			break;
		}
		else{
			usleep(10 * 1000); //sleep for 10ms
		}
	}
	fprintf(file, "connect ok, time: %d\n", cnntResult);

	/* 注册pid和pname */
	ssize_t sendSize;
	unsigned char msg[SEND_LENGTH];
	memset(msg, '\0', SEND_LENGTH);
	// !!!need notify
	sprintf(msg, "reg: %d %s need_notify \n", pid, pname);
	sendSize = send(mySocket, msg, strlen(msg) + 1, 0);
	fprintf(file, "reg ok : %s\n", msg);
	/* 每次接收RECV_LENGTH字节，并且超过CHECK_LENGTH时检查是否有协议 */
	int nLength, code, nowLen;
	while (1){
		nLength = recv(mySocket, totalBuf + totalBufLen, RECV_LENGTH, 0);
		if (nLength > 0){
			totalBufLen += nLength;
			if (totalBufLen > CHECK_LENGTH){
				totalBuf[totalBufLen] = '\0';
				code = getInfoFromBuffer();
				if (GAMEOVER == code){//game-over！！！
					fprintf(file, "---------\ngame over!\n");
					break;
				}
			}

		}
	}

	/* 释放资源 */
	fclose(file);
	close(mySocket);

	return 0;
}
