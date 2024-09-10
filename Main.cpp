#define no_init_all deprecated

/*
	TODO:
		Load API keys and such from a JSON file
		Loop through multiple DNS entries and update
		Use spdlog ?
*/

#include <iostream>
#include <fstream>
#include <string>
#include <chrono> 
#include <thread> 
#include <vector>

#include <time.h>

#include "cpr/cpr.h"
#include "nlohmann/json.hpp"

enum COLOR
{
	WHITE,
	GREY,
	RED,
	LIGHTRED,
	GREEN,
	LIGHTGREEN,
	BLUE,
	LIGHTBLUE,
	YELLOW,
	LIGHTYELLOW
};

#define CLOUDFLARE_API_TOKEN "93550c542040ceaa954acba532ad39d648fed"
#define CLOUDFLARE_API_DNS_TOKEN "f9Rh7ZuSaUg3afKAhUToOXxuTZIywr9mUCO-7I9w"

#define CLOUDFLARE_ID_WWW "48c5e5d8b3bb0c546bdbc48bc36982d0"
#define CLOUDFLARE_ID_FILES "fd031235176fd513f6440836d92a939e"
#define CLOUDFLARE_ID_FORUM "09763e94f1e4573c2ba578225a7a5ae5"

#define CLOUDFLARE_RECORD "https://api.cloudflare.com/client/v4/zones/6f2c19b29ff686c66b893fb15fce79ad/dns_records/"


#define cmd_exists(x) cmdOptionExists(argv, argv + argc, x)
#define cmd_get(x) getCmdOption(argv, argv + argc, x)

bool cmdOptionExists(char **begin, char **end, const std::string &option);
char* getCmdOption(char **begin, char **end, const std::string &option);
void set_text_color(HANDLE console, COLOR color);
void clear_screen(HANDLE console);

int main(int argc, char** argv) 
{
	HANDLE console_handle = GetStdHandle(STD_OUTPUT_HANDLE);

	std::string 
		ip_address, 
		old_ip_address = "0.0.0.0";

	bool active = true;

	unsigned int update_rate = 60;

	if (cmd_exists("-r"))
	{
		update_rate = atoi(cmd_get("-r"));
	}

	while (active)
	{
		bool error_occured = false;

		std::stringstream log_ouput;

		time_t _tm = time(NULL);
		struct tm * curtime = localtime(&_tm);
		std::string time = std::string(asctime(curtime));

		std::replace(time.begin(), time.end(), ' ', '_');
		std::replace(time.begin(), time.end(), ':', '-');
		std::replace(time.begin(), time.end(), '\n', '-');

		log_ouput << "TIME: " << time << "\n\n";

		std::string filename = "logs/" + time;
		std::ofstream file;

		clear_screen(console_handle);
		set_text_color(console_handle, GREY);

		set_text_color(console_handle, LIGHTYELLOW);
		std::cout << "OPERATION ";
		set_text_color(console_handle, GREY);
		std::cout << "Retrieving Public IP Address...\n\n";

		cpr::Response response = cpr::Get(cpr::Url{ "https://api.ipify.org?format=json" });

		if (response.status_code == 200L)
		{
			std::string json = response.text;
			std::string sub = json.substr(json.find_first_of(':') + 2);
			ip_address = sub.substr(0, sub.length() - 2);

			set_text_color(console_handle, LIGHTGREEN);
			std::cout << "SUCCESS";
			set_text_color(console_handle, GREY);
			std::cout << ": Received response\n\n";
			std::cout << "       > Public IP Address: " << ip_address << "\n\n";

			if (ip_address == old_ip_address)
			{
				set_text_color(console_handle, LIGHTYELLOW);
				std::cout << "MESSAGE: ";
				set_text_color(console_handle, GREY);
				std::cout << "IP address has not changed since last check. \n\n";

				goto sleep;
			}

			old_ip_address = ip_address;

			std::vector<nlohmann::json> payload;
			std::vector<std::string> records;

			payload.emplace_back(nlohmann::json{
					{ "content", ip_address},
					{ "name", "forum" },
					{ "proxied", true },
					{ "type", "A" },
					{ "comment", "" },
					{ "id", CLOUDFLARE_ID_FORUM },
					{ "ttl", 1 }
				});
			records.emplace_back(std::string(CLOUDFLARE_RECORD CLOUDFLARE_ID_FORUM));

			payload.emplace_back(nlohmann::json{
					{ "content", ip_address},
					{ "name", "www" },
					{ "proxied", true },
					{ "type", "A" },
					{ "comment", "" },
					{ "id", CLOUDFLARE_ID_WWW },
					{ "ttl", 1 }
			});
			records.emplace_back(std::string(CLOUDFLARE_RECORD CLOUDFLARE_ID_WWW));

			payload.emplace_back(nlohmann::json{
					{ "content", ip_address},
					{ "name", "files" },
					{ "proxied", true },
					{ "type", "A" },
					{ "comment", "" },
					{ "id", CLOUDFLARE_ID_FILES },
					{ "ttl", 1 }
				});
			records.emplace_back(std::string(CLOUDFLARE_RECORD CLOUDFLARE_ID_FILES));

			for (auto i = 0; i < payload.size(); i++)
			{
				cpr::CurlHolder data;

				set_text_color(console_handle, LIGHTBLUE);
				std::cout << "PAYLOAD: \n";
				set_text_color(console_handle, GREY);
				std::cout << payload.at(i).dump(4) << "\n\n";

				set_text_color(console_handle, LIGHTYELLOW);
				std::cout << "OPERATION ";
				set_text_color(console_handle, GREY);
				std::cout << "Sending Payload to Cloudflare Servers...\n\n";

				cpr::Response post = cpr::Put(
					cpr::Url{ records.at(i) },
					cpr::Bearer{ CLOUDFLARE_API_DNS_TOKEN },
					cpr::Header{ {"Content-Type", "application/json"} },
					cpr::Body{ payload.at(i).dump() });

				if (post.status_code == 200)
				{
					set_text_color(console_handle, LIGHTGREEN);
					std::cout << "SUCCESS";
					set_text_color(console_handle, GREY);
					std::cout << "  : POST operation success\n\n";

					nlohmann::json response_json = nlohmann::json::parse(post.text);
					std::cout << response_json.dump(4) << "\n\n";

					log_ouput <<
						"PREV IP: " << old_ip_address << "\n" <<
						"NEW IP : " << ip_address << "\n\n" <<
						"POST:\n" << response_json.dump(4) << std::endl;
				}
				else
				{
					error_occured = true;

					set_text_color(console_handle, LIGHTRED);
					std::cout << "ERROR";
					set_text_color(console_handle, GREY);
					std::cout << "  : POST Request returned status code " << post.status_code << "\n\n";

					std::cout << "       > " << post.text << "\n";

					log_ouput << "ERROR\n\n" << post.text;
				}
			}
		}
		else
		{
			error_occured = true;

			set_text_color(console_handle, LIGHTRED);
			std::cout << "ERROR";
			set_text_color(console_handle, GREY);
			std::cout << "  : GET Request returned status code " << response.status_code << "\n\n";

			std::cout << "       > " << response.text << "\n";

			log_ouput << "ERROR\n\n" << response.text;
		}

		file.open(filename + (error_occured ? "FAILED.log" : "SUCCESS.log"));
		file << log_ouput.str();
		file.close();
		log_ouput.clear();

		std::cout << std::endl;

sleep:
		std::this_thread::sleep_for(std::chrono::seconds(update_rate));
	}

	return 0;
}

bool cmdOptionExists(char **begin, char **end, const std::string &option)
{
	return std::find(begin, end, option) != end;
}

char* getCmdOption(char **begin, char **end, const std::string &option)
{
	char **itr = std::find(begin, end, option);

	if (itr != end && ++itr != end)
	{
		return *itr;
	}

	return 0;
}

void set_text_color(HANDLE console, COLOR color)
{
	switch (color)
	{
	case WHITE:
		SetConsoleTextAttribute(console,
			FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE |
			FOREGROUND_INTENSITY);
		break;

	case GREY:
		SetConsoleTextAttribute(console,
			FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
		break;

	case RED:
		SetConsoleTextAttribute(console,
			FOREGROUND_RED);
		break;

	case LIGHTRED:
		SetConsoleTextAttribute(console,
			FOREGROUND_RED | FOREGROUND_INTENSITY);
		break;

	case GREEN:
		SetConsoleTextAttribute(console,
			FOREGROUND_GREEN);
		break;

	case LIGHTGREEN:
		SetConsoleTextAttribute(console,
			FOREGROUND_GREEN | FOREGROUND_INTENSITY);
		break;

	case BLUE:
		SetConsoleTextAttribute(console,
			FOREGROUND_BLUE);
		break;

	case LIGHTBLUE:
		SetConsoleTextAttribute(console,
			FOREGROUND_BLUE | FOREGROUND_INTENSITY);
		break;

	case YELLOW:
		SetConsoleTextAttribute(console,
			FOREGROUND_RED | FOREGROUND_GREEN);
		break;

	case LIGHTYELLOW:
		SetConsoleTextAttribute(console,
			FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
		break;

	default:
		SetConsoleTextAttribute(console,
			FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
		break;
	}
}

void clear_screen(HANDLE console)
{
	COORD topLeft = { 0, 0 };
	CONSOLE_SCREEN_BUFFER_INFO screen;
	DWORD written;

	GetConsoleScreenBufferInfo(console, &screen);
	FillConsoleOutputCharacterA(
		console, ' ', screen.dwSize.X * screen.dwSize.Y, topLeft, &written
	);

	FillConsoleOutputAttribute(
		console, FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_BLUE,
		screen.dwSize.X * screen.dwSize.Y, topLeft, &written
	);

	SetConsoleCursorPosition(console, topLeft);
}
