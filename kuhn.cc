#include <iostream>
#include <vector>
#include <deque>
#include <set>
#include <map>
#include <random>
using namespace std;

/*
This file implements CFR for Kuhn Poker.

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
const int T = 100000;
// can only check or bet
const int NUM_ACTIONS = 2;
// have 3 cards
const int NUM_CARDS = 3;
// terminal states are pre-defined
const set<string> TERMINAL_STATES = {"rrcc", "rrbc", "rrbb", "rrcbc", "rrcbb"};
// actions are pre-defined
const vector<string> ACTIONS = {"c", "b"};

class Node {
private:
    double total_reach_probability;
    double strategy[NUM_ACTIONS];
    double strategy_sum[NUM_ACTIONS];
    double regret_sum[NUM_ACTIONS];
public:
    int player_to_move;
    int c1;
    int c2;
    string history;
    vector<Node*> children;
    double reach_probability;

    Node(int player_to_move, int c1, int c2, string history, double reach_probability)
    : player_to_move(player_to_move), c1(c1), c2(c2), history(history),
    reach_probability(reach_probability) {
        // init with uniform strategy
        for (int a = 0; a < NUM_ACTIONS; ++a)
            this->strategy[a] = 1.0 / NUM_ACTIONS;
    }

    void update_strategy() {
        for (int a = 0; a < NUM_ACTIONS; ++a)
            this->strategy_sum[a] += this->reach_probability * this->strategy[a];
        this->match_regrets();
        this->total_reach_probability += this->reach_probability;
        this->reach_probability = 0;
    }

    double cfr(double p1, double p2, double pc) {
        // if no history, we are the root node
        if (!this->history.length()) {
            double EV = 0;
            int n = this->children.size();
            for (Node* v : this->children)
                EV += v->cfr(1, 1, 1.0 / n);
            return EV / n;
        }

        // if we're in a terminal state (game is over)
        if (this->is_terminal())
            return this->EV();

        // otherwise, we're still playing
        if (player_to_move == 1)
            this->reach_probability += p1;
        else
            this->reach_probability += p2;

        // define counterfactual utility array
        double cf_utility[NUM_ACTIONS];
        for (int a = 0; a < NUM_ACTIONS; ++a) {
            Node* v = this->children[a];
            if (player_to_move == 1)
                cf_utility[a] = -1 * v->cfr(p1 * this->strategy[a], p2, pc);
            else
                cf_utility[a] = -1 * v->cfr(p1, p2 * this->strategy[a], pc);
        }

        // compute utility
        double utility = 0;
        for (int a = 0; a < NUM_ACTIONS; ++a)
            utility += cf_utility[a] * this->strategy[a];

        // update regret sum
        for (int a = 0; a < NUM_ACTIONS; ++a) {
            if (player_to_move == 1)
                this->regret_sum[a] += p2 * pc * (cf_utility[a] - utility);
            else
                this->regret_sum[a] += p1 * pc * (cf_utility[a] - utility);
        }

        return utility;
    }

    void match_regrets() {
        for (int a = 0; a < NUM_ACTIONS; ++a)
            this->strategy[a] = std::max(0.0, this->regret_sum[a]);

        double norm = std::accumulate(this->strategy, this->strategy + NUM_ACTIONS, 0.0);

        for (int a = 0; a < NUM_ACTIONS; ++a) {
            if (norm > 0)
                this->strategy[a] /= norm;
            else
                this->strategy[a] = 1.0 / NUM_ACTIONS;
            this->strategy_sum[a] += this->strategy[a];
        }
    }

    double EV() {
        if (!is_terminal())
            throw std::runtime_error("you tried to call Node.EV() on a non-terminal node.");

        int this_card = (this->player_to_move == 1) ? c1 : c2;
        int other_card = (this->player_to_move == 1) ? c2 : c1;

        // we bet and they folded
        if (this->history == "rrbc" || this->history == "rrcbc")
            return 1;

        // action went check check
        if (this->history == "rrcc")
            return (this_card > other_card) ? 1 : -1;

        // otherwise, history = "rrcbb" or "rrbb"
        return (this_card > other_card) ? 2 : -2;
    }

    bool is_terminal() {
        return TERMINAL_STATES.count(this->history);
    }
};

void update_tree(Node* u) {
    if (u->history.length() > 2 && !u->is_terminal()) {
        u->update_strategy();
        for (Node* v : u->children)
            update_tree(v);
    }
}

Node* build_tree() {
    deque<Node*> Q;
    Node* root = new Node(1, -1, -1, "", 1);
    for (int c1 = 1; c1 <= NUM_CARDS; ++c1) {
        for (int c2 = 1; c2 <= NUM_CARDS; ++c2) {
            if (c1 == c2) continue;

            Node* u = new Node(1, c1, c2, "rr", 1);
            Q.push_back(u);
            root->children.push_back(u);
        }
    }

    while (Q.size()) {
        Node* curr = Q.front();
        Q.pop_front();

        if (curr->is_terminal()) {
            continue;
        }

        Node* bet = new Node((curr->player_to_move == 1) ? 2 : 1,
            curr->c1, curr->c2, curr->history + "b", 1);
        Q.push_back(bet);
        curr->children.push_back(bet);
        Node* check = new Node((curr->player_to_move == 1) ? 2 : 1,
            curr->c1, curr->c2, curr->history + "c", 1);
        Q.push_back(check);
        curr->children.push_back(check);
    }

    return root;
}

void train() {
    Node* tree = build_tree();

    double EV = 0;
    for (int i = 0; i < T; ++i) {
        EV += tree->cfr(1, 1, 1);
        update_tree(tree);
    }
    cout << "Expected game value: " << EV / T << '\n';
}

int main() {
    train();
}
