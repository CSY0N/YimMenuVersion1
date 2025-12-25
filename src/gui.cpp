#include "gui.hpp"

#include "lua/lua_manager.hpp"
#include "natives.hpp"
#include "renderer/renderer.hpp"
#include "script.hpp"
#include "views/view.hpp"

#include <imgui.h>

namespace big
{
	/**
	 * @brief The later an entry comes in this enum to higher up it comes in the z-index.
	 */
	enum eRenderPriority
	{
		// low priority
		ESP,
		CONTEXT_MENU,

		// medium priority
		MENU = 0x1000,
		VEHICLE_CONTROL,
		LUA,

		// high priority
		INFO_OVERLAY = 0x2000,
		CMD_EXECUTOR,

		GTA_DATA_CACHE = 0x3000,
		ONBOARDING,

		// should remain in a league of its own
		NOTIFICATIONS = 0x4000,
	};

	gui::gui() :
	    m_is_open(false),
	    m_override_mouse(false)
	{
		g_renderer.add_dx_callback(view::notifications, eRenderPriority::NOTIFICATIONS);
		g_renderer.add_dx_callback(view::onboarding, eRenderPriority::ONBOARDING);
		g_renderer.add_dx_callback(view::gta_data, eRenderPriority::GTA_DATA_CACHE);
		g_renderer.add_dx_callback(view::cmd_executor, eRenderPriority::CMD_EXECUTOR);
		g_renderer.add_dx_callback(view::overlay, eRenderPriority::INFO_OVERLAY);

		g_renderer.add_dx_callback(view::vehicle_control, eRenderPriority::VEHICLE_CONTROL);
		g_renderer.add_dx_callback(esp::draw, eRenderPriority::ESP); // TODO: move to ESP service
		g_renderer.add_dx_callback(view::context_menu, eRenderPriority::CONTEXT_MENU);

		g_renderer.add_dx_callback(
		    [this] {
			    dx_on_tick();
		    },
		    eRenderPriority::MENU);

		g_renderer.add_dx_callback(
		    [] {
			    g_lua_manager->draw_always_draw_gui();
		    },
		    eRenderPriority::LUA);

		g_renderer.add_wndproc_callback([](HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
			g_lua_manager->trigger_event<menu_event::Wndproc>(hwnd, msg, wparam, lparam);
		});
		g_renderer.add_wndproc_callback([this](HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
			wndproc(hwnd, msg, wparam, lparam);
		});
		g_renderer.add_wndproc_callback([](HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
			if (g.cmd_executor.enabled && msg == WM_KEYUP && wparam == VK_ESCAPE)
			{
				g.cmd_executor.enabled = false;
			}
		});


		dx_init();

		g_gui = this;
		g_renderer.rescale(g.window.gui_scale);
	}

	gui::~gui()
	{
		g_gui = nullptr;
	}

	bool gui::is_open()
	{
		return m_is_open;
	}

	void gui::toggle(bool toggle)
	{
		m_is_open = toggle;

		toggle_mouse();
	}

	void gui::override_mouse(bool override)
	{
		m_override_mouse = override;

		toggle_mouse();
	}

		void gui::dx_init()
	{
		ImGuiStyle& style                            = ImGui::GetStyle();
		style.Colors[ImGuiCol_Text]                  = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
		style.Colors[ImGuiCol_TextDisabled]          = ImVec4(0.70f, 0.70f, 0.70f, 1.00f);
		style.Colors[ImGuiCol_WindowBg]              = ImVec4(0.00f, 0.20f, 0.00f, 0.95f);
		style.Colors[ImGuiCol_ChildBg]               = ImVec4(0.00f, 0.15f, 0.00f, 0.00f);
		style.Colors[ImGuiCol_PopupBg]               = ImVec4(0.00f, 0.18f, 0.00f, 0.95f);
		style.Colors[ImGuiCol_Border]                = ImVec4(0.80f, 0.00f, 0.00f, 0.30f);
		style.Colors[ImGuiCol_BorderShadow]          = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		style.Colors[ImGuiCol_FrameBg]               = ImVec4(0.00f, 0.30f, 0.00f, 0.54f);
		style.Colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.00f, 0.40f, 0.00f, 0.40f);
		style.Colors[ImGuiCol_FrameBgActive]         = ImVec4(0.00f, 0.50f, 0.00f, 0.67f);
		style.Colors[ImGuiCol_TitleBg]               = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
		style.Colors[ImGuiCol_TitleBgActive]         = ImVec4(0.80f, 0.00f, 0.00f, 1.00f);
		style.Colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
		style.Colors[ImGuiCol_MenuBarBg]             = ImVec4(0.00f, 0.20f, 0.00f, 0.57f);
		style.Colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
		style.Colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.80f, 0.60f, 0.00f, 1.00f);
		style.Colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
		style.Colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(1.00f, 0.80f, 0.00f, 1.00f);
		style.Colors[ImGuiCol_CheckMark]             = ImVec4(0.80f, 0.00f, 0.00f, 1.00f);
		style.Colors[ImGuiCol_SliderGrab]            = ImVec4(0.80f, 0.60f, 0.00f, 1.00f);
		style.Colors[ImGuiCol_SliderGrabActive]      = ImVec4(1.00f, 0.80f, 0.00f, 1.00f);
		style.Colors[ImGuiCol_Button]                = ImVec4(0.80f, 0.00f, 0.00f, 0.40f);
		style.Colors[ImGuiCol_ButtonHovered]         = ImVec4(0.90f, 0.00f, 0.00f, 1.00f);
		style.Colors[ImGuiCol_ButtonActive]          = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
		style.Colors[ImGuiCol_Header]                = ImVec4(0.00f, 0.30f, 0.00f, 0.31f);
		style.Colors[ImGuiCol_HeaderHovered]         = ImVec4(0.00f, 0.40f, 0.00f, 0.80f);
		style.Colors[ImGuiCol_HeaderActive]          = ImVec4(0.00f, 0.50f, 0.00f, 1.00f);
		style.Colors[ImGuiCol_Separator]             = ImVec4(0.80f, 0.00f, 0.00f, 0.50f);
		style.Colors[ImGuiCol_SeparatorHovered]      = ImVec4(0.90f, 0.00f, 0.00f, 0.78f);
		style.Colors[ImGuiCol_SeparatorActive]       = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
		style.Colors[ImGuiCol_ResizeGrip]            = ImVec4(0.80f, 0.60f, 0.00f, 0.25f);
		style.Colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.90f, 0.70f, 0.00f, 0.67f);
		style.Colors[ImGuiCol_ResizeGripActive]      = ImVec4(1.00f, 0.80f, 0.00f, 0.95f);
		style.Colors[ImGuiCol_Tab]                   = ImVec4(0.00f, 0.20f, 0.00f, 0.86f);
		style.Colors[ImGuiCol_TabHovered]            = ImVec4(0.80f, 0.00f, 0.00f, 0.80f);
		style.Colors[ImGuiCol_TabActive]             = ImVec4(0.80f, 0.00f, 0.00f, 1.00f);
		style.Colors[ImGuiCol_TabUnfocused]          = ImVec4(0.00f, 0.15f, 0.00f, 0.97f);
		style.Colors[ImGuiCol_TabUnfocusedActive]    = ImVec4(0.00f, 0.30f, 0.00f, 1.00f);
		style.Colors[ImGuiCol_PlotLines]             = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
		style.Colors[ImGuiCol_PlotLinesHovered]      = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
		style.Colors[ImGuiCol_PlotHistogram]         = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
		style.Colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
		style.Colors[ImGuiCol_TextSelectedBg]        = ImVec4(0.80f, 0.00f, 0.00f, 0.35f);
		style.Colors[ImGuiCol_DragDropTarget]        = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
		style.Colors[ImGuiCol_NavHighlight]          = ImVec4(0.80f, 0.00f, 0.00f, 1.00f);
		style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
		style.Colors[ImGuiCol_NavWindowingDimBg]     = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
		style.Colors[ImGuiCol_ModalWindowDimBg]      = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
		style.GrabRounding = style.FrameRounding = style.ChildRounding = style.WindowRounding = 8.0f;
		save_default_style();
	}
	void gui::dx_on_tick()
	{
		if (m_is_open)
		{
			push_theme_colors();
			view::root(); // frame bg
			pop_theme_colors();
		}
	}

	void gui::save_default_style()
	{
		memcpy(&m_default_config, &ImGui::GetStyle(), sizeof(ImGuiStyle));
	}

	void gui::restore_default_style()
	{
		memcpy(&ImGui::GetStyle(), &m_default_config, sizeof(ImGuiStyle));
	}

	void gui::push_theme_colors()
	{
		auto button_color = ImGui::ColorConvertU32ToFloat4(g.window.button_color);
		auto button_active_color =
		    ImVec4(button_color.x + 0.33f, button_color.y + 0.33f, button_color.z + 0.33f, button_color.w);
		auto button_hovered_color =
		    ImVec4(button_color.x + 0.15f, button_color.y + 0.15f, button_color.z + 0.15f, button_color.w);
		auto frame_color = ImGui::ColorConvertU32ToFloat4(g.window.frame_color);
		auto frame_hovered_color =
		    ImVec4(frame_color.x + 0.14f, frame_color.y + 0.14f, frame_color.z + 0.14f, button_color.w);
		auto frame_active_color =
		    ImVec4(frame_color.x + 0.30f, frame_color.y + 0.30f, frame_color.z + 0.30f, button_color.w);

		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImGui::ColorConvertU32ToFloat4(g.window.background_color));
		ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(g.window.text_color));
		ImGui::PushStyleColor(ImGuiCol_Button, button_color);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, button_hovered_color);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, button_active_color);
		ImGui::PushStyleColor(ImGuiCol_FrameBg, frame_color);
		ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, frame_hovered_color);
		ImGui::PushStyleColor(ImGuiCol_FrameBgActive, frame_active_color);
	}

	void gui::pop_theme_colors()
	{
		ImGui::PopStyleColor(8);
	}

	void gui::script_on_tick()
	{
		if (g_gui->m_is_open || g_gui->m_override_mouse)
		{
			for (uint8_t i = 0; i <= 6; i++)
				PAD::DISABLE_CONTROL_ACTION(2, i, true);
			PAD::DISABLE_CONTROL_ACTION(2, 106, true);
			PAD::DISABLE_CONTROL_ACTION(2, 329, true);
			PAD::DISABLE_CONTROL_ACTION(2, 330, true);

			PAD::DISABLE_CONTROL_ACTION(2, 14, true);
			PAD::DISABLE_CONTROL_ACTION(2, 15, true);
			PAD::DISABLE_CONTROL_ACTION(2, 16, true);
			PAD::DISABLE_CONTROL_ACTION(2, 17, true);
			PAD::DISABLE_CONTROL_ACTION(2, 24, true);
			PAD::DISABLE_CONTROL_ACTION(2, 69, true);
			PAD::DISABLE_CONTROL_ACTION(2, 70, true);
			PAD::DISABLE_CONTROL_ACTION(2, 84, true);
			PAD::DISABLE_CONTROL_ACTION(2, 85, true);
			PAD::DISABLE_CONTROL_ACTION(2, 99, true);
			PAD::DISABLE_CONTROL_ACTION(2, 92, true);
			PAD::DISABLE_CONTROL_ACTION(2, 100, true);
			PAD::DISABLE_CONTROL_ACTION(2, 114, true);
			PAD::DISABLE_CONTROL_ACTION(2, 115, true);
			PAD::DISABLE_CONTROL_ACTION(2, 121, true);
			PAD::DISABLE_CONTROL_ACTION(2, 142, true);
			PAD::DISABLE_CONTROL_ACTION(2, 241, true);
			PAD::DISABLE_CONTROL_ACTION(2, 261, true);
			PAD::DISABLE_CONTROL_ACTION(2, 257, true);
			PAD::DISABLE_CONTROL_ACTION(2, 262, true);
			PAD::DISABLE_CONTROL_ACTION(2, 331, true);
		}
	}

	void gui::script_func()
	{
		while (true)
		{
			g_gui->script_on_tick();
			script::get_current()->yield();
		}
	}

	void gui::wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
	{
		if (msg == WM_KEYUP && wparam == g.settings.hotkeys.menu_toggle)
		{
			//Persist and restore the cursor position between menu instances.
			static POINT cursor_coords{};
			if (g_gui->m_is_open)
			{
				GetCursorPos(&cursor_coords);
			}
			else if (cursor_coords.x + cursor_coords.y != 0)
			{
				SetCursorPos(cursor_coords.x, cursor_coords.y);
			}

			toggle(g.settings.hotkeys.editing_menu_toggle || !m_is_open);
			if (g.settings.hotkeys.editing_menu_toggle)
				g.settings.hotkeys.editing_menu_toggle = false;
		}
	}

	void gui::toggle_mouse()
	{
		if (m_is_open || g_gui->m_override_mouse)
		{
			ImGui::GetIO().MouseDrawCursor = true;
			ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
		}
		else
		{
			ImGui::GetIO().MouseDrawCursor = false;
			ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouse;
		}
	}
}
