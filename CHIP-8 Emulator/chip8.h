#pragma once
#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>

#define MEMORY_START_ADRESS 0x200
#define FONTSET_START_ADRESS 0x050
#define NUMBER_OF_CHARACTERS 16
#define VF 0xF
#define SCREEN_WIDTH 64
#define SCREEN_HEIGHT 32

struct chip8;

typedef void (chip8::*func)();
typedef std::unordered_map<uint16_t, func> instruction_table;

// Check progress.txt for more informations
struct chip8
{
	// Opcode
	uint16_t opcode{};

	// 16 8 - Bit registers
	uint8_t registers[16]{};

	// 4K bites of memory
	uint8_t memory[4096]{};

	// 16 Bit index registers
	uint16_t index{};

	// 16 Bit program counter (PC)
	uint16_t pc{};

	// 16-level stack (LIFO)
	uint16_t stack[16]{};
	
	// 8 Bit stack pointer
	uint8_t stack_pointer{};
	
	// 8 Bit delay timer
	uint8_t delay_timer{};

	// 8 Bit sound timer
	uint8_t sound_timer{};

	// 16 Input Keys
	uint8_t keypad[16]{};

	// 64x32 display
	uint32_t display[64 * 32]{};

	void initialize();
	void reset();
	void load_rom(const std::string& filepath);
	void execute_instuction(uint16_t opcode);

	void cycle();

private:
	void load_font();
	void load_instructions();

	// Chip-8 instructions
	void op_00E0();
	void op_00EE();
	void op_1nnn();
	void op_2nnn();
	void op_3xkk();
	void op_4xkk();
	void op_5xy0();
	void op_6xkk();
	void op_7xkk();
	void op_8xy0();
	void op_8xy1();
	void op_8xy2();
	void op_8xy3();
	void op_8xy4();
	void op_8xy5();
	void op_8xy6();
	void op_8xy7();
	void op_8xyE();
	void op_9xy0();
	void op_Annn();
	void op_Bnnn();
	void op_Cxkk();
	void op_Dxyn();
	void op_Ex9E();
	void op_ExA1();
	void op_Fx07();
	void op_Fx0A();
	void op_Fx15();
	void op_Fx18();
	void op_Fx1E();
	void op_Fx29();
	void op_Fx33();
	void op_Fx55();
	void op_Fx65();

private:
	instruction_table instructions;
};