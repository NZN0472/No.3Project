#include "Blackjack.h"
#include "../GameLib/game_lib.h"
#include "all.h"
#include "Timer.h"


#include <algorithm>
#include <random>
#include <sstream>
#include <cmath>

//================================================
// 画像（cpp内だけ）
//================================================
static Sprite* sprBJKA_B = nullptr;     // Black_JQKA.png  (64x64, 2x2)
static Sprite* sprBJKA_R = nullptr;     // Red_JQKA.png    (64x64, 2x2)

static Sprite* sprB36_B = nullptr;     // Black3_4_5_6.png (64x64, 2x2)
static Sprite* sprB36_R = nullptr;     // Red3_4_5_6.png   (64x64, 2x2)

static Sprite* sprB710_B = nullptr;     // Black7_8_9_10.png (64x64, 2x2)
static Sprite* sprB710_R = nullptr;     // Red7_8_9_10.png   (64x64, 2x2)

static Sprite* spr2Jo = nullptr;     // 2_jo.png (64x64, 上段:2黒/2赤 下段:jo(左下))
static Sprite* sprMark = nullptr;     // mark.png (64x64, 2x2)
static Sprite* sprBack = nullptr;     // backCard.png (64x64)
static Sprite* sprBackBJ = nullptr;   // Blackjack用 64x32 裏
static Sprite* titleBtn = nullptr;
static Sprite* bet = nullptr;
static Sprite* plus = nullptr;
static Sprite* minus = nullptr;
static Sprite* game = nullptr;

//================================================
// ボタン配置（ここだけ触れば全体が揃う）
//================================================
void BlackjackGame::layoutButtons()
{
    // 画面下に揃える
    ui.btnY = (float)SCREEN_H - 100.0f;

    // ---- BET列 ----
    float x = ui.betX0;

    btnBetMinus100.setRect(x, ui.btnY, ui.betBW, ui.betBH); x += ui.betBW + ui.betGap;
    btnBetMinus50.setRect(x, ui.btnY, ui.betBW, ui.betBH); x += ui.betBW + ui.betGap;
    btnBetMinus10.setRect(x, ui.btnY, ui.betBW, ui.betBH); x += ui.betBW + ui.betGap;

    btnBetOK.setRect(x, ui.btnY, ui.betOkW, ui.betBH); x += ui.betOkW + ui.betGap;

    btnBetPlus10.setRect(x, ui.btnY, ui.betBW, ui.betBH); x += ui.betBW + ui.betGap;
    btnBetPlus50.setRect(x, ui.btnY, ui.betBW, ui.betBH); x += ui.betBW + ui.betGap;
    btnBetPlus100.setRect(x, ui.btnY, ui.betBW, ui.betBH);

    // ---- Action ----
    btnHit.setRect(ui.actXHit, ui.btnY, ui.actBW, ui.actBH);
    btnStand.setRect(ui.actXStand, ui.btnY, ui.actBW, ui.actBH);
    btnDouble.setRect(ui.actXDouble, ui.btnY, ui.actBW, ui.actBH);

    // ---- Title ----
    btnToTitle.setRect(ui.titleX, ui.titleY, ui.titleW, ui.titleH);

    // ---- CHEAT ----
    ui.cheatY = ui.btnY - 90.0f;
    btnCheatToggle.setRect(40.0f, ui.cheatY, 300.0f, 80.0f);
    btnCheatMinus.setRect(350.0f, ui.cheatY, 80.0f, 80.0f);
    btnCheatPlus.setRect(440.0f, ui.cheatY, 80.0f, 80.0f);

    // ---- SWAP ----
    ui.swapY = ui.cheatY - 20.0f;
    btnSwapCpu1.setRect(40.0f, ui.swapY, 160.0f, 70.0f);
    btnSwapCpu2.setRect(210.0f, ui.swapY, 160.0f, 70.0f);
    btnSwapCpu3.setRect(380.0f, ui.swapY, 160.0f, 70.0f);
    btnSwapSkip.setRect(550.0f, ui.swapY, 160.0f, 70.0f);

    ////ボタンsprite/////
    


}






//================================================
// 切り抜き設定
//================================================
static constexpr float SHEET = 64.0f;   // 画像全体
static constexpr float TILE = 32.0f;   // 32x32 を切り抜く（2x2）

static int clampInt(int v, int lo, int hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

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

static void forceBust(BJHand& hand)
{
    // 10を足し続ければ必ず21を超える（Aの調整があってもいずれ超える）
    while (hand.bestScore() <= 21) {
        hand.add(makeCardByValue(10));
    }
}

static void cheatDrawTo21OrBust(BJParticipant& p)
{
    p.cheatedThisRound = true;

    int now = p.hand.bestScore();
    int need = 21 - now;

    if (need == 0) {
        // 初手21 → 必ずバースト
        forceBust(p.hand);
    }
    else if (1 <= need && need <= 10) {
        // 1枚で21にできる
        p.hand.add(makeCardByValue(need));
    }
    else {
        // 1枚で21にできない（例：合計が低すぎる）→とりあえず10（ここは仕様次第で調整可）
        p.hand.add(makeCardByValue(10));
    }

    p.stood = true;
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

// 1枚絵(64x64)を描画（裏面など）
static void drawFull64(Sprite* sheet, float x, float y, float size) {
    if (!sheet) return;

    const float scale = size / SHEET;
    sprite_render(sheet,
        x, y, scale, scale,
        0, 0, SHEET, SHEET,
        0, 0, 0,
        1, 1, 1, 1,
        false
    );
}

//最終結果表示用
static void split3LinesNumber(const std::string& sIn, int chunk,
    std::string out[3])
{
    out[0].clear(); out[1].clear(); out[2].clear();

    std::string s = sIn;
    std::string sign;
    if (!s.empty() && s[0] == '-') { sign = "-"; s = s.substr(1); }

    int i = 0;
    if ((int)s.size() > i) { out[0] = s.substr(i, chunk); i += chunk; }
    if ((int)s.size() > i) { out[1] = s.substr(i, chunk); i += chunk; }
    if ((int)s.size() > i) { out[2] = s.substr(i); }

    if (!sign.empty() && !out[0].empty()) out[0] = sign + out[0];
    if (!sign.empty() && out[0].empty())  out[0] = sign; // 念のため
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
bool BJHand::isBlackjack() const { return cards.size() == 2 && bestScore() == 21; }

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

    pendingFinalResult = false;
   
    game = sprite_load(L"./Data/Images/game.png");

    sprBJKA_B = sprite_load(L"Data/Images/Black_JQKA.png");
    sprBJKA_R = sprite_load(L"Data/Images/Red_JQKA.png");

    sprB36_B = sprite_load(L"Data/Images/Black3_4_5_6.png");
    sprB36_R = sprite_load(L"Data/Images/Red3_4_5_6.png");

    sprB710_B = sprite_load(L"Data/Images/Black7_8_9_10.png");
    sprB710_R = sprite_load(L"Data/Images/Red7_8_9_10.png");

    spr2Jo = sprite_load(L"Data/Images/2_jo.png");
    sprMark = sprite_load(L"Data/Images/mark.png");
    sprBack = sprite_load(L"Data/Images/backCard.png");

    sprBackBJ = sprite_load(L"Data/Images/D_back.png"); 

    titleBtn = sprite_load(L"./Data/Images/titleBtn2.png");
    bet = sprite_load(L"./Data/Images/bet2.png");
    plus = sprite_load(L"./Data/Images/+.png");
    minus = sprite_load(L"./Data/Images/-.png");


    roundNo = 0;
    matchOver = false;

    buildGuaranteeTargets(); // 試合開始時の保証ターゲットを作る

    toBetting();
}

void BlackjackGame::deinit() {
    safe_delete(sprBJKA_B);
    safe_delete(sprBJKA_R);
    safe_delete(sprB36_B);
    safe_delete(sprB36_R);
    safe_delete(sprB710_B);
    safe_delete(sprB710_R);
    safe_delete(spr2Jo);
    safe_delete(sprMark);
    safe_delete(sprBack);
    safe_delete(sprBackBJ);
    safe_delete(titleBtn);
    safe_delete(bet);
    safe_delete(plus);
    safe_delete(minus);
    safe_delete(game);

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
        state = State::Accuse;
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

void BlackjackGame::runAccusePhase()
{
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

    // 2) 通常抽選（各人個別）
    auto doLotteryOnce = [&]() {
        for (int i = 0; i < 4; ++i) {
            float pe = effectiveAccuseProb(players[i]);
            std::bernoulli_distribution bd(pe);
            if (bd(gameRng())) players[i].accusedThisRound = true;
        }
        };

    // R5 は「誰も選ばれなかったら再抽選（当たるまで）」
    if (roundNo == 5) {
        while (true) {
            // 一度クリアして抽選
            for (auto& pl : players) pl.accusedThisRound = false;
            doLotteryOnce();

            bool any = false;
            for (auto& pl : players) if (pl.accusedThisRound) { any = true; break; }
            if (any) break;
        }
    }
    else {
        // R1〜R4：保証＋通常抽選
        doLotteryOnce();
    }

    // 3) 冤罪/正解の判定（今は cheatedThisRound を基準）
    for (auto& pl : players) {
        if (!pl.accusedThisRound) continue;

        if (pl.cheatedThisRound) {
            pl.caughtCheating = true;   // 正解指摘
        }
        else {
            pl.falseAccused = true;     // 冤罪
        }
    }

    // メッセージ（任意）
    std::string msg = "ACCUSE: ";
    bool first = true;
    for (int i = 0; i < 4; ++i) {
        if (!players[i].accusedThisRound) continue;
        if (!first) msg += ", ";
        msg += players[i].name;
        first = false;
    }
    if (first) msg += "none"; // 基本来ない（R1-4保証/R5再抽選）
    setMsg(msg);
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


}


void BlackjackGame::toBetting()
{
    state = State::Betting;

    // ここで毎回リセットしておくと「タイトル→再開始で前の手札が残る」事故を防げる
    pendingFinalResult = false;
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
    activeCpuIndex = 1;

    // 5ラウンド終了後にOK押したら「新しい5ラウンド」を始める設計にする
    if (matchOver) {
        matchOver = false;
        roundNo = 0;

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
    if (matchOver) {
        state = State::RoundEnd;
        return;
    }

    state = State::Dealing;
    beginRound();

    // ラウンド進行
    roundNo++;
    if (roundNo > kMaxRounds) {
        matchOver = true;
        state = State::RoundEnd;
        return;
    }

    // ===== ベース指摘確率（毎ラウンド配布） =====
    assignBaseAccuseProbs();

    // ===== 手番順ローテ（毎ラウンド） =====
    setupTurnOrderForRound();

    //========================
    // YOU bet（借金OK）
    //========================
    int betV = normalizeBet(uiPlayerBet, kMinBet, kBetStep, kMaxUserBet);
    players[0].bet = betV;
    players[0].chips -= betV;

    //========================
    // CPU bet（所持額に応じて10%刻みランダム）
    //========================
    for (int i = 1; i <= 3; ++i) {
        int cpuBet = cpuRandomBet(players[i].chips, kMinBet, kBetStep);
        players[i].bet = cpuBet;
        players[i].chips -= cpuBet;
    }

    //========================
    // 配る（全員2枚 + Dealer2枚）
    //========================
    for (int k = 0; k < 2; ++k) {
        for (auto& p : players) p.hand.add(deck.draw());
        dealer.hand.add(deck.draw());
    }

    //========================
// ベットOK時イカサマ（スルー時：後で強制）
// - ONなら：ここで 17..21 に固定（cheatedThisRound=true, mustCheatLater=false）
// - OFFなら：mustCheatLater=true（スタンド不可、ヒット/ダブルで強制イカサマ）
//========================

// 配る（全員2枚 + Dealer2枚）した「あと」に入れるのが分かりやすい
// （あなたのコードだと配った後にブロックがありますね。そこを置き換え）

// まず全員リセット
    for (auto& p : players) {
        p.cheatMode = CheatMode::None;
        p.cheatedThisRound = false;
        p.cheatBetTarget = 21;
    }

    // YOU：ベット画面で選んだモード
    players[0].cheatMode = uiCheatMode;
    players[0].cheatBetTarget = uiCheatBetTarget;

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


    // BetTotal の人は、初手を固定（=ベット時チート）
    for (auto& p : players) {
        if (p.cheatMode == CheatMode::BetTotal) {
            int t = p.cheatBetTarget;
            if (t < 17) t = 17;
            if (t > 21) t = 21;

            rigHandToTotal(p.hand, t);
            p.cheatedThisRound = true;        // 1R1回使用済み扱い
            setMsg(p.name + " CHEAT(BET)");
        }
    }

    toBaseProbSwap(); // 交換フェーズへ

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



void BlackjackGame::update()
{
    float dt = Timer::getInstance()->getDeltaTime();
    if (dt > 0.1f) dt = 0.1f; // デバッグ停止後の暴走防止（任意）
    // クリック判定と描画を一致させる
    layoutButtons();

    btnToTitle.update();
    if (btnToTitle.isClicked()) {

        // 次回スタート時に前回の手札が残らないようにクリア
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

        activeCpuIndex = 1;
        state = State::Betting;

        extern int nextScene;
        nextScene = SCENE_TITLE;
        return;
    }

    switch (state) {
    case State::Betting: {
        btnBetMinus100.update();
        btnBetMinus50.update();
        btnBetMinus10.update();
        btnBetPlus10.update();
        btnBetPlus50.update();
        btnBetPlus100.update();
        btnBetOK.update();

       
        btnCheatToggle.update();
        btnCheatMinus.update();
        btnCheatPlus.update();

        if (btnCheatToggle.isClicked()) {
            uiCheatMode = (uiCheatMode == CheatMode::None)
                ? CheatMode::BetTotal
                : CheatMode::None;
        }


        // BetTotalの時だけ 17..21 をいじれる
        if (uiCheatMode == CheatMode::BetTotal) {
            if (btnCheatMinus.isClicked()) uiCheatBetTarget--;
            if (btnCheatPlus.isClicked())  uiCheatBetTarget++;
            if (uiCheatBetTarget < 17) uiCheatBetTarget = 17;
            if (uiCheatBetTarget > 21) uiCheatBetTarget = 21;
        }

        

        if (btnBetMinus100.isClicked()) uiPlayerBet -= 100;
        if (btnBetMinus50.isClicked())  uiPlayerBet -= 50;
        if (btnBetMinus10.isClicked())  uiPlayerBet -= 10;

        if (btnBetPlus10.isClicked())   uiPlayerBet += 10;
        if (btnBetPlus50.isClicked())   uiPlayerBet += 50;
        if (btnBetPlus100.isClicked())  uiPlayerBet += 100;

        uiPlayerBet = normalizeBet(uiPlayerBet, kMinBet, kBetStep, kMaxUserBet);

        if (btnBetOK.isClicked()) toDealing();
        break;
    }
    case State::BaseProbSwap: {

        btnSwapCpu1.update();
        btnSwapCpu2.update();
        btnSwapCpu3.update();
        btnSwapSkip.update();

        if (btnSwapCpu1.isClicked()) {
            swapBaseDenom(0, 1);
            setMsg("SWAP: YOU <-> CPU1");
        }
        else if (btnSwapCpu2.isClicked()) {
            swapBaseDenom(0, 2);
            setMsg("SWAP: YOU <-> CPU2");
        }
        else if (btnSwapCpu3.isClicked()) {
            swapBaseDenom(0, 3);
            setMsg("SWAP: YOU <-> CPU3");
        }
        else if (btnSwapSkip.isClicked()) {
            setMsg("SWAP: SKIP");
        }
        else {
            break; // まだ選んでない
        }

        // 交換 or スキップしたらターン開始へ
        int first = currentActorIndex();
        if (first == 0) toPlayerTurn();
        else            toCpuTurn();
        setMsg("TURN START: " + players[first].name);
        break;
    }


    case State::PlayerTurn: {
        btnHit.update();
        btnStand.update();
        btnDouble.update();

        int idx = currentActorIndex();
        BJParticipant& you = players[idx];

       

        //========================
        // 通常行動
        //========================
        if (!you.stood) {
            if (btnDouble.isClicked() && canDoubleDown(you)) doDoubleDown(you);
            else if (btnHit.isClicked())                     doHit(you);
            else if (btnStand.isClicked())                   doStand(you);
        }

        if (you.stood) {
            advanceActor();
        }

        break;
    }





    case State::CpuTurn: {
        int idx = currentActorIndex();
        BJParticipant& cpu = players[idx];

        // 行動者が変わったら待ち時間リセット（次のCPUにすぐ行動させない）
        if (idx != lastCpuIdx) {
            lastCpuIdx = idx;
            cpuWait = 0.0f;
        }

        cpuWait += dt;
        if (cpuWait < kActInterval) break;   // まだ待つ

        cpuWait = 0.0f;                      // 2秒経ったので1回行動
        cpuAct(cpu);                         // ここが「1回ぶん」だけ実行される

        if (cpu.stood) {
            advanceActor();                  // 次の人へ（次のCPUになったら上でリセットされる）
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
            }
            break; // REVEAL中はHITしない
        }

        // REVEAL後に、1秒ごとに1回だけ行動
        dealerWait += dt;
        if (dealerWait < kActInterval) break;
        dealerWait = 0.0f;

        if (dealer.hand.bestScore() < 17) {
            dealer.hand.add(deck.draw());
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
            pendingFinalResult = true;     // NEXTで最終結果へ
            toRoundEnd("ROUND 5 END");     // 盤面を残したままRoundEndへ
        }
        else {
            toRoundEnd("ROUND END");
        }
        break;
    }



    case State::RoundEnd: {
        btnBetOK.update(); // NEXT

        if (btnBetOK.isClicked()) {
            if (matchOver) {
                // ラウンド5盤面を残したまま、NEXTで最終結果へ
                state = State::FinalResult;
            }
            else {
                toBetting(); // 次ラウンドへ
            }
        }
        break;
    }

    case State::FinalResult: {
        btnBetOK.update(); // NEW GAME

        if (btnBetOK.isClicked()) {
            // NEW GAME開始
            toBetting();
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

            std::string msg = "ACCUSE CHECK: " + players[i].name + " -> ";
            if (players[i].accusedThisRound) {
                msg += (players[i].caughtCheating ? "CAUGHT" : "ACCUSED");
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
    if (rank == 11) { drawSheet_2x2_32(red ? sprBJKA_R : sprBJKA_B, 0, 0, x, y, size); return; } // J
    if (rank == 12) { drawSheet_2x2_32(red ? sprBJKA_R : sprBJKA_B, 1, 0, x, y, size); return; } // Q
    if (rank == 13) { drawSheet_2x2_32(red ? sprBJKA_R : sprBJKA_B, 0, 1, x, y, size); return; } // K
    if (rank == 1) { drawSheet_2x2_32(red ? sprBJKA_R : sprBJKA_B, 1, 1, x, y, size); return; } // A

    // 2_jo：上段に2(黒/赤)
    if (rank == 2) {
        drawSheet_2x2_32(spr2Jo, red ? 1 : 0, 0, x, y, size);
        return;
    }

    // 3-6（2x2）
    if (3 <= rank && rank <= 6) {
        // [3 4]
        // [5 6]
        int idx = rank - 3;        // 0..3
        int col = idx % 2;         // 0,1
        int row = idx / 2;         // 0,1
        drawSheet_2x2_32(red ? sprB36_R : sprB36_B, col, row, x, y, size);
        return;
    }

    // 7-10（2x2）
    if (7 <= rank && rank <= 10) {
        // [7 8]
        // [9 10]
        int idx = rank - 7;        // 0..3
        int col = idx % 2;
        int row = idx / 2;
        drawSheet_2x2_32(red ? sprB710_R : sprB710_B, col, row, x, y, size);
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
    drawSheet_2x2_32(sprMark, col, row, x, y, size);
}

void BlackjackGame::drawCardFaceImage(const BJCard& c, float x, float y) {
    const float half = 32.0f; // 左右それぞれ32
    drawRankImage(c.rank, c.suit, x, y, half);
    drawSuitImage(c.suit, x + half, y, half);
}

void BlackjackGame::drawCardBackImage(float x, float y) {
    if (!sprBackBJ) return;

    // 64x32をそのまま描く（等倍）
    sprite_render(sprBackBJ,
        x, y, 1.0f, 1.0f,
        0, 0, 64, 32,
        0, 0, 0,
        1, 1, 1, 1,
        false
    );
}





//================================================
// 文字描画：align を指定しない（デフォルト左揃え）
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
    ctx.textL("ROUND: " + std::to_string(roundNo) + " / " + std::to_string(kMaxRounds),
        ui.topRoundX, ui.topRoundY, 1.0f, 1.0f, 1.0f);

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

    if (state == State::CpuTurn) {
        float rem = kActInterval - cpuWait;
        if (rem < 0) rem = 0;
        float v = (int)(rem * 10) / 10.0f;
        ctx.textL("NEXT ACT IN: " + std::to_string(v) + "s", ui.topNextX, ui.topNextY, ctx.FS_S, ctx.FS_S, 1.0f);
    }

    if (state == State::DealerTurn) {
        float rem = dealerHoleRevealed ? (kActInterval - dealerWait) : (kActInterval - dealerRevealTimer);
        if (rem < 0) rem = 0;
        float v = (int)(rem * 10) / 10.0f;
        ctx.textL("NEXT ACT IN: " + std::to_string(v) + "s", ui.topNextX, ui.topNextY, ctx.FS_S, ctx.FS_S, 1.0f);
    }
}


//==========================
// TITLEボタン（左上）
//==========================
void BlackjackGame::drawTitleUI(const RenderCtx& ctx)
{
    ctx.drawBtn(btnToTitle, true);
    ctx.textL("TITLE", 60.0f, 80.0f, ctx.FS, ctx.FS, 1.0f);

    
    if (titleBtn) sprite_render(titleBtn, 40, 70, 0.792f, 0.82f);
}

//==========================
// BET UI
//==========================
void BlackjackGame::drawBetUI(const RenderCtx& ctx)
{
    if (state != State::Betting) return;

    ctx.drawBtnTextCenter(btnBetMinus100, "-100", ctx.FS_S, ctx.FS_S, ctx.labelYBet, true);
    ctx.drawBtnTextCenter(btnBetMinus50, "-50", ctx.FS_S, ctx.FS_S, ctx.labelYBet, true);
    ctx.drawBtnTextCenter(btnBetMinus10, "-10", ctx.FS_S, ctx.FS_S, ctx.labelYBet, true);
    ctx.drawBtnTextCenter(btnBetOK, "OK", ctx.FS, ctx.FS, ctx.labelYBet, true);
    ctx.drawBtnTextCenter(btnBetPlus10, "+10", ctx.FS_S, ctx.FS_S, ctx.labelYBet, true);
    ctx.drawBtnTextCenter(btnBetPlus50, "+50", ctx.FS_S, ctx.FS_S, ctx.labelYBet, true);
    ctx.drawBtnTextCenter(btnBetPlus100, "+100", ctx.FS_S, ctx.FS_S, ctx.labelYBet, true);

    const bool betTotal = (uiCheatMode == CheatMode::BetTotal);

    ctx.drawBtn(btnCheatToggle, true);
    ctx.drawBtn(btnCheatMinus, betTotal);
    ctx.drawBtn(btnCheatPlus, betTotal);

    // トグル文字：左寄せ
    {
        std::string cap = betTotal ? "CHEAT: BET" : "CHEAT: OFF";
        const float PAD = 12.0f;
        ctx.textL(cap,
            btnCheatToggle.getX() + PAD,
            btnCheatToggle.getY() + ctx.labelYCheat,
            ctx.FS_S, ctx.FS_S, 1.0f);
    }

    // - / + 中央
    {
        float w = ctx.measureW("-", ctx.FS, ctx.FS);
        ctx.textL("-",
            btnCheatMinus.getX() + (btnCheatMinus.getW() - w) * 0.5f,
            btnCheatMinus.getY() + ctx.labelYCheat,
            ctx.FS, ctx.FS, 1.0f);
    }
    {
        float w = ctx.measureW("+", ctx.FS, ctx.FS);
        ctx.textL("+",
            btnCheatPlus.getX() + (btnCheatPlus.getW() - w) * 0.5f,
            btnCheatPlus.getY() + ctx.labelYCheat,
            ctx.FS, ctx.FS, 1.0f);
    }

    // TARGET（BetTotalの時だけ）
    if (betTotal) {
        ctx.textL("TARGET: " + std::to_string(uiCheatBetTarget),
            btnCheatToggle.getX(),
            btnCheatToggle.getY() - 26.0f,
            ctx.FS_S, ctx.FS_S, 1.0f);
    }

    // BET表示（+ボタンの右）
    ctx.textL("BET: " + std::to_string(uiPlayerBet),
        (btnCheatPlus.getX() + btnCheatPlus.getW()) + 16.0f,
        btnCheatPlus.getY() + ctx.labelYCheat,
        ctx.FS_S, ctx.FS_S, 1.0f);
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

    auto drawLeft = [&](Button& b, const std::string& s, bool enabled) {
        ctx.drawBtn(b, enabled);
        ctx.textL(s, b.getX() + ui.padL, b.getY() + ui.btnTextYO, ctx.FS, ctx.FS, 1.0f);
        };

    drawLeft(btnHit, "HIT", enableAct);
    drawLeft(btnStand, "STAND", enableAct);
    drawLeft(btnDouble, "DD", enableAct && canDoubleDown(players[idx]));
}


//==========================
// RoundEnd UI（matchOver最終結果なら true を返して render() をreturn）
//==========================
bool BlackjackGame::drawRoundEndUI(const RenderCtx& ctx)
{
    if (state != State::RoundEnd && state != State::FinalResult) return false;

    const bool final = (state == State::FinalResult);

    // ボタン文字：RoundEnd は NEXT、FinalResult は NEW GAME
    {
        std::string cap = final ? "NEW " : "NEXT";
        const float PAD = 14.0f; // 左余白
        ctx.drawBtn(btnBetOK, true);
        ctx.textL(cap,
            btnBetOK.getX() + PAD,
            btnBetOK.getY() + ctx.labelYBet,
            ctx.FS, ctx.FS, 1.0f);
    }

    // RoundEnd では盤面を残したいので、ここで止めない（盤面描画に続く）
    if (!final) return false;

    // ===== ここから最終結果（FinalResult の時だけ）=====
    ctx.textL("FINAL RESULT", 520.0f, 150.0f, 1.2f, 1.2f, 1.0f);
    ctx.textL("Press OK to start NEW GAME", 430.0f, 185.0f, ctx.FS_S, ctx.FS_S, 1.0f);

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

    // ---- CHIPS を3行に分割する ----
    auto split3LinesNumber = [](long long value, int chunk, std::string out[3]) {
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

    const float RX = 420.0f;
    const float RY = 240.0f;

    const float SUB = 18.0f;        // 1行の行間（CHIPSの3行用）
    const float ROW_H = SUB * 3.0f;   // 1人ぶんの縦サイズ
    const float LBL_Y = -30.0f;

    ctx.textL("RANK", RX, RY + LBL_Y, ctx.FS_S, ctx.FS_S, 1.0f);
    ctx.textL("NAME", RX + 120, RY + LBL_Y, ctx.FS_S, ctx.FS_S, 1.0f);
    ctx.textL("CHIPS", RX + 320, RY + LBL_Y, ctx.FS_S, ctx.FS_S, 1.0f);
    ctx.textL("DIFF", RX + 500, RY + LBL_Y, ctx.FS_S, ctx.FS_S, 1.0f);

    for (int pos = 0; pos < 4; ++pos) {
        int i = es[pos].idx;
        int r = rankOf[i];
        bool tie = (rankCount[r] >= 2);

        std::string rStr = std::to_string(r) + (tie ? u8"位タイ" : u8"位");

        long long chips = es[pos].chips;
        long long diff = chips - (long long)kStartChips;
        std::string dStr = (diff >= 0) ? ("+" + std::to_string(diff)) : std::to_string(diff);

        float y0 = RY + pos * ROW_H;

        ctx.textL(rStr, RX, y0, ctx.FS, ctx.FS, 1.0f);
        ctx.textL(players[i].name, RX + 120, y0, ctx.FS, ctx.FS, 1.0f);

        // CHIPS 3行
        std::string lines[3];
        split3LinesNumber(chips, 6, lines); // 6桁ごと（好みで4〜7に変更OK）
        for (int k = 0; k < 3; ++k) {
            if (!lines[k].empty()) {
                ctx.textL(lines[k], RX + 320, y0 + k * SUB, ctx.FS, ctx.FS, 1.0f);
            }
        }

        ctx.textL(dStr, RX + 500, y0, ctx.FS, ctx.FS, 1.0f);
    }

    return true; // FinalResult のときだけ render() をここで止める
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

    const bool hideDealerFirst =
        (state == State::Dealing) ||
        (state == State::BaseProbSwap) ||
        (state == State::PlayerTurn) ||
        (state == State::CpuTurn) ||
        (state == State::Accuse) ||
        (state == State::DealerTurn && !dealerHoleRevealed);

    ctx.textL("DEALER", DEALER_X, DEALER_Y - 34.0f, ctx.FS, ctx.FS, 1.0f);

    for (int i = 0; i < dealer.hand.cardCount(); ++i) {
        float x = DEALER_X + i * DEALER_DX;
        float y = DEALER_Y;

        if (i == 0 && hideDealerFirst) drawCardBackImage(x, y);
        else                           drawCardFaceImage(dealer.hand.cardAt(i), x, y);
    }

    if (hideDealerFirst) {
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

    //  ui 参照に統一
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
    if (INFO_BASE_Y > INFO_BOTTOM_LIMIT) INFO_BASE_Y = INFO_BOTTOM_LIMIT;

    for (int p = 0; p < 4; ++p) {
        float baseX = COL_X0 + p * COL_DX;

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
                else { tag = "ACCUSED";tr = 1.0f; tg = 0.0f; tb = 0.0f; }
            }
            else {
                tag = "SAFE";
            }

            ctx.textC(tag, baseX, COL_Y0 + ui.plJudgeYOff, ctx.FS_S, ctx.FS_S, tr, tg, tb, 1.0f);
        }
        else if (state == State::Accuse) {
            ctx.textC("...", baseX, COL_Y0 + ui.plJudgeYOff, ctx.FS_S, ctx.FS_S, 0.5f, 0.5f, 0.5f, 1.0f);
        }

        // ---- 手札 ----
        for (int i = 0; i < players[p].hand.cardCount(); ++i) {
            float x, y;
            if (i < WRAP_AT) { x = baseX;           y = COL_Y0 + i * ROW_STEP; }
            else { x = baseX + WRAP_DX; y = COL_Y0 + (i - WRAP_AT) * ROW_STEP; }
            drawCardFaceImage(players[p].hand.cardAt(i), x, y);
        }

        const bool bust = players[p].hand.isBust();
        const int  score = players[p].hand.bestScore();

        if (bust) ctx.textL("BUST", baseX, INFO_BASE_Y - LINE, ctx.FS_S, ctx.FS_S, 1.0f);

        drawKV(baseX, INFO_BASE_Y + 0 * LINE, "total:", std::to_string(score));

        if (state == State::Betting) {
            drawKV(baseX, INFO_BASE_Y + 1 * LINE, "bet:",
                (p == 0) ? std::to_string(uiPlayerBet) : "--");
        }
        else {
            drawKV(baseX, INFO_BASE_Y + 1 * LINE, "bet:", std::to_string(players[p].bet));
        }

        // chips を2行表示
        const float chipsY = INFO_BASE_Y + 2 * LINE;
        ctx.textL("chips:", baseX, chipsY, ctx.FS_S, ctx.FS_S, 1.0f);
        ctx.textL(std::to_string(players[p].chips), baseX, chipsY + LINE, ctx.FS_S, ctx.FS_S, 1.0f);

        // BASE確率（BET中は表示しない）
        if (state != State::Betting) {
            int denom = players[p].baseAccuseDenom;
            float baseP = baseAccuseProb(players[p]);

            const float baseY = INFO_BASE_Y + 4 * LINE;
            ctx.textL("BASE 1/" + std::to_string(denom), baseX, baseY, ctx.FS_S, ctx.FS_S, 1.0f);
            ctx.textL(pct1(baseP), baseX, baseY + 1 * LINE, ctx.FS_S, ctx.FS_S, 1.0f);

            if (players[p].doubled) {
                ctx.textL("DD", baseX, baseY + 2 * LINE, ctx.FS_S, ctx.FS_S, 1.0f);
            }
        }
    }
}

void BlackjackGame::drawBaseProbSwapUI(const RenderCtx& ctx)
{
    if (state != State::BaseProbSwap) return;

    ctx.textL("SWAP BASE PROB (1 time):",
        ui.swapTitleX,
        btnSwapCpu1.getY() + ui.swapTitleYOff,
        ctx.FS_S, ctx.FS_S, 1.0f);

    auto drawLeft = [&](Button& b, const std::string& s) {
        ctx.drawBtn(b, true);
        ctx.textL(s, b.getX() + ui.padL, b.getY() + ui.btnTextYO, ctx.FS, ctx.FS, 1.0f);
        };

    drawLeft(btnSwapCpu1, "CPU1");
    drawLeft(btnSwapCpu2, "CPU2");
    drawLeft(btnSwapCpu3, "CPU3");
    drawLeft(btnSwapSkip, "SKIP");
}




//================================================
// render 完成形（分割呼び出しだけ）
//================================================
void BlackjackGame::render()
{
    GameLib::clear(1, 1, 1);
    sprite_render(game, 0, 0);
    GameLib::setBlendMode(Blender::BS_ALPHA);

    layoutButtons();

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

    // uiから渡す
    ctx.labelYBet = ui.labelYBet;
    ctx.labelYCheat = ui.labelYCheat;

    ctx.textL = textL;
    ctx.measureW = measureW;
    ctx.drawBtn = drawBtn;
    ctx.drawBtnTextCenter = drawBtnTextCenter;
    ctx.textC = textC;
    // ---- 追加した2分割 ----
    drawTopUI(ctx);
    drawTitleUI(ctx);

    // ---- 指定の5分割 ----
    drawBetUI(ctx);
    drawBaseProbSwapUI(ctx);
    drawActionUI(ctx);
    if (drawRoundEndUI(ctx)) return; // 最終結果ならここで終了

    drawDealerUI(ctx);
    drawPlayersUI(ctx);
}