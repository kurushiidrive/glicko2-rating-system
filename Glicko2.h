#pragma once
#include "Player.h"
#include <functional>
#include <map>
#include <string>
#include <vector>
#include <cmath>

class Glicko2
{
public:
	Glicko2(double tau = 0.6);
	Glicko2(std::map<std::string, Player>, double tau = 0.6);

	void Run();
	void Run(const std::string& name);
	void Add_Player(const Player&);

	std::map<std::string, Player> Get_Players() const
	{ return players; }

	Player Get_Player(const std::string& name)
	{ return players[name]; }

private:
	double SYS_CONST;
	std::map<std::string, Player> players;
	
	double f (double x, std::function<double(double)> compute)
	{ return compute(x); }

	std::vector<double> Single_Run(double rating, double rd, double volatility, int num_matches, const std::vector<double>& opp_rating, const std::vector<double>& opp_rd, const std::vector<int>& scores);

	double g (double p)
	{ return 1/(std::sqrt(1+(3*(std::pow(p,2))/(std::pow(M_PI,2))))); }

	double E (double m, double mj, double pj)
	{ return 1/(1+std::exp(-1*g(pj)*(m-mj))); }
};
