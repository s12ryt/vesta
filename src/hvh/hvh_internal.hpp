#pragma once

// Internal, self-contained helpers for the injected vesta_hvh.dll. This header
// is NOT part of the external executable and deliberately depends only on the
// Windows SDK plus the shared IPC contract.

#include <windows.h>
#include <cstdint>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <string>

#include <hvh_shared/hvh_shared.hpp>

namespace vesta::hvh
{
	// ---- logging -------------------------------------------------------------
	// Appends to %TEMP%\vesta_hvh.log. The DLL never writes next to the game so
	// a failed load cannot leave artefacts inside the CS2 install.
	void log_line( const char* format, ... );
	void log_shutdown( );

	// ---- memory --------------------------------------------------------------
	[[nodiscard]] std::uint8_t* module_base( const wchar_t* module_name, std::size_t* out_size );
	[[nodiscard]] std::uint8_t* scan_pattern( std::uint8_t* base, std::size_t size, const char* pattern );
	[[nodiscard]] bool readable( const void* address, std::size_t size );
	[[nodiscard]] bool writable( void* address, std::size_t size );
	[[nodiscard]] bool patch( void* address, const std::uint8_t* bytes, std::size_t size );

	template <typename T>
	[[nodiscard]] bool safe_read( const void* address, T& out )
	{
		if ( !readable( address, sizeof( T ) ) )
		{
			return false;
		}
		std::memcpy( &out, address, sizeof( T ) );
		return true;
	}

	// ---- inline x64 hook -----------------------------------------------------
	// Overwrites the first prologue bytes of a target with a rel32 JMP to the
	// detour and keeps a executable copy of the overwritten bytes followed by a
	// JMP back (the trampoline) so the original can still be called.
	class inline_hook
	{
	public:
		~inline_hook( ) { remove( ); }

		bool install( void* target, void* detour );
		void remove( );
		[[nodiscard]] bool installed( ) const { return m_target != nullptr; }
		[[nodiscard]] void* trampoline( ) const { return m_trampoline; }

	private:
		void* m_target{ nullptr };
		void* m_trampoline{ nullptr };
		std::uint8_t m_original[ 16 ]{};
		std::size_t m_length{ 0 };
	};

	// ---- resolved game surface ----------------------------------------------
	// Entry-point signatures live here so a CS2 update only requires editing
	// this table. An empty signature makes the module report "signature not
	// found" and stay hooked-out instead of guessing addresses.
	struct game_offsets
	{
		const char* create_move_sig{ "" };
		const char* local_player_sig{ "" };
		const char* entity_list_sig{ "" };
	};

	[[nodiscard]] const game_offsets& offsets( );
}
