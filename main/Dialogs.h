#pragma once
#include <windows.h>
#include <string>

void ShowEmbed(HWND parent); // afficher le message rentre
void ShowExtract(HWND parent, const std::string& txt); // afficher le message extrait

LRESULT CALLBACK EmbedWndProc(HWND, UINT, WPARAM, LPARAM); //appel a la fonction pour intégre le message a l'image
LRESULT CALLBACK ExtractWndProc(HWND, UINT, WPARAM, LPARAM); //appel a la fonction pour extraire le message de l'image

