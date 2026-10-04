#pragma once

#include <hvh_shared/hvh_shared.hpp>

namespace features::hvh {

	enum class inject_status
	{
		not_injected,
		injected,
		failed,
	};

	// Owns the cross-process shared block and the loader/ejector for the injected
	// vesta_hvh.dll. `settings` is the live, externally-editable mirror of the
	// mapping's config half.
	class controller_t
	{
	public:
		void initialize( );
		void publish( );
		void shutdown( );

		bool inject( );
		bool eject( );

		[[nodiscard]] inject_status status( ) const;
		[[nodiscard]] bool dll_active( ) const;
		[[nodiscard]] bool signature_found( ) const;
		[[nodiscard]] bool hook_ready( ) const;
		[[nodiscard]] int targets_found( ) const;
		[[nodiscard]] std::string_view last_error( ) const;
		[[nodiscard]] const std::filesystem::path& dll_path( ) const;

		void load( );
		void save( );

		hvh_shared::settings settings{};

	private:
		bool ensure_mapping( );
		void release_mapping( );
		void refresh_paths( );

		void* m_mapping{ nullptr };
		hvh_shared::shared_state* m_view{ nullptr };
		std::filesystem::path m_directory{};
		std::filesystem::path m_dll_path{};
		std::string m_last_error{};
		inject_status m_status{ inject_status::not_injected };
		bool m_initialized{ false };
	};

	[[nodiscard]] controller_t& controller( );

} // namespace features::hvh
