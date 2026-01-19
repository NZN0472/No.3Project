#pragma once
#include <vector>
#include <string>
#include <random>
#include <functional>
#include "Button.h"
#include "Fade.h"

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
        BaseProbSwap,
        PlayerTurn,
        CpuTurn, 
        Accuse,
        DealerTurn,
        Settle,
        RoundEnd,
        FinalResult,
        TutorialEnd,
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
    Button btnTutSkip{ 0,0,0,0 }; 

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

    bool canBetDelta(int d) const;

    void applyBetDelta(int d);

    void setMsg(const std::string& msg);
    void drawRankImage(int rank, int suit, float x, float y, float size);
    void drawSuitImage(int suit, float x, float y, float size);
    void drawCardFaceImage(const BJCard& c, float x, float y);
    void drawCardBackImage(float x, float y);

    
    

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
        std::function<void(const std::string&, float, float, float, float, float, float, float, float)> textC;

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
    void drawTutorialSkipUI(const RenderCtx& ctx);


    // ===== 基礎確率 交換UI =====
    Button btnSwapCpu1{ 0,0,0,0 };
    Button btnSwapCpu2{ 0,0,0,0 };
    Button btnSwapCpu3{ 0,0,0,0 };
    Button btnSwapSkip{ 0,0,0,0 };

    void toBaseProbSwap();
    void swapBaseDenom(int a, int b);
    void drawBaseProbSwapUI(const RenderCtx& ctx);

    // ===== 指摘の「順番演出」用 =====
    bool  accuseRunning = false;
    int   accuseStep = 0;          // 0..4（何人目まで表示したか）
    float accuseWait = 0.0f;       // 1秒待ち
    bool  accuseRevealed[4] = { false,false,false,false };

    void prepareAccuseResults();   // 結果だけ先に決める（表示は順番に）

    bool pendingFinalResult = false;

    private:
        struct UiLayout
        {
            // -------------------------
            // 共通（文字の左余白 / ボタン内文字Y）
            // -------------------------
            float padL = 14.0f;
            float btnTextYO = 18.0f;

            // RenderCtxへ渡すラベル位置
            float labelYBet = 18.0f;
            float labelYCheat = 26.0f;

            // -------------------------
            // 画面下の基準Y
            // -------------------------
            float btnY = 0.0f;

            // -------------------------
            // BETボタン列
            // -------------------------
            float betX0 = 60.0f;
            float betBW = 90.0f;
            float betBH = 70.0f;
            float betGap = 12.0f;
            float betOkW = 120.0f;

            // -------------------------
            // Actionボタン
            // -------------------------
            float actXHit = 820.0f;
            float actXStand = 960.0f;
            float actXDouble = 1100.0f;
            float actBW = 120.0f;
            float actBH = 70.0f;

            // -------------------------
            // Titleボタン（左上）
            // -------------------------
            float titleX = 40.0f;
            float titleY = 70.0f;
            float titleW = 180.0f;
            float titleH = 70.0f;

            // -------------------------
            // CHEAT UI
            // -------------------------
            float cheatY = 0.0f;

            // -------------------------
            // SWAP UI
            // -------------------------
            float swapY = 0.0f;
            float swapTitleX = 40.0f;   // タイトルのX
            float swapTitleYOff = -28.0f;  // btnSwapCpu1.getY() + off

            // -------------------------
            // Top UI（ROUND / MSG / TURN / NEXT）
            // -------------------------
            float topRoundX = 40.0f;
            float topRoundY = 20.0f;

            float msgX = 40.0f;
            float msgY = 580.0f;

            float topTurnX = 420.0f;
            float topTurnY = 20.0f;

            float topNextX = 420.0f;
            float topNextY = 50.0f;

            // -------------------------
            // Players UI（列配置）
            // -------------------------
            float plColX0 = 220.0f;
            float plColDx = 260.0f;
            float plColY0 = 200.0f;

            float plNameYOff = -40.0f;
            float plJudgeYOff = -18.0f;

            float plInfoPadY = 14.0f;   // カード列の下にINFOを置く余白
            float plInfoBottomGap = 130.0f;  // btnBetOK.getY() - gap を上限にする

            //----------------------
            // skip UI
            //----------------------
            float tutSkipW = 160.0f;
            float tutSkipH = 70.0f;
            float tutSkipPad = 20.0f; // 右上余白
        };

        UiLayout ui;
        void layoutButtons(); 
        bool swappedThisRound = false; // BaseProbSwapで1回選んだらtrue

        //チュートリアル
          enum class TutStep {
            NormalBJ = 0,     // 普通BJ 1R
            CheatCaught,      // チート→指摘される
            CheatPaid,        // チート→指摘されず賞金
            FalseAccuse,      // 冤罪
            Done
        };

        struct Tutorial {
            bool enabled = false;     // tutorial中か
            TutStep step = TutStep::NormalBJ;

            bool skipSwap = false;    // BaseProbSwapを飛ばすか
            bool skipAccuse = false;  // Accuseを飛ばすか

            bool lockCheatMode = false;   // チートモード固定
            CheatMode lockedMode = CheatMode::None;

            bool lockCheatTarget = false; // ターゲット固定
            int lockedTarget = 21;
        };

        Tutorial tut;

        void tutorialBeginIfRequested();
        void tutorialSkipOneRound();  
        bool tutorialBetOkEnabled() const; 

        bool tutorialActive = false;     // タイトルのONで開始
        int  tutorialStep = 0;           // 0..3（計4ラウンド）

        // 0:通常（Accuseなし）
        // 1:CHEAT CAUGHT
        // 2:CHEAT SAFE
        // 3:FALSE ACCUSE
        int  tutAccusePreset = 0;
        Fade fade;                 
        bool fadeNewGameReq = false;
        bool fadeFromTutorial = false;

        void startFadeNewGame(bool fromTutorial);
        void resetForRealMatch();  // チュートリアル終了後の本番開始リセット

};

