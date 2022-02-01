#include "Player.h"
#include <vector>
#include <utility>

Player::Player(std::string name, double rating, double rd, double volatility) :
	name{ name },
	rating{ rating },
	rd{ rd },
	volatility{ volatility }
{}

Player::Player(std::string name, double rating, double rd, double volatility, std::vector<Player>& opps, std::vector<int>& scores) :
	name{ name },
	rating{ rating },
	rd{ rd },
	volatility{ volatility }
{
	for (size_t i = 0; i < opps.size(); i++)
	{
		std::pair<Player, int> match { opps[i], scores[i] };
		match_history.push_back(match);
	}
}

void Player::Add_Match (const Player& player, int result)
{
	std::pair<Player, int> match { player, result };
	match_history.push_back(match);
}

void Player::Add_Match (const std::pair<Player, int>& match)
{
	match_history.push_back(match);
}
