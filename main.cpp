#define no_init_all deprecated

#include <iostream>
#include <fstream>
#include <string>
#include <chrono> 
#include <thread> 

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

#define DNS_A_RECORD "YOUR A RECORD NAME HERE"
#define PROXIED true // If you want to use Cloudflares proxy true/false

#define CLOUDFLARE_API_TOKEN "GLOBAL API TOKEN HERE"
#define CLOUDFLARE_API_DNS_TOKEN "DNS ZONE API TOKEN HERE"

#define CLOUDFLARE_ID_FORUM "YOUR A RECORD DNS ID HERE"
#define CLOUDFLARE_RECORD_FORUM "https://api.cloudflare.com/client/v4/zones/YOUR A RECORD DNS ID HERE/dns_records/YOUR A RECORD ID TOKEN HERE"

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

			nlohmann::json payload = {
					{ "content", ip_address},
					{ "name", DNS_A_RECORD },
					{ "proxied", PROXIED },
					{ "type", "A" },
					{ "comment", "" },
					{ "id", CLOUDFLARE_ID_FORUM },
					{ "ttl", 1 }
			};

			cpr::CurlHolder data;
			
			set_text_color(console_handle, LIGHTBLUE);
			std::cout << "PAYLOAD: \n";
			set_text_color(console_handle, GREY);
			std::cout << payload.dump(4) << "\n\n";

			set_text_color(console_handle, LIGHTYELLOW);
			std::cout << "OPERATION ";
			set_text_color(console_handle, GREY);
			std::cout << "Sending Payload to Cloudflare Servers...\n\n";

			cpr::Response post = cpr::Put(
				cpr::Url{ CLOUDFLARE_RECORD_FORUM },
				cpr::Bearer{ CLOUDFLARE_API_DNS_TOKEN },
				cpr::Header{ {"Content-Type", "application/json"} },
				cpr::Body{ payload.dump() });

			if (post.status_code == 200)
			{
				set_text_color(console_handle, LIGHTGREEN);
				std::cout << "SUCCESS";
				set_text_color(console_handle, GREY);
				std::cout << "  : POST operation success\n\n";

				nlohmann::json response_json = nlohmann::json::parse(post.text);
				std::cout << response_json.dump(4) << "\n\n";

				time_t _tm = time(NULL);
				struct tm * curtime = localtime(&_tm);
				std::string time = std::string(asctime(curtime));

				std::replace(time.begin(), time.end(), ' ', '_');
				std::replace(time.begin(), time.end(), ':', '-');
				std::replace(time.begin(), time.end(), '\n', '-');
				
				std::string filename = "logs/" + time + "Success.log";

				std::ofstream file;

				file.open(filename);

				file << "TIME: " << time << "\n\n" << 
						"PREV IP: " << old_ip_address << "\n" << 
						"NEW IP : " << ip_address << "\n\n" << 
						"POST:\n" << response_json.dump(4) << std::endl;

				file.close();
			}
			else
			{
				set_text_color(console_handle, LIGHTRED);
				std::cout << "ERROR";
				set_text_color(console_handle, GREY);
				std::cout << "  : POST Request returned status code " << post.status_code << "\n\n";

				std::cout << "       > " << post.text << "\n";

				break;
			}
		}
		else
		{
			set_text_color(console_handle, LIGHTRED);
			std::cout << "ERROR";
			set_text_color(console_handle, GREY);
			std::cout << "  : GET Request returned status code " << response.status_code << "\n\n";

			std::cout << "       > " << response.text << "\n";

			break;
		}

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
