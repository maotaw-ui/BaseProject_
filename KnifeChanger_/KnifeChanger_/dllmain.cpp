#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <Psapi.h>

#include <atomic>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>
#include <algorithm>

#pragma comment(lib, "Psapi.lib")

namespace off {
	constexpr std::ptrdiff_t dwEntityList = 0x2554050;
	constexpr std::ptrdiff_t dwLocalPlayerPawn = 0x23A9118;

	constexpr std::ptrdiff_t m_pWeaponServices = 0x1208;
	constexpr std::ptrdiff_t m_hActiveWeapon = 0x60;

	constexpr std::ptrdiff_t m_nSubclassID = 0x380;

	constexpr std::ptrdiff_t m_AttributeManager = 0x11A8;
	constexpr std::ptrdiff_t m_Item = 0x50;

	constexpr std::ptrdiff_t m_iItemDefinitionIndex = 0x1BA;
	constexpr std::ptrdiff_t m_iEntityQuality = 0x1BC;
	constexpr std::ptrdiff_t m_iItemIDHigh = 0x1D0;
	constexpr std::ptrdiff_t m_iItemIDLow = 0x1D4;
	constexpr std::ptrdiff_t m_bInitialized = 0x1E8;

	constexpr std::ptrdiff_t m_OriginalOwnerXuidLow = 0x1678;
	constexpr std::ptrdiff_t m_OriginalOwnerXuidHigh = 0x167C;
	constexpr std::ptrdiff_t m_nFallbackPaintKit = 0x1680;
	constexpr std::ptrdiff_t m_nFallbackSeed = 0x1684;
	constexpr std::ptrdiff_t m_flFallbackWear = 0x1688;
	constexpr std::ptrdiff_t m_nFallbackStatTrak = 0x168C;
}

namespace sig {
	constexpr std::string_view SetModel =
		"40 53 48 83 EC 20 48 8B D9 4C 8B C2 48 8B 0D 9D";
	constexpr std::string_view UpdateSubClass =
		"4C 8B DC 53 48 81 EC 90 01 00 00 48 8B 41 10 48";
	constexpr std::string_view ApplyEconCustomization =
		"48 89 5C 24 08 57 48 83 EC 20 8B FA 48 8B D9 E8 ? ? ? ? 48 8B CB E8 ? ? ? ? 48 85 C0 74";
	constexpr std::string_view RegenerateWeaponSkin =
		"40 55 53 41 57 48 8D AC 24 00 FE FF FF 48 81 EC";
}

using SetModelFn = void(__fastcall*)(void*, const char*);
using UpdateSubClassFn = void(__fastcall*)(void*);
using ApplyEconFn = std::int64_t(__fastcall*)(void*, char);
using RegenSkinFn = void(__fastcall*)(void*, char);

static HMODULE g_module = nullptr;
static std::atomic_bool g_running{ true };
static std::atomic_int g_selectedKnife{ 5 }; // 0-based: 5 = M9 Bayonet
static std::uintptr_t g_lastWeaponEntity = 0;
static std::atomic_uint32_t g_selectionRevision{ 1 };
static std::uint32_t g_lastAppliedRevision = 0;
static ULONGLONG g_lastF11Tick = 0;
static std::atomic_uint32_t g_itemSerial{ 0x60000000u };

static SetModelFn g_SetModel = nullptr;
static UpdateSubClassFn g_UpdateSubClass = nullptr;
static ApplyEconFn g_ApplyEcon = nullptr;
static RegenSkinFn g_RegenSkin = nullptr;

struct KnifeEntry {
	std::uint16_t def;
	int paint;
	const char* name;
	const char* model;
	const char* subclassName;
};

static constexpr std::array<KnifeEntry, 20> kKnives{ {
	{500, 558,  "Bayonet",         "weapons/models/knife/knife_bayonet/weapon_knife_bayonet.vmdl",       "weapon_knife_bayonet"},
	{503, 38,    "Classic Knife",   "weapons/models/knife/knife_css/weapon_knife_css.vmdl",               "weapon_knife_css"},
	{505, 559,  "Flip Knife",      "weapons/models/knife/knife_flip/weapon_knife_flip.vmdl",             "weapon_knife_flip"},
	{506, 560,  "Gut Knife",       "weapons/models/knife/knife_gut/weapon_knife_gut.vmdl",               "weapon_knife_gut"},
	{507, 561,  "Karambit",        "weapons/models/knife/knife_karambit/weapon_knife_karambit.vmdl",     "weapon_knife_karambit"},
	{508, 562,  "M9 Bayonet",      "weapons/models/knife/knife_m9/weapon_knife_m9.vmdl",                 "weapon_knife_m9_bayonet"},
	{509, 1107, "Huntsman Knife",  "weapons/models/knife/knife_tactical/weapon_knife_tactical.vmdl",     "weapon_knife_tactical"},
	{512, 1106, "Falchion Knife",  "weapons/models/knife/knife_falchion/weapon_knife_falchion.vmdl",     "weapon_knife_falchion"},
	{514, 1104, "Bowie Knife",     "weapons/models/knife/knife_bowie/weapon_knife_bowie.vmdl",           "weapon_knife_survival_bowie"},
	{515, 1105, "Butterfly Knife", "weapons/models/knife/knife_butterfly/weapon_knife_butterfly.vmdl",   "weapon_knife_butterfly"},
	{516, 1108, "Shadow Daggers",  "weapons/models/knife/knife_push/weapon_knife_push.vmdl",             "weapon_knife_push"},
	{517, 38,    "Paracord Knife",  "weapons/models/knife/knife_cord/weapon_knife_cord.vmdl",             "weapon_knife_cord"},
	{518, 38,    "Survival Knife",  "weapons/models/knife/knife_canis/weapon_knife_canis.vmdl",           "weapon_knife_canis"},
	{519, 38,    "Ursus Knife",     "weapons/models/knife/knife_ursus/weapon_knife_ursus.vmdl",           "weapon_knife_ursus"},
	{520, 38,    "Navaja Knife",    "weapons/models/knife/knife_navaja/weapon_knife_navaja.vmdl",         "weapon_knife_gypsy_jackknife"},
	{521, 38,    "Nomad Knife",     "weapons/models/knife/knife_outdoor/weapon_knife_outdoor.vmdl",       "weapon_knife_outdoor"},
	{522, 38,    "Stiletto Knife",  "weapons/models/knife/knife_stiletto/weapon_knife_stiletto.vmdl",     "weapon_knife_stiletto"},
	{523, 38,    "Talon Knife",     "weapons/models/knife/knife_talon/weapon_knife_talon.vmdl",           "weapon_knife_widowmaker"},
	{525, 38,    "Skeleton Knife",  "weapons/models/knife/knife_skeleton/weapon_knife_skeleton.vmdl",     "weapon_knife_skeleton"},
	{526, 38,    "Kukri Knife",     "weapons/models/knife/knife_kukri/weapon_knife_kukri.vmdl",           "weapon_knife_kukri"},
} };

template <typename T>
static bool Read(std::uintptr_t address, T& out) {
	if (!address) return false;
	__try {
		out = *reinterpret_cast<T*>(address);
		return true;
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		out = T{};
		return false;
	}
}

template <typename T>
static bool Write(std::uintptr_t address, const T& value) {
	if (!address) return false;
	__try {
		*reinterpret_cast<T*>(address) = value;
		return true;
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		return false;
	}
}

static std::vector<int> PatternToBytes(std::string_view pattern) {
	std::vector<int> bytes;
	for (std::size_t i = 0; i < pattern.size();) {
		while (i < pattern.size() && pattern[i] == ' ') ++i;
		if (i >= pattern.size()) break;
		if (pattern[i] == '?') {
			++i;
			if (i < pattern.size() && pattern[i] == '?') ++i;
			bytes.push_back(-1);
			continue;
		}
		if (i + 1 >= pattern.size()) break;
		char temp[3] = { pattern[i], pattern[i + 1], 0 };
		bytes.push_back(static_cast<int>(std::strtoul(temp, nullptr, 16)));
		i += 2;
	}
	return bytes;
}

static std::uintptr_t PatternScan(HMODULE module, std::string_view pattern) {
	if (!module) return 0;
	auto* dos = reinterpret_cast<PIMAGE_DOS_HEADER>(module);
	if (dos->e_magic != IMAGE_DOS_SIGNATURE) return 0;
	auto* nt = reinterpret_cast<PIMAGE_NT_HEADERS>(reinterpret_cast<std::uint8_t*>(module) + dos->e_lfanew);
	if (nt->Signature != IMAGE_NT_SIGNATURE) return 0;

	const auto bytes = PatternToBytes(pattern);
	if (bytes.empty()) return 0;

	auto* sections = IMAGE_FIRST_SECTION(nt);
	for (WORD s = 0; s < nt->FileHeader.NumberOfSections; ++s) {
		const auto& sec = sections[s];
		if ((sec.Characteristics & IMAGE_SCN_MEM_EXECUTE) == 0) continue;
		auto* start = reinterpret_cast<std::uint8_t*>(module) + sec.VirtualAddress;
		const std::size_t size = sec.Misc.VirtualSize;
		if (size < bytes.size()) continue;

		for (std::size_t i = 0; i <= size - bytes.size(); ++i) {
			bool found = true;
			for (std::size_t j = 0; j < bytes.size(); ++j) {
				if (bytes[j] != -1 && start[i + j] != static_cast<std::uint8_t>(bytes[j])) {
					found = false;
					break;
				}
			}
			if (found) return reinterpret_cast<std::uintptr_t>(start + i);
		}
	}
	return 0;
}

// Source-2 CUtlStringToken-style lower-case MurmurHash2 helper.
static std::uint32_t Murmur2Lower(std::string_view text) {
	std::string lower(text);
	std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) {
		return static_cast<char>((c >= 'A' && c <= 'Z') ? (c + ('a' - 'A')) : c);
		});

	constexpr std::uint32_t m = 0x5bd1e995u;
	constexpr int r = 24;
	std::uint32_t h = 0x31415926u ^ static_cast<std::uint32_t>(lower.size());
	const auto* data = reinterpret_cast<const std::uint8_t*>(lower.data());
	std::size_t len = lower.size();

	while (len >= 4) {
		std::uint32_t k;
		std::memcpy(&k, data, sizeof(k));
		k *= m;
		k ^= k >> r;
		k *= m;
		h *= m;
		h ^= k;
		data += 4;
		len -= 4;
	}

	switch (len) {
	case 3: h ^= static_cast<std::uint32_t>(data[2]) << 16; [[fallthrough]];
	case 2: h ^= static_cast<std::uint32_t>(data[1]) << 8; [[fallthrough]];
	case 1: h ^= static_cast<std::uint32_t>(data[0]); h *= m; break;
	default: break;
	}

	h ^= h >> 13;
	h *= m;
	h ^= h >> 15;
	return h;
}

static std::uint32_t KnifeSubclass(const KnifeEntry& knife) {
	const std::string defText = std::to_string(knife.def);
	return Murmur2Lower(defText);
}

static bool IsKnifeDefinition(std::uint16_t def) {
	if (def == 42 || def == 59) return true;
	switch (def) {
	case 500: case 503: case 505: case 506: case 507: case 508:
	case 509: case 512: case 514: case 515: case 516: case 517:
	case 518: case 519: case 520: case 521: case 522: case 523:
	case 525: case 526:
		return true;
	default:
		return false;
	}
}

static std::uintptr_t ResolveEntity(std::uintptr_t entityList, std::uint32_t handle) {
	if (!entityList || handle == 0 || handle == 0xFFFFFFFFu) return 0;
	const std::uint32_t index = handle & 0x7FFFu;
	std::uintptr_t listEntry = 0;
	if (!Read(entityList + 0x10ull + 0x8ull * (index >> 9), listEntry) || !listEntry)
		return 0;

	std::uintptr_t entity = 0;
	if (!Read(listEntry + 0x70ull * (index & 0x1FFu), entity))
		return 0;
	return entity;
}

static bool ResolveFunctions(HMODULE client) {
	g_SetModel = reinterpret_cast<SetModelFn>(PatternScan(client, sig::SetModel));
	g_UpdateSubClass = reinterpret_cast<UpdateSubClassFn>(PatternScan(client, sig::UpdateSubClass));
	g_ApplyEcon = reinterpret_cast<ApplyEconFn>(PatternScan(client, sig::ApplyEconCustomization));
	g_RegenSkin = reinterpret_cast<RegenSkinFn>(PatternScan(client, sig::RegenerateWeaponSkin));
	//std::cout << "[resolver] UpdateSubClass         = " << reinterpret_cast<void*>(g_UpdateSubClass) << '\n';
	//std::cout << "[resolver] RegenerateWeaponSkin   = " << reinterpret_cast<void*>(g_RegenSkin) << '\n';

	return g_SetModel && g_UpdateSubClass && g_ApplyEcon && g_RegenSkin;
}

static bool ApplySelectedKnife(std::uintptr_t clientBase) {
	std::uintptr_t localPawn = 0;
	if (!Read(clientBase + off::dwLocalPlayerPawn, localPawn) || !localPawn)
		return false;

	std::uintptr_t entityList = 0;
	if (!Read(clientBase + off::dwEntityList, entityList) || !entityList)
		return false;

	std::uintptr_t weaponServices = 0;
	if (!Read(localPawn + off::m_pWeaponServices, weaponServices) || !weaponServices)
		return false;

	std::uint32_t activeHandle = 0;
	if (!Read(weaponServices + off::m_hActiveWeapon, activeHandle))
		return false;

	const std::uintptr_t weapon = ResolveEntity(entityList, activeHandle);
	if (!weapon) return false;
	const auto revision =
		g_selectionRevision.load(std::memory_order_relaxed);

	if (g_lastWeaponEntity == weapon &&
		g_lastAppliedRevision == revision) {
		return true;
	}
	const KnifeEntry& selected = kKnives[static_cast<std::size_t>(g_selectedKnife.load(std::memory_order_relaxed))];
	const std::uintptr_t item = weapon + off::m_AttributeManager + off::m_Item;

	std::uint16_t currentDef = 0;
	if (!Read(item + off::m_iItemDefinitionIndex, currentDef) || !IsKnifeDefinition(currentDef))
		return false;

	const bool falseValue = false;
	const bool trueValue = true;
	const std::uint32_t idHigh = 0xFFFFFFFFu;
	const std::uint32_t idLow = g_itemSerial.fetch_add(1, std::memory_order_relaxed) + 1;
	const std::int32_t quality = 3;
	const std::uint32_t zero32 = 0;
	const std::int32_t seed = 0;
	const float wear = 0.001f;
	const std::int32_t stattrak = -1;
	const std::uint32_t subclass = KnifeSubclass(selected);

	if (!Write(item + off::m_bInitialized, falseValue)) return false;
	Write(item + off::m_iItemIDHigh, idHigh);
	Write(item + off::m_iItemIDLow, idLow);

	Write(weapon + off::m_nSubclassID, subclass);
	Write(item + off::m_iItemDefinitionIndex, selected.def);
	Write(item + off::m_iEntityQuality, quality);

	Write(weapon + off::m_OriginalOwnerXuidLow, zero32);
	Write(weapon + off::m_OriginalOwnerXuidHigh, zero32);
	Write(weapon + off::m_nFallbackPaintKit, selected.paint);
	Write(weapon + off::m_nFallbackSeed, seed);
	Write(weapon + off::m_flFallbackWear, wear);
	Write(weapon + off::m_nFallbackStatTrak, stattrak);

	__try {
		// Apply identity/model once.
		g_UpdateSubClass(reinterpret_cast<void*>(weapon));
		g_SetModel(reinterpret_cast<void*>(weapon), selected.model);

		// One econ/material refresh only. The previous build called this
		// sequence twice back-to-back on every F11 switch.
		g_ApplyEcon(reinterpret_cast<void*>(weapon), 1);

		if (g_RegenSkin)
			g_RegenSkin(reinterpret_cast<void*>(weapon), 1);

		Write(item + off::m_bInitialized, trueValue);

		// Final model set only; do not regenerate a second time immediately.
		g_SetModel(reinterpret_cast<void*>(weapon), selected.model);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		Write(item + off::m_bInitialized, trueValue);
		return false;
	}

	g_lastWeaponEntity = weapon;
	g_lastAppliedRevision = revision;
	return true;
}


static DWORD WINAPI MainThread(void*) {
	HMODULE client = nullptr;

	while (g_running.load(std::memory_order_relaxed) && !client) {
		client = GetModuleHandleA("client.dll");
		Sleep(100);
	}

	if (!client)
		FreeLibraryAndExitThread(g_module, 0);

	const auto clientBase = reinterpret_cast<std::uintptr_t>(client);

	ResolveFunctions(client);

	while (g_running.load(std::memory_order_relaxed)) {
		if (GetAsyncKeyState(VK_END) & 1) {
			g_running.store(false, std::memory_order_relaxed);
			break;
		}

		// F11 = next knife model. Add a cooldown so a single physical press
		// cannot cause rapid back-to-back model/subclass changes.
		if (GetAsyncKeyState(VK_F11) & 1) {
			const ULONGLONG now = GetTickCount64();

			if (!g_lastF11Tick || (now - g_lastF11Tick) >= 350) {
				int next =
					g_selectedKnife.load(std::memory_order_relaxed) + 1;

				if (next >= static_cast<int>(kKnives.size()))
					next = 0;

				g_selectedKnife.store(next, std::memory_order_relaxed);
				g_selectionRevision.fetch_add(1, std::memory_order_relaxed);

				g_lastWeaponEntity = 0;
				g_lastAppliedRevision = 0;
				g_lastF11Tick = now;
			}
		}

		if (g_SetModel && g_UpdateSubClass && g_ApplyEcon && g_RegenSkin)
			ApplySelectedKnife(clientBase);

		Sleep(50);
	}

	FreeLibraryAndExitThread(g_module, 0);
	return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
	if (reason == DLL_PROCESS_ATTACH) {
		g_module = hModule;
		DisableThreadLibraryCalls(hModule);
		if (HANDLE thread = CreateThread(nullptr, 0, MainThread, nullptr, 0, nullptr))
			CloseHandle(thread);
	}
	return TRUE;
}