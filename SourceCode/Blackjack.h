#pragma once
#include <vector>
#include <string>
#include <random>
#include "Button.h"   

struct BJCard {
    int rank; // 1..13 (A..K)
    int suit; // 0..3
    int value() const { return (rank >= 10) ? 10 : rank; } // A=1
};

class BJDeck {
public:
    BJDeck();
    BJCard draw();

private:
    void rebuild();
    void shuffle();

    std::vector<BJCard> cards;
    std::mt19937 rng;
};

class BJHand {
public:
    void clear();
    void add(const BJCard& c);
    int bestScore() const;         // A=1/11で有利な方
    bool isBust() const;
    bool isBlackjack() const;      // 2枚で21
    int cardCount() const;
    const BJCard& cardAt(int i) const;

private:
    std::vector<BJCard> cards;
};

struct BJParticipant {
    std::string name;
    bool isHuman = false;

    int chips = 1000;
    int bet = 0;            // 現在ラウンドのベット額
    bool doubled = false;   // ダブルダウンしたか
    bool stood = false;     // スタンド済みか
    BJHand hand;
};

class BlackjackGame {
public:
    void init();
    void deinit();
    void update();
    void render();

    
private:
    enum class State {
        Betting,
        Dealing,
        PlayerTurn,
        CpuTurn,
        DealerTurn,
        Settle,
        RoundEnd
    };

    static constexpr int kStartChips = 1000;
    static constexpr int kMinBet = 10;
    static constexpr int kBetStep = 10;

    State state = State::Betting;
    BJDeck deck;

    BJParticipant dealer;
    std::vector<BJParticipant> players;
    int activeCpuIndex = 1;

    int uiPlayerBet = 100;
    std::string lastMessage;

    // ====== ボタン（クリックUI） ======
    Button btnBetMinus{ 80.0f, 560.0f, 120.0f, 80.0f };
    Button btnBetPlus{ 220.0f, 560.0f, 120.0f, 80.0f };
    Button btnBetOK{ 360.0f, 560.0f, 220.0f, 80.0f };

    Button btnHit{ 720.0f, 560.0f, 160.0f, 80.0f };
    Button btnStand{ 900.0f, 560.0f, 160.0f, 80.0f };
    Button btnDouble{ 1080.0f, 560.0f, 160.0f, 80.0f };

    Button btnToTitle{ 40.0f,  40.0f, 180.0f, 70.0f };
    
private:
    void beginRound();
    void toBetting();
    void toDealing();
    void toPlayerTurn();
    void toCpuTurn();
    void toDealerTurn();
    void toSettle();
    void toRoundEnd(const std::string& msg);

    bool canDoubleDown(const BJParticipant& p) const;

    void doHit(BJParticipant& p);
    void doStand(BJParticipant& p);
    void doDoubleDown(BJParticipant& p);

    void cpuAct(BJParticipant& cpu);
    void settleOne(BJParticipant& p);

    void setMsg(const std::string& msg);
    void drawRankImage(int rank, int suit, float x, float y, float size);
    void drawSuitImage(int suit, float x, float y, float size);

    void drawCardFaceImage(const BJCard& c, float x, float y);

    void drawCardBackImage(float x, float y);

};
