#ifndef POKERHAND_H
#define POKERHAND_H

enum HandRank { eBust, ePair, eTwoPair, eThree, eStraight, eFlush, eFull, eFour, eStraightFlush, eRoyal };
HandRank EvaluateHand(int hand[5]);
int BasePayout(HandRank rank);
const char* RankToText(HandRank rank);

#endif // POKERHAND_H
