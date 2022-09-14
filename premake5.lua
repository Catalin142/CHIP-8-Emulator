workspace "CHIP-8 Emulator"
	architecture "x86"
	configurations { "Debug", "Release" }
	startproject "CHIP-8 Emulator"
	flags
	{
		"MultiProcessorCompile"
	}

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

include "CHIP-8 Emulator"