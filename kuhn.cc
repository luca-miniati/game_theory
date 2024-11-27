#include <iostream>
#include <vector>
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
const int T;

struct GameState {
    GameState(int c1, int c2) : c1(c1), c2(c2) {}
    int c1, c2;
};

struct InfoSet {
    InfoSet(int player, int card, int history) : player(player), card(card), history(history) {}
    int player;
    int card;
    string history;
};

class Node {
private:
    InfoSet info_set;
    vector<GameState> game_state;
public:
    Node(InfoSet info_set, GameState game_state) : info_set(info_set), game_state(game_state) {}

    bool is_terminal() {
        string h = info_set.history;
        // if there are 2 moves, h terminal <=> (h = *c) or (h = b*)
        if (h.size() == 2) return h.back() == 'c' || h.front() == 'b';
        // otherwise, h terminal <=> h has 3 moves
        return h.size() == 3;
    }

    double utility(int player) {
        string h = info_set.history;

    }
};

double cfr(Node v, player p, int t, int s1, int s2) {
    if (v.is_terminal())
        return v.utility(p);

}

int main() {
    for (int t = 0; t < T; ++t) {
        // run cfr for P1
        cfr({}, 0, t, 1, 1);
        // run cfr for P2
        cfr({}, 1, t, 1, 1);
    }
}
