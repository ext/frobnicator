#ifndef FROBNICATOR_VECTOR_H
#define FROBNICATOR_VECTOR_H

#include <math.h>
#include <string>

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

	Vector& operator+=(const Vector<T>& rhs){
		x += rhs.x;
		y += rhs.y;
		return *this;
	}

	Vector operator-(const Vector<T>& rhs) const {
		return Vector(x-rhs.x, y-rhs.y);
	}

	Vector& operator-=(const Vector<T>& rhs){
		x -= rhs.x;
		y -= rhs.y;
		return *this;
	}

	/**
	 * Component-wise multiplication.
	 * @note NOT CROSS-PRODUCT.
	 */
	Vector operator*(const Vector& rhs) const {
		return Vector(x * rhs.x, y * rhs.y);
	}

	Vector operator*(T scalar) const {
		return Vector(x * scalar, y * scalar);
	}

	Vector& operator*=(T scalar){
		x *= scalar;
		y *= scalar;
		return *this;
	}

	bool operator==(const Vector& rhs) const {
		return x == rhs.x && y == rhs.y; /* won't work very well for floatingpoint */
	}

	/**
	 * Gives the squared length.
	 */
	T length2() const {
		return x*x + y*y;
	}

	/**
	 * Gives length of vector.
	 */
	T length() const {
		return sqrt(length2());
	}

	/**
	 * Get a normalized copy of the vector.
	 */
	Vector normalized() const {
		const T len = length();
		return Vector(x/len, y/len);
	}

	/**
	 * Get a vector from yaml markup.
	 */
	static Vector from_yaml(yaml_parser_t* parser);

	/**
	 * Get distance between two vectors.
	 */
	static T distance(const Vector& a, const Vector& b){
		return (b-a).length();
	}

	/**
	 * Get squared distance between two vectors.
	 */
	static T distance2(const Vector& a, const Vector& b){
		return (b-a).length2();
	}

	/**
	 * Linear interpolation.
	 */
	static Vector lerp(const Vector& a, const Vector& b, float s){
		return (b-a) * s + a;
	}

	const std::string str() const;

	union {
		struct {
			T x;
			T y;
		};
		T v[2];
	};
};

typedef Vector<float> Vector2f;
typedef Vector<int> Vector2i;

#endif /* FROBNICATOR_VECTOR_H */
