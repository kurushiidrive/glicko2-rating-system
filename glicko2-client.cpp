#include "Glicko2.h"
#include <limits>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdio>
#include <memory>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <ctime>

#include <getopt.h>

double tau = 0.6;
Glicko2 glicko_system { tau };

bool prompt (const char* message, char& readch);

int create (const char* filename);

int load (const char* filename);

int run_glicko2(const char* filename);

int main (int argc, char* argv[])
{
	std::unique_ptr<char[]> filename;

	/*
	 * Format of a line in a system CSV file:
	 * [player name],[rating],[rd],[volatility],[results-file.csv]
	 *
	 * Format of a line in a results CSV file:
	 * [opponent name],[rating],[rd],[volatility],[score]
	 */
	bool did_something = false;

	static struct option long_opts[] = {
		{"create", required_argument, 0, 'c'},
		{"load", required_argument, 0, 'l'},
		{"run", no_argument, 0, 'r'},
		{0, 0, 0, 0}
	};

	int opt_index = 0;
	int val = getopt_long(argc, argv, "c:l:r", long_opts, &opt_index);
	while (val != -1)
	{
		switch (val)
		{
			case 'c':
				{
					size_t tmp_optarglength = std::strlen(optarg);
					filename.reset(new char[tmp_optarglength+1]);
					for (size_t i = 0; i < tmp_optarglength; i++)
						filename[i] = optarg[i];
					filename[tmp_optarglength] = '\0';
					int ret = create(filename.get());
					did_something = true;
					if (ret == 1)	// file already exists
					{
						std::fprintf(stderr, "%s: could not open file %s, it may already exist.\nIf you would like to load an existing Glicko-2 system (csv file) instead, use:\n`%s --load=filename`\n\n", argv[0], filename.get(), argv[0]);
						std::exit(1);
					}
					break;
				}
			case 'l':
				{
					size_t tmp_optarglength = std::strlen(optarg);
					filename.reset(new char[tmp_optarglength+1]);
					for (size_t i = 0; i < tmp_optarglength; i++)
						filename[i] = optarg[i];
					filename[tmp_optarglength] = '\0';
					int ret = load(filename.get());
					did_something = true;
					if (ret == 1)	// couldn't find file
					{
						std::fprintf(stderr, "%s: could not open file %s, it may not be present in the current directory or the player-data directory.\n\n", argv[0], filename.get());
						std::exit(1);
					}
					if (ret == 2) 	// invalid csv: the results csv in the system csv couldn't be found
					{
						std::fprintf(stderr, "%s: invalid csv; could not open one of the results csv files that were in %s, it may not be present in the current directory or the player-data directory.\n\n", argv[0], filename.get());
						std::exit(1);
					}
				}
				break;
			case 'r':
				{
					int ret = run_glicko2(filename.get());
					did_something = true;
					if (ret == 1)	// no data currently loaded
					{
						std::fprintf(stderr, "%s: there are currently no data loaded on which to run the Glicko-2 system.\nEither load data from a CSV file with `%s --load=filename --run`,\nor create new data on which to run, with `%s --create=filename --run`.\n\n", argv[0], argv[0], argv[0]);
						std::exit(1);
					}
				}
				break;
			default:
				std::fprintf(stderr, "Usage: %s [--create=filename] [--load=filename] [--run]\n", argv[0]);
				std::exit(1);
		}
		val = getopt_long(argc, argv, "c:l:r", long_opts, &opt_index);
	}

	if (!did_something)
		std::fprintf(stderr, "Usage: %s [--create=filename] [--load=filename] [--run]\n", argv[0]);

	return 0;
}

void output_to_console()
{
	for (const auto& kv : glicko_system.Get_Players())
	{
		std::printf("Player:\t%s\n", kv.first.c_str());
		std::printf("Rating:\t%.0f\n", kv.second.Get_Rating());
		std::printf("Rating Deviation (+/-):\t%.0f\n", kv.second.Get_RD());
		std::printf("Volatility:\t%.6f\n", kv.second.Get_Vol());

		auto match_history { kv.second.Get_Match_History() };
		std::printf("Opponents:\t[");
		for (size_t i = 0; i < match_history.size(); i++)
			std::cout << match_history[i].first.Get_Name() << (i+1 < match_history.size() ? ", " : "]\n");
		std::printf("Scores:\t[");
		for (size_t i = 0; i < match_history.size(); i++)
			std::cout << match_history[i].second << (i+1 < match_history.size() ? ", " : "]\n");

		std::cout << std::endl;
	}
}

int output_to_csv(const char* filename, bool results_flag)
{
	// write to csv
	std::string tmp_filename {filename};
	tmp_filename.insert(0, "./player-data/");
	std::fstream fout; std::ifstream f{tmp_filename};
	if (f.good())
		return 1;
	fout.open(tmp_filename, std::fstream::out);

	for (const auto& kv : glicko_system.Get_Players())
	{
		fout << kv.first.c_str() << "," << kv.second.Get_Rating() << "," << kv.second.Get_RD() << "," << kv.second.Get_Vol() << ",";
		if (kv.second.Get_Match_History().size() != 0 && results_flag)
		{
			fout << kv.first.c_str() << "-results.csv";

			std::fstream fresultsout; fresultsout.open(("./player-data/" + kv.first + "-results.csv").c_str(), std::fstream::out);
			for (const auto& match : kv.second.Get_Match_History())
				fresultsout << match.first.Get_Name() << "," << match.first.Get_Rating() << "," << match.first.Get_RD() << "," << match.first.Get_Vol() << "," << match.second << std::endl;

			fresultsout.close();
		}
		fout << std::endl;
	}

	fout.close();

	return 0;
}

bool prompt (const char* message, char& readch)
{
	std::string tmp;
	std::cout << message;
	if (std::getline(std::cin, tmp))
	{
		if (tmp.length() == 1)
			readch = tmp[0];
		else if (tmp.length() == 0)
			readch = '\n';
		else
			readch = '\0';
		return true;
	}
	return false;
}

int load (const char* filename)
{
	std::string tmp_filename {filename};

	std::fstream fin;
	fin.open(filename, std::fstream::in);
	if (!fin.is_open())
	{
		fin.close();
		fin.clear();
		fin.open(("./player-data/"+tmp_filename).c_str(), std::fstream::in);
		if (!fin.is_open())
			return 1;
	}

	std::vector<std::string> row;
	std::string line;
	std::string token;
	while (std::getline(fin, line))
	{
		row.clear();
		std::stringstream linestream {line};

		while (std::getline(linestream, token, ','))
			row.push_back(token);

		Player tmp_player { row[0], std::stoi(row[1]), std::stoi(row[2]), std::stod(row[3]) };
		if (!linestream && !token.empty())
		{
			std::fstream fresultsin;
			fresultsin.open(row[4].c_str(), std::fstream::in);
			if (!fresultsin.is_open())
			{
				fresultsin.close();
				fresultsin.clear();
				fresultsin.open(("./player-data/"+row[4]).c_str(), std::fstream::in);
				if (!fresultsin.is_open())
					return 2;
			}

			std::vector<std::string> results_row;
			std::string results_line;
			std::string results_token;
			while (std::getline(fresultsin, results_line))
			{
				results_row.clear();
				std::stringstream results_linestream {results_line};

				while (std::getline(results_linestream, results_token, ','))
					results_row.push_back(results_token);

				Player tmp_opp { results_row[0], std::stoi(results_row[1]), std::stoi(results_row[2]), std::stod(results_row[3]) };
				tmp_player.Add_Match(tmp_opp, std::stoi(results_row[4]));
			}

			fresultsin.close();
		}
		glicko_system.Add_Player(tmp_player);
	}

	fin.close();

	std::cout << "\nSuccessfully loaded Glicko-2 System from \"" << filename << "\"." << std::endl;

	std::cout << std::endl << std::endl;

	if (glicko_system.Get_Players().size() == 0)
		return 0;

	std::cout << "Players in this Glicko-2 System:\n\n" << std::endl;
	output_to_console();
}

int create (const char* filename)
{
	bool yes;
	char yn = '\0';
	while (prompt("Would you like to add players to the Glicko-2 Rating System? (Y/n): ", yn))
	{
		if (yn == 'n' || yn == 'N')
		{ yes = false; break; }
		if (yn == '\n' || yn == 'Y' || yn == 'y')
		{ yes = true; break; }

		std::cout << "\nInvalid selection, please try again.\n\n";
	}

	while (yes)
	{
		std::cout << "\n\nEnter the player\'s name: ";
		std::string name; std::getline(std::cin, name);

		char selection = '\0';
		while(prompt("Initialise this player with default (unrated) Glicko-2 stats? (y/N): ", selection))
		{
			Player tmp {name};
			if (selection == 'y' || selection == 'Y')
			{
				bool match_yes = false;
				char match_selection = '\0';
				while (prompt("Would you like to add match results for this player? (y/N): ", match_selection))
				{
					if (match_selection == 'y' || match_selection == 'Y')
					{
						match_yes = true; break;
					}
					if (match_selection == '\n' || match_selection == 'N' || match_selection == 'n')
					{
						match_yes = false; break;
					}
					std::cout << "\nInvalid selection, please try again.\n\n";
				}

				while (match_yes)
				{
					std::cout << "\n\nEnter the opponent\'s name: ";
					std::string opponent_name; std::getline(std::cin, opponent_name);

					std::string rtg_line;
					int opp_tmp_rtg; std::cout << "Rating: "; std::getline(std::cin, rtg_line);
					std::stringstream opp_linestream{rtg_line}; opp_linestream >> opp_tmp_rtg;

					std::string rd_line;
					int opp_tmp_rd; std::cout << "RD: "; std::getline(std::cin, rd_line);
					std::stringstream opp_linestream2{rd_line}; opp_linestream2 >> opp_tmp_rd;

					std::string vol_line;
					double opp_tmp_vol; std::cout << "Volatility: "; std::getline(std::cin, vol_line);
					std::stringstream opp_linestream3{vol_line}; opp_linestream3 >> opp_tmp_vol;

					int opp_tmp_wins;
					int opp_tmp_losses;
					do {
						std::string wins_line; std::cout << "How many wins against " << opponent_name << "? (Enter a number): ";
						std::getline(std::cin, wins_line);
						std::stringstream opp_linestream4{wins_line}; opp_linestream4 >> opp_tmp_wins;
						if (opp_tmp_wins >= 0) break;
						std::cout << "\nPlease enter an integer >= 0.\n\n";
					} while (true);

					do {
						std::string losses_line; std::cout << "How many losses against " << opponent_name << "? (Enter a number): ";
						std::getline(std::cin, losses_line);
						std::stringstream opp_linestream5{losses_line}; opp_linestream5 >> opp_tmp_losses;
						if (opp_tmp_losses >= 0) break;
						std::cout << "\nPlease enter an integer >= 0.\n\n";
					} while (true);

					Player opp_tmp { opponent_name, opp_tmp_rtg, opp_tmp_rd, opp_tmp_vol };
					for (int k = 0; k < opp_tmp_wins; k++)
						tmp.Add_Match(opp_tmp, 1);
					for (int k = 0; k < opp_tmp_losses; k++)
						tmp.Add_Match(opp_tmp, 0);

					std::cout << std::endl;

					match_selection = '\0';
					while (prompt("Would you like to add anymore match results to this player? (y/N): ", match_selection))
					{
						if (match_selection == 'y' || match_selection == 'Y')
						{
							match_yes = true; break;
						}
						if (match_selection == '\n' || match_selection == 'N' || match_selection == 'n')
						{
							match_yes = false; break;
						}
						std::cout << "\nInvalid selection, please try again.\n\n";
					}
				}
				glicko_system.Add_Player(tmp);
				break;
			}
			if (selection == '\n' || selection == 'N' || selection == 'n')
			{
				Player tmp { name };
				std::string line;
				int tmp_rating; std::cout << "Rating: "; std::getline(std::cin, line);
				std::stringstream linestream{line}; linestream >> tmp_rating;

				int tmp_rd; std::cout << "RD: "; std::getline(std::cin, line);
				std::stringstream linestream2{line}; linestream2 >> tmp_rd;

				double tmp_vol; std::cout << "Volatility: "; std::getline(std::cin, line);
				std::stringstream linestream3{line}; linestream3 >> tmp_vol;

				tmp.Set_Rating(tmp_rating);
				tmp.Set_RD(tmp_rd);
				tmp.Set_Vol(tmp_vol);

				bool match_yes = false;
				char match_selection = '\0';
				while (prompt("Would you like to add match results for this player? (y/N): ", match_selection))
				{
					if (match_selection == 'y' || match_selection == 'Y')
					{
						match_yes = true; break;
					}
					if (match_selection == '\n' || match_selection == 'N' || match_selection == 'n')
					{
						match_yes = false; break;
					}
					std::cout << "\nInvalid selection, please try again.\n\n";
				}

				while (match_yes)
				{
					std::cout << "\n\nEnter the opponent\'s name: ";
					std::string opponent_name; std::getline(std::cin, opponent_name);

					std::string rtg_line;
					int opp_tmp_rtg; std::cout << "Rating: "; std::getline(std::cin, rtg_line);
					std::stringstream opp_linestream{rtg_line}; opp_linestream >> opp_tmp_rtg;

					std::string rd_line;
					int opp_tmp_rd; std::cout << "RD: "; std::getline(std::cin, rd_line);
					std::stringstream opp_linestream2{rd_line}; opp_linestream2 >> opp_tmp_rd;

					std::string vol_line;
					double opp_tmp_vol; std::cout << "Volatility: "; std::getline(std::cin, vol_line);
					std::stringstream opp_linestream3{vol_line}; opp_linestream3 >> opp_tmp_vol;

					int opp_tmp_score;
					do {
						std::string score_line; std::cout << "Who won the match? (1 for player, 0 for opponent): ";
						std::getline(std::cin, score_line);
						std::stringstream opp_linestream4{score_line}; opp_linestream4 >> opp_tmp_score;
						if (opp_tmp_score == 0 || opp_tmp_score == 1) break;
						std::cout << "\nPlease enter either a value of 1 or 0, for whether the player or opponent won, respectively.\n\n";
					} while (true);

					Player opp_tmp { opponent_name, opp_tmp_rtg, opp_tmp_rd, opp_tmp_vol };
					tmp.Add_Match(opp_tmp, opp_tmp_score);

					std::cout << std::endl;

					match_selection = '\0';
					while (prompt("Would you like to add anymore match results to this player? (y/N): ", match_selection))
					{
						if (match_selection == 'y' || match_selection == 'Y')
						{
							match_yes = true; break;
						}
						if (match_selection == '\n' || match_selection == 'N' || match_selection == 'n')
						{
							match_yes = false; break;
						}
						std::cout << "\nInvalid selection, please try again.\n\n";
					}
				}

				glicko_system.Add_Player(tmp);
				break;
			}
			std::cout << "\nInvalid selection, please try again.\n\n";
		}

		std::cout << std::endl;

		selection = '\0';
		while(prompt("Would you like to add anyone else? (y/N): ", selection))
		{
			if (selection == 'y' || selection == 'Y')
			{
				yes = true; break;
			}
			if (selection == '\n' || selection == 'N' || selection == 'n')
			{
				yes = false; break;
			}
			std::cout << "\nInvalid selection, please try again.\n\n";
		}
	}

	std::cout << std::endl << std::endl;

	if (glicko_system.Get_Players().size() == 0)
		return 0;

	std::cout << "Players in this Glicko-2 System:\n\n" << std::endl;
	output_to_console();

	std::cout << "Writing data to \"" << filename << "\"\n\n"; 
	return output_to_csv(filename, true);
}

int run_glicko2 (const char* filename)
{
	if (glicko_system.Get_Players().size() == 0)
		return 1;

	std::cout << "\nRunning Glicko-2 on current player data ... \n\n";

	glicko_system.Run();

	std::cout << "Calibrated Glicko-2 ratings:\n\n" << std::endl;
	output_to_console();

	time_t as_time_t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	struct tm* now = std::localtime(&as_time_t);
	char buffer[80];
	std::strftime(buffer, 80, "%Y-%m-%d_%H-%M-%S_calibrated-", now);

	std::string fnm { buffer };
	fnm += filename;

	std::cout << "Writing calibrated data to \"" << fnm << "\"\n\n";
	return output_to_csv(fnm.c_str(), false);
}
