/*
imgui_menu_server.h
Copyright (C) 2021 Moemod Hymei
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#pragma once

#include <string>
#include <functional>

namespace ui {
	void ImGui_Server_Init();
	void ImGui_Server_OnGui();
	void Server_Open();
}
