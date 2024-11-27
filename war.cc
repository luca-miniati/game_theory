#include <iostream>
#include <vector>
#include <ctime>
#include <iomanip>
#include <string>
using namespace std;

/*
PROBLEM STATEMENT
Colonel Blotto and his arch-enemy, Boba Fett, are at war. Each commander has S soldiers in total,
and each soldier can be assigned to one of N < S battlefields. Naturally, these commanders do not
communicate and hence direct their soldiers independently. Any number of soldiers can be allocated
to each battlefield, including zero. A commander claims a battlefield if they send more soldiers to
the battlefield than their opponent. The commander’s job is to break down his pool of soldiers into
groups to which he assigned to each battlefield. The winning commander is the one who claims the
most battlefields. For example, with (S, N) = (10, 4) a Colonel Blotto may choose to play (2, 2, 2, 4)
while Boba Fett may choose to play (8, 1, 1, 0). In this case, Colonel Blotto would win by claiming
three of the four battlefields. The war ends in a draw if both commanders claim the same number of
battlefields.
Write a program where each player alternately uses regret-matching to find a Nash equilibrium for
this game with S = 5 and N = 3. Some advice: before starting the training iterations, first think
about all the valid pure actions for one player; then, assign each pure strategy an ID number. Pure
actions can be represented as strings, objects, or 3-digit numbers: make a global array of these pure
actions whose indices refer to the ID of the strategy. Then, make a separate function that returns
the utility of the one of the players given the IDs of the actions used by each commander.

RESULTS
The algorithm never picks unbalanced strategies ((5, 0, 0), (0, 1, 4), etc.).
*/

// number of battlefields
const int N = 3;
// number of soldiers
const int S = 5;

int NUM_ACTIONS;
// stragegies[i] = string s where s[i] = the number of soldiers allocated to battlefield i
vector<string> actions;
// utility[s][t] = utility from playing s against t
vector<vector<int> > utility;

// player strategies, where s[a] = probability of playing action a
vector<double> s0, s1;
// sum of player strategies, accumulated during training
vector<double> sum0, sum1;
// player regret arrays
vector<double> r0, r1;

void dfs(string t, int i, int s) {
    if (i == N - 1) actions.push_back(t + to_string(s));
    else for (int j = 0; j <= s; ++j) dfs(t + to_string(j), i + 1, s - j);
}

void init_actions() {
    // generate all actions
    dfs("", 0, S);

    NUM_ACTIONS = actions.size();
    utility.resize(NUM_ACTIONS, vector<int>(NUM_ACTIONS));

    // populate utility lookup table
    for (int a = 0; a < NUM_ACTIONS; ++a) {
        for (int b = 0; b < NUM_ACTIONS; ++b) {
            int u = 0;
            for (int i = 0; i < N; ++i) {
                int s = (actions[a][i] - '0');
                int t = (actions[b][i] - '0');
                if (s > t) ++u;
                if (s < t) --u;
            }
            utility[a][b] = u;
        }
    }
}

void reset_arrays() {
    // initialize arrays
    s0.resize(NUM_ACTIONS, 0);
    s1.resize(NUM_ACTIONS, 0);
    sum0.resize(NUM_ACTIONS, 0);
    sum1.resize(NUM_ACTIONS, 0);
    r0.resize(NUM_ACTIONS, 0);
    r1.resize(NUM_ACTIONS, 0);
}

// get random action from strategy vector s
int get_action(vector<double>& s) {
    double r = static_cast<double>(rand()) / RAND_MAX;
    int a = 0;
    double p = 0;
    while (a < NUM_ACTIONS - 1) {
        p += s[a];
        if (r < p) break;
        ++a;
    }
    return a;
}

// update strategy s with regret matching on regret array r
void update_strategy(vector<double>& s, vector<double>& sum, vector<double>& r) {
    double norm = 0;
    for (int a = 0; a < NUM_ACTIONS; ++a) {
        s[a] = (r[a] > 0) ? r[a] : 0;
        norm += s[a];
    }
    for (int a = 0; a < NUM_ACTIONS; ++a) {
        if (norm > 0)
            s[a] /= norm;
        else
            s[a] = 1.0 / NUM_ACTIONS;
        sum[a] += s[a];
    }
}

// get average strategy from cumulative strategy array s
vector<double> get_average_strategy(vector<double>& sum) {
    vector<double> res(NUM_ACTIONS, 0);
    double norm = 0;

    for (int a = 0; a < NUM_ACTIONS; ++a)
        norm += sum[a];
    for (int a = 0; a < NUM_ACTIONS; ++a) {
        if (norm > 0)
            res[a] = sum[a] / norm;
        else
            res[a] = 1.0 / NUM_ACTIONS;
    }

    return res;
}

void train(int n) {
    for (int i = 0; i < n; ++i) {
        reset_arrays();

        int a0 = get_action(s0);
        int a1 = get_action(s1);

        for (int a = 0; a < NUM_ACTIONS; ++a) {
            r0[a] += utility[a][a1] - utility[a0][a1];
            r1[a] += utility[a][a0] - utility[a1][a0];
        }

        update_strategy(s0, sum0, r0);
        update_strategy(s1, sum1, r1);
    }
}

void print_solution() {
    cout << fixed << setprecision(3);
    cout << "ACTIONS:\n     ";
    for (string a : actions) {
        cout << a; for (int i = 0; i <= N; ++i) cout << " ";
    }
    cout << "\n";
    cout << "P0:\n    ";
    for (double p : get_average_strategy(sum0))
        cout << p << "  ";
    cout << "\n";
    cout << "P1:\n    ";
    for (double p : get_average_strategy(sum1))
        cout << p << "  ";
    cout << "\n";
}

int main() {
    // seed random
    srand(static_cast<unsigned>(time(0)));

    init_actions();

    for (int i = 0; i < 10; ++i) {
        train(10000);

        cout << "EPOCH " << i + 1 << "\n";
        print_solution();
        cout << "\n";
    }
}
