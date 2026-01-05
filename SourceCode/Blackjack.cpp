#include "Blackjack.h"
#include "../GameLib/game_lib.h"
#include "all.h"

#include <algorithm>
#include <random>
#include <sstream>

using namespace std;

//---------------- ユーティリティ ----------------
static int clampInt(int v, int lo, int hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static std::string rankToString(int rank) {
    if (rank == 1)  return "A";
    if (rank == 11) return "J";
    if (rank == 12) return "Q";
    if (rank == 13) return "K";
    return std::to_string(rank);
}

static std::string suitToStringUTF8(int suit) {
    // suit: 0..3 を想定（Deckで入れてる値に合わせる）
    // 文字化けする場合は、下の ASCII 版に差し替えてください。
    static const char* sym[4] = { u8"♣", u8"♦", u8"♥", u8"♠" };
    if (suit < 0 || suit > 3) return "?";
    return sym[suit];
}

static std::string cardToStringUTF8(const BJCard& c) {
    return suitToStringUTF8(c.suit) + rankToString(c.rank);
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
    // 簡易：残りが少なければ作り直し
    if (cards.size() < 15) {
        rebuild();
    }
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

    // Aを11として扱える分だけ足す（+10する）
    while (ace > 0 && sum + 10 <= 21) {
        sum += 10;
        ace--;
    }
    return sum;
}

const BJCard& BJHand::cardAt(int i) const {
    return cards[i];
}

bool BJHand::isBust() const { return bestScore() > 21; }
bool BJHand::isBlackjack() const { return cards.size() == 2 && bestScore() == 21; }


//================================================
// BlackjackGame
//================================================
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
    toBetting();
}

void BlackjackGame::deinit() {
    // 今は特に無し
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
    setMsg("BET: [-][+]で調整、[OK]で開始");
}

void BlackjackGame::toDealing() {
    state = State::Dealing;
    beginRound();

    // ---- ベット確定 ----
    // Human
    int maxBet = players[0].chips;
    int bet = clampInt(uiPlayerBet, kMinBet, maxBet);
    bet = (bet / kBetStep) * kBetStep;
    if (bet < kMinBet) bet = (maxBet >= kMinBet) ? kMinBet : maxBet;

    players[0].bet = bet;
    players[0].chips -= bet;

    // CPU：所持金の1/10、最低10、上限200、10刻み
    for (int i = 1; i <= 3; ++i) {
        int cpuMax = players[i].chips;
        int cpuBet = cpuMax / 10;
        cpuBet = clampInt(cpuBet, kMinBet, 200);
        cpuBet = (cpuBet / kBetStep) * kBetStep;
        if (cpuBet > cpuMax) cpuBet = cpuMax;

        players[i].bet = cpuBet;
        players[i].chips -= cpuBet;
    }

    // ---- 初期配布：全員2枚 + ディーラー2枚 ----
    for (int k = 0; k < 2; ++k) {
        for (auto& p : players) p.hand.add(deck.draw());
        dealer.hand.add(deck.draw());
    }

    toPlayerTurn();
}

void BlackjackGame::toPlayerTurn() {
    state = State::PlayerTurn;
    setMsg("YOUR TURN: [HIT][STAND][DOUBLE]");
}

void BlackjackGame::toCpuTurn() {
    state = State::CpuTurn;
    setMsg("CPU TURN...");
}

void BlackjackGame::toDealerTurn() {
    state = State::DealerTurn;
    setMsg("DEALER TURN...");
}

void BlackjackGame::toSettle() {
    state = State::Settle;
    setMsg("SETTLE...");
}

void BlackjackGame::toRoundEnd(const std::string& msg) {
    state = State::RoundEnd;
    setMsg(msg + "  (OKで次ラウンド / TITLEで戻る)");
}

bool BlackjackGame::canDoubleDown(const BJParticipant& p) const {
    // 「最初の2枚のあと1回だけ」＋「追加で同額払える」＋「未ダブル」
    if (p.doubled) return false;
    if (p.hand.cardCount() != 2) return false;
    if (p.chips < p.bet) return false;
    return true;
}

void BlackjackGame::doHit(BJParticipant& p) {
    p.hand.add(deck.draw());
    if (p.hand.isBust()) {
        p.stood = true; // バーストなら終了
    }
}

void BlackjackGame::doStand(BJParticipant& p) {
    p.stood = true;
}

void BlackjackGame::doDoubleDown(BJParticipant& p) {
    if (!canDoubleDown(p)) return;

    // 追加で bet を支払い、bet倍、1枚引いて強制スタンド
    p.chips -= p.bet;
    p.bet *= 2;
    p.doubled = true;

    doHit(p);       // 1枚だけ引く
    p.stood = true; // 強制スタンド
}

void BlackjackGame::cpuAct(BJParticipant& cpu) {
    if (cpu.stood) return;
    if (cpu.hand.isBust()) { cpu.stood = true; return; }

    int s = cpu.hand.bestScore();

    // 簡易ダブル：9/10/11 なら、可能ならダブル
    if (cpu.hand.cardCount() == 2 && canDoubleDown(cpu) && (s == 9 || s == 10 || s == 11)) {
        doDoubleDown(cpu);
        return;
    }

    // 16以下Hit、17以上Stand
    if (s <= 16) doHit(cpu);
    else doStand(cpu);
}

void BlackjackGame::settleOne(BJParticipant& p) {
    // ベットは既に chips から引かれている前提
    int ps = p.hand.bestScore();
    int ds = dealer.hand.bestScore();

    bool pBust = p.hand.isBust();
    bool dBust = dealer.hand.isBust();

    bool pBJ = p.hand.isBlackjack();
    bool dBJ = dealer.hand.isBlackjack();

    if (pBust) {
        // 既に払った bet を失うだけ
        return;
    }

    // Dealer BJ 優先
    if (dBJ) {
        if (pBJ) {
            // push：bet返却
            p.chips += p.bet;
        }
        // else lose：何もしない
        return;
    }

    // Player BJ（Dealer BJでない）
    if (pBJ) {
        // 3:2 payout（元手返却 + 利益 1.5倍）
        // 返却額 = bet + bet*3/2  => 合計 2.5*bet
        p.chips += p.bet + (p.bet * 3) / 2;
        return;
    }

    if (dBust) {
        // win：bet*2（元手返却+同額勝ち）
        p.chips += p.bet * 2;
        return;
    }

    if (ps > ds) {
        p.chips += p.bet * 2;
    }
    else if (ps == ds) {
        // push：bet返却
        p.chips += p.bet;
    }
    else {
        // lose：何もしない
    }
}

void BlackjackGame::update() {
    //========================
    // クリックUI：ボタン更新
    //========================

    // タイトルへ戻る（いつでも押せる）
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

        // 所持金不足
        if (players[0].chips < kMinBet) {
            toRoundEnd("YOU所持金不足");
            break;
        }

        int maxBet = players[0].chips;

        if (btnBetMinus.isClicked()) uiPlayerBet -= kBetStep;
        if (btnBetPlus.isClicked())  uiPlayerBet += kBetStep;

        uiPlayerBet = clampInt(uiPlayerBet, kMinBet, maxBet);
        uiPlayerBet = (uiPlayerBet / kBetStep) * kBetStep;

        if (btnBetOK.isClicked()) {
            toDealing();
        }
        break;
    }

    case State::Dealing:
        // toDealing内で次状態へ移るので通常ここには来ない
        break;

    case State::PlayerTurn: {
        btnHit.update();
        btnStand.update();
        btnDouble.update();

        BJParticipant& you = players[0];

        // BJなら行動なし
        if (you.hand.isBlackjack()) {
            you.stood = true;
        }

        if (!you.stood) {
            if (btnDouble.isClicked() && canDoubleDown(you)) {
                doDoubleDown(you);
            }
            else if (btnHit.isClicked()) {
                doHit(you);
            }
            else if (btnStand.isClicked()) {
                doStand(you);
            }
        }

        if (you.stood) {
            toCpuTurn();
        }
        break;
    }

    case State::CpuTurn: {
        // CPUを順番に動かす（見た目を気にしないなら全員一気に回してもOK）
        if (activeCpuIndex <= 3) {
            cpuAct(players[activeCpuIndex]);
            if (players[activeCpuIndex].stood) {
                activeCpuIndex++;
            }
        }
        else {
            toDealerTurn();
        }
        break;
    }

    case State::DealerTurn: {
        // 17以上でスタンド（ソフト17もスタンド扱い）
        while (dealer.hand.bestScore() < 17) {
            dealer.hand.add(deck.draw());
            if (dealer.hand.isBust()) break;
        }
        toSettle();
        break;
    }

    case State::Settle: {
        for (auto& p : players) {
            settleOne(p);
        }
        toRoundEnd("ROUND END");
        break;
    }

    case State::RoundEnd: {
        // 次ラウンド：OKボタンを使い回す
        btnBetOK.update();
        if (btnBetOK.isClicked()) {
            toBetting();
        }
        break;
    }

    default:
        break;
    }
}

void BlackjackGame::render() {
    //========================
    // ボタン描画（ラベル文字は未実装）
    //========================
    // 文字を描きたい場合は、あなたの環境の文字描画関数（debug::setString等）に合わせて後で追加してください。

    //========================
    // ボタン描画
    //========================
    btnToTitle.draw(1, 1, 1, 1);

    if (state == State::Betting) {
        btnBetMinus.draw(1, 1, 1, 1);
        btnBetPlus.draw(1, 1, 1, 1);
        btnBetOK.draw(1, 1, 1, 1);
    }

    if (state == State::PlayerTurn) {
        btnHit.draw(1, 1, 1, 1);
        btnStand.draw(1, 1, 1, 1);

        bool enableDD = canDoubleDown(players[0]);
        if (enableDD) btnDouble.draw(1, 1, 1, 1);
        else          btnDouble.draw(0.4f, 0.4f, 0.4f, 0.8f);
    }

    if (state == State::RoundEnd) {
        // 次ラウンドへ：OKボタンを表示
        btnBetOK.draw(1, 1, 1, 1);
    }

    //========================
    // 文字描画ヘルパ
    //========================
    auto put = [&](int x, int y, const std::string& s) {
        debug::setString(s.c_str(), x, y);
        };

    // カードを横に並べて表示（通常）
    auto drawHandCards = [&](int x, int y, const BJHand& hand) {
        for (int i = 0; i < hand.cardCount(); ++i) {
            std::string one = cardToStringUTF8(hand.cardAt(i)); // 例: ♠5
            put(x + i * 40, y, one);
        }
        };

    // ディーラー：1枚目を伏せる（??）
    auto drawDealerCardsHidden = [&](int x, int y) {
        for (int i = 0; i < dealer.hand.cardCount(); ++i) {
            if (i == 0) {
                // 伏せる条件：
                // プレイヤーターン中、CPUターン中は伏せたまま
                // DealerTurn/Settle/RoundEnd では公開
                bool hide =
                    (state == State::PlayerTurn) ||
                    (state == State::CpuTurn);

                if (hide) {
                    put(x + i * 40, y, "??");
                }
                else {
                    put(x + i * 40, y, cardToStringUTF8(dealer.hand.cardAt(i)));
                }
            }
            else {
                put(x + i * 40, y, cardToStringUTF8(dealer.hand.cardAt(i)));
            }
        }
        };

    //========================
    // 上部メッセージ
    //========================
    put(40, 30, lastMessage);

    //========================
    // Dealer表示
    //========================
    put(40, 70, "DEALER");
    drawDealerCardsHidden(140, 70);

    // ディーラーのスコア表示（伏せている時は見せない）
    {
        bool hideScore = (state == State::PlayerTurn) || (state == State::CpuTurn);
        if (hideScore) put(40, 95, "score: ??");
        else           put(40, 95, "score: " + std::to_string(dealer.hand.bestScore()));
    }

    //========================
    // プレイヤー/CPU表示（カードを1枚ずつ）
    //========================
    int baseY = 140;
    for (int i = 0; i < 4; ++i) {
        auto& p = players[i];

        // 名前・スコア
        put(40, baseY + i * 70, p.name +
            "  score:" + std::to_string(p.hand.bestScore()) +
            (p.hand.isBust() ? " [BUST]" : "") +
            (p.doubled ? " [DD]" : "")
        );

        // カード列
        drawHandCards(140, baseY + i * 70, p.hand);

        // chips / bet
        put(40, baseY + i * 70 + 20,
            "chips:" + std::to_string(p.chips) + "  bet:" + std::to_string(p.bet));
    }

    //========================
    // ベット画面のラベル
    //========================
    if (state == State::Betting) {
        put(60, 520, "BET: " + std::to_string(uiPlayerBet));
        put(100, 540, "-10");
        put(240, 540, "+10");
        put(420, 540, "OK");
    }

    //========================
    // 行動ラベル
    //========================
    if (state == State::PlayerTurn) {
        put(760, 540, "HIT");
        put(930, 540, "STAND");
        put(1100, 540, "DOUBLE");
    }

    if (state == State::RoundEnd) {
        put(420, 540, "NEXT");
    }

    // タイトルボタン表示名
    put(60, 60, "TITLE");
}