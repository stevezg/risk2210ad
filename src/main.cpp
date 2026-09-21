#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "risk2210/Agents.h"
#include "risk2210/Game.h"

using namespace risk2210;

static void usage() {
    std::cout << "Usage: risk2210_cli [--players N] [--human I[,I...]] [--seed S] [--quiet]\n"
                 "  --players N   number of players, 2-5 (default 3)\n"
                 "  --human I     player index(es) controlled interactively (default: none, all AI)\n"
                 "  --seed S      RNG seed for a reproducible game\n"
                 "  --quiet       only print final scoring\n";
}

int main(int argc, char** argv) {
    int players = 3;
    std::vector<int> humans;
    unsigned seed = std::random_device{}();
    bool quiet = false;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        auto next = [&]() -> std::string { return (i + 1 < argc) ? argv[++i] : ""; };
        if (a == "--players") players = std::stoi(next());
        else if (a == "--human") {
            std::string s = next();
            size_t pos = 0;
            while (pos < s.size()) {
                size_t comma = s.find(',', pos);
                humans.push_back(std::stoi(s.substr(pos, comma - pos)));
                if (comma == std::string::npos) break;
                pos = comma + 1;
            }
        } else if (a == "--seed") seed = static_cast<unsigned>(std::stoul(next()));
        else if (a == "--quiet") quiet = true;
        else {
            usage();
            return a == "--help" || a == "-h" ? 0 : 1;
        }
    }
    if (players < 2 || players > 5) {
        usage();
        return 1;
    }

    const char* names[] = {"Red", "Blue", "Green", "Yellow", "Black"};
    std::vector<std::string> playerNames(names, names + players);
    Game game(playerNames, seed);

    std::vector<std::unique_ptr<Agent>> agents;
    for (int p = 0; p < players; ++p) {
        bool human = std::find(humans.begin(), humans.end(), p) != humans.end();
        if (human) agents.push_back(std::make_unique<HumanCliAgent>(std::cin, std::cout));
        else agents.push_back(std::make_unique<RandomAgent>(seed + p + 1));
        game.setAgent(p, agents.back().get());
    }
    if (!quiet || !humans.empty()) game.setLogger([](const std::string& s) { std::cout << s << "\n"; });

    game.run();

    if (quiet) {
        for (int p = 0; p < players; ++p)
            std::cout << game.player(p).name << ": " << game.player(p).finalScore << "\n";
    }
    return 0;
}
