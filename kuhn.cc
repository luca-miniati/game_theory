#include <iostream>
#include <vector>
#include <random>
using namespace std;

/*
This project implements CFR for Kuhn Poker.

In Kuhn Poker, both players ante 1 chip. Then each player is dealt a card out of the deck
{1, 2, 3}. Play alternates starting with player 1. A player can check or bet 1 chip. When a
player passes after a bet, the opponent takes all chips in the pot. When there are two
successive passes or two successive bets, both players reveal their cards, and the player with
the higher card takes all chips in the pot.

Table of all possible game sequences:

----------------------------------------------------------
|  P1   |  P2   |  P1   |           Result               |
----------------------------------------------------------
| check | check |       | +1 to player with higher card  |
| check |  bet  | check | +1 to P2                       |
| check |  bet  |  bet  | +2 to player with higher card  |
|  bet  | check |       | +1 to P1                       |
|  bet  |  bet  |       | +2 to player with higher card  |
----------------------------------------------------------

DEFINITIONS
History           - Sequence of actions starting from the root of the game that result in a
                    game state.
Reach Probability - The probability of reaching a particular game state.
Information set   - Containins an active player, and all the information available to that player
                    at that decision in the game. Could consist of multiple possible game states,
                    if the player is missing information.

                    There are 12 possible information sets:
                    * PX = player to move, CX = card of PX, H = history of betting
                    1)  P1, H = {}    , C1 = 1
                    2)  P1, H = {}    , C1 = 2
                    3)  P1, H = {}    , C1 = 3
                    4)  P1, H = {c, b}, C1 = 1
                    5)  P1, H = {c, b}, C1 = 2
                    6)  P1, H = {c, b}, C1 = 3
                    7)  P2, H = {c}   , C2 = 1
                    8)  P2, H = {c}   , C2 = 2
                    9)  P2, H = {c}   , C2 = 3
                    10) P2, H = {b}   , C2 = 1
                    11) P2, H = {b}   , C2 = 2
                    12) P2, H = {b}   , C2 = 3
*/

// number of iterations to run CFR
const int T = 10000;
// can only check or bet
const int NUM_ACTIONS = 2;
// have 3 cards
const int NUM_CARDS = 3;

class Node {
private:
    string history;
public:
    Node() {}
    Node(string history) : history(history) {}
    bool is_terminal() {
        // if there are 2 moves, h terminal <=> (h = *c) or (h = b*)
        if (history.size() == 2) return history.back() == 'c' || history.front() == 'b';
        // otherwise, h terminal <=> h has 3 moves
        return history.size() == 3;
    }

    double get_utility() {
        return 0;
    }
};

double cfr(int cards[NUM_CARDS], string history, int s1, int s2) {
    return 0;
}

void shuffle(int (&cards)[NUM_CARDS]) {
    random_device rd;
    mt19937 rng(rd());
    uniform_int_distribution<int> dist(0, NUM_CARDS - 1);
    for (int c1 = NUM_CARDS - 1; c1 > 0; --c1) {
        int c2 = dist(rng);
    }
}

void train() {
    int cards[3] = {0, 1, 2};
    double utility = 0;
    for (int i = 0; i < T; ++i) {
        shuffle(cards);
        utility += cfr(cards, "", 1, 1);
    }
}

int main() {
    train();
}
