#ifndef FROBNICATOR_SPRITE_H
#define FROBNICATOR_SPRITE_H

#include "common.hpp"
#include <string>

class Sprite {
public:
	Sprite();
	virtual ~Sprite();

	static Sprite* from_yaml(yaml_parser_t* parser);

	virtual void load_texture(const std::string& filename) = 0;

	const Vector2f& FROB_PURE offset() const { return _offset; }
	const Vector2f& FROB_PURE scale() const { return _scale; }
	void set_offset(const Vector2f& offset){ _offset = offset; }
	void set_scale(const Vector2f& scale){ _scale = scale; }

private:
	Vector2f _offset;
	Vector2f _scale;
};

#endif /* FROBNICATOR_SPRITE_H */
