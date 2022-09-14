#include <windows.h>
#include <iostream>
#include <chrono>
#include <cmath>
#include <unordered_map>
#include <fstream>

#undef max
#undef min

namespace fm
{
	struct application;

	enum Key
	{
		A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
		SPACE, ENTER, CTRL, ALT, TAB, SHIFT,
		N0, N1, N2, N3, N4, N5, N6, N7, N8, N9,
		UP, DOWN, LEFT, RIGHT,
		F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
		MINUS, PLUS, LEFT_BRACKET /* [ */, RIGHT_BRACKET /* ] */,
		COUNT
	};

	// utility structs
	template <typename T>
	struct v2
	{
		T x;
		T y;

		v2() = default;
		v2(T _x, T _y) : x(_x), y(_y) {}
		inline T magnitude() { return sqrt(x * x + y * y); }
		inline v2 normalize() { T inv = 1 / magnitude(); return v2(x * inv, y * inv); }
		inline T dot(const v2& r) { return x * r.x + y * r.y; }
		inline T cross(const v2& r) { return x * r.y - y * r.x; }
		inline v2<T> operator* (const v2<T>& r) { return { x * r.x, y * r.y }; }
		inline v2<T> operator* (T value) { return { x * value, y * value }; }
	};

	template <typename T>
	struct v3
	{
		union { T x, r; };
		union { T y, g; };
		union { T z, b; };

		v3() = default;
		v3(T _x, T _y, T _z) : x(_x), y(_y), z(_z) {}
		inline T magnitude() { return sqrt(x * x + y * y + z * z); }
		inline v3 normalize() { T inv = 1.0f / magnitude(); return v3(x * inv, y * inv, z * inv); }
		inline T dot(const v3& r) { return x * r.x + y * r.y + z * r.z; }
		inline T cross(const v3& r) { return y * r.z - z * r.y, z * r.x - x * r.z, x * r.y - y * r.x; }
		inline v3<T> operator* (T value) { return { x * value, y * value, z * value }; }
		inline v3<T> operator/ (T value) { return { x / value, y / value, z / value }; }
		inline v3<T> operator+ (T value) { return { x + value, y + value, z + value }; }
		inline v3<T> operator+ (const v3<T>& r) { return { x + r.x, y + r.y, z + r.z }; }
		inline v3<T> operator- (const v3<T>& r) { return { x - r.x, y - r.y, z - r.z }; }
	};

	template <typename T>
	T max(T a, T b)
	{
		return a >= b ? a : b;
	}

	template <typename T>
	T min(T a, T b)
	{
		return a <= b ? a : b;
	}

	template <typename T>
	T clamp(T val, T min, T max)
	{
		if (val > max) val = max;
		else if (val < min) val = min;
		return val;
	}

	struct color
	{
		uint32_t hex;
		color() = default;
		color(float r, float g, float b)
		{
			r = max(r, 0.0f);
			g = max(g, 0.0f);
			b = max(b, 0.0f);
			hex = (int(r * 255) << 16) | (int(g * 255) << 8) | (int(b * 255));
		}
		color(v3<float> rgb)
		{
			rgb.r = max(rgb.r, 0.0f);
			rgb.g = max(rgb.g, 0.0f);
			rgb.b = max(rgb.b, 0.0f);
			hex = (int(rgb.r * 255) << 16) | (int(rgb.g * 255) << 8) | (int(rgb.b * 255));
		}
		color(uint32_t h) : hex(h) {}

		v3<float> rgb()
		{
			return v3<float>(float((hex >> 16) & 0xff), float((hex >> 8) & 0xff), float((hex) & 0xff)) / 255.0f;
		}

	};

	struct framebuffer
	{
		friend struct application;

	public:
		framebuffer(uint32_t w, uint32_t h) : width(w), height(h) 
		{
			buffer = new uint32_t[width * height];
		}

		framebuffer(uint32_t w, uint32_t h, void* buf) : width(w), height(h)
		{
			buffer = new uint32_t[width * height];
			memcpy(buffer, buf, width * height * sizeof(uint32_t));
		}

		~framebuffer()
		{
			delete[] buffer;
		}

		void set_buffer(void* buf);

	private:
		uint32_t width;
		uint32_t height;
		uint32_t* buffer;
	};

	struct texture
	{
		texture() = default;

		uint32_t* buffer;
		uint32_t width;
		uint32_t height;
	};

	struct Button
	{
		bool pressed = false;
		bool released = true;
		bool held = false;
	};

	struct application
	{
		friend LRESULT CALLBACK window_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	public:
		application() = default;
		virtual ~application()
		{
			free_memory();
		}

		// creates the necessary resources
		bool initialize(const wchar_t* name, uint32_t w, uint32_t h, uint32_t p = 1);
		bool initialize(const wchar_t* name, uint32_t w, uint32_t h, uint32_t buffer_w, uint32_t buffer_h);

		// contains the main loop
		void start();

		// function called at the beggining of the application
		virtual void on_create() {}

		// function called every frame
		virtual void on_update(float dt) {}

		uint32_t screen_width() { return pgraphics_context->buffer_width; }
		uint32_t screen_height() { return pgraphics_context->buffer_height; }

		void add_title_info(const std::wstring& info);

		v2<float> mouse_position();
		Button get_key(Key name);

	public:
		bool resizable = false;
		bool minimize_button = true;
		bool maximize_button = true;

	private:
		bool is_running = true;
		static application* app_instance;

	private:
		struct window
		{
			HWND handle;
			WNDCLASS window_class;
			HDC device_context;

			uint32_t width;
			uint32_t height;

			std::wstring name;
			std::string info_string;

		} *pwindow = nullptr;

		struct grahics_context
		{
			uint32_t* memory_buffer;
			BITMAPINFO bm_info;
			uint32_t buffer_size;

			uint32_t pixel_size;
			uint32_t buffer_width;
			uint32_t buffer_height;

		} *pgraphics_context = nullptr;

		void core_update();

		/*
			initialize *pwindow
			flags can be modified by changing resizable, minimize_button and maximize_button values
		*/
		bool create_window(const std::wstring& name, uint16_t w, uint16_t h, unsigned long flags);

		/*
			initiazlie *pgraphics_context
			this structure represents the actual buffer that gets modified and rendered to the window
			bigger pizel_size means better performance but reduced image quality
		*/
		bool create_graphics_context(uint32_t w, uint32_t h, uint32_t p);
		void poll_events();

		/*
			renders pgraphics_context->memory_buffer to the screen
		*/
		void present();

		/*
			clears resources
		*/
		void free_memory();

		std::unordered_map<std::string, texture*> textures;

		enum class font_type
		{
			CHARACTERS,
			NUMBERS,
			SYMBOLS
		};
		texture* font[3];
		std::unordered_map<char, uint32_t> glyph_width;
		uint32_t get_glyph_width(char c);
		uint32_t get_symbol_offet(char sym);
		bool glyph_exist(char c);
		const uint32_t font_glyph_width = 6;
		const uint32_t font_glyph_height = 7;

		Button keyboard_state[Key::COUNT];
		bool keyboard_new_state[Key::COUNT]{ false };
		bool keyboard_old_state[Key::COUNT]{ false };

		inline void update_key_state(uint32_t code, bool state) { keyboard_new_state[code] = state; }

	public:
		void clear(color c);
		void set_pixel(uint32_t x, uint32_t y, color c);
		void set_pixel(uint32_t x, uint32_t y, unsigned long c);

		void draw_line(const fm::color& c, uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1, uint32_t t = 1u);

		void draw_quad_fill(const fm::color& c, uint32_t x, uint32_t y, uint32_t w, uint32_t h);
		void draw_quad(const fm::color& c, uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t t = 1u);
		// s refers to pixel size not actual size
		void draw_framebuffer(framebuffer* fm, uint32_t x, uint32_t y, uint32_t s = 1);

		void draw_text(const std::string& text, uint32_t x, uint32_t y, 
			uint32_t s, fm::color c);

		texture* load_texture(const std::string& filepath);

		// only one font available for this framework
		void load_font(const std::string& filepath);
		uint32_t get_text_width(const std::string& text, uint32_t size = 1);
	};

#ifdef fm_def
#undef fm_def

	static std::unordered_map<uint32_t, uint32_t> VK_keys_map;
	application* application::app_instance;

	static void load_vk_keys()
	{
		/*
		A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
		SPACE, ENTER, CTRL, ALT, TAB, SHIFT,
		N1, N2, N3, N4, N5, N6, N7, N8, N9,
		UP, DOWN, LEFT, RIGHT,
		F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
		MINUS, PLUS, LEFT_BRACKET, RIGHT_BRACKET,
			COUNT
		*/
		for (uint32_t i = 'A'; i <= 'Z'; i++)
			VK_keys_map[i] = Key::A + (i - 'A');

		VK_keys_map[VK_SPACE] = Key::SPACE; VK_keys_map[VK_RETURN] = Key::ENTER; VK_keys_map[VK_CONTROL] = Key::CTRL;
		VK_keys_map[VK_PAUSE] = Key::ALT;   VK_keys_map[VK_TAB] = Key::TAB;   VK_keys_map[VK_SHIFT] = Key::SHIFT;

		for (uint32_t i = '0'; i <= '9'; i++)
			VK_keys_map[i] = Key::N0 + (i - '0');

		VK_keys_map[VK_UP] = Key::UP;       VK_keys_map[VK_DOWN] = Key::DOWN;  VK_keys_map[VK_LEFT] = Key::LEFT;
		VK_keys_map[VK_RIGHT] = Key::RIGHT;

		for (uint32_t i = 0x70; i <= 0x7B; i++)
			VK_keys_map[i] = Key::F1 + (i - 0x70);

		VK_keys_map[VK_OEM_MINUS] = Key::MINUS; VK_keys_map[VK_OEM_PLUS] = Key::PLUS;
		VK_keys_map[VK_OEM_4] = Key::LEFT_BRACKET; VK_keys_map[VK_OEM_6] = Key::RIGHT_BRACKET;
	}

	bool application::initialize(const wchar_t* name, uint32_t w, uint32_t h, uint32_t p)
	{
		app_instance = this;

		int flags = WS_OVERLAPPEDWINDOW;
		if (!resizable) flags ^= WS_THICKFRAME;
		if (!minimize_button) flags ^= WS_MINIMIZEBOX;
		if (!maximize_button) flags ^= WS_MAXIMIZEBOX;

		if (!create_window(name, w, h, flags))
			return false;

		if (!create_graphics_context(w / p, h / p, p))
			return false;

		load_vk_keys();
		return true;
	}

	bool application::initialize(const wchar_t* name, uint32_t w, uint32_t h, uint32_t buffer_w, uint32_t buffer_h)
	{
		app_instance = this;

		int flags = WS_OVERLAPPEDWINDOW;
		if (!resizable) flags ^= WS_THICKFRAME;
		if (!minimize_button) flags ^= WS_MINIMIZEBOX;
		if (!maximize_button) flags ^= WS_MAXIMIZEBOX;

		if (!create_window(name, w, h, flags))
			return false;

		if (!create_graphics_context(buffer_w, buffer_h, w / buffer_w))
			return false;

		load_vk_keys();
		return true;
	}

	void application::core_update()
	{
		for (uint32_t i = 0; i < Key::COUNT; i++)
		{
			keyboard_state[i].pressed = false;
			keyboard_state[i].released = false;
			if (keyboard_new_state[i] != keyboard_old_state[i])
			{
				if (keyboard_new_state[i])
				{
					keyboard_state[i].pressed = !keyboard_state[i].held;
					keyboard_state[i].held = true;
				}
				else
				{
					keyboard_state[i].released = true;
					keyboard_state[i].held = false;
				}
			}
			keyboard_old_state[i] = keyboard_new_state[i];
		}
	}

	void application::start()
	{
		float dt = 0.0f;
		std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
		std::chrono::time_point<std::chrono::system_clock> old = now;

		on_create();
		while (is_running)
		{
			now = std::chrono::system_clock::now();
			dt = std::chrono::duration<float>(now - old).count();
			old = now;

			core_update();
			on_update(dt);

			present();
			poll_events();
		}
	}

	void application::clear(color c)
	{
		uint32_t* first = (uint32_t*)pgraphics_context->memory_buffer;

		for (uint32_t i = 0; i < pgraphics_context->buffer_width * pgraphics_context->buffer_height; i++)
			*(first + i) = c.hex;
	}

	void application::set_pixel(uint32_t x, uint32_t y, color c)
	{
		uint32_t pos = y * pwindow->width + x;
		if (x >= 0 && x < pwindow->width && y >= 0 && y < pwindow->height)
			*(pgraphics_context->memory_buffer + pos) = c.hex;
	}

	void application::set_pixel(uint32_t x, uint32_t y, unsigned long c)
	{
		uint32_t pos = y * pwindow->width + x;
		if (x >= 0 && x < pwindow->width && y >= 0 && y < pwindow->height)
			*(pgraphics_context->memory_buffer + pos) = c;
	}

	void application::draw_line(const fm::color& c, uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1, uint32_t t)
	{
		v2<float> dt(x1 - x0, y1 - y0);
		uint32_t length = dt.magnitude();
		length = clamp(length, 0u, 
			(uint32_t)sqrt(pgraphics_context->buffer_width * pgraphics_context->buffer_width + 
				pgraphics_context->buffer_height * pgraphics_context->buffer_height));
		v2<float> addFactor = dt.normalize() * t;

		dt.x = x0;
		dt.y = y0;

		for (double i = 0; i < length / t; i++)
		{
			draw_quad_fill(c, dt.x, dt.y, t, t);
			dt.x += addFactor.x;
			dt.y += addFactor.y;
		}
	}

	void application::draw_quad_fill(const fm::color& c, uint32_t x, uint32_t y, uint32_t w, uint32_t h)
	{
		uint32_t start_x = clamp(x, 0u, pgraphics_context->buffer_width);
		uint32_t end_x = clamp(x + w, 0u, pgraphics_context->buffer_width);
		uint32_t start_y = clamp(y, 0u, pgraphics_context->buffer_height);
		uint32_t end_y = clamp(y + h, 0u, pgraphics_context->buffer_height);

		uint32_t* pixel;
		for (uint32_t y = start_y; y < end_y; y++)
		{
			pixel = (uint32_t*)pgraphics_context->memory_buffer + 
				start_x + y * pgraphics_context->buffer_width;
			for (uint32_t x = start_x; x < end_x; x++)
			{
				*(pixel++) = c.hex;
			}
		}
	}

	void application::draw_quad(const fm::color& c, uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t t)
	{
		draw_line(c, x, y, x + w, y, t);
		draw_line(c, x, y, x, y + h, t);
			
		// one pixel was left out
		draw_line(c, x, y + h, x + w + 1, y + h, t);
		draw_line(c, x + w, y, x + w, y + h, t);
	}

	void application::draw_framebuffer(framebuffer* fm, uint32_t x, uint32_t y, uint32_t s)
	{
		uint32_t pos_y = y + fm->height * s;

		for (uint32_t i = 0; i < fm->height; i++)
		{
			uint32_t pos_x = x;
			for (uint32_t j = 0; j < fm->width; j++)
			{
				draw_quad_fill(fm::color(fm->buffer[i * fm->width + j]),
					pos_x, pos_y, s, s);
				pos_x += s;
			}
			pos_y -= s;
		}
	}

	void application::draw_text(const std::string& text, uint32_t x, uint32_t y,
		uint32_t s, fm::color col)
	{
		for (uint32_t ch = 0; ch < text.size(); ch++)
		{
			char c = text[ch];
			if (c == ' ')
			{
				x += 5;
				continue;
			}

			// daca e litera mare nu gaseste bine offsetul
			if (std::isupper(c))
				c = std::tolower(c);

			font_type type;
			uint32_t offset;

			if (std::isalpha(c))
				type = font_type::CHARACTERS, offset = c - 'a';
			else if (std::isdigit(c))
				type = font_type::NUMBERS, offset = c - '0';
			else if (glyph_exist(c))
				type = font_type::SYMBOLS, offset = get_symbol_offet(c);
			else abort();

			uint32_t pos_x = x;
			uint32_t pos_y = y;

			int width = get_glyph_width(c);

			for (uint32_t i = 0; i < font_glyph_height; i++)
			{
				pos_x = x;

				for (uint32_t j = offset * font_glyph_width + 1;
					j < offset * font_glyph_width + width + 1; j++)
				{
					if (!(font[(uint32_t)type]->buffer[i * font[(uint32_t)type]->width + j] == -1))
						draw_quad_fill(col, pos_x, pos_y, s, s);
					pos_x += s;
				}
				pos_y += s;
			}
			x = pos_x + 1;
		}
	}

	texture* application::load_texture(const std::string& filepath)
	{
		if (textures.find(filepath) != textures.end())
			return textures[filepath];

		texture* tex = new texture();

		std::ifstream readFile(filepath.c_str(), std::ios::in | std::ios::binary);
		std::string extension = std::string(filepath.begin() + filepath.find_last_of(".") + 1, filepath.end());

		if (!readFile.good())
			abort();

		readFile.read((char*)&tex->width, sizeof(uint32_t));
		readFile.read((char*)&tex->height, sizeof(uint32_t));

		tex->buffer = new uint32_t[tex->width * tex->height];
		long temp = 0;
		for (uint32_t i = 0; i < tex->width * tex->height; i++)
		{
			readFile.read((char*)&temp, sizeof(uint32_t));
			tex->buffer[i] = temp;
		}

		textures.insert(std::make_pair(filepath, tex));
		return tex;
	}

	// Location of the "font" folder
	void application::load_font(const std::string& filepath)
	{
		font[(uint32_t)font_type::CHARACTERS] = load_texture(filepath + "characters.spr");
		font[(uint32_t)font_type::NUMBERS] = load_texture(filepath + "numbers.spr");
		font[(uint32_t)font_type::SYMBOLS] = load_texture(filepath + "symbols.spr");

		// each character spacing
		glyph_width['a'] = font_glyph_width - 2; glyph_width['b'] = font_glyph_width - 2;
		glyph_width['c'] = font_glyph_width - 2; glyph_width['d'] = font_glyph_width - 2;
		glyph_width['e'] = font_glyph_width - 2; glyph_width['f'] = font_glyph_width - 2;
		glyph_width['g'] = font_glyph_width - 2; glyph_width['h'] = font_glyph_width - 2;
		glyph_width['i'] = font_glyph_width - 5; glyph_width['j'] = font_glyph_width - 2;
		glyph_width['k'] = font_glyph_width - 2; glyph_width['l'] = font_glyph_width - 2;
		glyph_width['m'] = font_glyph_width - 1; glyph_width['n'] = font_glyph_width - 2;
		glyph_width['o'] = font_glyph_width - 2; glyph_width['p'] = font_glyph_width - 2;
		glyph_width['q'] = font_glyph_width - 2; glyph_width['r'] = font_glyph_width - 2;
		glyph_width['s'] = font_glyph_width - 2; glyph_width['t'] = font_glyph_width - 1;
		glyph_width['u'] = font_glyph_width - 2; glyph_width['v'] = font_glyph_width - 1;
		glyph_width['w'] = font_glyph_width - 1; glyph_width['x'] = font_glyph_width - 3;
		glyph_width['y'] = font_glyph_width - 2; glyph_width['z'] = font_glyph_width - 2;
		glyph_width['0'] = font_glyph_width - 2; glyph_width['1'] = font_glyph_width - 4;
		glyph_width['2'] = font_glyph_width - 2; glyph_width['3'] = font_glyph_width - 2;
		glyph_width['4'] = font_glyph_width - 2; glyph_width['5'] = font_glyph_width - 2;
		glyph_width['6'] = font_glyph_width - 2; glyph_width['7'] = font_glyph_width - 2;
		glyph_width['8'] = font_glyph_width - 2; glyph_width['9'] = font_glyph_width - 2;
		glyph_width['!'] = font_glyph_width - 5; glyph_width['?'] = font_glyph_width - 2;
		glyph_width[':'] = font_glyph_width - 5; glyph_width[';'] = font_glyph_width - 4;
		glyph_width['.'] = font_glyph_width - 5; glyph_width[','] = font_glyph_width - 4;
		glyph_width['['] = font_glyph_width - 4; glyph_width[']'] = font_glyph_width - 4;
		glyph_width['='] = font_glyph_width - 2; glyph_width['*'] = font_glyph_width - 5;
		glyph_width['/'] = font_glyph_width - 1; glyph_width['-'] = font_glyph_width - 2;
		glyph_width['+'] = font_glyph_width - 1; glyph_width['<'] = font_glyph_width - 1;
		glyph_width['>'] = font_glyph_width - 1; glyph_width['\\'] = font_glyph_width - 4;
	}

	uint32_t application::get_text_width(const std::string& text, uint32_t s)
	{
		if (text.empty())
			return 0;

		uint32_t size = 0;

		for (uint32_t i = 0; i < text.size(); i++)
		{
			if (text[i] != ' ')
				size += get_glyph_width(text[i]) * s;
			else size += 5;
		}

		size += text.size() - 1;

		return size;
	}

	uint32_t application::get_glyph_width(char c)
	{
		if (!(c >= -1 && c <= 255))
			return 0;

		if (std::isupper(c))
			c = tolower(c);

		if (glyph_width.find(c) == glyph_width.end())
			return 0;

		return glyph_width[c];
	}

	// can t use the ascii code to get the offset directly
	uint32_t application::get_symbol_offet(char sym)
	{
		int n = 0;
		switch (sym)
		{
		case '!':  n = 0;  break;
		case '?':  n = 1;  break;
		case ':':  n = 2;  break;
		case ';':  n = 3;  break;
		case '.':  n = 4;  break;
		case ',':  n = 5;  break;
		case '[':  n = 6;  break;
		case ']':  n = 7;  break;
		case '=':  n = 8;  break;
		case '*':  n = 9;  break;
		case '/':  n = 10;  break;
		case '-':  n = 11;  break;
		case '+':  n = 12;  break;
		case '<':  n = 13;  break;
		case '>':  n = 14;  break;
		case '\\': n = 15;  break;
		}

		return n;
	}

	bool application::glyph_exist(char c)
	{
		if (glyph_width.find(c) == glyph_width.end())
			return false;
		return true;
	}

	v2<float> application::mouse_position()
	{
		POINT p = { 0.0, 0.0 };
		GetCursorPos(&p);
		ScreenToClient(pwindow->handle, &p);

		v2<float> mouse_pos = { (float)p.x, (float)p.y };

		v2<float> norm = { (float)screen_width() / pwindow->width, (float)screen_height() / pwindow->height };
		mouse_pos = mouse_pos * norm;

		mouse_pos.y = screen_height() - mouse_pos.y;

		return mouse_pos;
	}

	Button application::get_key(Key name)
	{
		return keyboard_state[name];
	}

	static LRESULT CALLBACK window_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		switch (msg)
		{
		case WM_CLOSE: DestroyWindow(hwnd);  break;
		case WM_DESTROY: PostQuitMessage(0); break;
		case WM_KEYDOWN: application::app_instance->update_key_state(VK_keys_map[wParam], true); break;
		case WM_KEYUP: application::app_instance->update_key_state(VK_keys_map[wParam], false); break;
		}

		return DefWindowProc(hwnd, msg, wParam, lParam);
	}

	bool application::create_window(const std::wstring& name, uint16_t w, uint16_t h, unsigned long flags = WS_OVERLAPPEDWINDOW)
	{
		pwindow = new window();
		pwindow->window_class = {};
		pwindow->width = w;
		pwindow->height = h;
		pwindow->info_string = "";
		pwindow->name = name;

		HINSTANCE wnd_instance = GetModuleHandle(NULL);
		pwindow->window_class.lpfnWndProc = window_proc;
		pwindow->window_class.hInstance = GetModuleHandle(NULL);
		pwindow->window_class.lpszClassName = L"fm_class";

		RegisterClass(&pwindow->window_class);

		pwindow->handle = CreateWindowEx(0, L"fm_class", name.c_str(), flags, CW_USEDEFAULT, CW_USEDEFAULT, w, h,
			NULL, NULL, wnd_instance, NULL);

		if (!pwindow->handle)
			return false;

		pwindow->device_context = GetDC(pwindow->handle);

		ShowWindow(pwindow->handle, SW_SHOW);
		return true;
	}

	void application::poll_events()
	{
		MSG msg = { 0 };
		PeekMessage(&msg, NULL, 0, 0, PM_REMOVE);

		if (msg.message == WM_QUIT)
			is_running = false;

		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	bool application::create_graphics_context(uint32_t w, uint32_t h, uint32_t p)
	{
		if (!pwindow)
			return false;

		pgraphics_context = new grahics_context();

		pgraphics_context->buffer_width = w;
		pgraphics_context->buffer_height = h;
		pgraphics_context->pixel_size = p;

		if (pgraphics_context->memory_buffer)
			VirtualFree(pgraphics_context->memory_buffer, 0, MEM_RELEASE);

		pgraphics_context->memory_buffer = (uint32_t*)VirtualAlloc(0, 
			pgraphics_context->buffer_width * pgraphics_context->buffer_height * sizeof(uint32_t), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

		pgraphics_context->bm_info.bmiHeader.biWidth = pgraphics_context->buffer_width;
		pgraphics_context->bm_info.bmiHeader.biHeight = pgraphics_context->buffer_height;

		pgraphics_context->buffer_size = (pgraphics_context->buffer_width * pgraphics_context->buffer_height);

		pgraphics_context->bm_info.bmiHeader.biSize = sizeof(pgraphics_context->bm_info.bmiHeader);
		pgraphics_context->bm_info.bmiHeader.biPlanes = 1;
		pgraphics_context->bm_info.bmiHeader.biBitCount = sizeof(uint32_t) * 8;
		pgraphics_context->bm_info.bmiHeader.biCompression = BI_RGB;

		return true;
	}

	void application::present()
	{
		StretchDIBits(pwindow->device_context, 
			0, 0, pwindow->width, pwindow->height, 
			0, 0, pgraphics_context->buffer_width, pgraphics_context->buffer_height,
			pgraphics_context->memory_buffer, &pgraphics_context->bm_info, DIB_RGB_COLORS, SRCCOPY);
	}

	void application::add_title_info(const std::wstring& info)
	{
		std::wstring title = std::wstring(pwindow->name) + L"  " + info;
		SetWindowText(pwindow->handle, title.c_str());
	}

	void application::free_memory()
	{
		VirtualFree(pgraphics_context->memory_buffer, 0, MEM_FREE);
		DestroyWindow(pwindow->handle);

		delete pwindow;
		delete pgraphics_context;

		for (auto& tex : textures)
			delete tex.second;
	}

	void framebuffer::set_buffer(void* buf)
	{
		memcpy(buffer, buf, width * height * sizeof(uint32_t));
	}

#endif
}