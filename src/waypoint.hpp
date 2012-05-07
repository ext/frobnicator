#ifndef DVB021_WAYPOINT_H
#define DVB021_WAYPOINT_H

#include "region.hpp"

class Waypoint: public Region {
public:
	static Waypoint* from_yaml(yaml_parser_t* parser);
	virtual void set(const std::string& key, const std::string& value);

	/* name of the next inner waypoint */
	const std::string& inner() const { return _inner; }

	/* name of the next waypoint */
	const std::string& next() const { return _next; }

private:
	Waypoint();

public:
	std::string _inner;
	std::string _next;
};

#endif /* DVB021_WAYPOINT_H */
