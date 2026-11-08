/* gogreg.c: replaces nothing in the DOS build. Windows only: the games GOG's installers (GOG
   Galaxy's and the offline ones) recorded in the registry, for sys/gamedir.c's search (first
   written for UW2Decomp's port). Each is a key SOFTWARE\GOG.com\Games\<product id>, in
   the 32-bit view (WOW6432Node) on 64-bit Windows, under HKEY_LOCAL_MACHINE (an install for all
   users) or HKEY_CURRENT_USER, with the game's folder in the value "path" and its title in
   "gameName". A file of its own because windows.h and the port's headers both name things such
   as mouse_event. */
#ifdef _WIN32
#include <string.h>
#include <windows.h>

int port_gog_registry(int (*fn)(const char *id, const char *name, const char *path, void *ctx), void *ctx);
int port_steam_root(char *out, unsigned outsz);

static int reg_str(HKEY k, const char *name, char *out, DWORD outsz)
{
    DWORD type, n = outsz - 1;
    if (RegQueryValueExA(k, name, NULL, &type, (BYTE *)out, &n) != ERROR_SUCCESS || type != REG_SZ) return -1;
    out[n < outsz ? n : outsz - 1] = 0;
    return 0;
}

int port_gog_registry(int (*fn)(const char *id, const char *name, const char *path, void *ctx), void *ctx)
{
    static const struct { HKEY root; const char *key; } keys[] = {
        { HKEY_LOCAL_MACHINE, "SOFTWARE\\WOW6432Node\\GOG.com\\Games" },
        { HKEY_LOCAL_MACHINE, "SOFTWARE\\GOG.com\\Games" },
        { HKEY_CURRENT_USER, "SOFTWARE\\WOW6432Node\\GOG.com\\Games" },
        { HKEY_CURRENT_USER, "SOFTWARE\\GOG.com\\Games" },
    };
    size_t i;
    int r = 0;
    for (i = 0; !r && i < sizeof keys / sizeof keys[0]; i++) {
        HKEY games, g;
        DWORD idx;
        char id[256], path[1024], name[256];
        if (RegOpenKeyExA(keys[i].root, keys[i].key, 0, KEY_READ, &games) != ERROR_SUCCESS) continue;
        for (idx = 0; !r; idx++) {
            DWORD n = sizeof id;
            if (RegEnumKeyExA(games, idx, id, &n, NULL, NULL, NULL, NULL) != ERROR_SUCCESS) break;
            if (RegOpenKeyExA(games, id, 0, KEY_READ, &g) != ERROR_SUCCESS) continue;
            if (reg_str(g, "path", path, sizeof path) == 0) {
                if (reg_str(g, "gameName", name, sizeof name)) name[0] = 0;
                RegCloseKey(g);
                r = fn(id, name, path, ctx);
            } else {
                RegCloseKey(g);
            }
        }
        RegCloseKey(games);
    }
    return r;
}
/* Steam's folder, from the registry: the user's SteamPath (written with forward slashes), else the
   install's InstallPath. 0 with it in out. */
int port_steam_root(char *out, unsigned outsz)
{
    static const struct { HKEY root; const char *key; const char *value; } keys[] = {
        { HKEY_CURRENT_USER, "Software\\Valve\\Steam", "SteamPath" },
        { HKEY_LOCAL_MACHINE, "SOFTWARE\\WOW6432Node\\Valve\\Steam", "InstallPath" },
        { HKEY_LOCAL_MACHINE, "SOFTWARE\\Valve\\Steam", "InstallPath" },
    };
    size_t i;
    for (i = 0; i < sizeof keys / sizeof keys[0]; i++) {
        HKEY k;
        int r;
        if (RegOpenKeyExA(keys[i].root, keys[i].key, 0, KEY_READ, &k) != ERROR_SUCCESS) continue;
        r = reg_str(k, keys[i].value, out, (DWORD)outsz);
        RegCloseKey(k);
        if (r == 0 && out[0]) return 0;
    }
    return -1;
}
#else
typedef int gogreg_unused;      /* ISO C wants a translation unit to declare something */
#endif
