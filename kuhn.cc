#include <algorithm>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <unordered_map>
#include <map>
#include <unordered_set>
#include <random>
#include <functional>
#include <ctime>
using namespace std;

// This file implements CFR for Kuhn Poker.

// In Kuhn Poker, both players ante 1 chip. Then each player is dealt a card out of the deck
// {1, 2, 3}. Play alternates starting with player 1. A player can check or bet 1 chip. When a
// player passes after a bet, the opponent takes all chips in the pot. When there are two
// successive passes or two successive bets, both players reveal their cards, and the player with
// the higher card takes all chips in the pot.

// Table of all possible game sequences:

// ----------------------------------------------------------
// |  P1   |  P2   |  P1   |           Result               |
// ----------------------------------------------------------
// | check | check |       | +1 to player with higher card  |
// | check |  bet  | check | +1 to P2                       |
// | check |  bet  |  bet  | +2 to player with higher card  |
// |  bet  | check |       | +1 to P1                       |
// |  bet  |  bet  |       | +2 to player with higher card  |
// ----------------------------------------------------------

// DEFINITIONS
// History           - Sequence of actions starting from the root of the game that result in a
//                     game state.
// Reach Probability - The probability of reaching a particular game state.
// Information set   - Containins an active player, and all the information available to that player
//                     at that decision in the game. Could consist of multiple possible game states,
//                     if the player is missing information.

// There are 12 possible information sets:
// * PX = player to move, CX = card of PX, H = history of betting
// 1)  P1, H = {}    , C1 = 1
// 2)  P1, H = {}    , C1 = 2
// 3)  P1, H = {}    , C1 = 3
// 4)  P1, H = {c, b}, C1 = 1
// 5)  P1, H = {c, b}, C1 = 2
// 6)  P1, H = {c, b}, C1 = 3
// 7)  P2, H = {c}   , C2 = 1
// 8)  P2, H = {c}   , C2 = 2
// 9)  P2, H = {c}   , C2 = 3
// 10) P2, H = {b}   , C2 = 1
// 11) P2, H = {b}   , C2 = 2
// 12) P2, H = {b}   , C2 = 3

const string ACTIONS = "cb";
const int NUM_ACTIONS = ACTIONS.length();
const unordered_set<string> TERMINAL_HISTORIES = {"cc", "bb", "bc", "cbc", "cbb"};

class Node {
private:
    string info_set;
    vector<double> regret_sum, strategy, strategy_sum;
public:
    Node(string info_set) : info_set(info_set) {
        // zero out all arrays
        this->regret_sum.resize(NUM_ACTIONS);
        this->strategy.resize(NUM_ACTIONS);
        for (int a = 0; a < NUM_ACTIONS; ++a) this->strategy[a] = 1.0 / NUM_ACTIONS;
        this->strategy_sum.resize(NUM_ACTIONS);
    }

    // Returns whether the game is over in this state
    bool is_terminal() {
        // extract history from info set
        string h = this->info_set.substr(1);
        return TERMINAL_HISTORIES.count(h);
    }

    // Returns payoff for terminal state
    double get_payoff(vector<int>& cards) {
        if (!this->is_terminal())
            throw runtime_error("called get_payoff on non-terminal node.");

        // extract history from info set
        string h = this->info_set.substr(1);
        int t = h.length();

        // we bet and they folded
        if (h.substr(t - 2) == "bc")
            return 1;

        int c1 = cards[t % 2];
        int c2 = cards[1 - (t % 2)];

        // action went check check
        if (h == "cc")
            return (c1 > c2) ? 1 : -1;

        // action went bet call
        return (c1 > c2) ? 2 : -2;
    }

    // Returns probability of the player to move to make action a
    double get_action_p(int a) {
        return this->strategy[a];
    }

    // Update strategy using regret matching, using p as the probability
    // of being in this state
    vector<double> get_strategy(double p) {
        double norm = 0;
        for (int a = 0; a < NUM_ACTIONS; a++) {
            this->strategy[a] = max(this->regret_sum[a], 0.0);
            norm += this->strategy[a];
        }
        for (int a = 0; a < NUM_ACTIONS; a++) {
            if (norm > 0)
                this->strategy[a] /= norm;
            else
                this->strategy[a] = 1.0 / NUM_ACTIONS;
            this->strategy_sum[a] += p * this->strategy[a];
        }
        return this->strategy;
    }

    // Update regret value
    void update_regret(int a, double v) {
        this->regret_sum[a] += v;
    }

    // Return computed strategy at this node
    vector<double> get_average_strategy() {
        vector<double> average_strategy(NUM_ACTIONS);
        double norm = 0;
        for (int a = 0; a < NUM_ACTIONS; ++a)
            norm += this->strategy_sum[a];
        for (int a = 0; a < NUM_ACTIONS; ++a) {
            if (norm > 0)
                average_strategy[a] = this->strategy_sum[a] / norm;
            else
                average_strategy[a] = 1.0 / NUM_ACTIONS;
        }
        return average_strategy;
    }
};

class Solver {
private:
    unordered_map<string, Node*> tree;

    // Return node if it exists, otherwise make it and return it
    Node* get_node(string info_set) {
        if (this->tree.count(info_set))
            return this->tree[info_set];

        this->tree[info_set] = new Node(info_set);
        return this->tree[info_set];
    }

    // Use counterfactual regret minimization to compute utility of node
    double cfr(vector<int>& cards, string h, double p1, double p2) {
        int player_idx = h.length() % 2;
        string info_set = to_string(cards[player_idx]) + h;
        Node* node = this->get_node(info_set);

        // base case: return payoff for terminal state
        if (node->is_terminal())
            return node->get_payoff(cards);

        // compute reach probability
        double reach_p = (player_idx == 0) ? p2 : p1;
        // update current player's strategy
        vector<double> strategy = node->get_strategy(reach_p);

        vector<double> util(NUM_ACTIONS);
        // utility of this node (the eventual return value of the cfr)
        double node_util = 0;

        // traverse over possible actions
        for (int a = 0; a < NUM_ACTIONS; ++a) {
            char action = ACTIONS[a];
            // player_idx = 0 corresponds to player 1's action
            if (player_idx == 0)
                util[a] = -cfr(cards, h + action, p1 * strategy[a], p2);
            else
                util[a] = -cfr(cards, h + action, p1, p2 * strategy[a]);

            // update total node utility
            node_util += strategy[a] * util[a];
        }

        // update regret for each action
        for (int a = 0; a < NUM_ACTIONS; ++a) {
            double regret = util[a] - node_util;
            node->update_regret(a, reach_p * regret);
        }

        return node_util;
    }

    // Traverse game tree, returning expected value
    double compute_terminal_payoffs(vector<int>& cards, string h) {
        int player_idx = h.length() % 2;
        string info_set = to_string(cards[player_idx]) + h;
        Node* node = this->get_node(info_set);

        // base case: return payoff for terminal state
        if (node->is_terminal())
            return node->get_payoff(cards);

        vector<double> strategy = node->get_average_strategy();
        // utility of this node (the eventual return value of the cfr)
        double node_util = 0;

        // traverse over possible actions
        for (int a = 0; a < NUM_ACTIONS; ++a) {
            char action = ACTIONS[a];
            node_util += -compute_terminal_payoffs(cards, h + action) * strategy[a];
        }

        return node_util;
    }

    void print_solution(int T, clock_t start_time) {
        // compute EV
        double EV = this->compute_expected_value();
        cout << fixed << setprecision(4) << "Player 1 EV: " << EV << endl;
        cout << fixed << setprecision(4) << "Player 2 EV: " << -EV << endl;
        cout << endl;

        // compute solution for each player
        map<int, vector<pair<string, vector<double>>>> strategy1, strategy2;
        // card values
        map<int, char> cards = {{1, 'J'}, {2, 'Q'}, {3, 'K'}};
        for (auto [info_set, node] : this->tree) {
            if (node->is_terminal())
                continue;
            vector<double> strategy = node->get_average_strategy();
            int card = info_set[0] - '0';
            string h = info_set.substr(1);
            if (info_set.length() % 2)
                strategy1[card].push_back({h, strategy});
            else
                strategy2[card].push_back({h, strategy});
        }
        cout << "Player 1 Strategy:" << endl;
        for (int c = 1; c <= 3; ++c) {
            for (auto [h, strategy] : strategy1[c]) {
                cout << fixed << setprecision(2) <<
                "Card: " << cards[c] <<
                ", History: " << (h.length() ? h : "--") <<
                ", Strategy: check " << strategy[0] * 100 <<
                "% | bet " << strategy[1] * 100 << "%" << endl;
            }
        }
        cout << endl;
        cout << "Player 2 Strategy:" << endl;
        for (int c = 1; c <= 3; ++c) {
            for (auto [h, strategy] : strategy2[c]) {
                cout << fixed << setprecision(2) <<
                "Card: " << cards[c] <<
                ", History: " << (h.length() ? h : "--") <<
                ", Strategy: check " << strategy[0] * 100 <<
                "% | bet " << strategy[1] * 100 << "%" << endl;
            }
        }
        cout << endl;
        cout << "Ran " << T << " iterations." << endl;
        cout << "Runtime: " << static_cast<double>(clock() - start_time) / CLOCKS_PER_SEC << " seconds" << endl;
    }
public:
    // Train cfr algorithm
    void train(int T) {
        vector<int> cards = {1, 2, 3};

        clock_t start_time = clock();
        double util = 0;
        for (int i = 0; i < T; ++i) {
            util += cfr(cards, "", 1, 1);
            next_permutation(cards.begin(), cards.end());
        }
    }

    // Return expected game value
    double compute_expected_value() {
        double EV = 0;

        vector<int> cards = {1, 2, 3};
        // simulate all possible card combos
        for (int i = 0; i < 6; ++i) {
            EV += compute_terminal_payoffs(cards, "") / 6;
            next_permutation(cards.begin(), cards.end());
        }

        return EV;
    }

    // Returns the tree
    unordered_map<string, Node*> get_solution() {
        return this->tree;
    }
};

int main() {
    cout << "Welcome to Kuhn Poker!" << endl;
    cout << "Enter difficulty level:\n(1/2/3) ";
    int difficulty; cin >> difficulty;
    while (difficulty != 1 && difficulty != 2 && difficulty != 3) {
        cout << "(1/2/3) ";
        cin >> difficulty;
    }

    cout << endl;
    cout << "Training the algorithm..." << endl;
    Solver solver;
    switch (difficulty) {
        case 1:
            solver.train(1);
            break;
        case 2:
            solver.train(100);
            break;
        case 3:
            solver.train(500000);
            break;
    }
    cout << "Done!" << endl << endl;

    unordered_map<string, Node*> solution = solver.get_solution();
    cout << "Choose player:\n(1/2) ";
    int p; cin >> p;
    while (p != 1 && p != 2) {
        cout << "(1/2) ";
        cin >> p;
    }

    cout << endl;
    bool is_game_over = false;
    while (!is_game_over) {

    }
}
