#ifndef DVB021_LEVEL_H
#define DVB021_LEVEL_H

#include <string>

class Level {
public:
	~Level();

	/**
	 * Load a level from filename.
	 */
	static Level* from_filename(const std::string& filename);

	const std::string& title() const;
	const Tilemap& tilemap() const;

private:
	Level(const std::string& filename); /* use from_filename */
	Level(const Level&); /* prevent copying */
	class LevelPimpl* pimpl;
};

#endif /* DVB021_LEVEL_H */
