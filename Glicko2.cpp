#include "Glicko2.h"
#include <vector>
#include <utility>
#include <cmath>

Glicko2::Glicko2(double tau) :
	SYS_CONST{ tau }
{}

Glicko2::Glicko2(std::map<std::string, Player> players, double tau) :
	SYS_CONST{ tau },
	players{ players }
{}

void Glicko2::Run()
{
	for (const auto& player : players)
		Run(player.first);
}

void Glicko2::Run(const std::string& name)
{
	double rating = players[name].Get_Rating();
	double rd = players[name].Get_RD();
	double volatility = players[name].Get_Vol();
	
	std::vector<std::pair<Player, int>> mh = players[name].Get_Match_History();
	int num_matches = mh.size();

	std::vector<double> opp_rating;
	std::vector<double> opp_rd;
	std::vector<int> scores;
	for (const auto& match : mh)
	{
		opp_rating.push_back(match.first.Get_Rating());
		opp_rd.push_back(match.first.Get_RD());
		scores.push_back(match.second);
	}

	std::vector<double> primes = Single_Run(rating, rd, volatility, num_matches, opp_rating, opp_rd, scores);

	players[name].Set_Rating(primes[0]);
	players[name].Set_RD(primes[1]);
	players[name].Set_Vol(primes[2]);
}

void Glicko2::Add_Player(const Player& player)
{
	players.insert(std::pair<std::string, Player>{ player.Get_Name(), player });
}

std::vector<double> Glicko2::Single_Run(double rating, double rd, double volatility, int num_matches, const std::vector<double>& opp_rating, const std::vector<double>& opp_rd, const std::vector<int>& scores)
{
	double rating_p;
	double rd_p;
	double volatility_p;
	double mu_p, phi_p;

	if (num_matches == 0)
	{
		phi_p = std::sqrt(std::pow(rd / 173.7178, 2) + std::pow(volatility,2));
		std::vector<double> primes { rating, 173.7178*phi_p, volatility };
		return primes;
	}

	double mu = (rating-1500) / 173.7178, phi = rd / 173.7178;
	double nu, delta;
	std::vector<double> mu_opp;
	std::vector<double> phi_opp;
	for (double opprtg : opp_rating)
		mu_opp.push_back((opprtg - 1500) / 173.7178);
	for (double opprd : opp_rd)
		phi_opp.push_back(opprd / 173.7178);

	// set nu
	double sum1 = 0;
	for (int j = 0; j < num_matches; j++)
		sum1 += (std::pow(g(phi_opp[j]),2) * E(mu, mu_opp[j], phi_opp[j]) * (1-E(mu, mu_opp[j], phi_opp[j])));
	nu = 1 / sum1;

	// set delta
	double sum2 = 0;
	for (int j = 0; j < num_matches; j++)
		sum2 += (g(phi_opp[j]) * (scores[j] - E(mu, mu_opp[j], phi_opp[j])));
	delta = nu * sum2;

	// get new volatility
	double a = std::log(std::pow(volatility, 2));
	auto comp = [=](double x) -> double
	{
		return ((std::exp(x)*(std::pow(delta,2)-std::pow(phi,2)-nu-std::exp(x))) / (2*std::pow(std::pow(phi,2)+nu+std::exp(x),2)))-((x-a) / std::pow(SYS_CONST,2));
	};
	double epsilon = 0.000001;

	double A = a;
	double B;
	if (std::pow(delta, 2) > std::pow(phi, 2) + nu)
		B = std::log(std::pow(delta, 2)-std::pow(phi, 2)-nu);
	else
	{
		int k = 1;
		while (f(a-k*SYS_CONST, comp) < 0)
			++k;
		B = a - k*SYS_CONST;
	}

	double fa = f(A, comp); double fb = f(B, comp);
	while (std::abs(B-A) > epsilon)
	{
		double C = A + (A-B)*fa/(fb-fa);
		double fc = f(C, comp);
		if (fc*fb < 0)
		{ A = B; fa = fb; }
		else
			fa /= 2;
		B = C; fb = fc;
		if (std::abs(B-A) <= epsilon)
			break;
	}

	volatility_p = std::exp(A/2);

	// calibrate the rating and the rating deviation
	double phi_star = std::sqrt(std::pow(phi,2) + std::pow(volatility_p,2));
	phi_p = 1 / std::sqrt((1/std::pow(phi_star,2)) + (1/nu));

	double sum3 = 0;
	for (int j = 0; j < num_matches; j++)
		sum3 += g(phi_opp[j]) * (scores[j] - E(mu, mu_opp[j], phi_opp[j]));
	mu_p = mu + std::pow(phi_p,2)*sum3;

	rating_p = 173.7178 * mu_p + 1500;
	rd_p = 173.7178 * phi_p;

	std::vector<double> primes { rating_p, rd_p, volatility_p };
	return primes;
}
