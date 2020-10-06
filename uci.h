#pragma once

#include <map>
#include <string>

#include "defines.h"

class Position;

namespace UCI {
	class Option;

	struct CaseInsensitiveLess {
		bool operator() (const std::string&, const std::string&) const;
	};

	typedef std::map<std::string, Option, CaseInsensitiveLess> OptionsMap;

	class Option {
		typedef void (*OnChange)(const Option&);

	public:
		Option(OnChange = nullptr);
		Option(bool v, OnChange = nullptr);
		Option(const char* v, OnChange = nullptr);
		Option(int v, int min, int max, OnChange = nullptr);

		Option& operator=(const std::string&);
		void operator<<(const Option&);
		operator int() const;
		operator std::string() const;

	private:
		friend std::ostream& operator<<(std::ostream&, const OptionsMap&);

		std::string defaultValue, currentValue, type;
		int min, max;
		size_t idx;
		OnChange on_change;
	};

	void init(OptionsMap&);
	void loop(int argc, char* argv[]);
	std::string value(Value v);
	std::string square(Square s);
	std::string move(Move move);
	std::string pv(const Position& position, Depth depth, Value alpha, Value beta);
	Move to_move(const Position& position, std::string& str);

}

extern UCI::OptionsMap Options;