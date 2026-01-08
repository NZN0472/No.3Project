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

    titleBtn = sprite_load(L"./Data/Images/titleBtn.png");
    bet = sprite_load(L"./Data/Images/bet2.png");
    plus = sprite_load(L"./Data/Images/+.png");
    minus = sprite_load(L"./Data/Images/-.png");


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

    dealer.hand.clear();
    for (auto& p : players) {
        p.hand.clear();
        p.bet = 0;
        p.doubled = false;
        p.stood = false;
    }
    activeCpuIndex = 1;
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

void BlackjackGame::toPlayerTurn() { state = State::PlayerTurn;  }
void BlackjackGame::toCpuTurn() { state = State::CpuTurn;}
void BlackjackGame::toDealerTurn() { state = State::DealerTurn; }
void BlackjackGame::toSettle() { state = State::Settle;}

void BlackjackGame::toRoundEnd(const std::string& msg) {
    state = State::RoundEnd;
    
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
// render（白背景 / 文字を画像の下に）
//================================================
void BlackjackGame::render()
{
    //========================
    // 背景：白
    //========================
    GameLib::clear(1, 1, 1);
    GameLib::setBlendMode(Blender::BS_ALPHA);

    //========================
    // フォント（環境に合わせて変更OK）
    //========================
    const int   FONT = 2;
    const float FS = 1.0f;
    const float FS_S = 0.9f;
    const float TXT_R = 0.0f, TXT_G = 0.0f, TXT_B = 0.0f, TXT_A = 1.0f;

    auto textL = [&](const std::string& s, float x, float y, float sx, float sy) {
        text_outL(FONT, s, x, y, sx, sy, TXT_R, TXT_G, TXT_B, TXT_A);
        };

    //========================
    // カードサイズ（素材64のまま）
    //========================
    const float ICON = 64.0f;
    const float CARD_W = ICON * 2.0f;   // rank64 + suit64
    const float CARD_H = ICON;

    // ★カード文字(text)は出さないので詰めてOK
    const float ROW_STEP = CARD_H + 10.0f; // 好みで 8〜16

    //========================
    // 下段ボタン位置（少し下へ）
    //========================
    const float BTN_Y = (float)SCREEN_H - 100.0f; // ←560→もう少し下（例：620付近）
    // Bettingボタン（- / + / OK）
    const float BET_X_MINUS = 80.0f;
    const float BET_X_PLUS = 220.0f;
    const float BET_X_OK = 360.0f;
    btnBetMinus.setRect(BET_X_MINUS, BTN_Y, 120.0f, 80.0f);
    btnBetPlus.setRect(BET_X_PLUS, BTN_Y, 120.0f, 80.0f);
    btnBetOK.setRect(BET_X_OK, BTN_Y, 220.0f, 80.0f);

    // 行動ボタン（HIT/STAND/DOUBLE）も同じ高さに
    const float ACT_X_HIT = 720.0f;
    const float ACT_X_STAND = 900.0f;
    const float ACT_X_DOUBLE = 1080.0f;
    btnHit.setRect(ACT_X_HIT, BTN_Y, 160.0f, 80.0f);
    btnStand.setRect(ACT_X_STAND, BTN_Y, 160.0f, 80.0f);
    btnDouble.setRect(ACT_X_DOUBLE, BTN_Y, 160.0f, 80.0f);

    //========================
    // レイアウト（カード）
    //========================
    // Dealer（上）
    const float DEALER_X = 420.0f;
    const float DEALER_Y = 40.0f;
    const float DEALER_DX = CARD_W + 12.0f; // カード間隔を詰める

    // Players（下：4列）
    const float COL_X0 = 220.0f; // 4枚目以降を左に置くスペース確保
    const float COL_DX = 260.0f;
    const float COL_Y0 = 200.0f; // 少し上げて、下の情報とボタンが見切れにくく

    // 折り返し設定：4枚目から左へ
    const int   WRAP_AT = 3;                 // 0,1,2が右列（3枚）／3枚目以降は左列
    const float WRAP_DX = CARD_W + 12.0f;    // 左列との距離（詰めたいなら 8〜16）

    // 情報表示（total/bet/chips）
    const float LINE = 22.0f;
    const float KV_GAP = 150.0f;  // ← chips の詰まり対策（120→150）

    auto drawKV = [&](float x, float y, const std::string& key, const std::string& val) {
        textL(key, x, y, FS_S, FS_S);
        textL(val, x + KV_GAP, y, FS_S, FS_S);
        };

    //========================
    // ボタン色（白背景なので黒系）
    //========================
    auto drawBtn = [&](Button& b, bool enabled = true) {
        if (enabled) b.draw(0.15f, 0.15f, 0.15f, 1.0f);
        else         b.draw(0.60f, 0.60f, 0.60f, 0.75f);
        };

    //========================
    // ボタン描画
    //========================
    drawBtn(btnToTitle);
    textL("TITLE", 60, 60, FS, FS);

    if (state == State::Betting) {
        drawBtn(btnBetMinus);
        drawBtn(btnBetPlus);
        drawBtn(btnBetOK);

        // BET 表示（ボタンの少し上）
        textL("BET: " + std::to_string(uiPlayerBet), BET_X_MINUS, BTN_Y - 32.0f, FS, FS);

        // ラベル（任意）
        textL("-", BET_X_MINUS + 48.0f, BTN_Y + 20.0f, FS, FS);
        textL("+", BET_X_PLUS + 48.0f, BTN_Y + 20.0f, FS, FS);
        textL("OK", BET_X_OK + 80.0f, BTN_Y + 20.0f, FS, FS);
    }

    if (state == State::PlayerTurn) {
        drawBtn(btnHit);
        drawBtn(btnStand);
        drawBtn(btnDouble, canDoubleDown(players[0]));

        // ラベル（任意）
        textL("HIT", ACT_X_HIT + 30.0f, BTN_Y + 20.0f, FS, FS);
        textL("STAND", ACT_X_STAND + 18.0f, BTN_Y + 20.0f, FS, FS);
        textL("DOUBLE", ACT_X_DOUBLE + 10.0f, BTN_Y + 20.0f, FS, FS);
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
    if (state == State::RoundEnd) {
        drawBtn(btnBetOK);
        textL("NEXT", BET_X_OK + 60.0f, BTN_Y + 20.0f, FS, FS);
    }

    //========================
    // Dealer（1枚目伏せは空白＝描かない）
    //========================
    const bool hideDealerFirst = (state == State::PlayerTurn) || (state == State::CpuTurn);

    textL("DEALER", DEALER_X, DEALER_Y - 30.0f, FS, FS);

    for (int i = 0; i < dealer.hand.cardCount(); ++i) {
        float x = DEALER_X + i * DEALER_DX;
        float y = DEALER_Y;

        if (i == 0 && hideDealerFirst) {
            // 空白（裏面も描かない）
        }
        else {
            drawCardFaceImage(dealer.hand.cardAt(i), x, y);
        }
    }

    // Dealer total（画像の数字とは別でOKなら表示）
    if (hideDealerFirst) drawKV(DEALER_X, DEALER_Y + CARD_H + 10.0f, "total:", "??");
    else                drawKV(DEALER_X, DEALER_Y + CARD_H + 10.0f, "total:", std::to_string(dealer.hand.bestScore()));

    //========================
    // Players：情報欄のY（全員で揃える）
    // 折り返し後も見切れにくいように「最大段数」から計算
    //========================
    int maxRightRows = 0;
    int maxLeftRows = 0;
    for (int p = 0; p < 4; ++p) {
        int n = players[p].hand.cardCount();
        int right = (n < WRAP_AT) ? n : WRAP_AT;
        int left = (n > WRAP_AT) ? (n - WRAP_AT) : 0;
        if (right > maxRightRows) maxRightRows = right;
        if (left > maxLeftRows) maxLeftRows = left;
    }
    int maxRows = (maxRightRows > maxLeftRows) ? maxRightRows : maxLeftRows;

    float INFO_BASE_Y = COL_Y0 + (float)maxRows * ROW_STEP + 14.0f;

    // 情報欄がボタンに近すぎるなら少し上に逃がす
    const float INFO_BOTTOM_LIMIT = BTN_Y - 70.0f;
    if (INFO_BASE_Y > INFO_BOTTOM_LIMIT) INFO_BASE_Y = INFO_BOTTOM_LIMIT;

    //========================
    // Players：4枚目以降は「1枚目の左」に折り返し
    // textのカード文字は出さない（画像だけ）
    // BUST は total の上に出す
    //========================
    const char* label[4] = { "Player", "cpu1", "cpu2", "cpu3" };

    for (int p = 0; p < 4; ++p) {
        float baseX = COL_X0 + p * COL_DX;

        // 列ラベル
        textL(label[p], baseX, COL_Y0 - 40.0f, FS, FS);

        // カード描画（画像だけ）
        for (int i = 0; i < players[p].hand.cardCount(); ++i) {
            float x, y;

            if (i < WRAP_AT) {
                x = baseX;
                y = COL_Y0 + i * ROW_STEP;
            }
            else {
                x = baseX - WRAP_DX;
                y = COL_Y0 + (i - WRAP_AT) * ROW_STEP;
            }

            drawCardFaceImage(players[p].hand.cardAt(i), x, y);
        }

        // ------ 情報欄 ------
        const bool bust = players[p].hand.isBust();
        const int  score = players[p].hand.bestScore();

        // ★BUST を total の上に別行で（重ならない）
        if (bust) {
            textL("BUST", baseX, INFO_BASE_Y - LINE, FS_S, FS_S);
        }

        drawKV(baseX, INFO_BASE_Y + 0 * LINE, "total:", std::to_string(score));

        if (state == State::Betting) {
            drawKV(baseX, INFO_BASE_Y + 1 * LINE, "bet:", (p == 0) ? std::to_string(uiPlayerBet) : "--");
        }
        else {
            drawKV(baseX, INFO_BASE_Y + 1 * LINE, "bet:", std::to_string(players[p].bet));
        }

        drawKV(baseX, INFO_BASE_Y + 2 * LINE, "chips:", std::to_string(players[p].chips));

        if (players[p].doubled) {
            textL("DD", baseX, INFO_BASE_Y + 3 * LINE, FS_S, FS_S);
        }
    }
}
