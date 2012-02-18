#ifndef DVB021_COMMON_H
#define DVB021_COMMON_H

const char* real_path(const char* filename);
int yaml_error(struct yaml_parser_s* parser);

template <class T> T min(T a, T b){ return (a < b) ? a : b; }
template <class T> T max(T a, T b){ return (a > b) ? a : b; }
template <class T> T clamp(T v, T a, T b){
	if ( v < a ) return a;
	if ( v > b ) return b;
	return v;
}

template <class T>
class Vector {
public:
	Vector()
		: x(0)
		, y(0) {}

	Vector(T x, T y)
		: x(x)
		, y(y) {}

	Vector(const Vector<T>& rhs)
		: x(rhs.x)
		, y(rhs.y) {}

	Vector operator+(const Vector<T>& rhs) const {
		return Vector(x+rhs.x, y+rhs.y);
	}

	Vector operator*(T scalar) const {
		return Vector(x * scalar, y * scalar);
	}

	union {
		struct {
			T x;
			T y;
		};
		T v[2];
	};
};

typedef Vector<float> Vector2f;

#endif /* DVB021_COMMON_H */
