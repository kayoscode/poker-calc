#include "Poker.h"
#include <string.h>

using std::max;
using std::min;

namespace sim 
{
    enum ClassTested : uint8_t
    {
        Fail,
        Pass
    };

    static inline void SetBitFieldsAndAddCard(card_t card, tClassificationData& data)
    {
        data.AllCards[card] = 1;
        
        int rank = static_cast<int>(Card::GetRank(card));
        int suit = static_cast<int>(Card::GetSuit(card));

        data.RankBitFields[rank] |= 1 << suit;
        data.SuitBitFields[suit] |= 1 << rank;

        data.RankBitField |= 1 << rank;
        
        data.RankCardCount[rank]++;
        data.SuitCardCount[suit]++;
        
        if (card > data.HighestCard)
        {
            data.HighestCard = card;
        }
        
        if (card < data.LowestCard)
        {
            data.LowestCard = card;
        }
    }

    static void ProcessHand(card_t* cards, tClassificationData& data, 
                              ClassTested* classUsed);

    void HandClassification::ClassifyAllCards(card_t hand1, card_t hand2,
                                              card_t flop1, card_t flop2, card_t flop3,
                                              card_t turn, card_t river)
    {
        // Clear the bit fields.
        mAllCardsClassData.RankBitField = 0;
        mAllCardsClassData.HighestCard = hand1;
        mAllCardsClassData.LowestCard = hand1;

        memset(mAllCardsClassData.RankBitFields, 0, sizeof(mAllCardsClassData.RankBitFields));
        memset(mAllCardsClassData.SuitBitFields, 0, sizeof(mAllCardsClassData.SuitBitFields));
        
        // Clear the card data.
        memset(mAllCardsClassData.AllCards, 0, sizeof(mAllCardsClassData.AllCards));
        memset(mAllCardsClassData.RankCardCount, 0, sizeof(mAllCardsClassData.RankCardCount));
        
        ClassTested classTested[(int)HandClass::MaxClassification] = { Fail };
        classTested[(int)HandClass::HighCard] = Pass;
        mAllCardsClassData.HighCard.HighRank = Card::GetRank(mAllCardsClassData.HighestCard);

        SetBitFieldsAndAddCard(hand1, mAllCardsClassData);
        SetBitFieldsAndAddCard(hand2, mAllCardsClassData);
        SetBitFieldsAndAddCard(flop1, mAllCardsClassData);
        SetBitFieldsAndAddCard(flop2, mAllCardsClassData);
        SetBitFieldsAndAddCard(flop3, mAllCardsClassData);
        SetBitFieldsAndAddCard(turn, mAllCardsClassData);
        SetBitFieldsAndAddCard(river, mAllCardsClassData);
        
        ProcessHand(mAllCardsClassData.AllCards, mAllCardsClassData, classTested);
    }

    static void ProcessHand(card_t* cards, tClassificationData& data, ClassTested* classTested)
    {
        int currentRankStreak = 0;

        for (int i = static_cast<int>(Card::GetRank(data.HighestCard)); 
             i >= static_cast<int>(Card::GetRank(data.LowestCard)); i--)
        {
            // Create straight bit field (to be used later).
            // Init to max value to ensure remove the need for another boolean checking if we have
            // the possibility for a straight.
            uint32_t straightBitField = std::numeric_limits<uint32_t>::max();

            if (i >= 4)
            {
                if (i >= 5)
                {
                    straightBitField = 0b11111 << (i - 5);
                }
                else {
                    // Include the ace bit since it can also count as zero.
                    straightBitField = 0b1111000000001;
                }
            }

            // Check for a standard straight at this rank.
            if ((data.RankBitField & straightBitField) == straightBitField &&
                 classTested[(int)HandClass::Straight] == Fail)
            {
                data.Straight.HighRank = static_cast<Rank>(i);
                classTested[(int)HandClass::Straight] = Pass;
            }
            
            // Check for straight flush and royal flush.
            for (int j = 0; j < static_cast<int>(Suit::MaxSuit); j++)
            {
                bool passedBitFieldTest = (data.SuitBitFields[j] & straightBitField) == straightBitField;

                if (passedBitFieldTest && i == static_cast<int>(Rank::Ace))
                {
                    classTested[(int)HandClass::RoyalFlush] = Pass;
                }

                if (passedBitFieldTest && classTested[(int)HandClass::StraightFlush] == Fail)
                {
                    classTested[(int)HandClass::StraightFlush] = Pass;
                    classTested[(int)HandClass::Straight] = Pass;
                    classTested[(int)HandClass::Flush] = Pass;

                    data.Straight.HighRank = static_cast<Rank>(i);
                    data.Flush.FlushSuit = static_cast<Suit>(j);
                }
            }
            
            uint32_t numCardsWithRank = data.RankCardCount[i];
            
            // If we got counts higher than 1, record here.
            if (numCardsWithRank == 2 && classTested[(int)HandClass::OnePair == Fail])
            {
                data.OnePair.HighRank = static_cast<Rank>(i);
                classTested[(int)HandClass::OnePair] = Pass;
            }
            else if (numCardsWithRank == 2 && classTested[(int)HandClass::TwoPair] == Fail)
            {
                data.TwoPair.SecondPairRank = static_cast<Rank>(i);
                classTested[(int)HandClass::TwoPair] = Pass;
            }
            else if (numCardsWithRank == 3 && classTested[(int)HandClass::ThreeOfAKind] == Fail)
            {
                data.ThreeOfAKind.HighRank = static_cast<Rank>(i);
                classTested[(int)HandClass::ThreeOfAKind] = Pass;
            }
            else if (numCardsWithRank == 4 && classTested[(int)HandClass::FourOfAKind] == Fail)
            {
                data.FourOfAKind.HighRank = static_cast<Rank>(i);
                classTested[(int)HandClass::FourOfAKind] = Pass;
            }
        }
        
        // Set flush data.
        for (int i = 0; i < static_cast<int>(Suit::MaxSuit); i++)
        {
            if (data.SuitCardCount[i] >= 5)
            {
                classTested[(int)HandClass::Flush] = Pass;
                data.Flush.FlushSuit = static_cast<Suit>(i);
            }
        }
        
        // Set full house data.
        if (classTested[(int)HandClass::OnePair] == Pass &&
            classTested[(int)HandClass::ThreeOfAKind] == Pass)
        {
            classTested[(int)HandClass::FullHouse] = Pass;
        }
    }
}