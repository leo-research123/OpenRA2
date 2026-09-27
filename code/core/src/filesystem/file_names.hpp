#pragma once
namespace game {
// Names use YRMemory's CRT domain, including names released by original hooks.
char* DuplicateName(const char* name);
void FreeName(char* name);
// Host ASCII conversion; original targets retain the original CRT locale rules.
void UppercaseName(char* name);
}
