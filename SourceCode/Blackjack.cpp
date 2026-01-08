#include "Blackjack.h"
#include "../GameLib/game_lib.h"
#include "all.h"

#include <algorithm>
#include <random>
#include <sstream>

//================================================
// ボタン配置（ここだけ触れば全体が揃う）
//================================================
void BlackjackGame::layoutButtons()
{
    // 画面下に揃える
    const float BTN_Y = (float)SCREEN_H - 100.0f;

    // BET ボタン列
    const float X0 = 40.0f;
    const float GAP = 10.0f;
    const float BW = 90.0f;
    const float BH = 70.0f;
    const float OK_W = 140.0f;

    float x = X0;

    btnBetMinus100.setRect(x, BTN_Y, BW, BH); x += BW + GAP;
    btnBetMinus50.setRect(x, BTN_Y, BW, BH); x += BW + GAP;
    btnBetMinus10.setRect(x, BTN_Y, BW, BH); x += BW + GAP;

    btnBetOK.setRect(x, BTN_Y, OK_W, BH);     x += OK_W + GAP;

    btnBetPlus10.setRect(x, BTN_Y, BW, BH);  x += BW + GAP;
    btnBetPlus50.setRect(x, BTN_Y, BW, BH);  x += BW + GAP;
    btnBetPlus100.setRect(x, BTN_Y, BW, BH);  // 最後は加算不要

    // 行動ボタン（右下）
    const float ACT_X_HIT = 720.0f;
    const float ACT_X_STAND = 900.0f;
    const float ACT_X_DOUBLE = 1080.0f;

    btnHit.setRect(ACT_X_HIT, BTN_Y, 160.0f, 80.0f);
    btnStand.setRect(ACT_X_STAND, BTN_Y, 160.0f, 80.0f);
    btnDouble.setRect(ACT_X_DOUBLE, BTN_Y, 160.0f, 80.0f);

    // タイトル（左上固定）
    btnToTitle.setRect(40.0f, 40.0f, 180.0f, 70.0f);
}


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
Sprite* titleBtn;
Sprite* bet;
Sprite* plus;
Sprite* minus;

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

    
    sprBJKA_B = sprite_load(L"Data/Images/Black_JQKA.png");
    sprBJKA_R = sprite_load(L"Data/Images/Red_JQKA.png");

    sprB36_B = sprite_load(L"Data/Images/Black3_4_5_6.png");
    sprB36_R = sprite_load(L"Data/Images/Red3_4_5_6.png");

    sprB710_B = sprite_load(L"Data/Images/Black7_8_9_10.png");
    sprB710_R = sprite_load(L"Data/Images/Red7_8_9_10.png");

    spr2Jo = sprite_load(L"Data/Images/2_jo.png");
    sprMark = sprite_load(L"Data/Images/mark.png");
    sprBack = sprite_load(L"Data/Images/backCard.png");


    titleBtn = sprite_load(L"./Data/Images/titleBtn.png");
    bet = sprite_load(L"./Data/Images/bet2.png");
    plus = sprite_load(L"./Data/Images/+.png");
    minus = sprite_load(L"./Data/Images/-.png");


    roundNo = 0;
    matchOver = false;

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
}

void BlackjackGame::beginRound() {
    dealer.hand.clear();
    for (auto& p : players) {
        p.hand.clear();
        p.bet = 0;
        p.doubled = false;
        p.stood = false;
    }
    activeCpuIndex = 1;
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
    activeCpuIndex = 1;

    // 5ラウンド終了後にOK押したら「新しい5ラウンド」を始める設計にする
    // （タイトルへ戻したいなら、update()のRoundEnd側で nextScene=SCENE_TITLE に変えてOK）
    if (matchOver) {
        matchOver = false;
        roundNo = 0;

        // 5ラウンド制の「新規ゲーム開始」：チップを初期化する
        for (auto& p : players) {
            p.chips = kStartChips;
        }
        uiPlayerBet = 100;
    }
}


void BlackjackGame::toDealing()
{
    // 5ラウンド終わってたらこれ以上進めない
    if (matchOver) {
        state = State::RoundEnd;
        return;
    }

    state = State::Dealing;
    beginRound();

    // ラウンド進行
    roundNo++;
    if (roundNo > kMaxRounds) {
        // 念のため
        matchOver = true;
        state = State::RoundEnd;
        return;
    }

    //========================
    // YOU bet（借金OK）
    //========================
    int bet = normalizeBet(uiPlayerBet, kMinBet, kBetStep, kMaxUserBet);
    players[0].bet = bet;
    players[0].chips -= bet;   // ★ここでマイナスになってもOK

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
    // ★ベットOK時イカサマ（暫定：ここだけ）
    // - プレイヤー：UIでONなら 17..21 に固定
    // - CPU：一定確率で 17..21 に固定（後でルール拡張）
    //========================
    if (uiCheatAtBet) {
        players[0].cheatedThisRound = true;
        int t = uiCheatBetTarget;
        if (t < 17) t = 17;
        if (t > 21) t = 21;
        rigHandToTotal(players[0].hand, t);
}


void BlackjackGame::toPlayerTurn() { state = State::PlayerTurn;  }
void BlackjackGame::toCpuTurn() { state = State::CpuTurn;}
void BlackjackGame::toDealerTurn() { state = State::DealerTurn; }
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
    if (cpu.hand.isBust()) { cpu.stood = true; return; }

    int s = cpu.hand.bestScore();

    if (cpu.hand.cardCount() == 2 && canDoubleDown(cpu) && (s == 9 || s == 10 || s == 11)) {
        doDoubleDown(cpu);
        return;
    }

    if (s <= 16) doHit(cpu);
    else         doStand(cpu);
}

void BlackjackGame::settleOne(BJParticipant& p) {
    int ps = p.hand.bestScore();
    int ds = dealer.hand.bestScore();

    bool pBust = p.hand.isBust();
    bool dBust = dealer.hand.isBust();

    bool pBJ = p.hand.isBlackjack();
    bool dBJ = dealer.hand.isBlackjack();

    if (pBust) return;

    if (dBJ) {
        if (pBJ) p.chips += p.bet;  // push
        return;
    }

    if (pBJ) {
        p.chips += p.bet + (p.bet * 3) / 2; // 3:2
        return;
    }

    if (dBust) {
        p.chips += p.bet * 2;
        return;
    }

    if (ps > ds) p.chips += p.bet * 2;
    else if (ps == ds) p.chips += p.bet; // push
}

void BlackjackGame::update()
{
    // ★クリック判定と描画を一致させる
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

        if (btnBetMinus100.isClicked()) uiPlayerBet -= 100;
        if (btnBetMinus50.isClicked())  uiPlayerBet -= 50;
        if (btnBetMinus10.isClicked())  uiPlayerBet -= 10;

        if (btnBetPlus10.isClicked())   uiPlayerBet += 10;
        if (btnBetPlus50.isClicked())   uiPlayerBet += 50;
        if (btnBetPlus100.isClicked())  uiPlayerBet += 100;

        // ※ kMaxUserBet が未定義なら、Blackjack.h に定義が必要です
        uiPlayerBet = normalizeBet(uiPlayerBet, kMinBet, kBetStep, kMaxUserBet);

        if (btnBetOK.isClicked()) toDealing();
        break;
    }

    case State::PlayerTurn: {
        btnHit.update();
        btnStand.update();
        btnDouble.update();

        BJParticipant& you = players[0];

        if (you.hand.isBlackjack()) you.stood = true;

        if (!you.stood) {
            if (btnDouble.isClicked() && canDoubleDown(you)) doDoubleDown(you);
            else if (btnHit.isClicked())                     doHit(you);
            else if (btnStand.isClicked())                   doStand(you);
        }

        if (you.stood) toCpuTurn();
        break;
    }

    case State::CpuTurn: {
        if (activeCpuIndex <= 3) {
            cpuAct(players[activeCpuIndex]);
            if (players[activeCpuIndex].stood) activeCpuIndex++;
        }
        else {
            toDealerTurn();
        }
        break;
    }

    case State::DealerTurn: {
        while (dealer.hand.bestScore() < 17) {
            dealer.hand.add(deck.draw());
            if (dealer.hand.isBust()) break;
        }
        toSettle();
        break;
    }

    case State::Settle: {
        for (auto& p : players) settleOne(p);

        // ★5ラウンド終わったらフラグ
        if (roundNo >= kMaxRounds) {
            matchOver = true;
        }

        toRoundEnd(matchOver ? "MATCH OVER" : "ROUND END");
        break;
    }


    case State::RoundEnd: {
        btnBetOK.update(); // NEXT/TITLE 用

        if (btnBetOK.isClicked()) {

            if (matchOver) {
                // ★タイトルへ戻る前にクリア（残り防止）
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

            // まだ5R終わってないなら次ラウンドへ
            toBetting();
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
    // mark.png はスクショを見る限り
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
    // 裏は64x64をそのまま描く
    drawFull64(sprBack, x, y, 64.0f);
}

//================================================
// 文字描画：align を指定しない（デフォルト左揃え）
//================================================
static float text_outL(int fontNo, const std::string& str,
    float x, float y, float scaleX, float scaleY,
    float r, float g, float b, float a)
{
    // ★最後の TEXT_ALIGN を渡さない版（=デフォルト左揃え）
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

//================================================
// render（白背景 / 画像だけでカード表示 / UI文字）
//================================================
void BlackjackGame::render()
{
    GameLib::clear(1, 1, 1);
    GameLib::setBlendMode(Blender::BS_ALPHA);

    // ボタン位置を毎フレーム揃える
    layoutButtons();

    // フォント
    const int   FONT = 2;
    const float FS = 1.0f;
    const float FS_S = 0.9f;
    const float TXT_R = 0.0f, TXT_G = 0.0f, TXT_B = 0.0f, TXT_A = 1.0f;

    auto textL = [&](const std::string& s, float x, float y, float sx, float sy, float a = 1.0f) {
        text_outL(FONT, s, x, y, sx, sy, TXT_R, TXT_G, TXT_B, a);
        };

    // 文字幅を取る（画面外に透明で描画して幅だけ使う）
    auto measureW = [&](const std::string& s, float sx, float sy) -> float {
        return text_outL(FONT, s, -10000.0f, -10000.0f, sx, sy, 0, 0, 0, 0);
        };

    // カード
    const float CARD_W = 64.0f;
    const float CARD_H = 32.0f;
    const float ROW_STEP = CARD_H + 14.0f;

    // ボタン位置（layoutButtonsと同じ）
    const float BTN_Y = (float)SCREEN_H - 100.0f;

    const float X0 = 40.0f;
    const float GAP = 10.0f;
    const float BW = 90.0f;
    const float BH = 70.0f;
    const float OK_W = 140.0f;

    const float BET_X_MINUS100 = X0 + (BW + GAP) * 0;
    const float BET_X_MINUS50 = X0 + (BW + GAP) * 1;
    const float BET_X_MINUS10 = X0 + (BW + GAP) * 2;
    const float BET_X_OK = X0 + (BW + GAP) * 3; // OKボタン左端
    const float BET_X_PLUS10 = BET_X_OK + OK_W + GAP;
    const float BET_X_PLUS50 = BET_X_PLUS10 + (BW + GAP);
    const float BET_X_PLUS100 = BET_X_PLUS50 + (BW + GAP);

    const float ACT_X_HIT = 720.0f;
    const float ACT_X_STAND = 900.0f;
    const float ACT_X_DOUBLE = 1080.0f;

    auto drawBtn = [&](Button& b, bool enabled = true) {
        if (enabled) b.draw(0.15f, 0.15f, 0.15f, 1.0f);
        else         b.draw(0.60f, 0.60f, 0.60f, 0.75f);
        };

    // 現在のラウンド
    textL("ROUND: " + std::to_string(roundNo) + " / " + std::to_string(kMaxRounds),
        0.0f, 10.0f, 1.0f, 1.0f);

    // タイトルボタン
    drawBtn(btnToTitle);
    textL("TITLE", 60.0f, 60.0f, FS, FS);

    //========================
    // BET（Betting）
    //========================
    if (state == State::Betting) {
        drawBtn(btnBetMinus100);
        drawBtn(btnBetMinus50);
        drawBtn(btnBetMinus10);
        drawBtn(btnBetOK);
        drawBtn(btnBetPlus10);
        drawBtn(btnBetPlus50);
        drawBtn(btnBetPlus100);

        textL("BET: " + std::to_string(uiPlayerBet), X0, BTN_Y - 32.0f, FS, FS);

        textL("-100", BET_X_MINUS100 + 18.0f, BTN_Y + 18.0f, FS_S, FS_S);
        textL("-50", BET_X_MINUS50 + 26.0f, BTN_Y + 18.0f, FS_S, FS_S);
        textL("-10", BET_X_MINUS10 + 26.0f, BTN_Y + 18.0f, FS_S, FS_S);

        // OK中央寄せ
        {
            float w = measureW("OK", FS, FS);
            textL("OK", BET_X_OK + (OK_W - w) * 0.5f, BTN_Y + 18.0f, FS, FS);
        }

        textL("+10", BET_X_PLUS10 + 22.0f, BTN_Y + 18.0f, FS_S, FS_S);
        textL("+50", BET_X_PLUS50 + 22.0f, BTN_Y + 18.0f, FS_S, FS_S);
        textL("+100", BET_X_PLUS100 + 16.0f, BTN_Y + 18.0f, FS_S, FS_S);
    }

    //========================
    // 行動（PlayerTurn）
    //========================
    if (state == State::PlayerTurn) {
        drawBtn(btnHit);
        drawBtn(btnStand);
        drawBtn(btnDouble, canDoubleDown(players[0]));

        textL("HIT", ACT_X_HIT + 42.0f, BTN_Y + 18.0f, FS, FS);
        textL("STAND", ACT_X_STAND + 24.0f, BTN_Y + 18.0f, FS, FS);
        textL("DOUBLE", ACT_X_DOUBLE + 18.0f, BTN_Y + 18.0f, FS, FS);
    }


    sprite_render(titleBtn, 40, 40);
    

    // ボタンラベル（必要なら）
    //text_outL(FONT, "TITLE", 60, 60, FS, FS, TXT_R, TXT_G, TXT_B, TXT_A);
    if (state == State::Betting) {
       
        sprite_render(minus, 85, 560);
        sprite_render(plus, 225, 560);
        sprite_render(bet, 360, 560);
        //text_outL(FONT, "-", 120, 590, FS, FS, TXT_R, TXT_G, TXT_B, TXT_A);
        //text_outL(FONT, "+", 260, 590, FS, FS, TXT_R, TXT_G, TXT_B, TXT_A);
        //text_outL(FONT, "OK", 440, 590, FS, FS, TXT_R, TXT_G, TXT_B, TXT_A);
    }
    if (state == State::PlayerTurn) {
        text_outL(FONT, "HIT", 760, 590, FS, FS, TXT_R, TXT_G, TXT_B, TXT_A);
        text_outL(FONT, "STAND", 920, 590, FS, FS, TXT_R, TXT_G, TXT_B, TXT_A);
        text_outL(FONT, "DOUBLE", 1080, 590, FS, FS, TXT_R, TXT_G, TXT_B, TXT_A);
    }

    //========================
    // RoundEnd：OKボタン
    //========================
    if (state == State::RoundEnd) {
        drawBtn(btnBetOK);

        std::string cap = matchOver ? "TITLE" : "NEXT";
        float w = measureW(cap, FS, FS);
        textL(cap, BET_X_OK + (OK_W - w) * 0.5f, BTN_Y + 18.0f, FS, FS);
    }

    //========================================================
    // ★最終結果（5ラウンド終了時）：ここで表示して return
    //========================================================
    if (state == State::RoundEnd && matchOver)
    {
        textL("FINAL RESULT", 520.0f, 150.0f, 1.2f, 1.2f);
        textL("Press OK to return TITLE", 470.0f, 185.0f, FS_S, FS_S);

        struct E { int idx; int chips; };
        std::vector<E> es;
        es.reserve(4);
        for (int i = 0; i < 4; ++i) es.push_back({ i, players[i].chips });

        std::sort(es.begin(), es.end(), [](const E& a, const E& b) {
            if (a.chips != b.chips) return a.chips > b.chips; // chips多い順
            return a.idx < b.idx;                             // 同点は固定
            });

        // 1位タイ,1位タイ,3位 を作る（競技順位）
        int rankOf[4] = { 1,1,1,1 };
        int curRank = 1;
        for (int pos = 0; pos < 4; ++pos) {
            if (pos > 0 && es[pos].chips < es[pos - 1].chips) {
                curRank = pos + 1; // 飛び番になる
            }
            rankOf[es[pos].idx] = curRank;
        }

        // 同順位人数
        int rankCount[5] = { 0,0,0,0,0 };
        for (int i = 0; i < 4; ++i) {
            int r = rankOf[i];
            if (1 <= r && r <= 4) rankCount[r]++;
        }

        const float RX = 420.0f;
        const float RY = 240.0f;
        const float L = 34.0f;

        textL("RANK", RX, RY - 30.0f, FS_S, FS_S);
        textL("NAME", RX + 120.0f, RY - 30.0f, FS_S, FS_S);
        textL("CHIPS", RX + 320.0f, RY - 30.0f, FS_S, FS_S);
        textL("DIFF", RX + 460.0f, RY - 30.0f, FS_S, FS_S);

        for (int pos = 0; pos < 4; ++pos) {
            int i = es[pos].idx;
            int r = rankOf[i];
            bool tie = (rankCount[r] >= 2);

            std::string rankStr = std::to_string(r) + (tie ? u8"位タイ" : u8"位");
            std::string nameStr = players[i].name;

            int chips = players[i].chips;
            int diff = chips - kStartChips;
            std::string diffStr = (diff >= 0) ? ("+" + std::to_string(diff)) : std::to_string(diff);

            textL(rankStr, RX, RY + pos * L, FS, FS);
            textL(nameStr, RX + 120.0f, RY + pos * L, FS, FS);
            textL(std::to_string(chips), RX + 320.0f, RY + pos * L, FS, FS);
            textL(diffStr, RX + 460.0f, RY + pos * L, FS, FS);
        }

        return; // ★ここで通常のDealer/Players描画をしない
    }

    //========================
    // Dealer
    //========================
    const float DEALER_X = 420.0f;
    const float DEALER_Y = 40.0f;
    const float DEALER_DX = CARD_W + 16.0f;

    const bool hideDealerFirst = (state == State::PlayerTurn) || (state == State::CpuTurn);

    textL("DEALER", DEALER_X, DEALER_Y - 34.0f, FS, FS);

    for (int i = 0; i < dealer.hand.cardCount(); ++i) {
        float x = DEALER_X + i * DEALER_DX;
        float y = DEALER_Y;

        if (i == 0 && hideDealerFirst) {
            // 空白
        }
        else {
            drawCardFaceImage(dealer.hand.cardAt(i), x, y);
        }
    }

    if (hideDealerFirst) {
        textL("total: ??", DEALER_X, DEALER_Y + CARD_H + 14.0f, FS_S, FS_S);
    }
    else {
        textL("total: " + std::to_string(dealer.hand.bestScore()),
            DEALER_X, DEALER_Y + CARD_H + 14.0f, FS_S, FS_S);
    }

    //========================
    // Players（4枚目以降は右に折り返し）
    //========================
    const float COL_X0 = 220.0f;
    const float COL_DX = 260.0f;
    const float COL_Y0 = 140.0f;

    const int   WRAP_AT = 3;
    const float WRAP_DX = CARD_W + 18.0f;

    const float LINE = 22.0f;
    const float KV_GAP = 150.0f;

    auto drawKV = [&](float x, float y, const std::string& key, const std::string& val) {
        textL(key, x, y, FS_S, FS_S);
        textL(val, x + KV_GAP, y, FS_S, FS_S);
        };

    // 情報欄Y：右列と折り返し列の最大段数で決める
    int maxRows = 0;
    for (int p = 0; p < 4; ++p) {
        int n = players[p].hand.cardCount();
        int right = (n < WRAP_AT) ? n : WRAP_AT;
        int wrap = (n > WRAP_AT) ? (n - WRAP_AT) : 0;
        int rows = (right > wrap) ? right : wrap;
        if (rows > maxRows) maxRows = rows;
    }

    float INFO_BASE_Y = COL_Y0 + (float)maxRows * ROW_STEP + 14.0f;

    const float INFO_BOTTOM_LIMIT = BTN_Y - 70.0f;
    if (INFO_BASE_Y > INFO_BOTTOM_LIMIT) INFO_BASE_Y = INFO_BOTTOM_LIMIT;

    const char* label[4] = { "Player", "cpu1", "cpu2", "cpu3" };

    for (int p = 0; p < 4; ++p) {
        float baseX = COL_X0 + p * COL_DX;

        textL(label[p], baseX, COL_Y0 - 40.0f, FS, FS);

        for (int i = 0; i < players[p].hand.cardCount(); ++i) {
            float x, y;
            if (i < WRAP_AT) {
                x = baseX;
                y = COL_Y0 + i * ROW_STEP;
            }
            else {
                x = baseX + WRAP_DX;
                y = COL_Y0 + (i - WRAP_AT) * ROW_STEP;
            }
            drawCardFaceImage(players[p].hand.cardAt(i), x, y);
        }

        const bool bust = players[p].hand.isBust();
        const int  score = players[p].hand.bestScore();

        if (bust) textL("BUST", baseX, INFO_BASE_Y - LINE, FS_S, FS_S);

        drawKV(baseX, INFO_BASE_Y + 0 * LINE, "total:", std::to_string(score));

        if (state == State::Betting) {
            drawKV(baseX, INFO_BASE_Y + 1 * LINE, "bet:",
                (p == 0) ? std::to_string(uiPlayerBet) : "--");
        }
        else {
            drawKV(baseX, INFO_BASE_Y + 1 * LINE, "bet:", std::to_string(players[p].bet));
        }

        drawKV(baseX, INFO_BASE_Y + 2 * LINE, "chips:", std::to_string(players[p].chips));

        if (players[p].doubled) textL("DD", baseX, INFO_BASE_Y + 3 * LINE, FS_S, FS_S);
    }
}
