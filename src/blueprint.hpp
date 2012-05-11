#ifndef FROBNICATOR_BLUEPRINT_H
#define FROBNICATOR_BLUEPRINT_H

#include <string>
#include <vector>

/**
 * A blueprint is essentially a flyweight for entities.
 * Everything that is common between each instance is held in the blueprint.
 */
class Blueprint  {
private:
	Blueprint();

public:
	static const Blueprint* from_filename(const std::string& filename);

	const Sprite* sprite(unsigned int level) const {
		return data[level].sprite;
	}

	const std::string name(unsigned int level) const {
		return data[level].name;
	}

	unsigned int amount(unsigned int level) const {
		return data[level].amount;
	}

	int cost(unsigned int level) const {
		return data[level].cost;
	}

	size_t num_levels() const {
		return data.size();
	}

private:
	friend class Entity;

	struct level {
		Sprite* sprite;
		std::string name;

		/* gameplay stats */
		int cost;
		float splash;
		float damage;
		float rof;
		float range;
		float slow;
		float poison;
		float speed;
		float armor;
		float hp;
		unsigned int amount;
	};

	static void parse_leveldata(struct level* level, yaml_parser_t* parser);

	std::vector<level> data;
};

#endif /* FROBNICATOR_BLUEPRINT_H */
