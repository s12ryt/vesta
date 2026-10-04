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

	// ---- vtable pointer lookup -----------------------------------------------
	// Finds the address of the first pointer-sized entry inside [base, base+size)
	// whose value equals target. Used to locate the virtual-table slot that holds
	// a scanned function (for example CreateMove) so it can be swapped without
	// patching executable bytes.
	[[nodiscard]] void** find_pointer_entry( std::uint8_t* base, std::size_t size, const void* target );

	// ---- vtable hook ---------------------------------------------------------
	// Swaps a single function pointer inside a virtual table. There is no
	// trampoline and no instruction-length guesswork, so it is safe for targets
	// whose first instruction is shorter than the five bytes a rel32 JMP needs.
	class vtable_hook
	{
	public:
		~vtable_hook( ) { remove( ); }

		bool install( void** slot, void* detour );
		void remove( );
		[[nodiscard]] bool installed( ) const { return m_slot != nullptr; }
		[[nodiscard]] void* original( ) const { return m_original; }

	private:
		void** m_slot{ nullptr };
		void* m_original{ nullptr };
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
