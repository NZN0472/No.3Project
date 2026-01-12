#pragma once
#include <vector>
#include <string>
#include <random>
#include <functional>
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

enum class CheatMode { None, BetTotal };

struct BJParticipant {
    std::string name;
    bool isHuman = false;

    int chips = 1000;
    int bet = 0;
    bool doubled = false;
    bool stood = false;
    BJHand hand;

    // ===== イカサマ関連（仕様更新）=====
    CheatMode cheatMode = CheatMode::None; // None / BetTotal 
    bool cheatedThisRound = false;         // 1R1回まで（BetTotalなら開始時にtrue）
    int  cheatBetTarget = 21;              // BetTotal用（17..21）

    // ===== 指摘関連 =====
    bool accusedThisRound = false;
    bool falseAccused = false;
    bool caughtCheating = false;

    int baseAccuseDenom = 8;

    // ===== CPU思考用=====
    int aiStandThreshold = 17;
    int aiDoubleMin = 9;
    int aiDoubleMax = 11;

    

    // ベットでOFFだった場合、行動で強制的にイカサマ選択させる
    bool mustCheatLater = false;

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
        Accuse,
        DealerTurn,
        Settle,
        RoundEnd
    };



    static constexpr int kStartChips = 1000;
    static constexpr int kMinBet = 10;
    static constexpr int kBetStep = 10;

    // ラウンド制
    static constexpr int kMaxRounds = 5;

    // 借金OKでもUI上限
    static constexpr int kMaxUserBet = 5000;

    State state = State::Betting;
    BJDeck deck;

    BJParticipant dealer;
    std::vector<BJParticipant> players;

 
    int activeCpuIndex = 1;

    int uiPlayerBet = 100;
    std::string lastMessage;

    // 現在ラウンド（1..5）
    int roundNo = 0;

    // 5ラウンド終わったか
    bool matchOver = false;

    // ====== ボタン（クリックUI） ======
    Button btnBetMinus100{ 0,0,0,0 };
    Button btnBetMinus50{ 0,0,0,0 };
    Button btnBetMinus10{ 0,0,0,0 };

    Button btnBetPlus10{ 0,0,0,0 };
    Button btnBetPlus50{ 0,0,0,0 };
    Button btnBetPlus100{ 0,0,0,0 };

    Button btnBetOK{ 0,0,0,0 };

    Button btnHit{ 0,0,0,0 };
    Button btnStand{ 0,0,0,0 };
    Button btnDouble{ 0,0,0,0 };

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

    void layoutButtons();

private:
    // ===== 手番順ローテ =====
    int turnOrder[4] = { 0,1,2,3 }; // 今ラウンドの行動順（players index）
    int turnPos = 0;               // いま誰の番か（0..3）

    void setupTurnOrderForRound(); // roundNo に応じて作る
    int  currentActorIndex() const { return turnOrder[turnPos]; }
    void advanceActor();           // 次の人へ（全員終わったら Accuse へ）

    // ===== 指摘保証 =====
    int guarantee[4] = { 0,1,2,3 };  // 試合開始時にシャッフル
    void buildGuaranteeTargets();    // guarantee を作る（試合開始/再開時）
    void assignBaseAccuseProbs();    // {2,4,8,8} を各人に配る（毎ラウンド）
    void runAccusePhase();           // 指摘（保証＋確率抽選＋再抽選）

    // ===== 指摘確率（ベース＋隠し追加） =====
    float baseAccuseProb(const BJParticipant& p) const;      // 例: 1.0f / p.baseAccuseDenom
    float hiddenAccuseAdd(const BJParticipant& p) const;     // あなたの追加確率ルール
    float effectiveAccuseProb(const BJParticipant& p) const; // clampした最終確率

    // ===== 配当 =====
    double winMultiplier(const BJParticipant& p) const;        // BJ50 / Bust10 / 僅差式 / push1 / lose0
    double probBonusMultiplier(const BJParticipant& p) const;  // 1/2→3, 1/4→2, 1/8→1

    // ===== UI：ベット時イカサマ =====
    CheatMode uiCheatMode = CheatMode::None; // None / BetTotal / DrawTo21
    int uiCheatBetTarget = 21;               // BetTotal用（17..21）

    Button btnCheatToggle{ 0,0,0,0 };
    Button btnCheatMinus{ 0,0,0,0 };
    Button btnCheatPlus{ 0,0,0,0 };

    // ===== UI：行動時イカサマ（4～21） =====
   

    static constexpr float kActInterval = 1.0f; // 1秒

    float cpuWait = 0.0f;
    int   lastCpuIdx = -1;

    float dealerWait = 0.0f;

    // =====表示用（ターン/待ち/ログ）=====
    
    // ===== Dealer穴札オープン演出 =====
    bool  dealerHoleRevealed = false;   // 穴札がオープン済みか
    float dealerRevealTimer = 0.0f;     // オープン待ちタイマー
    

  

    struct RenderCtx {
        float FS = 1.0f;
        float FS_S = 0.9f;
        float labelYBet = 18.0f;
        float labelYCheat = 26.0f;

        std::function<void(const std::string&, float, float, float, float, float)> textL;
        std::function<float(const std::string&, float, float)> measureW;

        std::function<void(Button&, bool)> drawBtn;
        std::function<void(Button&, const std::string&, float, float, float, bool)> drawBtnTextCenter;
    };

    void drawBetUI(const RenderCtx& ctx);
    void drawActionUI(const RenderCtx& ctx);
    bool drawRoundEndUI(const RenderCtx& ctx); // matchOver時は true 返して render() をreturnさせる
    void drawDealerUI(const RenderCtx& ctx);
    void drawPlayersUI(const RenderCtx& ctx);
    void drawTopUI(const RenderCtx& ctx);
    void drawTitleUI(const RenderCtx& ctx);


};
