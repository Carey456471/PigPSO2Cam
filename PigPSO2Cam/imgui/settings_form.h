#pragma once
#include "d3d9.h"
#include "imgui.h"
#include "imgui_impl_dx9.h"
#include "imgui_impl_win32.h"
#include "imgui_internal.h"
#include <string>
#include <sstream>
#include "../Camera.h"

#include "../PSO2CameraTool.hpp"

typedef HRESULT(__stdcall* pso2hDoLua)(const char* a1);
static pso2hDoLua _executeLua = 0;

bool runLua(std::string a1);

bool initLuaHook();

bool runLuaAsync(std::string a1);

void menu_init(void*, LPDIRECT3DDEVICE9);

static int currTab = 0;

static bool farCullDisabled = false;
static bool farCullObjectsDisabled = false;
static bool nearCullDisabled = false;
static bool freeCamToggle = false;

static DWORD oNearCullBytes = 0x0;
static char bgmbuf[1024];

static bool pushCamera()
{
	return runLuaAsync("Camera.PushActorTest()");
}

static bool adjustZoom() {
	std::stringstream ss;
	ss << "Camera.ActorTrackTestNormal.Offset.Dist = \"" << Camera::cameraBase.Offset.Dist << "\"";
	std::string dist(ss.str());
	runLuaAsync(dist.c_str());
	pushCamera();

	return true;
}
static const char* adjustDistmin(float f) {
	std::stringstream ss;
	ss << "Camera.ActorTrackTestNormal.DistMin = " << f;
	return ss.str().c_str();
}
static const char* adjustDistmax(float f) {
	std::stringstream ss;
	ss << "Camera.ActorTrackTestNormal.DistMax = " << f;
	return ss.str().c_str();
}
static bool adjustFovy() {
	std::stringstream ss;
	ss << "Camera.ActorTrackTestNormal.Fovy = " << Camera::cameraBase.Fovy;
	std::string dist(ss.str());
	runLuaAsync(dist.c_str());
	pushCamera();

	return true;
}

static void draw_menu(bool* status)
{

	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	ImGui::GetIO().MouseDrawCursor = true;

	ImGui::SetNextWindowPos(ImVec2(ImGui::GetWindowSize().x / 2, ImGui::GetWindowSize().y / 2), ImGuiCond_Appearing);
	ImGui::SetNextWindowSize(ImVec2(480.0f, 200.f), ImGuiCond_Appearing);

	ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar;

	ImGui::Begin("PIG PSO2", status, window_flags);

	
	switch (currTab) {
	case 0:
	{
		ImGui::BeginGroup();

		{
			ImGui::Columns(2);

			ImGui::Text(("Zoom "));
			ImGui::SameLine(50.0, ImGui::GetStyle().ItemSpacing.y);
			ImGui::SliderInt(("##zoomslider"), &Camera::cameraBase.Offset.Dist, 6, 25);
			ImGui::Text(("Fovy "));
			ImGui::SameLine(50.0, ImGui::GetStyle().ItemSpacing.y);
			ImGui::SliderInt(("##fovslider"), &Camera::cameraBase.Fovy, 40, 90);

			ImGui::NextColumn();
			if (ImGui::Button("Set Zoom", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
				adjustZoom();
			}
			if (ImGui::Button("Set FOV", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
				adjustFovy();
			}

			ImGui::EndColumns();

			ImGui::Separator();

			ImGui::Columns(2);
			ImGui::Text("Culling Patches");
			ImGui::Spacing();
			ImGui::Spacing();

			ImGui::Text("BGM");
			ImGui::SameLine();
			ImGui::InputText("##BGM", bgmbuf, sizeof(bgmbuf));

			ImGui::NextColumn();
			if (ImGui::Checkbox("##cullcheck", &farCullDisabled))
			{
				BYTE b = 0x76; //original conditional jmp byte
				if (farCullDisabled)
					b = 0xEB;	//unconditional jmp

				uintptr_t tFarCull = getTerrainFarCullAddy();
				if (tFarCull)
					*(BYTE*)(tFarCull) = b;

				//if (ImGui::Checkbox("##farcullcheckobjects", &farCullObjectsDisabled))

				uintptr_t tFarCullObj = getObjectFarCullAddy();
				if (tFarCullObj)
				{
					BYTE c = 0x0F;
					if (farCullDisabled)
					{
						c = 0xE9;

						*(BYTE*)(tFarCullObj) = 0x90;
						*(BYTE*)(tFarCullObj + 1) = c;
					}
					else
					{
						*(BYTE*)(tFarCullObj) = b;
						*(BYTE*)(tFarCullObj + 1) = 0x84;
					}
				}

				//if (ImGui::Checkbox("##nearcullcheck", &nearCullDisabled))

				uintptr_t tNearCull = getCameraNearCullAddy();
				if (tNearCull) {

					if (oNearCullBytes == 0x0)
						oNearCullBytes = *(DWORD*)(tNearCull);

					//meme
					if (farCullDisabled)
						*(DWORD*)(tNearCull) = (DWORD)(0x90909090);
					else
						*(DWORD*)(tNearCull) = (DWORD)(oNearCullBytes);
				}
			}
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);//tism
			if (ImGui::Button("Play BGM", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
				std::string s("Skit.Sound.BGM.Play('"+std::string(bgmbuf)+"')");
				runLuaAsync(s);
			}
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);
			if (ImGui::Button("Get BGM", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
				std::string s("Window.ShowError(Skit.Sound.BGM.Get('" + std::string(bgmbuf) + "'))");
				runLuaAsync(s);
			}
			ImGui::EndColumns();

			
		}
		ImGui::EndGroup();
		break;
		}
	}

	std::string pig("pig");
	ImVec2 txt = ImGui::CalcTextSize(pig.c_str());
	ImGui::SetCursorPosY(ImGui::GetWindowSize().y * .87f);
	//ImGui::SetCursorPosY(ImGui::GetContentRegionAvail().y * 0.94f);
	ImGui::Separator();

	ImGui::SetCursorPosX(ImGui::GetContentRegionAvailWidth() * 0.49f);
	ImGui::TextColored(ImVec4(0.95f, .36f, .03f, 0.95f), pig.c_str());

	ImGui::End();

	ImGui::EndFrame();
	ImGui::Render();
	ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());

}