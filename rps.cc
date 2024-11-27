#include <iostream>
#include <ctime>
#include <iomanip>
using namespace std;

const int NUM_ACTIONS = 3;
double regret_sum[2][NUM_ACTIONS];
double strategy[2][NUM_ACTIONS];
double strategy_sum[2][NUM_ACTIONS];
double avg_strategy[2][NUM_ACTIONS];

void set_strategy(int p) {
    double norm = 0;
    for (int a = 0; a < NUM_ACTIONS; ++a) {
        strategy[p][a] = (regret_sum[p][a] > 0) ? regret_sum[p][a] : 0;
        norm += strategy[p][a];
    }
    for (int a = 0; a < NUM_ACTIONS; ++a) {
        if (norm > 0)
            strategy[p][a] /= norm;
        else
            strategy[p][a] = 1.0 / NUM_ACTIONS;
        strategy_sum[p][a] += strategy[p][a];
    }
}

void set_average_strategy(int p) {
    double norm = 0;
    for (int a = 0; a < NUM_ACTIONS; ++a)
        norm += strategy_sum[p][a];
    for (int a = 0; a < NUM_ACTIONS; ++a) {
        if (norm > 0)
            avg_strategy[p][a] = strategy_sum[p][a] / norm;
        else
            avg_strategy[p][a] = 1.0 / NUM_ACTIONS;
    }
}

int get_action(int p) {
    double r = static_cast<double>(rand()) / RAND_MAX;
    int a = 0;
    double cumulative_prob = 0;
    while (a < NUM_ACTIONS - 1) {
        cumulative_prob += strategy[p][a];
        if (r < cumulative_prob) break;
        ++a;
    }
    return a;
}

void train(int n) {
    double utility[2][NUM_ACTIONS];
    for (int i = 0; i < n; ++i) {
        int j = get_action(0);
        int k = get_action(1);

        // set P1's utility
        // utility of identical action is 0
        // if k is scissors, then rock has utility 1, otherwise k + 1 has utility 1
        // if k is rock, then scissors has utility -1, otherwise k - 1 has utility -1
        utility[0][k] = 0;
        utility[0][(k == NUM_ACTIONS - 1) ? 0 : k + 1] = 1;
        utility[0][(k == 0) ? NUM_ACTIONS - 1 : k - 1] = -1;
        // set P2's utility with same reasoning
        utility[1][j] = 0;
        utility[1][(j == NUM_ACTIONS - 1) ? 0 : j + 1] = 1;
        utility[1][(j == 0) ? NUM_ACTIONS - 1 : j - 1] = -1;

        for (int a = 0; a < NUM_ACTIONS; ++a) {
            regret_sum[0][a] += utility[a] - utility[k];
            regret_sum[1][a] += utility[a] - utility[j];
        }

        set_strategy(0);
        set_strategy(1);
    }
}

int main() {
    // seed random
    srand(static_cast<unsigned>(time(0)));

    int n = 10;
    for (int i = 0; i < n; ++i) {
        // zero out all arrays
        for (int p = 0; p < 1; ++p)
            for (int a = 0; a < NUM_ACTIONS; ++a)
                regret_sum[p][a] = strategy[p][a] = strategy_sum[p][a] = avg_strategy[p][a] = 0;

        train(100000);

        set_average_strategy(0);
        set_average_strategy(1);

        cout << fixed << setprecision(2);
        cout << "EPOCH " << i + 1 << '\n';
        cout << "P1: ";
        for (int a = 0; a < NUM_ACTIONS; ++a)
            cout << avg_strategy[0][a] << ' ';
        cout << '\n';
        cout << "P2: ";
        for (int a = 0; a < NUM_ACTIONS; ++a)
            cout << avg_strategy[1][a] << ' ';
        cout << '\n';
        cout << '\n';
    }
}
