#ifndef DVB021_GAME_H
#define DVB021_GAME_H

#include <cstddef>
#include <string>

class Game {
public:
	Game();
	~Game();

	void load_level(const std::string& filename);
	void frobnicate();

private:
	bool running() const;

	class GamePimpl* pimpl;
};

#endif /* DVB021_GAME_H */
