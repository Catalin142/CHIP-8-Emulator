#include "chip8.h"
#include <fstream>
#include <string>
#include <iostream>

void chip8::initialize()
{
	srand(time(NULL));

	pc = MEMORY_START_ADRESS;
	load_font();
	load_instructions();
}
void chip8::reset()
{
	pc = MEMORY_START_ADRESS;
	index = 0;
	stack_pointer = 0;
	delay_timer = 0;
	sound_timer = 0;

}
void chip8::cycle()
{
	opcode = (memory[pc] << 8u) | memory[pc + 1];
	pc += 2;

	execute_instuction(opcode);

	if (delay_timer > 0)
		delay_timer--;

	if (sound_timer > 0)
		sound_timer--;
}

void chip8::load_rom(const std::string& filepath)
{
	reset();
	FILE* file;
	int length;

	fopen_s(&file, filepath.c_str(), "rb");
	if (file == NULL)
		return;

	fseek(file, 0, SEEK_END);
	length = ftell(file);

	if (length <= 0)
		return;

	fseek(file, 0, SEEK_SET);
	fread(&memory[MEMORY_START_ADRESS], 1, length, file);
}

void chip8::load_font()
{
	// each character is made out of 5 bytes
	uint8_t font_set[5 * NUMBER_OF_CHARACTERS] = {

		0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
		0x20, 0x60, 0x20, 0x20, 0x70, // 1
		0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
		0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
		0x90, 0x90, 0xF0, 0x10, 0x10, // 4
		0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
		0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
		0xF0, 0x10, 0x20, 0x40, 0x40, // 7
		0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
		0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
		0xF0, 0x90, 0xF0, 0x90, 0x90, // A
		0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
		0xF0, 0x80, 0x80, 0x80, 0xF0, // C
		0xE0, 0x90, 0x90, 0x90, 0xE0, // D
		0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
		0xF0, 0x80, 0xF0, 0x80, 0x80  // F

	};

	memcpy(&memory[FONTSET_START_ADRESS], font_set, 5 * NUMBER_OF_CHARACTERS * sizeof(uint8_t));
}

// Sets the display to black (0)
void chip8::op_00E0()
{
	memset(display, 0x00000000, sizeof(display));
}

// Goes to the previous instruction in the stack
void chip8::op_00EE()
{
	--stack_pointer;
	pc = stack[stack_pointer];
}

// Sets program counter (pc) to the new adress, no need to 
// interact with the stack
void chip8::op_1nnn()
{
	// 0x0FFFu = 0b1111111111
	uint16_t adress = opcode & 0x0FFFu;
	pc = adress;
}

// Sets program counter (pc) to the new adress and saves the 
// last adress in the stack
void chip8::op_2nnn()
{
	uint16_t adress = opcode & 0x0FFFu;
	
	stack[stack_pointer++] = pc;
	pc = adress;
}

// If the condision is met, program counter (pc) gets
// incremented by 2 so we skip the next instruction
void chip8::op_3xkk()
{
	// gets 4 bits from the left of the binary number
	// and shift them right 8 times
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t kk = opcode & 0x00FFu;

	if (registers[Vx] == kk)
		pc += 2;
}

// Same as above
void chip8::op_4xkk()
{
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t kk = opcode & 0x00FFu;

	if (registers[Vx] != kk)
		pc += 2;
}

// Same as above
void chip8::op_5xy0()
{
	// gets 4 bits from the left of the binary number
	// and shift them right 8 times
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;

	// gets 4 bits from the middle of the binary number
	// and shift them right 4 times
	uint8_t Vy = (opcode & 0x00F0u) >> 4u;

	if (registers[Vx] == registers[Vy])
		pc += 2;
}

void chip8::op_6xkk()
{
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t kk = opcode & 0x00FFu;
	registers[Vx] = kk;
}

void chip8::op_7xkk()
{
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t kk = opcode & 0x00FFu;
	registers[Vx] += kk;
}

void chip8::op_8xy0()
{
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t Vy = (opcode & 0x00F0u) >> 4u;
	registers[Vx] = registers[Vy];
}

void chip8::op_8xy1()
{
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t Vy = (opcode & 0x00F0u) >> 4u;
	registers[Vx] |= registers[Vy];
}

void chip8::op_8xy2()
{
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t Vy = (opcode & 0x00F0u) >> 4u;
	registers[Vx] &= registers[Vy];
}

void chip8::op_8xy3()
{
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t Vy = (opcode & 0x00F0u) >> 4u;
	registers[Vx] ^= registers[Vy];
}

void chip8::op_8xy4()
{
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t Vy = (opcode & 0x00F0u) >> 4u;
	uint8_t sum = registers[Vx] + registers[Vy];

	registers[VF] = (sum > 255U);
	registers[Vx] = sum & 0xFFu; // % 256
}

void chip8::op_8xy5()
{
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t Vy = (opcode & 0x00F0u) >> 4u;

	registers[VF] = (registers[Vx] > registers[Vy]);
	registers[Vx] -= registers[Vy];
}

void chip8::op_8xy6()
{
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;

	// Gets the least significant bit
	registers[VF] = registers[Vx] & 0x1u;

	registers[Vx] >>= registers[Vx];
}

void chip8::op_8xy7()
{
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t Vy = (opcode & 0x00F0u) >> 4u;

	registers[VF] = (registers[Vy] > registers[Vx]);
	registers[Vx] = registers[Vy] - registers[Vx];
}

void chip8::op_8xyE()
{
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;

	// Gets the most significant bit
	registers[VF] = (registers[Vx] & 0x80u) >> 7u;

	registers[Vx] <<= registers[Vx];
}

void chip8::op_9xy0()
{
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t Vy = (opcode & 0x00F0u) >> 4u;

	if (registers[Vx] != registers[Vy])
		pc += 2;
}

void chip8::op_Annn()
{
	uint16_t address = opcode & 0x0FFFu;
	index = address;
}

void chip8::op_Bnnn()
{
	uint16_t address = opcode & 0x0FFFu;
	pc = registers[0] + address;
}

void chip8::op_Cxkk()
{
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t kk = opcode & 0x00FFu;
	uint8_t rnd = rand() % 256;

	registers[Vx] = rnd & kk;
}

void chip8::op_Dxyn()
{
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t Vy = (opcode & 0x00F0u) >> 4u;
	uint8_t height = opcode & 0x000Fu;

	uint8_t x_pos = registers[Vx] % SCREEN_WIDTH;
	uint8_t y_pos = registers[Vy] % SCREEN_HEIGHT;

	registers[VF] = 0;

	for (uint32_t y = 0; y < height; y++)
	{
		uint8_t sprite_byte = memory[index + y];
		for (uint32_t x = 0; x < 8; x++)
		{
			// gets every byte from the current row
			uint8_t sprite_pixel = sprite_byte & (0x80u >> x);

			// if the current pixel is white
			if (sprite_pixel && x_pos + x < 64 && y_pos + y < 32) 
			{
				uint32_t& screen_pixel = display[(y_pos + y) * SCREEN_WIDTH + (x_pos + x)];
				// collision
				if (screen_pixel == 0xFFFFFFFF)
					registers[VF] = 1;
				screen_pixel ^= 0xFFFFFFFF;
			}
		}
	}
}

void chip8::op_Ex9E()
{
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t key = registers[Vx];

	if (keypad[key])
		pc += 2;
}

void chip8::op_ExA1()
{
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t key = registers[Vx];

	if (!keypad[key])
		pc += 2;
}

void chip8::op_Fx07()
{
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	registers[Vx] = delay_timer;
}

void chip8::op_Fx0A()
{
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;

	bool key_pressed = false;
	for(uint8_t i = 0; i < 16; i++)
		if (keypad[i])
		{
			registers[Vx] = i;
			key_pressed = true;
		}

	if (!key_pressed)
		pc -= 2;

}

void chip8::op_Fx15()
{
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	delay_timer = registers[Vx];
}

void chip8::op_Fx18()
{
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	sound_timer = registers[Vx];
}

void chip8::op_Fx1E()
{
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	index += registers[Vx];
}

void chip8::op_Fx29()
{
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t digit = registers[Vx];
	index = FONTSET_START_ADRESS + (5 * digit);
}

void chip8::op_Fx33()
{
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t value = registers[Vx];

	memory[index + 2] = value % 10;
	value /= 10;

	memory[index + 1] = value % 10;
	value /= 10;

	memory[index] = value % 10;
}

void chip8::op_Fx55()
{
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	for (int i = 0; i <= Vx; i++)
		memory[index + i] = registers[i];
}

void chip8::op_Fx65()
{
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	for (int i = 0; i <= Vx; i++)
		registers[i] = memory[index + i];
}

void chip8::load_instructions()
{
	instructions[0x00E0] = &chip8::op_00E0;
	instructions[0x00EE] = &chip8::op_00EE;
	instructions[0x1000] = &chip8::op_1nnn;
	instructions[0x2000] = &chip8::op_2nnn;
	instructions[0x3000] = &chip8::op_3xkk;
	instructions[0x4000] = &chip8::op_4xkk;
	instructions[0x5000] = &chip8::op_5xy0;
	instructions[0x6000] = &chip8::op_6xkk;
	instructions[0x7000] = &chip8::op_7xkk;
	instructions[0x8000] = &chip8::op_8xy0;
	instructions[0x8001] = &chip8::op_8xy1;
	instructions[0x8002] = &chip8::op_8xy2;
	instructions[0x8003] = &chip8::op_8xy3;
	instructions[0x8004] = &chip8::op_8xy4;
	instructions[0x8005] = &chip8::op_8xy5;
	instructions[0x8006] = &chip8::op_8xy6;
	instructions[0x8007] = &chip8::op_8xy7;
	instructions[0x800E] = &chip8::op_8xyE;
	instructions[0x9000] = &chip8::op_9xy0;
	instructions[0xA000] = &chip8::op_Annn;
	instructions[0xB000] = &chip8::op_Bnnn;
	instructions[0xC000] = &chip8::op_Cxkk;
	instructions[0xD000] = &chip8::op_Dxyn;
	instructions[0xE00E] = &chip8::op_Ex9E;
	instructions[0xE001] = &chip8::op_ExA1;
	instructions[0xF007] = &chip8::op_Fx07;
	instructions[0xF00A] = &chip8::op_Fx0A;
	instructions[0xF015] = &chip8::op_Fx15;
	instructions[0xF018] = &chip8::op_Fx18;
	instructions[0xF01E] = &chip8::op_Fx1E;
	instructions[0xF029] = &chip8::op_Fx29;
	instructions[0xF033] = &chip8::op_Fx33;
	instructions[0xF055] = &chip8::op_Fx55;
	instructions[0xF065] = &chip8::op_Fx65;
}

void chip8::execute_instuction(uint16_t opcode)
{
	uint16_t code = (0xF0FF & opcode);
	if (instructions.find(code) == instructions.end())
		code = (code & 0xFF0F);
	if (instructions.find(code) == instructions.end())
		code = (code & 0xFFF0);
	if (instructions.find(code) == instructions.end())
	{
		std::cout << "instruction doesnt exist: " << code << "\n";
		return;
	}

	(this->*(instructions[code]))();
}
