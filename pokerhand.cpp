#include "pokerhand.h"


#define SUIT(cardval) (cardval % 4)
#define RANK(cardval) ((cardval-1) / 4)

bool is_royal(int* hand)
{
    for (int i=0; i<5; i++) {
        if (hand[i] > 20)
            return false;  // contains card lower than 10
        if (SUIT(hand[i]) != SUIT(hand[0]))
            return false;  // contains mismatched suits
    }
    return true;
}

bool is_four(int* rankcount)
{
    for (int i=0; i<13; i++)
        if (rankcount[i] == 4)
            return true;
    return false;
}

bool is_full(int* rankcount)
{
    bool found_3 = false;
    bool found_2 = false;
    for (int i=0; i<13; i++) {
        if (rankcount[i] == 3)
            found_3 = true;
        else if (rankcount[i] == 2)
            found_2 = true;
    }
    return found_3 && found_2;
}

bool is_straight(int* rankcount)
{
    int lowest = -1;
    int highest;
    //skipping over ace
    for (int i=1; i< 13; i++) {
        if (rankcount[i] == 1) {
            if (lowest == -1)
                lowest = i;
            highest = i;
        }
        if (rankcount[i] > 1)
            return false;
    }
    if (rankcount[0] > 1)
        return false;
    // we know every rank is 0 or 1
    // middle straight or we allow ACE (0) at bottom or top
    if ((highest - lowest == 4 && rankcount[0] == 0) ||
            (highest == 4) || (lowest == 9))
        return true;
    return false;
}
bool is_flush(int* hand)
{
    for (int i =1; i< 5; i++)
        if (SUIT(hand[i]) != SUIT(hand[0]))
            return false;
    return true;
}
bool is_straight_flush(int* hand, int* rankcount)
{
    return is_straight(rankcount) && is_flush(hand);
}

bool is_three(int* rankcount)
{
    for (int i=0; i< 13; i++)
        if (rankcount[i] == 3)
            return true;
    return false;
}

bool is_TwoPair(int* rankcount)
{
    int count = 0;
    for (int i=0; i< 13; i++)
        if (rankcount[i] == 2)
            count++;
    return count > 1;
}
bool is_Pair(int* rankcount)
{
    // note only Jacks or better
    for (int i=0; i< 4; i++)
        if (rankcount[i] == 2)
            return true;
    return false;
}

HandRank EvaluateHand(int hand[5])
{    
    int rankcount[13]= {0};
    for (int i=0; i< 5; i++)
        rankcount[RANK(hand[i])]++;

    if (is_royal(hand))
        return eRoyal;
    if (is_straight_flush(hand, rankcount))
        return eStraightFlush;
    if (is_four(rankcount))
        return eFour;
    if (is_full(rankcount))
        return eFull;
    if (is_flush(hand))
        return eFlush;
    if (is_straight(rankcount))
        return eStraight;
    if (is_three(rankcount))
        return eThree;
    if (is_TwoPair(rankcount))
        return eTwoPair;
    if (is_Pair(rankcount))
        return ePair;

    return eBust;
}
int BasePayout(HandRank rank)
{
    switch(rank)
    {
    case ePair : return 1;
    case eTwoPair : return 2;
    case eThree : return 3;
    case eStraight : return 4;
    case eFlush : return 6;
    case eFull : return 9;
    case eFour : return 25;
    case eStraightFlush : return 50;
    case eRoyal : return 250;
    }
    return 0;
}

const char* RankToText(HandRank rank)
{
    switch(rank)
    {
    case ePair : return "Jacks or better";
    case eTwoPair : return "two pair";
    case eThree : return "three of a kind";
    case eStraight : return "straight";
    case eFlush : return "flush";
    case eFull : return "FULL HOUSE";
    case eFour : return "FOUR OF A KIND";
    case eStraightFlush : return "STRAIGHT FLUSH";
    case eRoyal : return "ROYAL FLUSH";
    }
    return 0;
}
