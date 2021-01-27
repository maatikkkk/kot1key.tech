#pragma once
#include "../globals.h"
#include "../features/features.h"
#include "lib/detours.h"

namespace hooks {
	void init();

	void undo();

	using get_clr_mdl_fn = void(__thiscall*)(void*, float*, float*, float*);
	extern get_clr_mdl_fn get_clr_original;

	void __fastcall get_clr_modulation(void* ecx, void* edx, float* r, float* g, float* b);
	bool __stdcall is_using_static_props();
	bool __stdcall should_skip_animation_frame();

	void __cdecl send_move(float m1, float m2);
	extern decltype(&send_move) original_cl_move;

	typedef void(__thiscall* update_client_side_animation_t)(c_cs_player*);
	extern update_client_side_animation_t origin_update_client_side_animation;
	void __fastcall update_client_side_animation(c_cs_player* player, uint32_t);

	typedef void(__thiscall* do_extra_bone_processing_t)(void*, studiohdr_t*, vec3_t*, quaternion_t*, const matrix3x4_t&, uint8_t*, void*);
	extern do_extra_bone_processing_t orig_do_extra_bone_processing;
	void __fastcall  do_extra_bone_processing(void* ecx, uint32_t, studiohdr_t* hdr, vec3_t* pos, quaternion_t* q, const matrix3x4_t& matrix, uint8_t* bone_computed, void* context);

	typedef void(__thiscall* standard_blending_rules_t)(c_cs_player*, studiohdr_t*, vec3_t*, quaternion_t*, float, int);
	extern standard_blending_rules_t origin_standard_blending_rules;
	void __fastcall standard_blending_rules(c_cs_player* player, uint32_t, studiohdr_t* hdr, vec3_t* pos, quaternion_t* q, const float time, int mask);

	namespace d3d_device {
		namespace reset {
			constexpr auto index = 16u;
			using T = long(__stdcall*)(IDirect3DDevice9*, D3DPRESENT_PARAMETERS*);
			long __stdcall fn(IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* present_params);
		}

		namespace present {
			constexpr auto index = 17u;
			using T = long(__stdcall*)(IDirect3DDevice9*, RECT*, RECT*, HWND, RGNDATA*);
			long __stdcall fn(IDirect3DDevice9* device, RECT* src_rect, RECT* dest_rect, HWND dest_wnd_override, RGNDATA* dirty_region);
		}
	}

	namespace client_dll {
		namespace frame_stage_notify {
			constexpr auto index = 37u;
			using T = void(__stdcall*)(e_client_frame_stage);
			void __stdcall fn(e_client_frame_stage stage);
		}

		namespace create_move {
			constexpr auto index = 22u;
			using T = void(__thiscall*)(void*, int, float, bool);
			void __stdcall gate(int sequence_number, float input_sample_frame_time, bool active);
			void __stdcall fn(int sequence_number, float input_sample_frame_time, bool active, bool& packet);
		}
	}

	namespace client_mode {
		namespace override_view {
			constexpr auto index = 18u;
			using T = void(__stdcall*)(c_view_setup*);
			void __stdcall fn(c_view_setup* view);
		}

		namespace override_view {
			constexpr auto index = 18;
			typedef void(__stdcall* T) (c_view_setup*);
			void __stdcall fn(c_view_setup* setup);
		}

		namespace post_screen_effects {
			constexpr auto index = 44;
			void __fastcall fn(void* thisptr, void* edx, c_view_setup* setup);
		}
	}

	namespace model_render {
		namespace draw_model_execute {
			constexpr auto index = 21u;
			using T = void(__thiscall*)(i_model_render*, void*, const draw_model_state_t&, const model_render_info_t&, matrix3x4_t*);
			void __fastcall fn(i_model_render* ecx, void* edx, void* context, const draw_model_state_t& state, const model_render_info_t& info, matrix3x4_t* bones);
		}
	}

	namespace panel {
		namespace paint_traverse {
			constexpr auto index = 41u;
			using T = void(__thiscall*)(void*, uint32_t, bool, bool);
			void __fastcall fn(void* ecx, void* edx, uint32_t id, bool force_repaint, bool allow_force);
		}
	}

	namespace surface {
		namespace lock_cursor {
			constexpr auto index = 67u;
			using T = void(__thiscall*)(i_surface*);
			void __fastcall fn(i_surface* ecx, void* edx);
		}
	}

	namespace player {
		namespace eye_angles {
			constexpr auto index = 169u;
			using T = qangle_t*(__thiscall*)(c_cs_player*);
			qangle_t* __fastcall fn(c_cs_player* ecx, void* edx);
		}
	}

	namespace renderable {
		namespace setup_bones {
			constexpr auto index = 13u;
			using T = bool(__thiscall*)(i_client_renderable*, matrix3x4_t*, int, int, float);
			bool __fastcall fn(i_client_renderable* ecx, void* edx, matrix3x4_t* bones, int max_bones, int mask, float time);
		}
	}

	extern std::unique_ptr<memory::hook_t> m_d3d_device;
	extern std::unique_ptr<memory::hook_t> m_client_dll;
	extern std::unique_ptr<memory::hook_t> m_client_mode;
	extern std::unique_ptr<memory::hook_t> m_model_render;
	extern std::unique_ptr<memory::hook_t> m_panel;
	extern std::unique_ptr<memory::hook_t> m_surface;
	extern std::unique_ptr<memory::hook_t> m_player;
	extern std::unique_ptr<memory::hook_t> m_renderable;
	extern std::unique_ptr<memory::hook_t> m_engine_vgui;
	extern std::unique_ptr<memory::hook_t> m_net_channel;
}
