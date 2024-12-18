#include <algorithm>
#include <cstdlib>
#include <iomanip>
#include <ios>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>
#include <unordered_map>
#include <map>
#include <unordered_set>
#include <set>
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
    double get_utility(vector<int>& cards) {
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
            return node->get_utility(cards);

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
            return node->get_utility(cards);

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
        // we do 6 permutations to guarantee that we cover all possible states
        // at least once
        for (int i = 0; i < T + 6; ++i) {
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

class Game {
private:
    unordered_map<string, Node*> solution;
    string p1, p2;
    int player_card, bot_card, player_stack = 10, bot_stack = 10;
    string player_card_string, bot_card_string;
    vector<int> cards = {1, 2, 3};
    map<int, string> num_to_card = {{1, "J"}, {2, "Q"}, {3, "K"}};

    void setup() {
        cout << "*  Enter difficulty level:" << endl << "   (1/2/3) ";
        int difficulty; cin >> difficulty;
        while (difficulty != 1 && difficulty != 2 && difficulty != 3) {
            cout << "   (1/2/3) ";
            cin >> difficulty;
        }

        cout << endl;
        cout << "-> Training the algorithm..." << endl;
        Solver solver;
        clock_t start_time = clock();
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
        cout << "-> Done! Trained for " << fixed << setprecision(4) <<
        static_cast<double>(clock() - start_time) / CLOCKS_PER_SEC << " seconds" << endl << endl;

        this->solution = solver.get_solution();
        cout << "*  Choose player: (player 1 goes first, player 2 goes second)" << endl << "   (1/2) ";
        int p; cin >> p;
        while (p != 1 && p != 2) {
            cout << "   (1/2) ";
            cin >> p;
        }
        this->p1 = (p == 1) ? "Player" : "Bot";
        this->p2 = (p == 1) ? "Bot" : "Player";
        cout << endl;
    }

    void display() {
        system("clear");
        cout <<
        "+---------------------------------+" << endl <<
        "|            +-------+            |" << endl <<
        "|            |  Bot  |            |" << endl <<
        "|            +-------+            |" << endl <<
        "|           +---+                 |" << endl <<
        "|   Card:   | ? |     Stack: " << setw(2) << to_string(bot_stack) << "   |" << endl <<
        "|           +---+                 |" << endl <<
        "|                                 |" << endl <<
        "|           +---+                 |" << endl <<
        "|   Card:   | " << this->num_to_card[player_card] << " |     Stack: " << setw(2) << to_string(player_stack) << "   |" << endl <<
        "|           +---+                 |" << endl <<
        "|           +----------+          |" << endl <<
        "|           |  Player  |          |" << endl <<
        "|           +----------+          |" << endl <<
        "+---------------------------------+" << endl << endl;
    }

    void display_hand_result(string h, int res) {
        if (h.back() == 'c' && h != "cc") {
            // 0 if p1 folded, 1 if p2 folded
            int folded = (h.length() + 1) % 2;
            cout << "!  " << ((folded) ? this->p2 : this->p1) << " folds" << endl;
        } else {
            // showdown
            cout << "!  Bot shows " << this->num_to_card[this->bot_card];
            cout << ", Player shows " << this->num_to_card[this->player_card] << endl;
        }

        // display change in stacks
        if (res > 0)
        cout << "+  Player wins " << to_string(abs(res)) <<
        " chip" << "s "[abs(res) == 1] << endl << endl;
        else
        cout << "-  Player loses " << to_string(abs(res)) <<
        " chip" << "s "[abs(res) == 1] << endl << endl;
    }

    void display_welcome_message() {
        cout <<
        "                                                        ****___*****" << endl <<
        "                                                      **____***____**__***" << endl <<
        "                                                    **___***____*******___**" << endl <<
        "                                                  *********************************" << endl <<
        "                                               *****------------------------------*" << endl <<
        "                                            ****__**------------------------------*" << endl <<
        "                                          ***_**__*_------------------------------*" << endl <<
        "                                       ***_**_***_*_------------------------------*" << endl <<
        "                                        **__*_*_****------------------------------*" << endl <<
        "                                         **_*_******------------------------------*" << endl <<
        "                                                                                   .-'''-.                                        " << endl <<
        "                                                                                  '   _    \\                                      " << endl <<
        "     .                   .            _..._             _________   _...._      /   /` '.   \\    .          __.....__             " << endl <<
        "   .'|                 .'|          .'     '.           \\        |.'      '-.  .   |     \\  '  .'|      .-''         '.           " << endl <<
        " .'  |                <  |         .   .-.   .           \\        .'```'.    '.|   '      |  .'  |     /     .-''\"'-.  `..-,.--.  " << endl <<
        "<    |                 | |         |  '   '  |            \\      |       \\     \\    \\     / <    |    /     /________\\   |  .-. | " << endl <<
        " |   | ____     _    _ | | .'''-.  |  |   |  |             |     |        |    |`.   ` ..' / |   | ___|                  | |  | | " << endl <<
        " |   | \\ .'    | '  / || |/.'''. \\ |  |   |  |             |      \\      /    .    '-...-'`  |   | \\ .\\    .-------------| |  | | " << endl <<
        " |   |/  .    .' | .' ||  /    | | |  |   |  |             |     |\\`'-.-'   .'               |   |/  . \\    '-.____...---| |  '-  " << endl <<
        " |    /\\  \\   /  | /  || |     | | |  |   |  |             |     | '-....-'`                 |    /\\  \\ `.             .'| |      " << endl <<
        " |   |  \\  \\ |   `'.  || |     | | |  |   |  |            .'     '.                          |   |  \\  \\  `''-...... -'  | |      " << endl <<
        " '    \\  \\  \'   .'|  '| '.    | '.|  |   |  |          '-----------'                        '    \\  \\  \\                |_|      " << endl <<
        "'------'  '---`-'  `--''---'   '---'--'   '--'                                              '------'  '---'                       " << endl <<
        "                                               *****------------------------------*" << endl <<
        "                                                ***_------------------------------*" << endl <<
        "                                                 **_------------------------------*" << endl <<
        "                                                  **------------------------------*" << endl <<
        "                                                  **------------------------------*" << endl <<
        "                                                   **__****************************" << endl <<
        "                                                   ************" << endl <<
        "                                                      ******" << endl;

        cout << "Welcome to Kuhn Poker!" << endl << endl;
    }

    void handle_terminal_state(string h, Node* node) {
        // get result of showdown
        int res = (int) node->get_utility(this->cards);
        // normalize res wrt p1
        if (h.length() == 3)
            res = -res;
        // normalize res wrt player
        if (p1 == "Bot")
            res = -res;

        // show res
        display_hand_result(h, res);
        player_stack += res;
        bot_stack -= res;

        // wait for player to hit enter
        cout << "(Enter) to continue" << endl;
        cin.ignore();
        char temp = 'x';
        while (temp != '\n')
        cin.get(temp);
    }

    string get_player_action() {
        cout << "*  Check or bet:" << endl << "(c/b) ";
        string c; cin >> c;
        while (c != "c" && c != "b") {
            cout << "   (c/b) ";
            cin >> c;
        }
        cout << "-> Player plays " << ((c == "c") ? "check" : "bet") << endl << endl;
        return c;
    }

    string get_bot_action(string h) {
        // get node corresponding to this history and the bot's card
        Node* node = this->solution[this->bot_card_string + h];
        // get the optimal strategy
        vector<double> strategy = node->get_average_strategy();
        // pick the move
        double r = ((double) rand()) / ((double) RAND_MAX);
        cout << "-> Bot plays " << ((r <= strategy[0]) ? "check" : "bet") << endl << endl;
        return (r <= strategy[0]) ? "c" : "b";
    }

    void play_hand() {
        this->display();
        string h;
        string turn = p1;
        // while hand is running
        while (!TERMINAL_HISTORIES.count(h)) {
            if (turn == "Player") {
                // get player's move
                h += get_player_action();
            } else {
                h += get_bot_action(h);
            }

            Node* node = this->solution["1" + h];

            // check if that move ended the game
            if (node->is_terminal())
            // if it did, do this
                handle_terminal_state(h, node);

            turn = (turn == "Player") ? "Bot" : "Player";
        }
    }
public:
    void play() {
        display_welcome_message();
        this->setup();

        // make rng
        random_device rd = random_device();
        default_random_engine rng = default_random_engine(rd());

        // game loop
        bool is_game_over = false;
        while (!is_game_over) {
            ranges::shuffle(cards, rng);
            bool oop = (this->p1 == "Player");
            this->player_card = cards[oop ? 0 : 1];
            this->player_card_string = to_string(cards[oop ? 0 : 1]);
            this->bot_card = cards[oop ? 1 : 0];
            this->bot_card_string = to_string(cards[oop ? 1 : 0]);

            this->play_hand();
            if (player_stack <= 0 || bot_stack <= 0)
            is_game_over = true;
        }

        cout << "Game over!" << endl;
        cout << ((player_stack <= 0) ? "Bot" : "Player") << " won." << endl;
    }
};

int main() {
    Game game;
    game.play();
}
