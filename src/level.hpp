#ifndef DVB021_LEVEL_H
#define DVB021_LEVEL_H

#include <string>

typedef struct yaml_parser_s yaml_parser_t;

class Level {
public:
	Level(const std::string& filename);
	~Level();

	const std::string& title() const;

private:
	void set_title(const std::string& title);

	std::string _title;

	/* for backend to render the level */
	friend class Backend;

	/* for yaml parsing to fill */
	friend void parse_level(Level*, yaml_parser_t*);
};

#endif /* DVB021_LEVEL_H */
