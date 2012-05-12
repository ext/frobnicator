#ifndef FROBNICATOR_COLOR_H
#define FROBNICATOR_COLOR_H

class Color {
public:
	Color()
		: r(0.0f)
		, g(0.0f)
		, b(0.0f)
		, a(1.0f) {

	}
	Color(const Color& color)
		: r(color.r)
		, g(color.g)
		, b(color.b)
		, a(color.a){

	}
	Color(float r, float g, float b, float a)
		: r(r)
		, g(g)
		, b(b)
		, a(a){

	}

	~Color(){}

	static Color rgb(float r, float g, float b){ return Color(r, g, b, 1.0f); }
	static Color rgba(float r, float g, float b, float a){ return Color(r,g,b,a); }

	static const Color black;
	static const Color blue;
	static const Color green;
	static const Color cyan;
	static const Color red;
	static const Color magenta;
	static const Color yellow;
	static const Color white;
	static const Color transparent;

	unsigned int to_32_bit(){
		unsigned int v = 0;
		static const int shift[4] = { 24, 16, 8, 0 };

		for ( int i = 0; i < 4; i++ ){
			v += static_cast<unsigned int>(value[i] / 1.0f * 0xFF) << shift[i];
		}

		return v;
	}

	static Color lerp(const Color& c1, const Color& c2, float s){
		return Color(
			c1.r + ( c2.r - c1.r ) * s,
			c1.g + ( c2.g - c1.g ) * s,
			c1.b + ( c2.b - c1.b ) * s,
			c1.a + ( c2.a - c1.a ) * s
			);
	}

	union {
		struct {
			float r;
			float g;
			float b;
			float a;
		};
		float value[4];
	};

};

#endif // __BLUEFLOWER_COLOR_H
