#ifndef FROBNICATOR_SPRITE_H
#define FROBNICATOR_SPRITE_H

#include "vector.hpp"
#include <string>

class Sprite {
public:
	Sprite();
	virtual ~Sprite();

	static Sprite* from_yaml(yaml_parser_t* parser);

	virtual Sprite* load_texture(const std::string& filename) = 0;

	const Vector2f& FROB_PURE offset() const { return _offset; }
	const Vector2f& FROB_PURE scale() const { return _scale; }
	Sprite* set_offset(const Vector2f& offset){ _offset = offset; return this; }
	Sprite* set_scale(const Vector2f& scale){ _scale = scale; return this; }

	/**
	 * Set scale to match texture size.
	 */
	virtual Sprite* autoscale() = 0;

private:
	Vector2f _offset;
	Vector2f _scale;
};

#endif /* FROBNICATOR_SPRITE_H */
