#include "hooks.h"

namespace hooks {

	struct renderable_info_t {
		i_client_renderable* renderable;
		void* alpha_property;
		int enum_count;
		int render_frame;
		unsigned short first_shadow;
		unsigned short leaf_list;
		short area;
		std::uint16_t flags;
		std::uint16_t flags2;
		vec3_t bloated_abs_mins;
		vec3_t bloated_abs_maxs;
		vec3_t abs_mins;
		vec3_t abs_maxs;
		int pad;
	};
	// ---------------------------------------------- //
	recv_prop_hook_t* sequence_hook = nullptr;
	std::unique_ptr<memory::hook_t> m_file = nullptr;
	std::unique_ptr<memory::hook_t> m_bsp = nullptr;
	std::unique_ptr<memory::hook_t> m_prediction = nullptr;
	std::unique_ptr<memory::hook_t> m_engine = nullptr;
	std::unique_ptr<memory::hook_t> m_cheats = nullptr;
	// ---------------------------------------------- //
	//bool __fastcall draw_fog(void* ecx, void* edx)
	//{
	//	return !m_cfg.removals.remove_fog;
	//}
	// ---------------------------------------------- //
	int __fastcall list_leaves_in_box(std::uintptr_t ecx, std::uintptr_t edx, vec3_t& mins, vec3_t& maxs, unsigned short* list, int list_max)
	{
		static auto original_fn = m_bsp->get_original< decltype(&list_leaves_in_box) >(6);
		static auto insert_into_tree_call_list_leaves_in_box = memory::pattern_scan(_("client.dll"), _("89 44 24 14 EB 08 C7 44 24 ? ? ? ? ? 8B 45"));

		if (reinterpret_cast<std::uintptr_t>(_ReturnAddress()) != insert_into_tree_call_list_leaves_in_box) // ADD ESP CHECK
			return original_fn(ecx, edx, mins, maxs, list, list_max);

		auto info = *reinterpret_cast<renderable_info_t**>(reinterpret_cast<std::uintptr_t>(_AddressOfReturnAddress()) + 0x14);

		if (!info || !info->renderable)
			return original_fn(ecx, edx, mins, maxs, list, list_max);

		auto base_entity = info->renderable->get_client_unknown()->get_base_entity();

		if (!base_entity || !base_entity->is_player())
			return original_fn(ecx, edx, mins, maxs, list, list_max);

		info->flags &= ~0x100;
		info->flags2 |= 0xC0;

		static vec3_t map_min = vec3_t(MIN_COORD_FLOAT, MIN_COORD_FLOAT, MIN_COORD_FLOAT);
		static vec3_t map_max = vec3_t(MAX_COORD_FLOAT, MAX_COORD_FLOAT, MAX_COORD_FLOAT);

		return original_fn(ecx, edx, map_min, map_max, list, list_max);
	}
	// ---------------------------------------------- //
	hooks::get_clr_mdl_fn hooks::get_clr_original;
	hooks::update_client_side_animation_t hooks::origin_update_client_side_animation;
	hooks::do_extra_bone_processing_t hooks::orig_do_extra_bone_processing;
	hooks::standard_blending_rules_t hooks::origin_standard_blending_rules;
	decltype(&send_move) original_cl_move;
	// ---------------------------------------------- //
	/*void __fastcall get_clr_modulation(void* ecx, void* edx, float* r, float* g, float* b)
	{
		get_clr_original(ecx, r, g, b);

		const auto material = reinterpret_cast<i_material*>(ecx);
		auto group = material->get_texture_group_name();

		bool is_prop = strstr(group, _("StaticProp"));
		bool is_wall = strstr(group, _("World textures"));
		bool is_sky = strstr(group, _("SkyBox"));

		if (m_cfg.esp.active)
		{
			if (is_prop && m_cfg.modulate_world.custom_props)
			{
				*r *= GetBValue(m_cfg.modulate_world.prop_clr) / 255.f;
				*g *= GetGValue(m_cfg.modulate_world.prop_clr) / 255.f;
				*b *= GetRValue(m_cfg.modulate_world.prop_clr) / 255.f;

				*(float*)((DWORD)material->get_shader_params()[5] + 0xC) = m_cfg.modulate_world.prop_alpha / 255.f;
			}
			else if (is_wall && m_cfg.modulate_world.custom_walls)
			{
				*r *= GetBValue(m_cfg.modulate_world.wall_clr) / 255.f;
				*g *= GetGValue(m_cfg.modulate_world.wall_clr) / 255.f;
				*b *= GetRValue(m_cfg.modulate_world.wall_clr) / 255.f;
			}
			else if (is_sky && m_cfg.modulate_world.custom_sky)
			{
				*r *= GetBValue(m_cfg.modulate_world.sky_clr) / 255.f;
				*g *= GetGValue(m_cfg.modulate_world.sky_clr) / 255.f;
				*b *= GetRValue(m_cfg.modulate_world.sky_clr) / 255.f;
			}
		}
	}*/
	// ---------------------------------------------- //
	void __fastcall update_client_side_animation(c_cs_player* player, uint32_t)
	{
		if (!player || !player->is_alive())
			return origin_update_client_side_animation(player);

		if (globals::m_call_client_update && player->get_index() == globals::m_local->get_index() || globals::m_call_client_update_enemy && player->get_index() != globals::m_local->get_index())
			origin_update_client_side_animation(player);
	}
	// ---------------------------------------------- //
	void __fastcall do_extra_bone_processing(void* ecx, uint32_t, studiohdr_t* hdr, vec3_t* pos, quaternion_t* q, const matrix3x4_t& matrix, uint8_t* bone_computed, void* context)
	{
		return;
	}
	// ---------------------------------------------- //
	void __fastcall standard_blending_rules(c_cs_player* player, uint32_t, studiohdr_t* hdr, vec3_t* pos, quaternion_t* q, const float time, int mask)
	{
		if (!player)
			return origin_standard_blending_rules(player, hdr, pos, q, time, mask);

		*(int*)((DWORD)player + 0x2698) = 0;
		mask |= 0x200;

		player->get_effects() |= 8;
		origin_standard_blending_rules(player, hdr, pos, q, time, mask);
		player->get_effects() &= ~8;
	}
	// ---------------------------------------------- //
	bool __stdcall should_skip_animation_frame()
	{
		return false;
	}
	// ---------------------------------------------- //
	using in_prediction_t = bool(__thiscall*) (void*);
	// ---------------------------------------------- //
	const auto ptr_setupbones = reinterpret_cast<void*>(SIG(("client.dll"), ("8B 40 ? FF D0 84 C0 74 ? F3 0F 10 05 ? ? ? ? EB ?")).m_ptr);
	const auto MaintainSequenceTransitions = (void*)SIG("client.dll", "84 C0 74 17 8B 87").m_ptr;
	// ---------------------------------------------- //
	bool __fastcall in_prediction(void* p)
	{
		const auto ofunc = m_prediction->get_original< in_prediction_t >(14);

		if (reinterpret_cast<uintptr_t> (_ReturnAddress()) == reinterpret_cast<uintptr_t> (MaintainSequenceTransitions))
			return true;

		if (reinterpret_cast<uintptr_t> (_ReturnAddress()) == reinterpret_cast<uintptr_t> (ptr_setupbones) + 5)
			return false;

		return ofunc(p);
	}
	// ---------------------------------------------- //
	auto ptr_accumulate_layers = reinterpret_cast<void*>(SIG("client.dll", "84 C0 75 0D F6 87").m_ptr);
	auto setupvelocity_call = reinterpret_cast<void*>(SIG(("client.dll"), ("84 C0 75 38 8B 0D ? ? ? ? 8B 01 8B 80 ? ? ? ? FF D0")).m_ptr);
	// ---------------------------------------------- //
	using is_hltv_t = bool(__fastcall*) ();
	// ---------------------------------------------- //
	bool __fastcall is_hltv()
	{
		const auto org_f = m_engine->get_original< is_hltv_t >(93);

		if (reinterpret_cast<uintptr_t> (_ReturnAddress()) == reinterpret_cast<uintptr_t> (ptr_accumulate_layers))
			return true;

		if (reinterpret_cast<uintptr_t> (_ReturnAddress()) == reinterpret_cast<uintptr_t> (setupvelocity_call))
			return true;

		return org_f();
	}
	// ---------------------------------------------- //
	typedef int32_t(__thiscall* BoxVisibleFn)(i_engine_client*, vec3_t&, vec3_t&);
	// ---------------------------------------------- //
	int32_t __fastcall is_box_visible(i_engine_client* engine_client, uint32_t, vec3_t& min, vec3_t& max)
	{
		static auto BoxVisible = m_engine->get_original< BoxVisibleFn >(32);

		static const auto ret = _("\x85\xC0\x74\x2D\x83\x7D\x10\x00\x75\x1C");

		if (!memcmp(_ReturnAddress(), ret, 10))
			return 1;

		return BoxVisible(engine_client, min, max);
	}
	// ---------------------------------------------- //
	using send_net_msg_fn = bool(__thiscall*)(i_net_channel*, i_net_msg&, bool, bool);
	// ---------------------------------------------- //
	bool __fastcall send_net_msg(i_net_channel* pNetChan, void* edx, i_net_msg& msg, bool bForceReliable, bool bVoice)
	{
		auto SendNetMsg = hooks::m_net_channel->get_original<send_net_msg_fn>(40);

		if (msg.get_type() == 14)
			return false;

		if (msg.get_group() == 9)
			bVoice = true;

		return SendNetMsg(pNetChan, msg, bForceReliable, bVoice);
	}
	// ---------------------------------------------- //
	bool __fastcall hkLooseFileAllowed(void* ecx, void* edx)
	{
		return true;
	}
	// ---------------------------------------------- //
	bool __fastcall sv_cheats_get_bool(PVOID pConVar, void* edx)
	{
		static auto dwCAM_Think = SIG("client.dll", "85 C0 75 30 38 86").m_ptr;
		static auto ofunc = m_cheats->get_original<bool(__thiscall*)(PVOID)>(13);

		if (!ofunc)
			return false;

		if (reinterpret_cast<DWORD>(_ReturnAddress()) == reinterpret_cast<DWORD>(dwCAM_Think))
			return true;

		return ofunc(pConVar);
	}
	// ---------------------------------------------- //
	void init() {
		// // // // // // // // // // // // // // // // // // // // // // //

		m_d3d_device = std::make_unique<memory::hook_t>(interfaces::m_d3d_device);

		m_d3d_device->hook(d3d_device::reset::index, d3d_device::reset::fn);
		m_d3d_device->hook(d3d_device::present::index, d3d_device::present::fn);

		// // // // // // // // // // // // // // // // // // // // // // //

		m_client_dll = std::make_unique<memory::hook_t>(interfaces::m_client_dll);

		m_client_dll->hook(client_dll::frame_stage_notify::index, client_dll::frame_stage_notify::fn);
		m_client_dll->hook(client_dll::create_move::index, client_dll::create_move::gate);

		// // // // // // // // // // // // // // // // // // // // // // //

		m_client_mode = std::make_unique<memory::hook_t>(interfaces::m_client_mode);

		m_client_mode->hook(client_mode::override_view::index, client_mode::override_view::fn);

		// // // // // // // // // // // // // // // // // // // // // // //

		m_model_render = std::make_unique<memory::hook_t>(interfaces::m_model_render);

		m_model_render->hook(model_render::draw_model_execute::index, model_render::draw_model_execute::fn);

		// // // // // // // // // // // // // // // // // // // // // // //

		m_panel = std::make_unique<memory::hook_t>(interfaces::m_panel);

		m_panel->hook(panel::paint_traverse::index, panel::paint_traverse::fn);

		// // // // // // // // // // // // // // // // // // // // // // //

		m_surface = std::make_unique<memory::hook_t>(interfaces::m_surface);

		m_surface->hook(surface::lock_cursor::index, surface::lock_cursor::fn);

		// // // // // // // // // // // // // // // // // // // // // // //

		m_player = std::make_unique<memory::hook_t>(c_cs_player::get_vtable());

		m_player->hook(player::eye_angles::index, player::eye_angles::fn);

		// // // // // // // // // // // // // // // // // // // // // // //

		m_renderable = std::make_unique<memory::hook_t>(i_client_renderable::get_vtable());

		m_renderable->hook(renderable::setup_bones::index, renderable::setup_bones::fn);

		// // // // // // // // // // // // // // // // // // // // // // //

		m_net_channel = std::make_unique<memory::hook_t>(interfaces::m_client_state->m_net_channel);
		m_net_channel->hook(40, send_net_msg);

		// // // // // // // // // // // // // // // // // // // // // // //

		m_engine = std::make_unique<memory::hook_t>(interfaces::m_engine);
		m_engine->hook(93, is_hltv);
		m_engine->hook(32, is_box_visible);

		// // // // // // // // // // // // // // // // // // // // // // //

		m_prediction = std::make_unique<memory::hook_t>(interfaces::m_prediction);
		m_prediction->hook(14, in_prediction);

		// // // // // // // // // // // // // // // // // // // // // // //

		m_bsp = std::make_unique<memory::hook_t>(interfaces::m_engine->get_bsp_tree_query());
		m_bsp->hook(6, list_leaves_in_box);

		//static const auto get_clr = SIG("materialsystem.dll", "55 8B EC 83 EC ? 56 8B F1 8A 46").m_ptr;
		//get_clr_original = (get_clr_mdl_fn)DetourFunction((PBYTE)get_clr, (PBYTE)get_clr_modulation);

		//static const auto send_move_add = SIG("engine.dll", "55 8B EC 81 EC ? ? ? ? 53 56 57 8B 3D ? ? ? ? 8A").m_ptr;
		//original_cl_move = (decltype(&send_move))DetourFunction((PBYTE)send_move_add, (PBYTE)send_move);

		//static const auto is_prop = SIG("engine.dll", "8B 0D ? ? ? ? 81 F9 ? ? ? ? 75 ? A1 ? ? ? ? 35 ? ? ? ? EB ? 8B 01 FF 50 ? 83 F8 ? 0F 85 ? ? ? ? 8B 0D").m_ptr;
		//DetourFunction((PBYTE)is_prop, (PBYTE)is_using_static_props);

		auto g_pFileSystem = **reinterpret_cast<void***>(SIG("engine.dll", "8B 0D ? ? ? ? 8D 95 ? ? ? ? 6A 00 C6").m_ptr + 0x2);
		if (g_pFileSystem) {
			m_file = std::make_unique<memory::hook_t>(reinterpret_cast<DWORD**>(g_pFileSystem));
			m_file->hook(128, hkLooseFileAllowed);
		}

		static auto* sv_cheats_con = interfaces::m_cvar_system->find_var(FNV1A("sv_cheats"));
		m_cheats = std::make_unique<memory::hook_t>(sv_cheats_con);
		m_cheats->hook(13, sv_cheats_get_bool);

		static const auto skip_frame = SIG("client.dll", "57 8B F9 8B 07 8B 80 ? ? ? ? FF D0 84 C0 75 02").m_ptr;
		DetourFunction((PBYTE)skip_frame, (PBYTE)should_skip_animation_frame);

		static const auto c_cs_player_table = SIG("client.dll", "55 8B EC 83 E4 F8 83 EC 18 56 57 8B F9 89 7C 24 0C").m_ptr + 0x47;
		DWORD* ex_pointer = (DWORD*)*(DWORD*)(c_cs_player_table);

		origin_update_client_side_animation = (update_client_side_animation_t)DetourFunction((PBYTE)ex_pointer[223], (PBYTE)update_client_side_animation);
		orig_do_extra_bone_processing = (do_extra_bone_processing_t)DetourFunction((PBYTE)ex_pointer[197], (PBYTE)do_extra_bone_processing);
		origin_standard_blending_rules = (standard_blending_rules_t)DetourFunction((PBYTE)ex_pointer[205], (PBYTE)standard_blending_rules);
	}

	void undo() {
		m_renderable->unhook();
		m_player->unhook();	
		m_surface->unhook();
		m_panel->unhook();
		m_model_render->unhook();
		m_client_mode->unhook();
		m_client_dll->unhook();
		m_d3d_device->unhook();
	}

	std::unique_ptr<memory::hook_t> m_d3d_device = nullptr;
	std::unique_ptr<memory::hook_t> m_client_dll = nullptr;
	std::unique_ptr<memory::hook_t> m_client_mode = nullptr;
	std::unique_ptr<memory::hook_t> m_model_render = nullptr;
	std::unique_ptr<memory::hook_t> m_panel = nullptr;
	std::unique_ptr<memory::hook_t> m_surface = nullptr;
	std::unique_ptr<memory::hook_t> m_player = nullptr;
	std::unique_ptr<memory::hook_t> m_renderable = nullptr;
	std::unique_ptr<memory::hook_t> m_engine_vgui = nullptr;
	std::unique_ptr<memory::hook_t> m_net_channel = nullptr;
}
