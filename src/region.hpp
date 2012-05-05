#ifndef DVB021_REGION_H
#define DVB021_REGION_H

#include <string>

typedef struct yaml_parser_s yaml_parser_t;

class Region {
protected:
	Region();

public:
	int x() const { return _x; }
	int y() const { return _y; }
	int w() const { return _w; }
	int h() const { return _h; }
	const std::string& name() const { return _name; }

protected:
	virtual void set(const std::string& key, const std::string& value);
	void parse(yaml_parser_t* parser);

private:
	std::string _name;
	int _x;
	int _y;
	int _w;
	int _h;
};

#endif /* DVB021_REGION_H */
