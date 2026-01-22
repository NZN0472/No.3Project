#include "Blackjack.h"
#include "../GameLib/game_lib.h"
#include "all.h"
#include "Timer.h"
#include <algorithm>
#include <random>
#include <sstream>
#include <cmath>
#include <unordered_set>
#include <array>

static constexpr int kTutorialSteps = 5;
static constexpr float kDealInterval = 0.35f;

//===============================================
//ポーズボタン
//=============================================== 

void BlackjackGame::layoutPauseMenuButtons()
{
    const float W = 500.0f;
    const float H = 200.0f;
    const float GAP = 20.0f;

    const float xL = 20.0f;
    const float xR = xL + W + GAP; // 540
    const float y0 = 20.0f;

    // 左列
    btnPReturn.setRect(xL, y0 + (H + GAP) * 0, W, H);
    btnPMult.setRect  (xL, y0 + (H + GAP) * 1, W, H);
    btnPWhatBJ.setRect(xL, y0 + (H + GAP) * 2, W, H);

    // 右列
    btnPCheat1.setRect(xR, y0 + (H + GAP) * 0, W, H);
    btnPCheat2.setRect(xR, y0 + (H + GAP) * 1, W, H);
    btnPCheat3.setRect(xR, y0 + (H + GAP) * 2, W, H);
    
    // 右上：TITLEボタン227x85
    const float TW = 227.0f;
    const float TH = 85.0f;
    btnPTitle.setRect((float)SCREEN_W - TW - 20.0f, 20.0f, TW, TH);
}

//===============================================
// PauseInfo（説明ページ中）: 左上「戻る」だけ
//===============================================
void BlackjackGame::layoutPauseInfoButtons()
{
    const float W = 500.0f/2;
    const float H = 200.0f/2;
    const float X = 20.0f;
    const float Y = 20.0f;

    btnPBack.setRect(X, Y, W, H);
}



//================================================
// ボタン配置
//================================================
void BlackjackGame::layoutButtons()
{
    // 画面下に揃える
    ui.btnY = (float)SCREEN_H - 100.0f;

    const float BET_BW = 90.0f;
    const float BET_BH = 70.0f;
    const float BET_OKW = 120.0f;
    const float BET_GAP = ui.betGap;   

    // ---- BET列 ----
    float x = ui.betX0;

    btnBetMinus100.setRect(x, ui.btnY, BET_BW, BET_BH); x += BET_BW + BET_GAP;
    btnBetMinus50.setRect(x, ui.btnY, BET_BW, BET_BH); x += BET_BW + BET_GAP;
    btnBetMinus10.setRect(x, ui.btnY, BET_BW, BET_BH); x += BET_BW + BET_GAP;

    btnBetOK.setRect(x, ui.btnY, BET_OKW, BET_BH); x += BET_OKW + BET_GAP;

    btnBetPlus10.setRect(x, ui.btnY, BET_BW, BET_BH); x += BET_BW + BET_GAP;
    btnBetPlus50.setRect(x, ui.btnY, BET_BW, BET_BH); x += BET_BW + BET_GAP;
    btnBetPlus100.setRect(x, ui.btnY, BET_BW, BET_BH);

    // ---- Action（120x70）----
    const float ACT_BW = 120.0f, ACT_BH = 70.0f;
    btnHit.setRect(ui.actXHit, ui.btnY, ACT_BW, ACT_BH);
    btnStand.setRect(ui.actXStand, ui.btnY, ACT_BW, ACT_BH);
    btnDouble.setRect(ui.actXDouble, ui.btnY, ACT_BW, ACT_BH);

    // ---- Title ----
    btnPause.setRect(ui.titleX, ui.titleY, ui.titleW, ui.titleH); //PAUSE

    // ---- CHEAT（300x80 と 80x80）----
    ui.cheatY = ui.btnY - 90.0f;
    btnCheatToggle.setRect(40.0f, ui.cheatY, 300.0f, 80.0f);
    btnCheatMinus.setRect(350.0f, ui.cheatY, 80.0f, 80.0f);
    btnCheatPlus.setRect(440.0f, ui.cheatY, 80.0f, 80.0f);

    // ---- SWAP（160x70）----
    ui.swapY = ui.cheatY - 60.0f;
    btnSwapCpu1.setRect(40.0f, ui.swapY, 160.0f, 70.0f);
    btnSwapCpu2.setRect(210.0f, ui.swapY, 160.0f, 70.0f);
    btnSwapCpu3.setRect(380.0f, ui.swapY, 160.0f, 70.0f);
    btnSwapSkip.setRect(550.0f, ui.swapY, 160.0f, 70.0f);

    // Tutorial Skip（右上）
    float sx = (float)SCREEN_W - ui.tutSkipW - ui.tutSkipPad;
    float sy = ui.tutSkipPad;
    btnTutSkip.setRect(SCREEN_W - 180.0f, 20.0f, 160.0f, 70.0f);
}

//================================================
// 切り抜き設定
//================================================
static constexpr float SHEET = 64.0f;   // 画像全体
static constexpr float TILE = 32.0f;   // 32x32 を切り抜く（2x2）


static bool isRedSuit(int suit) {
    // ♦(1), ♥(2) を赤
    return (suit == 1 || suit == 2);
}

//================================================
// BET 計算ユーティリティ
//================================================
static int roundDownToStep(int v, int step)
{
    if (step <= 0) return v;
    return (v / step) * step;
}

// 借金OKなので、ベット額は「最低額以上」「10刻み」にするだけ
static int normalizeBet(int bet, int minBet, int step, int maxBet)
{
    if (bet < minBet) bet = minBet;
    if (bet > maxBet) bet = maxBet;
    bet = roundDownToStep(bet, step);
    if (bet < minBet) bet = minBet;
    return bet;
}

// CPU：所持額に応じて最大%を変えて、10%刻みでランダム
// 例）chips=1000 なら 10〜60% のどれか → 100〜600 を10刻み
static int cpuRandomBet(int chips, int minBet, int step)
{
    static std::mt19937 rng{ std::random_device{}() };

    // 所持がマイナスなら「0扱い」で最低額だけ賭ける（借金状態でも暴走しない）
    int bankroll = (chips > 0) ? chips : 0;

    int maxPercent = 60;
    if (bankroll < 200)      maxPercent = 30; // 10/20/30
    else if (bankroll < 500) maxPercent = 40; // 10..40
    else if (bankroll < 1000)maxPercent = 50; // 10..50
    else                     maxPercent = 60; // 10..60

    std::uniform_int_distribution<int> dist(1, maxPercent / 10);
    int percent = dist(rng) * 10;

    int bet = (bankroll * percent) / 100;
    if (bet < minBet) bet = minBet;

    bet = roundDownToStep(bet, step);
    if (bet < minBet) bet = minBet;
    //5000上限
    const int kMaxCpuBet = 5000;
    if (bet > kMaxCpuBet) bet = kMaxCpuBet;
    return bet;
}

// ゲーム用乱数
static std::mt19937& gameRng() {
    static std::mt19937 rng{ std::random_device{}() };
    return rng;
}
static int randInt(int lo, int hi) {
    std::uniform_int_distribution<int> d(lo, hi);
    return d(gameRng());
}

// value(1..10) に対応する BJCard を作る（10は10/J/Q/Kのどれか）
static BJCard makeCardByValue(int v) {
    int suit = randInt(0, 3);
    int rank = 1;
    if (v == 1) rank = 1;
    else if (2 <= v && v <= 9) rank = v;
    else {
        // 10/J/Q/K
        rank = randInt(10, 13);
    }
    return BJCard{ rank, suit };
}




// hand を「合計 total」になるように2枚で作り直す（簡易版）
static void rigHandToTotal(BJHand& hand, int total) {
    hand.clear();

    if (total < 4) total = 4;
    if (total > 21) total = 21;

    if (total == 21) {
        hand.add(BJCard{ 1, randInt(0,3) });      // A
        hand.add(makeCardByValue(10));            // 10/J/Q/K
        return;
    }

    int v1 = 2;
    int v2 = total - 2;

    if (v2 > 10) {
        v1 = 10;
        v2 = total - 10; // 2..10 になる想定（12..20）
    }

    if (v2 < 1) v2 = 1;
    if (v2 > 10) v2 = 10;

    hand.add(makeCardByValue(v1));
    hand.add(makeCardByValue(v2));
}


// 64x64の中から (col,row) の32x32を切り抜いて描画
// size = 画面上の1タイルの表示サイズ（32なら等倍、64なら2倍表示）
static void drawSheet_2x2_32(Sprite* sheet, int col, int row, float x, float y, float size) {
    if (!sheet) return;

    const float tx = col * TILE;
    const float ty = row * TILE;
    const float scale = size / TILE;

    sprite_render(sheet,
        x, y, scale, scale,     // 位置 / スケール（tw,thの代わりにスケールで調整）
        tx, ty, TILE, TILE,     // 元画像の切り抜き（32x32）
        0.0f, 0.0f, 0.0f,       // 基準点 / 角度
        1, 1, 1, 1,             // 色
        false                   // スクリーン座標
    );
}



static constexpr float kBtnAlphaEnabled = 1.0f;
static constexpr float kBtnAlphaDisabled = 0.18f; // ←ここを好きに調整（0.05〜0.35 くらいが見やすい）
static constexpr float kBtnAlphaLocked = 0.10f; // さらに「ロック中」を薄くしたい時用（任意）

static void drawBtnImageFit(Sprite* spr, const Button& b,
    float srcW, float srcH,
    bool enabled,
    float alphaDisabled = kBtnAlphaDisabled)   // 引数で上書きもできる
{
    if (!spr) return;

    const float a = enabled ? kBtnAlphaEnabled : alphaDisabled;
    const float sx = b.getW() / srcW;
    const float sy = b.getH() / srcH;

    sprite_render(spr,
        b.getX(), b.getY(), sx, sy,
        0, 0, srcW, srcH,
        0, 0, 0,
        1, 1, 1, a,
        false
    );
}

bool BlackjackGame::canBetDelta(int d) const
{
    int next = normalizeBet(uiPlayerBet + d, kMinBet, kBetStep, kMaxUserBet);
    return next != uiPlayerBet;
}

void BlackjackGame::applyBetDelta(int d)
{
    int before = uiPlayerBet;
    uiPlayerBet = normalizeBet(uiPlayerBet + d, kMinBet, kBetStep, kMaxUserBet);

    if (uiPlayerBet != before) {
        AudioManager::PlayGameSE(CHIPS);  
    }
}


Sprite* BlackjackGame::getTutorialIntroSprite() const
{
    // 1/5の前に0.png, 2/5の前に1.png, 3/5の前に2.png, 4/5の前に3.png
    switch (tutorialStep) {
    case 0: return assets.spr0;
    case 1: return assets.spr1;
    case 2: return assets.spr2;
    case 3: return assets.spr3;
    default: return nullptr; // 5/5は出さない
    }
}
void BlackjackGame::layoutTutorialIntroButtons()
{
    const float W = 160.0f;
    const float H = 70.0f;
    btnBetOK.setRect((float)SCREEN_W - W - 70.0f, (float)SCREEN_H - H - 80.0f, W, H);
}


void BlackjackGame::openPause()
{
    if (state == State::PauseMenu || state == State::PauseInfo) return;
    if (fade.IsFading() || fadeNewGameReq) return; // フェード中は触らない

    stateBeforePause = state;
    pausePage = PausePage::None;
    state = State::PauseMenu;
    setMsg("PAUSE");
}

void BlackjackGame::resumeFromPause()
{
    pausePage = PausePage::None;
    state = stateBeforePause;
    setMsg("RESUME");
}


//================================================
// BJDeck
//================================================
BJDeck::BJDeck() {
    std::random_device rd;
    rng.seed(rd());
    rebuild();
}

void BJDeck::rebuild() {
    cards.clear();
    cards.reserve(52);
    for (int s = 0; s < 4; ++s) {
        for (int r = 1; r <= 13; ++r) {
            cards.push_back(BJCard{ r, s });
        }
    }
    shuffle();
}

void BJDeck::shuffle() {
    std::shuffle(cards.begin(), cards.end(), rng);
}

BJCard BJDeck::draw() {
    if (cards.size() < 15) rebuild();
    BJCard c = cards.back();
    cards.pop_back();
    return c;
}

//================================================
// BJHand
//================================================
void BJHand::clear() { cards.clear(); }
void BJHand::add(const BJCard& c) { cards.push_back(c); }
int BJHand::cardCount() const { return (int)cards.size(); }

int BJHand::bestScore() const {
    int sum = 0;
    int ace = 0;

    for (auto& c : cards) {
        sum += c.value();
        if (c.rank == 1) ace++;
    }

    while (ace > 0 && sum + 10 <= 21) {
        sum += 10;
        ace--;
    }
    return sum;
}

const BJCard& BJHand::cardAt(int i) const { return cards[i]; }
bool BJHand::isBust() const { return bestScore() > 21; }
bool BJHand::isBlackjack() const{return cards.size() >= 2 && bestScore() == 21;}
//================================================
// BlackjackGame
//================================================
void BlackjackGame::setMsg(const std::string& msg) {
    lastMessage = msg;
}

//基礎確率
static std::string pct1(float p) {
    int v = (int)std::lround(p * 1000.0f); // 0.1%単位
    int whole = v / 10;
    int frac = std::abs(v % 10);
    return std::to_string(whole) + "." + std::to_string(frac) + "%";
}


void BlackjackGame::init() {
    dealer.name = "Dealer";
    dealer.isHuman = false;
    dealer.chips = 0;
    resultSePlayed = false;
    players.clear();
    players.resize(4);

    players[0].name = "YOU";  players[0].isHuman = true;
    players[1].name = "CPU1";
    players[2].name = "CPU2";
    players[3].name = "CPU3";

    for (auto& p : players) {
        p.chips = kStartChips;
        p.bet = 0;
        p.doubled = false;
        p.stood = false;
        p.hand.clear();
    }

    uiPlayerBet = 100;

    assets.load();
    roundNo = 0;
    matchOver = false;

    buildGuaranteeTargets(); // 試合開始時の保証ターゲットを作る

    tutorialBeginIfRequested();

    toBetting();
    // ---- ＋/−系はButton側のクリック音を消す（CHIPSに統一するため）----
    btnBetMinus100.setClickSeEnabled(false);
    btnBetMinus50.setClickSeEnabled(false);
    btnBetMinus10.setClickSeEnabled(false);
    btnBetPlus10.setClickSeEnabled(false);
    btnBetPlus50.setClickSeEnabled(false);
    btnBetPlus100.setClickSeEnabled(false);
}

void BlackjackGame::deinit() {
    assets.unload();
    
}
static void applySwapDemeritIfCheating(BJParticipant& you)
{
    // 「チートしている時だけ」デメリット
    const bool cheating = (you.cheatMode != CheatMode::None) || you.cheatedThisRound;
    if (!cheating) return;

    const int denom = you.baseAccuseDenom;
    const int s = you.hand.bestScore();

    // 1/2 のときはそのまま
    if (denom == 2) return;

    // 1/4 のときは 17〜19 に（ただし今が19以下なら触らない）
    if (denom == 4) {
        if (s > 19) {
            int t = randInt(17, 19);
            rigHandToTotal(you.hand, t);
        }
        return;
    }

    // 1/8 のときは 16以下に（ただし今が16以下なら触らない）
    if (denom == 8) {
        if (s > 16) {
            int t = randInt(12, 16); 
            rigHandToTotal(you.hand, t);
        }
        return;
    }

   
}


void BlackjackGame::tutorialBeginIfRequested()
{
    extern bool gStartTutorial;

    tutorialActive = gStartTutorial;
    if (!tutorialActive) return;

    gStartTutorial = false;
    tutorialStep = 0;
    tutAccusePreset = 0;

    roundNo = 0;
    matchOver = false;
    
    for (auto& p : players) p.chips = kStartChips;

    uiPlayerBet = 100;
    uiCheatMode = CheatMode::None;
    uiCheatBetTarget = 21;

   
    state = State::TutorialIntro;     
    setMsg("TUTORIAL INTRO");
}


void BlackjackGame::tutorialSkipOneRound()
{
    if (!tutorialActive) return;

    tutorialStep++;
    tutAccusePreset = 0;

    if (tutorialStep >= kTutorialSteps) {
        tutorialActive = false;
        state = State::TutorialEnd;
        if (!resultSePlayed) {
            AudioManager::PlayGameSE(RESULT);
            resultSePlayed = true;
        }
        setMsg("TUTORIAL DONE");
        return;
    }

    // ---- スキップ時は「今のラウンドの支払い等」を無かったことにして安全に次へ ----
    dealer.hand.clear();
    dealer.bet = 0;
    dealer.doubled = false;
    dealer.stood = false;

    for (auto& p : players) {
        p.hand.clear();
        p.bet = 0;
        p.doubled = false;
        p.stood = false;
        p.cheatedThisRound = false;
        p.accusedThisRound = false;
        p.falseAccused = false;
        p.caughtCheating = false;

        p.chips = kStartChips; // チュートリアルは各回リセット
    }

    uiPlayerBet = 100;
    uiCheatMode = CheatMode::None;
    uiCheatBetTarget = 21;

    toBetting();
    setMsg("TUTORIAL SKIP -> " + std::to_string(tutorialStep + 1) + "/" + std::to_string(kTutorialSteps));
}


void BlackjackGame::startFadeNewGame(bool fromTutorial)
{
    if (fade.IsFading()) return;
    fadeNewGameReq = true;
    fadeFromTutorial = fromTutorial;
    resultSePlayed = false;
    fade.StartFadeOut(0.6f); 
}

void BlackjackGame::resetForRealMatch()
{
    // チュートリアル状態を終了
    tutorialActive = false;
    tutorialStep = 0;
    tutAccusePreset = 0;

    // 5R本番スタート状態にする
    roundNo = 0;
    matchOver = false;
   
    // chips初期化
    for (auto& p : players) {
        p.chips = kStartChips;
        p.bet = 0;
        p.doubled = false;
        p.stood = false;
        p.hand.clear();

        p.cheatedThisRound = false;
        p.accusedThisRound = false;
        p.falseAccused = false;
        p.caughtCheating = false;
        p.mustCheatLater = false;
    }

    dealer.hand.clear();
    dealer.bet = 0;
    dealer.doubled = false;
    dealer.stood = false;

    uiPlayerBet = 100;
    uiCheatMode = CheatMode::None;
    uiCheatBetTarget = 21;

    buildGuaranteeTargets();

    toBetting(); // state=Betting にする
    setMsg("START REAL MATCH");
    resultSePlayed = false;
}

void BlackjackGame::buildGuaranteeTargets()
{
    std::vector<int> ids = { 0,1,2,3 };
    std::shuffle(ids.begin(), ids.end(), gameRng());
    for (int i = 0; i < 4; ++i) guarantee[i] = ids[i];
}

void BlackjackGame::setupTurnOrderForRound()
{
    // roundNo: 1..5
    int start = (roundNo - 1) % 4;
    for (int i = 0; i < 4; ++i) turnOrder[i] = (start + i) % 4;
    turnPos = 0;
}

void BlackjackGame::advanceActor()
{
    turnPos++;
    if (turnPos >= 4) {
         if (tutorialActive && tutorialStep == 0) {
            toDealerTurn();
        }
        else {
            state = State::Accuse;
        }
        return;
    }

    int idx = currentActorIndex();
    if (idx == 0) toPlayerTurn();
    else          toCpuTurn();   
}

void BlackjackGame::assignBaseAccuseProbs()
{
    // {1/2,1/4,1/8,1/8} をランダムに配る
    std::vector<int> denoms = { 2,4,8,8 };
    std::shuffle(denoms.begin(), denoms.end(), gameRng());
    for (int i = 0; i < 4; ++i) players[i].baseAccuseDenom = denoms[i];
}

float BlackjackGame::baseAccuseProb(const BJParticipant& p) const
{
    if (p.baseAccuseDenom <= 0) return 0.0f;
    return 1.0f / (float)p.baseAccuseDenom;
}

float BlackjackGame::hiddenAccuseAdd(const BJParticipant& p) const
{
    float add = 0.0f;
    int s = p.hand.bestScore();

    // -------------------------
    // (A) イカサマした場合：スコアに応じて加算
    // 17:+3% 18:+4% 19:+5% 20:+5% 21:+10%
    // -------------------------
    if (p.cheatedThisRound) {
        if (s == 17) add += 0.03f;
        else if (s == 18) add += 0.04f;
        else if (s == 19) add += 0.05f;
        else if (s == 20) add += 0.05f;
        else if (s == 21) add += 0.10f;
    }

    // -------------------------
    // (B) ダブルダウンした場合：スコアに応じて加算
    // 19:+5% 20:+5% 21:+10%
    // -------------------------
    if (p.doubled) {
        if (s == 19) add += 0.05f;
        else if (s == 20) add += 0.05f;
        else if (s == 21) add += 0.10f;
    }

    // -------------------------
    // (C) ベットが大きい：ベット額に応じて加算（最終betで判定）
    // 1000..1999:+1% / 2000..2999:+2% / ... / 5000:+5%
    // -------------------------
    const int b = p.bet; // ※ダブル後は bet が2倍なので、そのまま疑われやすくなる
    if (b >= 5000) add += 0.05f;
    else if (b >= 4000) add += 0.04f;
    else if (b >= 3000) add += 0.03f;
    else if (b >= 2000) add += 0.02f;
    else if (b >= 1000) add += 0.01f;

    return add;
}


float BlackjackGame::effectiveAccuseProb(const BJParticipant& p) const
{
    float p0 = baseAccuseProb(p);
    float p1 = hiddenAccuseAdd(p);
    float pe = p0 + p1;
    if (pe < 0.0f) pe = 0.0f;
    if (pe > 0.95f) pe = 0.95f; // 上限
    return pe;
}


double BlackjackGame::probBonusMultiplier(const BJParticipant& p) const
{
    // ベース確率が高い状態で勝ったら倍率
    // 1/2 → 3倍、1/4 → 2倍、1/8 → 1倍
    if (p.baseAccuseDenom == 2) return 3.0;
    if (p.baseAccuseDenom == 4) return 2.0;
    return 1.0;
}

double BlackjackGame::winMultiplier(const BJParticipant& p) const
{
    // 勝利倍率カテゴリ（どれか1つ）
    // BJ勝利 50倍
    // Dealerバースト 10倍
    // 僅差勝利 2,2.5,3,... （Δ=1で2、以降+0.5）
    // push 1倍
    // lose 0倍

    int ps = p.hand.bestScore();
    int ds = dealer.hand.bestScore();

    bool pBust = p.hand.isBust();
    bool dBust = dealer.hand.isBust();
    bool pBJ = p.hand.isBlackjack();
    bool dBJ = dealer.hand.isBlackjack();

    if (pBust) return 0.0;

    // dealer BJ
    if (dBJ) {
        if (pBJ) return 1.0; // push
        return 0.0;
    }

    // player BJ
    if (pBJ) {
        return 50.0;
    }

    // dealer bust
    if (dBust) {
        return 10.0;
    }

    // compare
    if (ps > ds) {
        int diff = ps - ds; // 1以上
        return 1.5 + 0.5 * (double)diff; // diff=1 -> 2.0
    }
    if (ps == ds) return 1.0; // push
    return 0.0;
}

void BlackjackGame::beginRound() {
    dealer.hand.clear();

    for (auto& p : players) {
        p.hand.clear();
        p.bet = 0;
        p.doubled = false;
        p.stood = false;

        //ラウンド開始でリセット
        p.cheatedThisRound = false;
        p.accusedThisRound = false;
        p.falseAccused = false;
        p.caughtCheating = false;
        p.mustCheatLater = false;
    }
    dealerHoleRevealed = false;
    dealerRevealTimer = 0.0f;
    dealerWait = 0.0f;
    accuseRunning = false;
    accuseStep = 0;
    accuseWait = 0.0f;
    for (int i = 0; i < 4; ++i) accuseRevealed[i] = false;

     swappedThisRound = false; 
     dealSePlayed = false;
}


void BlackjackGame::toBetting()
{
    state = State::Betting;

    // ここで毎回リセットしておくと「タイトル→再開始で前の手札が残る」事故を防げる
    dealer.hand.clear();
    dealer.bet = 0;
    dealer.doubled = false;
    dealer.stood = false;

    for (auto& p : players) {
        p.hand.clear();
        p.bet = 0;
        p.doubled = false;
        p.stood = false;
    }
    

    // 5ラウンド終了後にOK押したら「新しい5ラウンド」を始める設計にする
    if (matchOver) {
        matchOver = false;
        roundNo = 0;
        resultSePlayed = false;
        // 5ラウンド制の「新規ゲーム開始」：チップを初期化する
        for (auto& p : players) {
            p.chips = kStartChips;
        }
        uiPlayerBet = 100;
        buildGuaranteeTargets(); // 新しい試合なので保証ターゲットも作り直す

    }
}



void BlackjackGame::toDealing()
{
    if (matchOver) { state = State::RoundEnd; return; }

    beginRound();

    roundNo++;
    if (roundNo > kMaxRounds) {
        matchOver = true;
        state = State::RoundEnd;
        return;
    }

    assignBaseAccuseProbs();
    setupTurnOrderForRound();

    // YOU bet
    int betV = normalizeBet(uiPlayerBet, kMinBet, kBetStep, kMaxUserBet);
    players[0].bet = betV;
    players[0].chips -= betV;

    // CPU bet
    for (int i = 1; i <= 3; ++i) {
        int cpuBet = cpuRandomBet(players[i].chips, kMinBet, kBetStep);
        players[i].bet = cpuBet;
        players[i].chips -= cpuBet;
    }

    // ---- ここから「配る演出用キュー」作成 ----
    dealQ.clear();
    dealQ.reserve(10); // (4人+Dealer)*2 = 10

    for (int k = 0; k < 2; ++k) {
        for (int i = 0; i < 4; ++i) {
            dealQ.push_back({ DealTo::Player, i, deck.draw() });
        }
        dealQ.push_back({ DealTo::Dealer, -1, deck.draw() });
    }

    dealPos = 0;
    dealWait = 0.0f;

    state = State::Dealing;
    setMsg("DEALING...");
}

void BlackjackGame::finishDealing()
{
    
    if (!tutorialActive) {
        // まず全員リセット
        for (auto& p : players) {
            p.cheatMode = CheatMode::None;
            p.cheatedThisRound = false;
            p.cheatBetTarget = 21;
        }

        // YOU：ベット画面で選んだモード
        players[0].cheatMode = uiCheatMode;
        players[0].cheatBetTarget = uiCheatBetTarget;

        // CPU：20%でチート
        for (int i = 1; i <= 3; ++i) {
            int r = randInt(0, 99);
            if (r < 20) {
                players[i].cheatMode = CheatMode::BetTotal;
                players[i].cheatBetTarget = randInt(17, 21);
            }
            else {
                players[i].cheatMode = CheatMode::None;
            }
        }

        // BetTotal の人は初手固定
        for (auto& p : players) {
            if (p.cheatMode == CheatMode::BetTotal) {
                int t = p.cheatBetTarget;
                if (t < 17) t = 17;
                if (t > 21) t = 21;
                rigHandToTotal(p.hand, t);
                p.cheatedThisRound = true;
                setMsg(p.name + " CHEAT(BET)");
            }
        }
    }

    // ---- チュートリアル固定----
    tutAccusePreset = 0;

    if (tutorialActive) {
        uiCheatMode = CheatMode::None;
        uiCheatBetTarget = 21;
        for (auto& p : players) {
            p.cheatMode = CheatMode::None;
            p.cheatedThisRound = false;
        }

        if (tutorialStep == 0) {
            tutAccusePreset = 0;
            setMsg("TUTORIAL 1/5: NORMAL (NO CHEAT, NO ACCUSE)");
            int first = currentActorIndex();
            if (first == 0) toPlayerTurn();
            else            toCpuTurn();
            return;
        }
        else if (tutorialStep == 1) {
            players[0].cheatMode = CheatMode::BetTotal;
            players[0].cheatBetTarget = 21;
            rigHandToTotal(players[0].hand, 21);
            players[0].cheatedThisRound = true;
            rigHandToTotal(dealer.hand, 18);
            tutAccusePreset = 1;
            setMsg("TUTORIAL 2/5: CHEAT -> YOU WILL BE CAUGHT");
            int first = currentActorIndex();
            if (first == 0) toPlayerTurn();
            else            toCpuTurn();
            return;
        }
        else if (tutorialStep == 2) {
            players[0].cheatMode = CheatMode::BetTotal;
            players[0].cheatBetTarget = 21;
            rigHandToTotal(players[0].hand, 21);
            players[0].cheatedThisRound = true;
            rigHandToTotal(dealer.hand, 19);
            tutAccusePreset = 2;
            setMsg("TUTORIAL 3/5: CHEAT -> NOT ACCUSED -> WIN");
            int first = currentActorIndex();
            if (first == 0) toPlayerTurn();
            else            toCpuTurn();
            return;
        }
        else if (tutorialStep == 3) {
            players[0].baseAccuseDenom = 8;
            players[1].baseAccuseDenom = 2;
            players[2].baseAccuseDenom = 4;
            players[3].baseAccuseDenom = 8;

            rigHandToTotal(players[0].hand, 20);
            rigHandToTotal(dealer.hand, 18);

            tutAccusePreset = 2;
            setMsg("TUTORIAL 4/5: SWAP BASE PROB (try swapping with CPU1)");
            toBaseProbSwap();
            return;
        }
        else if (tutorialStep == 4) {
            rigHandToTotal(players[0].hand, 20);
            rigHandToTotal(dealer.hand, 18);

            tutAccusePreset = 3;
            setMsg("TUTORIAL 5/5: FALSE ACCUSATION");
            int first = currentActorIndex();
            if (first == 0) toPlayerTurn();
            else            toCpuTurn();
            return;
        }
    }

    // 通常は交換フェーズへ
    toBaseProbSwap();
}


void BlackjackGame::toPlayerTurn() { state = State::PlayerTurn;  }
void BlackjackGame::toCpuTurn() {
    state = State::CpuTurn;
    cpuWait = 0.0f;
    lastCpuIdx = -1; // 次のCpuTurn開始時に必ずリセットされるように
}
void BlackjackGame::toDealerTurn() {
    state = State::DealerTurn;
    dealerHoleRevealed = false;
    dealerRevealTimer = 0.0f;
    dealerWait = 0.0f;
    
}

void BlackjackGame::toSettle() { state = State::Settle;}

void BlackjackGame::toRoundEnd(const std::string& msg) {
    state = State::RoundEnd;
    setMsg(msg);
}

bool BlackjackGame::canDoubleDown(const BJParticipant& p) const {
    if (p.doubled) return false;
    if (p.hand.cardCount() != 2) return false;
    //借金はありにする//if (p.chips < p.bet) return false;
    return true;
}

void BlackjackGame::doHit(BJParticipant& p) {
    p.hand.add(deck.draw());
    AudioManager::PlayGameSE(cardOpen);   
    if (p.hand.isBust()) p.stood = true;
}


void BlackjackGame::doStand(BJParticipant& p) {
    p.stood = true;
}

void BlackjackGame::doDoubleDown(BJParticipant& p) {
    if (!canDoubleDown(p)) return;

    p.chips -= p.bet;
    p.bet *= 2;
    p.doubled = true;

    doHit(p);      // 1枚
    p.stood = true;
}

void BlackjackGame::cpuAct(BJParticipant& cpu) {
    

    if (cpu.stood) return;
    if (cpu.hand.isBust()) { cpu.stood = true; setMsg(cpu.name + " BUST"); return; }

    int s = cpu.hand.bestScore();

    if (cpu.hand.cardCount() == 2 && canDoubleDown(cpu) && (s == 9 || s == 10 || s == 11)) {
        doDoubleDown(cpu);
        setMsg(cpu.name + " DOUBLE");
        return;
    }

    if (s <= 16) { doHit(cpu); setMsg(cpu.name + " HIT"); }
    else { doStand(cpu); setMsg(cpu.name + " STAND"); }
}


void BlackjackGame::settleOne(BJParticipant& p)
{
    // 正解指摘（=cheatしてて指摘された）
    // → 受取なし。もし「勝ってた場合」は本来受け取れた総受取分を借金として追加で引く
    if (p.caughtCheating) {
        double w = winMultiplier(p);
        if (w > 1.0) {
            double bonusProb = probBonusMultiplier(p);
            double bonusFalse = (p.falseAccused ? 1.5 : 1.0); // 正解指摘なら基本 falseAccused は立たないが一応
            double finalMult = w * bonusProb * bonusFalse;

            long long penalty = (long long)std::llround((double)p.bet * finalMult);
            p.chips -= (int)penalty;
        }
        return;
    }

    double w = winMultiplier(p);

    // lose
    if (w <= 0.0) return;

    // push は倍率1固定（追加倍率なし）
    if (w == 1.0) {
        p.chips += p.bet; // 掛け金返却
        return;
    }

    // win：勝利倍率カテゴリ（w）× 冤罪1.5 × 指摘確率ボーナス
    double bonusFalse = (p.falseAccused ? 1.5 : 1.0);
    double bonusProb = probBonusMultiplier(p);

    double finalMult = w * bonusFalse * bonusProb;
    long long receive = (long long)std::llround((double)p.bet * finalMult);

    p.chips += (int)receive;
}

void BlackjackGame::swapBaseDenom(int a, int b)
{
    std::swap(players[a].baseAccuseDenom, players[b].baseAccuseDenom);
}

void BlackjackGame::toBaseProbSwap()
{
    state = State::BaseProbSwap;
    setMsg("BASE PROB SWAP: choose CPU or SKIP");
}

void BlackjackGame::prepareAccuseResults()
{
    // チュートリアル固定
    if (tutorialActive && tutAccusePreset != 0) {
        for (auto& pl : players) {
            pl.accusedThisRound = false;
            pl.falseAccused = false;
            pl.caughtCheating = false;
        }

        if (tutAccusePreset == 1) {
            // CHEAT CAUGHT
            players[0].accusedThisRound = true;
            players[0].caughtCheating = true;
        }
        else if (tutAccusePreset == 2) {
            // CHEAT SAFE（誰もYOUを指摘しない）
            // 必要なら CPU1 を冤罪にする等も可能：
            // players[1].accusedThisRound = true;
            // players[1].falseAccused = true;
        }
        else if (tutAccusePreset == 3) {
            // FALSE ACCUSE
            players[0].accusedThisRound = true;
            players[0].falseAccused = true;
        }
        return; // 通常抽選をやらない
    }

    // フラグ初期化
    for (auto& pl : players) {
        pl.accusedThisRound = false;
        pl.falseAccused = false;
        pl.caughtCheating = false;
    }

    // 1) 保証（R1〜R4）
    if (1 <= roundNo && roundNo <= 4) {
        int g = guarantee[roundNo - 1];
        players[g].accusedThisRound = true;
    }

    // 2) 抽選（各人個別）
    auto doLotteryOnce = [&]() {
        for (int i = 0; i < 4; ++i) {
            float pe = effectiveAccuseProb(players[i]);
            std::bernoulli_distribution bd(pe);
            if (bd(gameRng())) players[i].accusedThisRound = true;
        }
        };

    // R5 は「誰も選ばれなかったら再抽選」
    if (roundNo == 5) {
        while (true) {
            for (auto& pl : players) pl.accusedThisRound = false;
            doLotteryOnce();

            bool any = false;
            for (auto& pl : players) if (pl.accusedThisRound) { any = true; break; }
            if (any) break;
        }
    }
    else {
        doLotteryOnce(); // R1〜R4：保証＋抽選で上書きはしない（trueが残る）
    }

    // 3) 冤罪/正解判定
    for (auto& pl : players) {
        if (!pl.accusedThisRound) continue;
        if (pl.cheatedThisRound) pl.caughtCheating = true;
        else                    pl.falseAccused = true;
    }
}



void BlackjackGame::applyBaseProbSwapChoice(SwapChoice c)
{
    swappedThisRound = true; // 1回押したら確定（※state移動するので保険）

    std::string msg;

    switch (c) {
    case SwapChoice::Cpu1:
        swapBaseDenom(0, 1);
        msg = "SWAP: YOU <-> CPU1";
        break;
    case SwapChoice::Cpu2:
        swapBaseDenom(0, 2);
        msg = "SWAP: YOU <-> CPU2";
        break;
    case SwapChoice::Cpu3:
        swapBaseDenom(0, 3);
        msg = "SWAP: YOU <-> CPU3";
        break;
    case SwapChoice::Skip:
        msg = "SWAP: SKIP";
        break;
    default:
        return;
    }


    applySwapDemeritIfCheating(players[0]);
    // 次のターンへ
    int first = currentActorIndex();
    if (first == 0) toPlayerTurn();
    else            toCpuTurn();

    // setMsgを1回にして上書きを防ぐ
    setMsg(msg + " | TURN START: " + players[first].name);
}
bool BlackjackGame::pollBetOk()
{
    btnBetOK.update();
    return btnBetOK.isClicked();
}

void BlackjackGame::handleRoundEndNext()
{
    // ---- チュートリアル中 ----
    if (tutorialActive) {
        tutorialStep++;
        tutAccusePreset = 0;

        if (tutorialStep >= kTutorialSteps) {
            tutorialActive = false;
            state = State::TutorialEnd;
            if (!resultSePlayed) {
                AudioManager::PlayGameSE(RESULT);
                resultSePlayed = true;
            }
            setMsg("TUTORIAL COMPLETE!");
            return;
        }


        // 次のチュートリアルラウンドへ
        if (tutorialStep <= 3) {
            state = State::TutorialIntro;
        }
        else {
            toBetting(); // 5/5 は Intro無しで開始
        }
        return;
    }

    // ---- 通常ゲーム ----
    if (matchOver) {
        state = State::FinalResult;

        if (!resultSePlayed) {
            AudioManager::PlayGameSE(RESULT);
            resultSePlayed = true;
        }
    }
    else           toBetting();
}


void BlackjackGame::update()
{
    input::update();
    float dt = Timer::getInstance()->getDeltaTime();
    if (dt > 0.1f) dt = 0.1f;

    const bool inPause = (state == State::PauseMenu || state == State::PauseInfo);

    if (state == State::PauseMenu)      layoutPauseMenuButtons();
    else if (state == State::PauseInfo) layoutPauseInfoButtons();


    // ---- ポーズ中は Pauseボタン/Skipボタンを無効 ----
    if (!inPause) {
        btnTutSkip.update();
        btnPause.update();

        if (btnPause.isClicked()) {
            openPause();
            return;
        }

        // ---- チュートリアルSKIP----
        if (tutorialActive && state != State::TutorialEnd && btnTutSkip.isClicked()) {
            tutorialSkipOneRound();
            return;
        }
    }
    // フェード更新は毎フレーム
    fade.Update(dt);

    // NEW GAME フェード中の「切り替え点」
    if (fadeNewGameReq) {
        // FadeOut完了 → Wait になった瞬間に初期化して FadeIn
        if (fade.IsFadeOutFinished()) {

            if (fadeFromTutorial) {
                resetForRealMatch();   // チュートリアル後の本番初期化
            }
            else {
                // FinalResult の NEW GAME：
                toBetting();
            }

            fade.StartFadeIn(0.6f);
        }

        // フェード中は他入力を全部止める
        if (fade.IsFading()) return;

        // FadeInも終わったら解除
        if (fade.IsFadeInFinished()) {
            fadeNewGameReq = false;
            fadeFromTutorial = false;
        }
    }

    switch (state) {
    case State::Betting: {

        // (A) ボタン更新
        btnBetMinus100.update();
        btnBetMinus50.update();
        btnBetMinus10.update();
        btnBetOK.update();
        btnBetPlus10.update();
        btnBetPlus50.update();
        btnBetPlus100.update();

        btnCheatToggle.update();
        btnCheatMinus.update();
        btnCheatPlus.update();

        const bool betTotal = (uiCheatMode == CheatMode::BetTotal);
        const BetPermissions perm = calcBetPermissions();

        // ---- チート切替 ----
        if (perm.allowCheatToggle && btnCheatToggle.isClicked()) {
            uiCheatMode = (uiCheatMode == CheatMode::None) ? CheatMode::BetTotal : CheatMode::None;
        }

        // ---- チートターゲット ----
        if (betTotal && perm.allowCheatTarget) {
            if (btnCheatMinus.isClicked()) uiCheatBetTarget--;
            if (btnCheatPlus.isClicked())  uiCheatBetTarget++;
            if (uiCheatBetTarget < 17) uiCheatBetTarget = 17;
            if (uiCheatBetTarget > 21) uiCheatBetTarget = 21;
        }

        // ---- BET増減 ----
        if (perm.allowBetAdjust) {
            const bool eM100 = canBetDelta(-100);
            const bool eM50 = canBetDelta(-50);
            const bool eM10 = canBetDelta(-10);
            const bool eP10 = canBetDelta(+10);
            const bool eP50 = canBetDelta(+50);
            const bool eP100 = canBetDelta(+100);

            if (btnBetMinus100.isClicked() && eM100) applyBetDelta(-100);
            if (btnBetMinus50.isClicked() && eM50)  applyBetDelta(-50);
            if (btnBetMinus10.isClicked() && eM10)  applyBetDelta(-10);

            if (btnBetPlus10.isClicked() && eP10)  applyBetDelta(+10);
            if (btnBetPlus50.isClicked() && eP50)  applyBetDelta(+50);
            if (btnBetPlus100.isClicked() && eP100) applyBetDelta(+100);
        }

        // ---- OK ----
        if (perm.allowBetOK && btnBetOK.isClicked()) {
            toDealing();
        }

        break;
    }

    case State::BaseProbSwap: {

        const bool baseEnabled = !swappedThisRound;

        // 例：チュートリアルの swap 回（tutorialStep==3）のとき CPU1 だけ押せる
        const bool onlyCpu1 = (tutorialActive && tutorialStep == 3);

        const bool eCpu1 = baseEnabled;
        const bool eCpu2 = baseEnabled && !onlyCpu1;
        const bool eCpu3 = baseEnabled && !onlyCpu1;
        const bool eSkip = baseEnabled;

        //enabled のときだけ update（無効なら updateしない）
        if (eCpu1) btnSwapCpu1.update();
        if (eCpu2) btnSwapCpu2.update();
        if (eCpu3) btnSwapCpu3.update();
        if (eSkip) btnSwapSkip.update();

        SwapChoice c = SwapChoice::None;
        if (eCpu1 && btnSwapCpu1.isClicked()) c = SwapChoice::Cpu1;
        if (eCpu2 && btnSwapCpu2.isClicked()) c = SwapChoice::Cpu2;
        if (eCpu3 && btnSwapCpu3.isClicked()) c = SwapChoice::Cpu3;
        if (eSkip && btnSwapSkip.isClicked()) c = SwapChoice::Skip;

        if (c != SwapChoice::None) applyBaseProbSwapChoice(c);
        break;
    }

    case State::PlayerTurn: {
        int idx = currentActorIndex();
        BJParticipant& you = players[idx];

        // 通常入力
        btnHit.update();
        btnStand.update();
        btnDouble.update();

        if (!you.stood) {
            if (canDoubleDown(you) && btnDouble.isClicked()) {
                doDoubleDown(you);         
                setMsg("YOU DOUBLE");
            }
            else if (btnHit.isClicked()) {
                doHit(you);                 
                setMsg("YOU HIT");
            }
            else if (btnStand.isClicked()) {
                doStand(you);
                setMsg("YOU STAND");
            }
        }

        if (you.stood) advanceActor();
        break;
    }


    case State::CpuTurn: {
        int idx = currentActorIndex();
        BJParticipant& cpu = players[idx];

        // 行動者が変わったら待ち時間リセット
        if (idx != lastCpuIdx) {
            lastCpuIdx = idx;
            cpuWait = 0.0f;
        }

        cpuWait += dt;
        if (cpuWait < kActInterval) break;   // まだ待つ

        cpuWait = 0.0f;                      // 2秒経ったので1回行動
        cpuAct(cpu);                         // ここが「1回ぶん」だけ実行される

        if (cpu.stood) {
            advanceActor();                  // 次の人へ
        }
        break;
    }

    case State::DealerTurn: {

        // まず1秒かけて伏せ札を表にする
        if (!dealerHoleRevealed) {
            dealerRevealTimer += dt;
            if (dealerRevealTimer >= kActInterval) {
                dealerHoleRevealed = true;
                dealerRevealTimer = 0.0f;
                setMsg("DEALER REVEAL");
                AudioManager::PlayGameSE(cardOpen);
            }
            break; // REVEAL中はHITしない
        }

        // REVEAL後に、1秒ごとに1回だけ行動
        dealerWait += dt;
        if (dealerWait < kActInterval) break;
        dealerWait = 0.0f;

        if (dealer.hand.bestScore() < 17) {
            dealer.hand.add(deck.draw());
            AudioManager::PlayGameSE(cardOpen);
            setMsg("DEALER HIT");
            if (dealer.hand.isBust()) {
                setMsg("DEALER BUST");
                toSettle();
            }
        }
        else {
            setMsg("DEALER STAND");
            toSettle();
        }
        break;
    }
    case State::Settle: {
        for (auto& p : players) settleOne(p);

        const bool isLastRound = (roundNo >= kMaxRounds);
        if (isLastRound) {
            matchOver = true;              // 5R終わったフラグ
            toRoundEnd("ROUND 5 END");     // 盤面を残したままRoundEndへ
        }
        else {
            toRoundEnd("ROUND END");
        }
        break;
    }
    case State::RoundEnd: {
        if (!pollBetOk()) break;
        handleRoundEndNext();
        break;
    }

    case State::FinalResult: {
        btnBetOK.update();
        if (btnBetOK.isClicked()) {
            startFadeNewGame(false);  
        }
        break;
    }
    case State::Accuse: {

        // Accuse開始時に「結果を確定」して、表示は順番に
        if (!accuseRunning) {
            prepareAccuseResults();

            accuseRunning = true;
            accuseStep = 0;
            accuseWait = 0.0f;
            for (int i = 0; i < 4; ++i) accuseRevealed[i] = false;

            setMsg("ACCUSE START");
        }

        accuseWait += dt;
        if (accuseWait < kActInterval) break;
        accuseWait = 0.0f;

        if (accuseStep < 4) {
            int i = accuseStep;
            accuseRevealed[i] = true;

            

            // 判定SE（SAFE / FALSE / CAUGHT）
            if (players[i].accusedThisRound) {
                if (players[i].caughtCheating) {
                    AudioManager::PlayGameSE(CAUGHT);
                }
                else if (players[i].falseAccused) {
                    AudioManager::PlayGameSE(THRY); // 冤罪（FALSE）
                }
                else {
                    // 保険（基本ここには来ない想定）
                    AudioManager::PlayGameSE(CAUGHT);
                }
            }
            else {
                AudioManager::PlayGameSE(SAFE);
            }

            // メッセージも UI 表示と揃えると分かりやすい
            std::string msg = "ACCUSE CHECK: " + players[i].name + " -> ";
            if (players[i].accusedThisRound) {
                if (players[i].caughtCheating)      msg += "CAUGHT";
                else if (players[i].falseAccused)   msg += "FALSE";
                else                                msg += "ACCUSED";
            }
            else {
                msg += "SAFE";
            }
            setMsg(msg);

            accuseStep++;
        }


        // 全員分表示し終わったら、ディーラーターンへ（ここから穴札オープン演出）
        if (accuseStep >= 4) {
            accuseRunning = false;
            toDealerTurn();
        }
        break;
    }
    case State::TutorialEnd: {
        btnBetOK.update();
        if (btnBetOK.isClicked()) {
            startFadeNewGame(true);   
        }
        break;
    }
    case State::PauseMenu: {
        // 7ボタン更新
        btnPTitle.update();
        btnPReturn.update();
        btnPWhatBJ.update();
        btnPCheat1.update();
        btnPCheat2.update();
        btnPCheat3.update();
        btnPMult.update();

        if (btnPTitle.isClicked()) { nextScene = SCENE_TITLE; return; }
        if (btnPReturn.isClicked()) { resumeFromPause(); break; }

        if (btnPWhatBJ.isClicked()) { pausePage = PausePage::WhatBJ;      state = State::PauseInfo; }
        if (btnPCheat1.isClicked()) { pausePage = PausePage::Cheat1;      state = State::PauseInfo; }
        if (btnPCheat2.isClicked()) { pausePage = PausePage::Cheat2;      state = State::PauseInfo; }
        if (btnPCheat3.isClicked()) { pausePage = PausePage::Cheat3;      state = State::PauseInfo; }
        if (btnPMult.isClicked()) { pausePage = PausePage::Multiplier;  state = State::PauseInfo; }

        break;
    }

    case State::PauseInfo: {

        // 左上「戻る」だけ更新
        btnPBack.update();

        if (btnPBack.isClicked()) {
            pausePage = PausePage::None;
            state = State::PauseMenu;
            break;
        }

        
        break;
    }
    case State::TutorialIntro: {
        layoutTutorialIntroButtons();

        const float UP = 60.0f; // ←上げたい量(px)
        btnBetOK.setRect(btnBetOK.getX(), btnBetOK.getY() - UP, btnBetOK.getW(), btnBetOK.getH());

        btnBetOK.update();
        if (btnBetOK.isClicked()) {
            toBetting();
        }
        break;
    }
   
    case State::Dealing: {

        // 1回だけ鳴らす
        if (!dealSePlayed) {
            AudioManager::PlayGameSE(handOutCards);
            dealSePlayed = true;
        }

        dealWait += dt;
        if (dealWait < kDealInterval) break;
        dealWait = 0.0f;

        if (dealPos < (int)dealQ.size()) {
            const DealItem& it = dealQ[dealPos++];

            if (it.to == DealTo::Player) players[it.index].hand.add(it.card);
            else                         dealer.hand.add(it.card);

             
        }
        else {
            finishDealing();
        }
        break;
    }



    default:
        break;
    }
}

//================================================
// 32x32 の切り抜きを使って「カード」を描く
// 1枚のカード=64x32（左32=ランク, 右32=スート）
//================================================
void BlackjackGame::drawRankImage(int rank, int suit, float x, float y, float size) {
    const bool red = isRedSuit(suit);

    // JQKA（2x2）
    // [J Q]
    // [K A]
    if (rank == 11) { drawSheet_2x2_32(red ? assets.sprBJKA_R : assets.sprBJKA_B, 0, 0, x, y, size); return; } // J
    if (rank == 12) { drawSheet_2x2_32(red ? assets.sprBJKA_R : assets.sprBJKA_B, 1, 0, x, y, size); return; } // Q
    if (rank == 13) { drawSheet_2x2_32(red ? assets.sprBJKA_R : assets.sprBJKA_B, 0, 1, x, y, size); return; } // K
    if (rank == 1) { drawSheet_2x2_32(red ? assets.sprBJKA_R : assets.sprBJKA_B, 1, 1, x, y, size); return; } // A

    // 2_jo：上段に2(黒/赤)
    if (rank == 2) {
        drawSheet_2x2_32(assets.spr2Jo, red ? 1 : 0, 0, x, y, size);
        return;
    }

    // 3-6（2x2）
    if (3 <= rank && rank <= 6) {
        // [3 4]
        // [5 6]
        int idx = rank - 3;        // 0..3
        int col = idx % 2;         // 0,1
        int row = idx / 2;         // 0,1
        drawSheet_2x2_32(red ? assets.sprB36_R : assets.sprB36_B, col, row, x, y, size);
        return;
    }

    // 7-10（2x2）
    if (7 <= rank && rank <= 10) {
        // [7 8]
        // [9 10]
        int idx = rank - 7;        // 0..3
        int col = idx % 2;
        int row = idx / 2;
        drawSheet_2x2_32(red ? assets.sprB710_R : assets.sprB710_B, col, row, x, y, size);
        return;
    }
}

void BlackjackGame::drawSuitImage(int suit, float x, float y, float size) {
    // 上段：♠ ♣
    // 下段：♥ ♦
    int col = 0, row = 0;
    switch (suit) {
    case 3: col = 0; row = 0; break; // ♠
    case 0: col = 1; row = 0; break; // ♣
    case 2: col = 0; row = 1; break; // ♥
    case 1: col = 1; row = 1; break; // ♦
    default: break;
    }
    drawSheet_2x2_32(assets.sprMark, col, row, x, y, size);
}

void BlackjackGame::drawCardFaceImage(const BJCard& c, float x, float y) {
    const float half = 32.0f; // 左右それぞれ32
    drawRankImage(c.rank, c.suit, x, y, half);
    drawSuitImage(c.suit, x + half, y, half);
}

void BlackjackGame::drawCardBackImage(float x, float y) {
    if (!assets.sprBackBJ) return;

    // 64x32をそのまま描く（等倍）
    sprite_render(assets.sprBackBJ,
        x, y, 1.0f, 1.0f,
        0, 0, 64, 32,
        0, 0, 0,
        1, 1, 1, 1,
        false
    );
}

void BlackjackGame::drawTutorialSkipUI(const RenderCtx& ctx)
{
    if (!tutorialActive) return;
    if (state == State::TutorialEnd) return;
    if (state == State::PauseMenu || state == State::PauseInfo) return;
    drawBtnImageFit(assets.sprSkip, btnTutSkip, 160, 70, true);
}


//================================================
// 文字描画：align を指定しない
//================================================
static float text_outL(int fontNo, const std::string& str,
    float x, float y, float scaleX, float scaleY,
    float r, float g, float b, float a)
{
    // TEXT_ALIGN を渡さない版（=デフォルト左揃え）
    return font::textOut(fontNo, str, x, y, scaleX, scaleY, r, g, b, a);
}

//================================================
// カード文字 " 6♠" / "10♣" の生成（10だけ2桁）
//================================================
static const char* suitUTF8(int suit)
{
    // suit: 0=♣ 1=♦ 2=♥ 3=♠（あなたの定義に合わせて）
    static const char* sym[4] = { u8"♣", u8"♦", u8"♥", u8"♠" };
    if (suit < 0 || suit > 3) return "?";
    return sym[suit];
}

static std::string rankStr(int rank)
{
    if (rank == 1)  return "A";
    if (rank == 11) return "J";
    if (rank == 12) return "Q";
    if (rank == 13) return "K";
    return std::to_string(rank);
}

static std::string cardText(const BJCard& c)
{
    std::string r = rankStr(c.rank);
    if (c.rank != 10) r = " " + r; // 10以外は左に空白
    return r + suitUTF8(c.suit);
}

//==========================
// 上部UI（ROUND / MSG / TURN / NEXT ACT）
//==========================
void BlackjackGame::drawTopUI(const RenderCtx& ctx)
{
    if (tutorialActive) {
        ctx.textL("" + std::to_string(tutorialStep + 1) + " / " + std::to_string(kTutorialSteps),
            ui.topRoundX, ui.topRoundY, 1.0f, 1.0f, 1.0f);
    }
    else {
        ctx.textL("ROUND: " + std::to_string(roundNo) + " / " + std::to_string(kMaxRounds),
            ui.topRoundX, ui.topRoundY, 1.0f, 1.0f, 1.0f);
    }

    const bool showMsg = (state != State::Betting);
    if (showMsg && !lastMessage.empty()) {
        ctx.textL("MSG: " + lastMessage, ui.msgX, ui.msgY, ctx.FS_S, ctx.FS_S, 1.0f);
    }

    if (state == State::PlayerTurn || state == State::CpuTurn) {
        int a = currentActorIndex();
        ctx.textL("TURN: " + players[a].name, ui.topTurnX, ui.topTurnY, ctx.FS_S, ctx.FS_S, 1.0f);
    }
    else if (state == State::DealerTurn) {
        ctx.textL("TURN: DEALER", ui.topTurnX, ui.topTurnY, ctx.FS_S, ctx.FS_S, 1.0f);
    }

   
}

//==========================
// ボタン（左上）
//==========================
void BlackjackGame::drawTitleUI(const RenderCtx& ctx)
{
    // 左上は PAUSE
    drawBtnImageFit(assets.sprPauseBtn, btnPause, 500, 200, true);
}


BlackjackGame::BetPermissions BlackjackGame::calcBetPermissions() const
{
    BetPermissions perm{};

    const bool tut = tutorialActive;
    const bool betTotal = (uiCheatMode == CheatMode::BetTotal);

    // 通常時
    if (!tut) {
        perm.allowBetAdjust = true;
        perm.allowCheatToggle = true;
        perm.allowCheatTarget = betTotal; // ONのときだけ意味がある
        perm.allowBetOK = true;
        return perm;
    }

    // チュートリアル時：デフォは全部禁止
    perm.allowBetAdjust = false;
    perm.allowCheatToggle = false;
    perm.allowCheatTarget = false;
    perm.allowBetOK = false;

    switch (tutorialStep) {
    case 0:
        // 1/5：普通に開始（OKだけ）
        perm.allowBetOK = true;
        break;

    case 1:
        // 2/5：CHEAT ON + TARGET 21
        perm.allowCheatToggle = true;
        perm.allowCheatTarget = betTotal;
        perm.allowBetOK = (betTotal && uiCheatBetTarget == 21);
        break;

    case 2:
        // 3/5：CHEAT ON + TARGET 21
        perm.allowCheatToggle = true;
        perm.allowCheatTarget = betTotal;
        perm.allowBetOK = (betTotal && uiCheatBetTarget == 21);
        break;

    case 3:
        // 4/5：SWAPを見せたいので CHEAT OFF で開始
        perm.allowCheatToggle = true;
        perm.allowBetOK = (!betTotal);
        break;

    case 4:
        // 5/5：冤罪（FALSE）も CHEAT OFF で開始
        perm.allowCheatToggle = true;
        perm.allowBetOK = (!betTotal);
        break;
    }


    return perm;
}


//==========================
// BET UI
//==========================
void BlackjackGame::drawBetUI(const RenderCtx& ctx)
{
    if (state != State::Betting) return;

    const bool betTotal = (uiCheatMode == CheatMode::BetTotal);
    const BetPermissions perm = calcBetPermissions();

    const bool eM100 = canBetDelta(-100);
    const bool eM50 = canBetDelta(-50);
    const bool eM10 = canBetDelta(-10);
    const bool eP10 = canBetDelta(+10);
    const bool eP50 = canBetDelta(+50);
    const bool eP100 = canBetDelta(+100);

    drawBtnImageFit(assets.sprM100, btnBetMinus100, 90, 70, eM100 && perm.allowBetAdjust);
    drawBtnImageFit(assets.sprM50, btnBetMinus50, 90, 70, eM50 && perm.allowBetAdjust);
    drawBtnImageFit(assets.sprM10, btnBetMinus10, 90, 70, eM10 && perm.allowBetAdjust);

    drawBtnImageFit(assets.sprBet, btnBetOK, 120, 70, perm.allowBetOK);

    drawBtnImageFit(assets.sprP10, btnBetPlus10, 90, 70, eP10 && perm.allowBetAdjust);
    drawBtnImageFit(assets.sprP50, btnBetPlus50, 90, 70, eP50 && perm.allowBetAdjust);
    drawBtnImageFit(assets.sprP100, btnBetPlus100, 90, 70, eP100 && perm.allowBetAdjust);

    drawBtnImageFit(betTotal ? assets.sprCheatOn : assets.sprCheatOff,
        btnCheatToggle, 300, 80, perm.allowCheatToggle);

    const bool eCheatTarget = betTotal && perm.allowCheatTarget;
    drawBtnImageFit(assets.sprMinus, btnCheatMinus, 80, 80, eCheatTarget);
    drawBtnImageFit(assets.sprPlus, btnCheatPlus, 80, 80, eCheatTarget);

    if (betTotal) {
        float tx = btnCheatToggle.getX() + btnCheatToggle.getW() + 16.0f;
        float ty = btnCheatToggle.getY() + 24.0f;
        float a = perm.allowCheatTarget ? 1.0f : 0.35f;

        ctx.textC("TARGET: " + std::to_string(uiCheatBetTarget),
            tx, ty - 60, ctx.FS_S, ctx.FS_S,
            1.0f, 0.0f, 0.0f, a);
    }
}

//==========================
// Action UI
//==========================
void BlackjackGame::drawActionUI(const RenderCtx& ctx)
{
    const bool showAct = (state == State::PlayerTurn || state == State::CpuTurn);
    if (!showAct) return;

    int idx = currentActorIndex();
    bool enableAct = (state == State::PlayerTurn);

    bool eHit = enableAct;
    bool eStand = enableAct;
    bool eDD = enableAct && canDoubleDown(players[idx]);

    drawBtnImageFit(assets.sprHit, btnHit, 120, 70, eHit);
    drawBtnImageFit(assets.sprStand, btnStand, 120, 70, eStand);
    drawBtnImageFit(assets.sprDoubl, btnDouble, 120, 70, eDD);
}

//==========================
// RoundEnd UI（FinalResult / TutorialEnd のときは true を返して render() をreturn）
//==========================
bool BlackjackGame::drawRoundEndUI(const RenderCtx& ctx)
{
    if (state != State::RoundEnd &&
        state != State::FinalResult &&
        state != State::TutorialEnd)
    {
        return false;
    }

    const bool isFinal = (state == State::FinalResult);
    const bool isTutEnd = (state == State::TutorialEnd);
    const bool enabled = true;

    // RoundEnd: NEXT / FinalResult & TutorialEnd: NEW GAME
    Sprite* btnSpr = (isFinal || isTutEnd) ? assets.sprNewGame : assets.sprNext;

    // ボタン描画（1回だけ）
    if (btnSpr) {
        drawBtnImageFit(btnSpr, btnBetOK, 120.0f, 70.0f, enabled);
    }
    else {
        ctx.drawBtn(btnBetOK, enabled);
        ctx.textL((isFinal || isTutEnd) ? "NEW GAME" : "NEXT",
            btnBetOK.getX() + 14.0f,
            btnBetOK.getY() + ctx.labelYBet,
            ctx.FS, ctx.FS, 1.0f);
    }

    // ---- TutorialEnd はここで終了（盤面は描かない）----
    if (isTutEnd) {
        ctx.textL("TUTORIAL COMPLETE!", 460.0f, 160.0f, 1.2f, 1.2f, 1.0f);
        ctx.textL("Press NEW GAME to start real match.", 400.0f, 200.0f,
            ctx.FS_S, ctx.FS_S, 1.0f);
        return true;
    }

    // ---- RoundEnd は「盤面を残す」ので render() は止めない ----
    if (!isFinal) return false;

    // ===== ここから最終結果（FinalResult の時だけ）=====
    ctx.textL("FINAL RESULT", 520.0f, 150.0f, 1.2f, 1.2f, 1.0f);
    ctx.textL("Press NEW GAME to start.", 430.0f, 185.0f, ctx.FS_S, ctx.FS_S, 1.0f);

    struct E { int idx; long long chips; };
    std::vector<E> es;
    es.reserve(4);
    for (int i = 0; i < 4; ++i) es.push_back({ i, (long long)players[i].chips });

    std::sort(es.begin(), es.end(), [](const E& a, const E& b) {
        if (a.chips != b.chips) return a.chips > b.chips;
        return a.idx < b.idx;
        });

    int rankOf[4] = { 1,1,1,1 };
    int curRank = 1;
    for (int pos = 0; pos < 4; ++pos) {
        if (pos > 0 && es[pos].chips < es[pos - 1].chips) curRank = pos + 1;
        rankOf[es[pos].idx] = curRank;
    }

    int rankCount[5] = { 0,0,0,0,0 };
    for (int i = 0; i < 4; ++i) {
        int r = rankOf[i];
        if (1 <= r && r <= 4) rankCount[r]++;
    }
    
    auto split3LinesNumberLL = [](long long value, int chunk, std::string out[3]) {
        out[0].clear(); out[1].clear(); out[2].clear();
        std::string s = std::to_string(value);
        std::string sign;
        if (!s.empty() && s[0] == '-') { sign = "-"; s = s.substr(1); }

        int i = 0;
        if ((int)s.size() > i) { out[0] = s.substr(i, chunk); i += chunk; }
        if ((int)s.size() > i) { out[1] = s.substr(i, chunk); i += chunk; }
        if ((int)s.size() > i) { out[2] = s.substr(i); }

        if (!sign.empty() && !out[0].empty()) out[0] = sign + out[0];
        };

#if defined(__cpp_char8_t)
    auto u8to8 = [](const char8_t* s) { return std::string(reinterpret_cast<const char*>(s)); };
#else
    auto u8to8 = [](const char* s) { return std::string(s); };
#endif

    const float RX = 420.0f;
    const float RY = 240.0f;

    const float SUB = 18.0f;
    const float ROW_H = SUB * 3.0f;
    const float LBL_Y = -30.0f;

    ctx.textL("RANK", RX, RY + LBL_Y, ctx.FS_S, ctx.FS_S, 1.0f);
    ctx.textL("NAME", RX + 120, RY + LBL_Y, ctx.FS_S, ctx.FS_S, 1.0f);
    ctx.textL("CHIPS", RX + 320, RY + LBL_Y, ctx.FS_S, ctx.FS_S, 1.0f);
    ctx.textL("DIFF", RX + 500, RY + LBL_Y, ctx.FS_S, ctx.FS_S, 1.0f);

    for (int pos = 0; pos < 4; ++pos) {
        int i = es[pos].idx;
        int r = rankOf[i];
        bool tie = (rankCount[r] >= 2);

        std::string rStr = std::to_string(r) + (tie ? u8to8(u8"位タイ") : u8to8(u8"位"));

        long long chips = es[pos].chips;
        long long diff = chips - (long long)kStartChips;
        std::string dStr = (diff >= 0) ? ("+" + std::to_string(diff)) : std::to_string(diff);

        float y0 = RY + pos * ROW_H;

        ctx.textL(rStr, RX, y0, ctx.FS, ctx.FS, 1.0f);
        ctx.textL(players[i].name, RX + 120, y0, ctx.FS, ctx.FS, 1.0f);

        std::string lines[3];
        split3LinesNumberLL(chips, 6, lines);
        for (int k = 0; k < 3; ++k) {
            if (!lines[k].empty()) {
                ctx.textL(lines[k], RX + 320, y0 + k * SUB, ctx.FS, ctx.FS, 1.0f);
            }
        }

        ctx.textL(dStr, RX + 500, y0, ctx.FS, ctx.FS, 1.0f);
    }

    return true; // FinalResult のときは render() を止める
}


//==========================
// Dealer UI 
//==========================
void BlackjackGame::drawDealerUI(const RenderCtx& ctx)
{
    const float CARD_W = 64.0f;
    const float CARD_H = 32.0f;

    const float DEALER_X = 420.0f;
    const float DEALER_Y = 80.0f;
    const float DEALER_DX = CARD_W + 16.0f;

    const bool hideAllDealer =
        (state == State::Dealing) ||
        (state == State::BaseProbSwap);

     const bool hideDealerFirstOnly =
        (state == State::PlayerTurn) ||
        (state == State::CpuTurn) ||
        (state == State::Accuse) ||
        (state == State::DealerTurn && !dealerHoleRevealed);

    ctx.textL("DEALER", DEALER_X, DEALER_Y - 34.0f, ctx.FS, ctx.FS, 1.0f);

    for (int i = 0; i < dealer.hand.cardCount(); ++i) {
        float x = DEALER_X + i * DEALER_DX;
        float y = DEALER_Y;

        if (hideAllDealer) {
            drawCardBackImage(x, y);                 // 全部裏
        }
        else if (i == 0 && hideDealerFirstOnly) {
            drawCardBackImage(x, y);                 // 1枚目だけ裏
        }
        else {
            drawCardFaceImage(dealer.hand.cardAt(i), x, y);
        }
    }

    // total 表示
    if (hideAllDealer || hideDealerFirstOnly) {
        ctx.textL("total: ??", DEALER_X, DEALER_Y + CARD_H + 14.0f, ctx.FS_S, ctx.FS_S, 1.0f);
    }
    else {
        ctx.textL("total: " + std::to_string(dealer.hand.bestScore()),
            DEALER_X, DEALER_Y + CARD_H + 14.0f, ctx.FS_S, ctx.FS_S, 1.0f);
    }
}


//==========================
// Players UI
//==========================
void BlackjackGame::drawPlayersUI(const RenderCtx& ctx)
{
    const float CARD_W = 64.0f;
    const float CARD_H = 32.0f;
    const float ROW_STEP = CARD_H + 14.0f;

    // ui 参照に統一
    const float COL_X0 = ui.plColX0;
    const float COL_DX = ui.plColDx;
    const float COL_Y0 = ui.plColY0;

    const int   WRAP_AT = 3;
    const float WRAP_DX = CARD_W + 18.0f;

    const float LINE = 22.0f;
    const float KV_GAP = 150.0f;

    auto drawKV = [&](float x, float y, const std::string& key, const std::string& val) {
        ctx.textL(key, x, y, ctx.FS_S, ctx.FS_S, 1.0f);
        ctx.textL(val, x + KV_GAP, y, ctx.FS_S, ctx.FS_S, 1.0f);
        };

    // 確率交換ターンだけ：ディーラー穴札と同様に「1枚目を裏・合計??」
    const bool hidePlayersFirst = (state == State::BaseProbSwap);

    int maxRows = 0;
    for (int p = 0; p < 4; ++p) {
        int n = players[p].hand.cardCount();
        int right = (n < WRAP_AT) ? n : WRAP_AT;
        int wrap = (n > WRAP_AT) ? (n - WRAP_AT) : 0;
        int rows = (right > wrap) ? right : wrap;
        if (rows > maxRows) maxRows = rows;
    }

    float INFO_BASE_Y = COL_Y0 + (float)maxRows * ROW_STEP + ui.plInfoPadY;

    // 下のUIに被らない（ボタン座標で制限）
    const float INFO_BOTTOM_LIMIT = btnBetOK.getY() - ui.plInfoBottomGap;

    // 表示行数ぶん下端が超えるなら INFO_BASE_Y を上げる
    {
        const float LINE2 = 22.0f;
        const float LAST_Y = INFO_BASE_Y + 7.0f * LINE2;
        if (LAST_Y > INFO_BOTTOM_LIMIT) {
            INFO_BASE_Y -= (LAST_Y - INFO_BOTTOM_LIMIT);
        }
    }

    if (INFO_BASE_Y > INFO_BOTTOM_LIMIT) INFO_BASE_Y = INFO_BOTTOM_LIMIT;

    for (int p = 0; p < 4; ++p) {
        float baseX = COL_X0 + p * COL_DX;
        // このプレイヤーのカード/情報を隠すか
        const bool hideAllCards =
            (state == State::Dealing) ||
            (state == State::BaseProbSwap);

        bool isActing = false;
        if (state == State::PlayerTurn || state == State::CpuTurn) {
            isActing = (p == currentActorIndex());
        }

        bool showJudge =
            (state == State::DealerTurn) ||
            (state == State::Settle) ||
            (state == State::RoundEnd) ||
            (state == State::Accuse && accuseRevealed[p]);

        // ---- 名前色 ----
        float nr = 0.0f, ng = 0.0f, nb = 0.0f;
        if (showJudge && players[p].accusedThisRound) { nr = 1.0f; ng = 0.0f; nb = 0.0f; }
        else if (isActing) { nr = 0.0f; ng = 0.0f; nb = 1.0f; }

        std::string title = players[p].name;
        ctx.textC(title, baseX, COL_Y0 + ui.plNameYOff, ctx.FS, ctx.FS, nr, ng, nb, 1.0f);

        // ---- 判定タグ（名前の下）----
        if (showJudge) {
            std::string tag;
            float tr = 0.3f, tg = 0.3f, tb = 0.3f; // SAFEはグレー

            if (players[p].accusedThisRound) {
                if (players[p].caughtCheating) { tag = "CAUGHT"; tr = 1.0f; tg = 0.0f; tb = 0.0f; }
                else if (players[p].falseAccused) { tag = "FALSE";  tr = 1.0f; tg = 0.5f; tb = 0.0f; }
                else { tag = "ACCUSED"; tr = 1.0f; tg = 0.0f; tb = 0.0f; }
            }
            else {
                tag = "SAFE";
            }

            ctx.textC(tag, baseX, COL_Y0 + ui.plJudgeYOff, ctx.FS_S, ctx.FS_S, tr, tg, tb, 1.0f);
        }
        else if (state == State::Accuse) {
            ctx.textC("...", baseX, COL_Y0 + ui.plJudgeYOff, ctx.FS_S, ctx.FS_S,
                0.5f, 0.5f, 0.5f, 1.0f);
        }

        
        // ---- 手札 ----
        for (int i = 0; i < players[p].hand.cardCount(); ++i) {
            float x, y;
            if (i < WRAP_AT) { x = baseX;           y = COL_Y0 + i * ROW_STEP; }
            else { x = baseX + WRAP_DX;  y = COL_Y0 + (i - WRAP_AT) * ROW_STEP; }

            if (hideAllCards) drawCardBackImage(x, y);
            else              drawCardFaceImage(players[p].hand.cardAt(i), x, y);
        }


        // ---- total / bust 表示 ----
        if (hideAllCards) {
            drawKV(baseX, INFO_BASE_Y + 0 * LINE, "total:", "??");
        }
        else {
            const bool bust = players[p].hand.isBust();
            const int  score = players[p].hand.bestScore();

            
            drawKV(baseX, INFO_BASE_Y + 0 * LINE, "total:", std::to_string(score));
        }


        // ---- bet 表示 ----
        if (state == State::Betting) {
            drawKV(baseX, INFO_BASE_Y + 1 * LINE, "bet:",
                (p == 0) ? std::to_string(uiPlayerBet) : "--");
        }
        else {
            const float betY = INFO_BASE_Y + 1 * LINE;

            ctx.textL("bet:", baseX, betY, ctx.FS_S, ctx.FS_S, 1.0f);

            std::string betStr;
            if (state == State::Betting) {
                betStr = (p == 0) ? std::to_string(uiPlayerBet) : "--";
            }
            else {
                betStr = std::to_string(players[p].bet);
            }
            ctx.textL(betStr, baseX, betY + LINE, ctx.FS_S, ctx.FS_S, 1.0f);
        }

        // chips を2行表示
        const float chipsY = INFO_BASE_Y + 3 * LINE;
        ctx.textL("chips:", baseX, chipsY, ctx.FS_S, ctx.FS_S, 1.0f);
        ctx.textL(std::to_string(players[p].chips), baseX, chipsY + LINE, ctx.FS_S, ctx.FS_S, 1.0f);

        // BASE確率（BET中は表示しない）
        if (state != State::Betting) {
            int denom = players[p].baseAccuseDenom;
            float baseP = baseAccuseProb(players[p]);

            const float baseY = INFO_BASE_Y + 5 * LINE;
            ctx.textL("BASE 1/" + std::to_string(denom), baseX, baseY, ctx.FS_S, ctx.FS_S, 1.0f);
            ctx.textL(pct1(baseP), baseX, baseY + 1 * LINE, ctx.FS_S, ctx.FS_S, 1.0f);

            // ---- 状態表示----
            if (!hideAllCards) {
                std::string st;
                if (players[p].doubled) st += "DD ";
                if (players[p].hand.isBust()) st += "BUST";

                if (!st.empty()) {
                    ctx.textL(st, baseX, INFO_BASE_Y + 7 * LINE, ctx.FS_S, ctx.FS_S, 1.0f);
                }
            }
        }
    }
}


void BlackjackGame::drawBaseProbSwapUI(const RenderCtx& ctx)
{
    if (state != State::BaseProbSwap) return;

    const bool baseEnabled = !swappedThisRound;
    const bool onlyCpu1 = (tutorialActive && tutorialStep == 3);

    const bool eCpu1 = baseEnabled;
    const bool eCpu2 = baseEnabled && !onlyCpu1;
    const bool eCpu3 = baseEnabled && !onlyCpu1;
    const bool eSkip = baseEnabled;

    drawBtnImageFit(assets.sprCpu1, btnSwapCpu1, 160, 70, eCpu1);
    drawBtnImageFit(assets.sprCpu2, btnSwapCpu2, 160, 70, eCpu2, kBtnAlphaLocked); // ロックは更に薄く等
    drawBtnImageFit(assets.sprCpu3, btnSwapCpu3, 160, 70, eCpu3, kBtnAlphaLocked);
    drawBtnImageFit(assets.sprSkip, btnSwapSkip, 160, 70, eSkip);
}
static void drawSpriteFitRect(Sprite* spr, float x, float y, float w, float h,
    float srcW, float srcH, float a = 1.0f)
{
    if (!spr) return;
    float sx = w / srcW;
    float sy = h / srcH;
    sprite_render(spr, x, y, sx, sy, 0, 0, srcW, srcH, 0, 0, 0, 1, 1, 1, a, false);
}

void BlackjackGame::drawPauseUI(const RenderCtx& ctx)
{
    if (state != State::PauseMenu && state != State::PauseInfo) return;

    // 暗幕
    GameLib::setBlendMode(Blender::BS_ALPHA);
    primitive::rect(0, 0, (float)SCREEN_W, (float)SCREEN_H, 0, 0, 0, 0, 0, 0, 0.5f);
        

    if (state == State::PauseMenu) {
        // 6ボタン（全部 500x200）
        drawBtnImageFit(assets.titleBtn,       btnPTitle,  227, 85, true);
        drawBtnImageFit(assets.sprReturnGame,  btnPReturn, 500, 200, true);
        drawBtnImageFit(assets.sprWhatBJ,      btnPWhatBJ, 500, 200, true);
        drawBtnImageFit(assets.sprCheat1,      btnPCheat1, 500, 200, true);
        drawBtnImageFit(assets.sprCheat2,      btnPCheat2, 500, 200, true);
        drawBtnImageFit(assets.sprCheat3,      btnPCheat3, 500, 200, true);
        drawBtnImageFit(assets.sprMultBtn,     btnPMult,   500, 200, true);
        return;
    }

    //========================
    // PauseInfo：左上「戻る」だけ + 中央説明
    //========================
    
    if (pausePage == PausePage::Multiplier) {
        float w = 700, h = 350;
        float x = ((float)SCREEN_W - w) * 0.5f;
        float y = ((float)SCREEN_H - h) * 0.5f;
        drawSpriteFitRect(assets.sprMultInfo, x, y, w, h, 700, 350, 1.0f);
    }
     
    const float M = 50.0f;
    const float dstX = M;
    const float dstY = M;
    const float dstW = (float)SCREEN_W - M * 2.0f;
    const float dstH = (float)SCREEN_H - M * 2.0f;

    Sprite* infoSpr = nullptr;
    if (pausePage == PausePage::WhatBJ) infoSpr = assets.spr0;
    else if (pausePage == PausePage::Cheat1) infoSpr = assets.spr1;
    else if (pausePage == PausePage::Cheat2) infoSpr = assets.spr2;
    else if (pausePage == PausePage::Cheat3) infoSpr = assets.spr3;

    if (infoSpr) {
        drawSpriteFitRect(infoSpr, dstX, dstY, dstW, dstH, 1280, 720, 1.0f);
    }

    // 戻るボタンは「一番上」にしたいので最後に描画
    drawBtnImageFit(assets.sprBackBtn, btnPBack, 500, 200, true);
}




//================================================
// render 
//================================================
void BlackjackGame::render()
{
    GameLib::clear(1, 1, 1);
    sprite_render(assets.gameBg, 0, 0);
    GameLib::setBlendMode(Blender::BS_ALPHA);

    //========================
    // 文字描画まわり（あなたの元コードを維持）
    //========================
    const int   FONT = 2;
    const float FS = 1.0f;
    const float FS_S = 0.9f;
    const float TXT_R = 0.0f, TXT_G = 0.0f, TXT_B = 0.0f, TXT_A = 1.0f;

    auto textL = [&](const std::string& s, float x, float y, float sx, float sy, float a = 1.0f) {
        text_outL(FONT, s, x, y, sx, sy, TXT_R, TXT_G, TXT_B, a);
        };
    auto textC = [&](const std::string& s, float x, float y, float sx, float sy,
        float r, float g, float b, float a = 1.0f)
        {
            font::textOut(FONT, s, x, y, sx, sy, r, g, b, a);
        };

    auto measureW = [&](const std::string& s, float sx, float sy) -> float {
        return text_outL(FONT, s, -10000.0f, -10000.0f, sx, sy, 0, 0, 0, 0);
        };

    auto drawBtn = [&](Button& b, bool enabled = true) {
        if (enabled) b.draw(0.15f, 0.15f, 0.15f, 1.0f);
        else         b.draw(0.60f, 0.60f, 0.60f, 0.75f);
        };

    auto drawBtnTextCenter = [&](Button& b, const std::string& s,
        float sx, float sy, float yOffset, bool enabled)
        {
            drawBtn(b, enabled);
            float w = measureW(s, sx, sy);
            textL(s,
                b.getX() + (b.getW() - w) * 0.5f,
                b.getY() + yOffset,
                sx, sy, 1.0f);
        };

    RenderCtx ctx;
    ctx.FS = FS;
    ctx.FS_S = FS_S;
    ctx.labelYBet = ui.labelYBet;
    ctx.labelYCheat = ui.labelYCheat;

    ctx.textL = textL;
    ctx.measureW = measureW;
    ctx.drawBtn = drawBtn;
    ctx.drawBtnTextCenter = drawBtnTextCenter;
    ctx.textC = textC;
    // ctx を作った直後あたりに追加
    if (tutorialActive && state == State::TutorialIntro) {

        // 暗幕（任意）
        GameLib::setBlendMode(Blender::BS_ALPHA);
        primitive::rect(0, 0,
            (float)SCREEN_W, (float)SCREEN_H,
            0, 0, 0,
            0, 0, 0, 0.5f);

        // 0~3.png を取得
        Sprite* spr = getTutorialIntroSprite();

        if (spr) {
            // 画面端から 縦50/横50 空けて縮小表示
            const float PAD_X = 50.0f;
            const float PAD_Y = 50.0f;

            const float x = PAD_X;
            const float y = PAD_Y;
            const float w = (float)SCREEN_W - PAD_X * 2.0f;
            const float h = (float)SCREEN_H - PAD_Y * 2.0f;

            // 元画像は 1280x720
            drawSpriteFitRect(spr, x, y, w, h, 1280, 720, 1.0f);
        }

        // NEXTボタン
        if (assets.sprNext) {
            drawBtnImageFit(assets.sprNext, btnBetOK, 120.0f, 70.0f, true);
        }
        

        // 最後にフェード
        fade.Draw();
        return; // ここ重要：下のUIを描かない
    }

    
    


    //================================================
    // 通常レイアウト（TutorialIntro以外）
    //================================================
    layoutButtons();

    if (state == State::PauseMenu) {
        layoutPauseMenuButtons();
    }
    else if (state == State::PauseInfo) {
        layoutPauseInfoButtons();
    }

    //================================================
    // 通常描画フロー
    //================================================
    drawTopUI(ctx);
    drawTitleUI(ctx);
    drawTutorialSkipUI(ctx);

    drawBetUI(ctx);
    drawBaseProbSwapUI(ctx);
    drawActionUI(ctx);

    const bool stopBelow = drawRoundEndUI(ctx);

    if (!stopBelow) {
        drawDealerUI(ctx);
        drawPlayersUI(ctx);
    }

    // PAUSEは盤面の上に
    drawPauseUI(ctx);

    // 最後にフェード
    fade.Draw();
}