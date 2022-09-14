#include "framework.h"
#include "chip8.h"

#include <sstream>
#include <queue>
#include <filesystem>
#include <algorithm>

class CHIP8_emulator : public fm::application
{
public:
	CHIP8_emulator() = default;
	~CHIP8_emulator()
	{
		delete fm;
	}

	void on_create() override
	{
		load_font("font/");

		interpreter.initialize();
		std::string path = "roms/";
		for (const auto& game : std::filesystem::directory_iterator(path))
			available_games.push_back(game.path().string());
		rom_title = available_games[game_index];
		rom_title = rom_title.substr(rom_title.find_first_of('/') + 1);
		interpreter.load_rom(path + rom_title);

		fm = new fm::framebuffer(64, 32);

	}
	void on_update(float dt) override
	{
		if (current_time > cycle_delay)
		{
			uint32_t separator_x = 64 * 3 - 1;
			uint32_t title_pos =  separator_x / 2.0f;
			std::string title = "< " + rom_title + " >";
			uint32_t title_width = get_text_width(title, 2);

			clear(fm::color(0.0f, 0.0f, 0.0f));
			draw_text(title, title_pos - title_width / 2, screen_height() - 20.0f, 2, fm::color(1.0f, 1.0f, 1.0f));
			draw_cpu();
			process_input();
			interpreter.cycle();
			current_time = 0.0f;
			present();
			draw_line(fm::color(1.0f, 1.0f, 1.0f), separator_x, 0, separator_x, screen_height());
		}

		bool change_game = false;
		if (get_key(fm::Key::RIGHT).pressed)
			game_index++, change_game = true;
		else if (get_key(fm::Key::LEFT).pressed)
			game_index--, change_game = true;

		if (change_game)
		{
			if (game_index < 0)
				game_index = available_games.size() - 1;
			if (game_index >= available_games.size())
				game_index = 0;

			//op_00E0
			memset(interpreter.display, 0x00000000, sizeof(interpreter.display));
			rom_title = available_games[game_index];
			rom_title = rom_title.substr(rom_title.find_first_of('/') + 1);
			interpreter.load_rom("roms/" + rom_title);
		}


		current_time += dt;
		if (get_key(fm::Key::LEFT_BRACKET).pressed)
			cycle_delay -= 0.01f;

		if (get_key(fm::Key::RIGHT_BRACKET).pressed)
			cycle_delay += 0.02f;

		if (cycle_delay < 0.0f)
			cycle_delay = 0.0f;

		if (cycle_delay > 0.5f)
			cycle_delay = 0.5f;

		if (get_key(fm::Key::N2).pressed)
			std::cout << "DA";

		fm::v2<uint32_t> text_pos(195u, 35u);
		// quick and dirty way to clear a portion of the screen
		draw_quad_fill(fm::color(0.0f, 0.0f, 0.0f), text_pos.x, 5u, 130u, 30u);
		text_pos.y -= 10;
		draw_text("Cycle delay: " + std::to_string(cycle_delay), text_pos.x, text_pos.y, 1, fm::color(1.0f, 1.0f, 1.0f));
		text_pos.y -= 10;
		draw_text("[ and ] to modify", text_pos.x, text_pos.y, 1, fm::color(1.0f, 1.0f, 1.0f));

	}

	void present()
	{
		fm->set_buffer(interpreter.display);
		draw_framebuffer(fm, 0, 35, 3);
	}

private:
	chip8 interpreter;
	std::string rom_title;
	fm::framebuffer* fm;
	float cycle_delay = 0.2f;
	float current_time = cycle_delay;
	uint16_t pcs[4];
	uint16_t first_available = 0;
	std::vector<std::string> available_games;
	uint32_t game_index = 0;

private:
	std::string hex(uint32_t n, uint8_t d)
	{
		std::string s(d, '0');
		for (int i = d - 1; i >= 0; i--, n >>= 4)
			s[i] = "0123456789ABCDEF"[n & 0xF];
		return s;
	};

	void draw_cpu()
	{
		fm::color text_color(1.0f, 1.0f, 1.0f);
		fm::v2<uint32_t> text_pos(195, 150);
		uint32_t separator_x = screen_width() - 64 * 3 - 1;
		uint32_t text_width = get_text_width("CPU:", 2);
		draw_text("CPU:", 64 * 3 - 1 + separator_x / 2 - text_width / 2,
			text_pos.y + 20, 2, fm::color(1.0f, 1.0f, 1.0f));
		draw_text("Registers:", text_pos.x, text_pos.y, 1, text_color);

		text_pos.y -= 10;
		for (int i = 0; i < 4; i++)
			draw_text(std::to_string(interpreter.registers[i]),
				text_pos.x + i * 32, text_pos.y, 1, text_color);

		text_pos.y -= 10;
		for (int i = 4; i < 8; i++)
			draw_text(std::to_string(interpreter.registers[i]),
				text_pos.x + (i - 4) * 32, text_pos.y, 1, text_color);

		text_pos.y -= 10;
		for (int i = 8; i < 12; i++)
			draw_text(std::to_string(interpreter.registers[i]),
				text_pos.x + (i - 8) * 32, text_pos.y, 1, text_color);

		text_pos.y -= 10;
		for (int i = 12; i < 16; i++)
			draw_text(std::to_string(interpreter.registers[i]),
				text_pos.x + (i - 12) * 32, text_pos.y, 1, text_color);

		draw_quad(fm::color(1.0f, 1.0f, 1.0f), text_pos.x - 2, text_pos.y - 2, 120, 40);
		text_pos.y -= 10;
		draw_text("Program counter: 0x" + hex(interpreter.pc, 4), text_pos.x, text_pos.y, 1, text_color);

		text_pos.y -= 10;
		draw_text("Stack pointer: " + std::to_string(interpreter.stack_pointer), text_pos.x, text_pos.y, 1, text_color);
	
		if (first_available == 4)
		{
			first_available = 3;
			for (int i = 0; i < 3; i++)
				pcs[i] = pcs[i + 1];
		}
		pcs[first_available++] = (interpreter.memory[interpreter.pc] << 8u) | interpreter.memory[interpreter.pc + 1];

		text_pos.y -= 10;
		draw_text("Instructions: ", text_pos.x, text_pos.y, 1, text_color);
		for (uint32_t i = 0; i < first_available; i++)
		{
			text_pos.y -= 10;
			draw_text("0x" + hex(pcs[i], 4) + " " + interpreter.get_instruction_name(pcs[i]), text_pos.x, text_pos.y, 1, text_color);
		}
		for (uint32_t i = 0; i < 4 - first_available; i++)
			text_pos.y -= 10;

		draw_quad(fm::color(1.0f, 1.0f, 1.0f), text_pos.x - 2, text_pos.y - 2, 120, 40);

	}

	void process_input()
	{
		/*
		 +-+-+-+-+    +-+-+-+-+
		 |1|2|3|C|    |1|2|3|4|
		 +-+-+-+-+    +-+-+-+-+
		 |4|5|6|D|    |Q|W|E|R|
		 +-+-+-+-+ => +-+-+-+-+
		 |7|8|9|E|    |A|S|D|F|
		 +-+-+-+-+    +-+-+-+-+
		 |A|0|B|F|    |Z|X|C|V|
		 +-+-+-+-+    +-+-+-+-+
		*/

		interpreter.keypad[0]   = get_key(fm::Key::X).held;
		interpreter.keypad[1]   = get_key(fm::Key::N1).held;
		interpreter.keypad[2]   = get_key(fm::Key::N2).held;
		interpreter.keypad[3]   = get_key(fm::Key::N3).held;
		interpreter.keypad[4]   = get_key(fm::Key::Q).held;
		interpreter.keypad[5]   = get_key(fm::Key::W).held;
		interpreter.keypad[6]   = get_key(fm::Key::E).held;
		interpreter.keypad[7]   = get_key(fm::Key::A).held;
		interpreter.keypad[8]   = get_key(fm::Key::S).held;
		interpreter.keypad[9]   = get_key(fm::Key::D).held;
		interpreter.keypad[0xA] = get_key(fm::Key::Z).held;
		interpreter.keypad[0xB] = get_key(fm::Key::C).held;
		interpreter.keypad[0xC] = get_key(fm::Key::N4).held;
		interpreter.keypad[0xD] = get_key(fm::Key::R).held;
		interpreter.keypad[0xE] = get_key(fm::Key::F).held;
		interpreter.keypad[0xF] = get_key(fm::Key::V).held;

	}
};

int main()
{
	CHIP8_emulator app;

	if (app.initialize(L"Chip-8", 1280, 720, 320, 200))
		app.start();

	return 0;
}