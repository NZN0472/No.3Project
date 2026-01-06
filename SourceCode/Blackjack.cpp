#include "Blackjack.h"
#include "../GameLib/game_lib.h"
#include "all.h"

#include <algorithm>
#include <random>
#include <sstream>

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

    // ★ パスはスクショ通り Data/Images
    sprBJKA_B = sprite_load(L"Data/Images/Black_JQKA.png");
    sprBJKA_R = sprite_load(L"Data/Images/Red_JQKA.png");

    sprB36_B = sprite_load(L"Data/Images/Black3_4_5_6.png");
    sprB36_R = sprite_load(L"Data/Images/Red3_4_5_6.png");

    sprB710_B = sprite_load(L"Data/Images/Black7_8_9_10.png");
    sprB710_R = sprite_load(L"Data/Images/Red7_8_9_10.png");

    spr2Jo = sprite_load(L"Data/Images/2_jo.png");
    sprMark = sprite_load(L"Data/Images/mark.png");
    sprBack = sprite_load(L"Data/Images/backCard.png");

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

void BlackjackGame::toBetting() {
    state = State::Betting;
    setMsg("BET: [-][+] OK");
}

void BlackjackGame::toDealing() {
    state = State::Dealing;
    beginRound();

    // YOU bet
    int maxBet = players[0].chips;
    int bet = clampInt(uiPlayerBet, kMinBet, maxBet);
    bet = (bet / kBetStep) * kBetStep;
    if (bet < kMinBet) bet = (maxBet >= kMinBet) ? kMinBet : maxBet;

    players[0].bet = bet;
    players[0].chips -= bet;

    // CPU bet
    for (int i = 1; i <= 3; ++i) {
        int cpuMax = players[i].chips;
        int cpuBet = cpuMax / 10;
        cpuBet = clampInt(cpuBet, kMinBet, 200);
        cpuBet = (cpuBet / kBetStep) * kBetStep;
        if (cpuBet > cpuMax) cpuBet = cpuMax;

        players[i].bet = cpuBet;
        players[i].chips -= cpuBet;
    }

    // deal 2 cards each + dealer 2
    for (int k = 0; k < 2; ++k) {
        for (auto& p : players) p.hand.add(deck.draw());
        dealer.hand.add(deck.draw());
    }

    toPlayerTurn();
}

void BlackjackGame::toPlayerTurn() { state = State::PlayerTurn; setMsg("YOUR TURN"); }
void BlackjackGame::toCpuTurn() { state = State::CpuTurn;    setMsg("CPU TURN"); }
void BlackjackGame::toDealerTurn() { state = State::DealerTurn; setMsg("DEALER TURN"); }
void BlackjackGame::toSettle() { state = State::Settle;     setMsg("SETTLE"); }

void BlackjackGame::toRoundEnd(const std::string& msg) {
    state = State::RoundEnd;
    setMsg(msg + " (OK=NEXT / TITLE)");
}

bool BlackjackGame::canDoubleDown(const BJParticipant& p) const {
    if (p.doubled) return false;
    if (p.hand.cardCount() != 2) return false;
    if (p.chips < p.bet) return false;
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

void BlackjackGame::update() {
    btnToTitle.update();
    if (btnToTitle.isClicked()) {
        extern int nextScene;
        nextScene = SCENE_TITLE;
        return;
    }

    switch (state) {
    case State::Betting: {
        btnBetMinus.update();
        btnBetPlus.update();
        btnBetOK.update();

        if (players[0].chips < kMinBet) {
            toRoundEnd("YOU NO CHIPS");
            break;
        }

        int maxBet = players[0].chips;

        if (btnBetMinus.isClicked()) uiPlayerBet -= kBetStep;
        if (btnBetPlus.isClicked())  uiPlayerBet += kBetStep;

        uiPlayerBet = clampInt(uiPlayerBet, kMinBet, maxBet);
        uiPlayerBet = (uiPlayerBet / kBetStep) * kBetStep;

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
        toRoundEnd("ROUND END");
        break;
    }

    case State::RoundEnd: {
        btnBetOK.update(); // NEXT用
        if (btnBetOK.isClicked()) toBetting();
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
// render
//================================================
void BlackjackGame::render() {
    // 背景：白
    GameLib::clear(1, 1, 1);
    GameLib::setBlendMode(Blender::BS_ALPHA);

    // ロード失敗チェック
    if (!sprMark || !sprBJKA_B || !sprB36_B || !sprB710_B || !spr2Jo) {
        debug::setString("Sprite load failed. Check Data/Images path & file names.", 0, 0);
        return;
    }

    //========================
    // レイアウト（見える配置）
    //========================
    const float CARD_W = 64.0f; // 表示幅（rank32 + suit32）
    const float CARD_H = 32.0f; // 表示高さ（32）
    const float DX = CARD_W + 10.0f;
    const float DY = CARD_H + 10.0f;

    // Dealer（上）
    const float DEALER_X = 420.0f;
    const float DEALER_Y = 40.0f;

    // Players（下：4列）
    const float P_Y0 = 220.0f;
    const float P_X0 = 120.0f;
    const float P_DX = 240.0f;

    for (int i = 0; i < dealer.hand.cardCount(); ++i) {
        float x = DEALER_X + i * DX;
        float y = DEALER_Y;

        bool hideFirst =
            (i == 0) &&
            ((state == State::PlayerTurn) || (state == State::CpuTurn));

        if (hideFirst) {
            // ★伏せカードは描かない（空白にする）
            continue;
        }

        // 2枚目以降は表を描く
        drawCardFaceImage(dealer.hand.cardAt(i), x, y);
    }


    //========================
    // Players cards（縦積み）
    //========================
    for (int p = 0; p < 4; ++p) {
        float colX = P_X0 + p * P_DX;
        for (int i = 0; i < players[p].hand.cardCount(); ++i) {
            float x = colX;
            float y = P_Y0 + i * DY;
            drawCardFaceImage(players[p].hand.cardAt(i), x, y);
        }
    }

    //========================
    // ボタン（背景白なので濃い色に）
    //========================
    const float br = 0.2f, bg = 0.2f, bb = 0.2f, ba = 1.0f;

    btnToTitle.draw(br, bg, bb, ba);

    if (state == State::Betting) {
        btnBetMinus.draw(br, bg, bb, ba);
        btnBetPlus.draw(br, bg, bb, ba);
        btnBetOK.draw(br, bg, bb, ba);
    }

    if (state == State::PlayerTurn) {
        btnHit.draw(br, bg, bb, ba);
        btnStand.draw(br, bg, bb, ba);

        if (canDoubleDown(players[0])) btnDouble.draw(br, bg, bb, ba);
        else                           btnDouble.draw(0.6f, 0.6f, 0.6f, 1.0f);
    }

    if (state == State::RoundEnd) {
        btnBetOK.draw(br, bg, bb, ba);
    }

    //========================
    // デバッグ文字（左上固定）
    //========================
    {
        std::ostringstream oss;
        oss << "MSG: " << lastMessage;
        debug::setString(oss.str().c_str(), 0, 0);

        if ((state == State::PlayerTurn) || (state == State::CpuTurn)) {
            debug::setString("DEALER score: ??", 0, 1);
        }
        else {
            std::string s = "DEALER score: " + std::to_string(dealer.hand.bestScore());
            debug::setString(s.c_str(), 0, 1);
        }

        for (int i = 0; i < 4; ++i) {
            std::ostringstream p;
            p << players[i].name
                << " score:" << players[i].hand.bestScore()
                << (players[i].hand.isBust() ? " BUST" : "")
                << " bet:" << players[i].bet
                << " chips:" << players[i].chips
                << (players[i].doubled ? " DD" : "");
            debug::setString(p.str().c_str(), 0, 3 + i);
        }
    }
}
