#pragma once
#include <vector>
#include <string>
#include <utility> // std::pair

class Player
{
public:
	Player (std::string name, double rating, double rd, double volatility, std::vector<Player>& opps, std::vector<int>& scores);
	Player(std::string name = "Player", double rating = 1500, double rd = 350, double volatility = 0.06);

	std::string Get_Name() const
	{ return name; }

	double Get_Rating() const
	{ return rating; }

	double Get_RD() const
	{ return rd; }

	double Get_Vol() const
	{ return volatility; }

	std::vector<std::pair<Player, int>> Get_Match_History() const
	{ return match_history; }

	void Set_Name(const std::string& name)
	{ this->name = name; }

	void Set_Rating(double rating)
	{ this->rating = rating; }

	void Set_RD(double rd)
	{ this->rd = rd; }

	void Set_Vol (double volatility)
	{ this->volatility = volatility; }

	void Set_Match_History (const std::vector<std::pair<Player, int>>& mh)
	{ this->match_history = mh; }

	void Add_Match (const Player&, int);
	void Add_Match (const std::pair<Player, int>&);

private:
	std::string name;
	double rating;
	double rd;
	double volatility;
	std::vector<std::pair<Player, int>> match_history;
};
