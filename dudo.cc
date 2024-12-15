#include <iostream>
#include <ratio>
#include <vector>
#include <unordered_map>
#include <random>
using namespace std;

const int D_TOTAL = 2;
const int NUM_SIDES = 6;
const int NUM_ACTIONS = 2 * NUM_SIDES + 1;
const int DUDO = NUM_ACTIONS - 1;
const vector<string> NUM_CLAIMS =
    {"1", "1", "1", "1", "1", "1", "2", "2", "2", "2", "2", "2"};
const vector<string> CLAIM_RANKS =
    {"2", "3", "4", "5", "6", "1", "2", "3", "4", "5", "6", "1"};

// InfoSet:
//
// i1
// x a*b a*b ...
// ^     ^^^--------n = a, r = b claim
// die I rolled
//
// i2
// y a*b a*b ...

class Node {
private:
    int n_actions;
    string info_set;
    vector<double> regret_sum, strategy, strategy_sum;
public:
    Node(string info_set) : info_set(info_set) {
        this->n_actions = (info_set.length() >= 4) ? NUM_ACTIONS : NUM_ACTIONS - 1;
        // zero out all arrays
        this->regret_sum.resize(n_actions);
        this->strategy.resize(n_actions);
        for (int a = 0; a < n_actions; ++a) this->strategy[a] = 1.0 / n_actions;
        this->strategy_sum.resize(n_actions);
    }

    bool is_terminal() {
        return this->info_set.back() == 'D';
    }

    int get_utility(vector<int>& dice) {
        if (!this->is_terminal())
            throw runtime_error("called get_utility on non-terminal node.");

        int h = this->info_set.length();
        int n = this->info_set[h - 4];
        int r = this->info_set[h - 2];
        int rank_count = (dice[0] == r || dice[0] == 1) + (dice[1] == r || dice[1] == 1);
        int diff = rank_count - n;

        // values are all wrt to challenged player
        if (diff > 0)
            // the claim was smaller
            return diff;
        else if (diff < 0)
            // the claim was bigger
            return -diff;
        else
            // the claim was right
            return 1;
    }

    int get_n_actions() { return this->n_actions; }

    // Update strategy using regret matching, using p as the probability
    // of being in this state
    vector<double> get_strategy(double p) {
        double norm = 0;
        for (int a = 0; a < this->n_actions; a++) {
            this->strategy[a] = max(this->regret_sum[a], 0.0);
            norm += this->strategy[a];
        }
        for (int a = 0; a < this->n_actions; a++) {
            if (norm > 0)
                this->strategy[a] /= norm;
            else
                this->strategy[a] = 1.0 / this->n_actions;
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
        for (int a = 0; a < this->n_actions; ++a)
            norm += this->strategy_sum[a];
        for (int a = 0; a < this->n_actions; ++a) {
            if (norm > 0)
            average_strategy[a] = this->strategy_sum[a] / norm;
            else
            average_strategy[a] = 1.0 / this->n_actions;
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
    double cfr(vector<int>& dice, string h, double p1, double p2) {
        int player_idx = h.length() % 2;
        string info_set = to_string(dice[player_idx]) + h;
        Node* node = this->get_node(info_set);

        // base case: return payoff for terminal state
        if (node->is_terminal())
            return node->get_utility(dice);

        // compute reach probability
        double reach_p = (player_idx == 0) ? p2 : p1;
        // update current player's strategy
        vector<double> strategy = node->get_strategy(reach_p);

        int n_actions = node->get_n_actions();
        vector<double> util(n_actions);
        // utility of this node (the eventual return value of the cfr)
        double node_util = 0;

        // traverse over possible actions
        for (int a = 0; a < n_actions; ++a) {
            string action = (a == DUDO) ? "D" : NUM_CLAIMS[a] + CLAIM_RANKS[a];
            // player_idx = 0 corresponds to player 1's action
            if (player_idx == 0)
                util[a] = -cfr(dice, h + action, p1 * strategy[a], p2);
            else
                util[a] = -cfr(dice, h + action, p1, p2 * strategy[a]);

            // update total node utility
            node_util += strategy[a] * util[a];
        }

        // update regret for each action
        for (int a = 0; a < n_actions; ++a) {
            double regret = util[a] - node_util;
            node->update_regret(a, reach_p * regret);
        }

        return node_util;
    }
public:
    void train(int T) {
        vector<int> dice;
        for (int d = 1; d <= 6; ++d) {
            dice.push_back(d);
            dice.push_back(d);
        }
        // random shuffle
        random_device rd = random_device();
        default_random_engine rng = default_random_engine(rd());
        ranges::shuffle(dice, rng);

        clock_t start_time = clock();
        double util = 0;
        for (int i = 0; i < T; ++i) {
            util += cfr(dice, "", 1, 1);
            next_permutation(dice.begin(), dice.end());
        }

        cout << "Expected game value: " << util / T << '\n';
    }
};


int main() {
    Solver solver;
    solver.train(1000);
}
