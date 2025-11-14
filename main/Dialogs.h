#pragma once
#include <windows.h>
#include <string>

void ShowEmbed(HWND parent);
void ShowExtract(HWND parent, const std::string& txt);

LRESULT CALLBACK EmbedWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK ExtractWndProc(HWND, UINT, WPARAM, LPARAM);

