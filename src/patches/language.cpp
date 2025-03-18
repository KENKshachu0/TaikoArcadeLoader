#include "constants.h"
#include "helpers.h"
#include "patches.h"
#include <map>


extern GameVersion gameVersion;

namespace patches::Language {
const std::vector<std::string> languages    = { "jpn", "en_us", "cn_tw", "kor", "cn_cn" };
std::vector<SafetyHookMid> language_patch   = { };
boolean cnFontPatches                       = false;
// TestMode::Value *languageItem               = TestMode::CreateValue (L"LanguageItem");
int language                                = 0;

FUNCTION_PTR (void,         lua_settop,     PROC_ADDRESS ("lua51.dll", "lua_settop"), i64, i32);
FUNCTION_PTR (void,         lua_replace,    PROC_ADDRESS ("lua51.dll", "lua_replace"), i64, i32);
FUNCTION_PTR (const char *, lua_pushstring, PROC_ADDRESS ("lua51.dll", "lua_pushstring"), i64, const char *);
FUNCTION_PTR (const char *, lua_tolstring,  PROC_ADDRESS ("lua51.dll", "lua_tolstring"), u64, i32, size_t *);
FUNCTION_PTR (void,         lua_pushnumber, PROC_ADDRESS ("lua51.dll", "lua_pushnumber"), i64, double);

typedef void *(*Taiko_string) (void *address, const char *src);
typedef void *(*Taiko_assign) (void *address, const char *src, size_t size);
typedef int   (*LanguageFix)  (int language);

Taiko_string taiko_string = nullptr;
Taiko_assign taiko_assign = nullptr;
Taiko_assign std_assign   = nullptr;
Taiko_assign std_append   = nullptr;
LanguageFix  languageFix  = [](int lang){ return lang; };

namespace Font {
typedef u64 *(*Taiko_mapping) (u64 *a1, u8 *a2);
Taiko_mapping taiko_mapping = nullptr;

typedef struct {
    const char *font_name;
    const char *font_shape;
    const char *font_file;
} LanguageItem;

void *languageData = nullptr;
const size_t languageOffset = languages.size () * 560;
//                             jp_64,  jp_30,     jp_32,     en_64,      en_30,     en_32,    tw_64,  tw_30,    tw_32,      ko_64,  ko_30,    ko_32,     cn_64,       cn_30, cn_32
const float upA[15]   = {      -1.0f,   0.0f,      0.0f,      0.0f,       0.0f,      0.0f,     0.0f,   0.0f,     0.0f,       0.0f,   0.0f,     0.0f,      0.0f,        0.0f,  0.0f };
const float upB[15]   = { -0.265625f,  0.15f, 0.078125f, -0.21875f, 0.2329999f, 0.140625f, -0.0625f, 0.333f, 0.28125f, -0.109375f, 0.333f, 0.28125f, -0.09375f, 0.30000001f, 0.25f };
const float downA[15] = {     -0.25f, 0.167f,    0.125f, -0.21875f,     0.233f, 0.140625f, -0.0625f, 0.333f,    0.25f,    -0.125f, 0.333f, 0.28125f, -0.09375f, 0.30000001f, 0.25f };
const float downB[15] = {      50.5f,  10.0f,     13.0f,     46.0f,       8.0f,     11.5f,    36.0f,   5.0f,     7.0f,      39.0f,   5.0f,     7.0f,     38.0f,        6.0f,  8.0f };
//                             xx_64,  00_30,     xx_32
const float downC[3]  = {      64.0f,  30.0f,     32.0f };
const float shapes[3] = {      20.0f,  12.0f,     16.0f };
const LanguageItem languageItems[5][5] = {
    {{"jp", "64", "jp_64"},{"jp", "30", "jp_30"},{"jp", "32_EB", "jp_32_EB"},{"jp", "32_B", "jp_32_B"},{"jp", "32_DB", "jp_32_DB"}},
    {{"en", "64", "en_64"},{"en", "30", "en_30"},{"en", "32_EB", "en_32"   },{"en", "32_B", "en_32"  },{"en", "32_DB", "en_32"   }},
    {{"tw", "64", "tw_64"},{"tw", "30", "tw_30"},{"tw", "32_EB", "tw_32"   },{"tw", "32_B", "tw_32"  },{"tw", "32_DB", "tw_32"   }},
    {{"kr", "64", "kr_64"},{"kr", "30", "kr_30"},{"kr", "32_EB", "kr_32"   },{"kr", "32_B", "kr_32"  },{"kr", "32_DB", "kr_32"   }},
    {{"cn", "64", "cn_64"},{"cn", "30", "cn_30"},{"cn", "32_EB", "cn_32_EB"},{"cn", "32_B", "cn_32_B"},{"cn", "32_DB", "cn_32_DB"}},
};

FAST_HOOK_DYNAMIC (int, initial_load_setting) {
    for (int repeat = 0; repeat < languages.size (); repeat ++) {
        u64 offsetR = repeat * languages.size () * 0x5 * 0x70;
        for (int lang = 0; lang < languages.size (); lang ++) {
            u64 offsetI = lang * 0x5 * 0x70;
            const LanguageItem *group = languageItems[lang];
            for (int shape = 0; shape < 0x5; shape ++) {
                LanguageItem item = group[shape];
                u64 basicOffset = (u64)languageData + offsetR + offsetI + shape * 0x70;
                u32 shapeIndex = shape > 2 ? 2 : shape;

                std::string *str = new std::string (item.font_name);
                memcpy ((void *)(basicOffset + 0x00), str, 0x20);
                str = new std::string (item.font_shape);
                memcpy ((void *)(basicOffset + 0x20), str, 0x20);
                str = new std::string (item.font_file);
                memcpy ((void *)(basicOffset + 0x40), str, 0x20);
                *(float *)(basicOffset + 0x60) = shapes[shapeIndex];
                *(u32 *)(basicOffset + 0x64)   = lang == 0 ? 0 : shapeIndex + 1;
                *(u32 *)(basicOffset + 0x68)   = lang == 0 ? 0 : (3 * lang + shapeIndex + 1);
            }
        }
    }
    return 0;
}

FAST_HOOK_DYNAMIC (double, GetCorrectionOffsetY, u64 a1, u32 a2) {
    if (!a2) return 0.0;
    double result;
    u8 type = *(u8 *)(a1 + 572);
    float varA = *(float *)(a1 + 532);
    if (type) {
        *(float *)&result = upA[a2 - 1] + upB[a2 - 1] * varA;
    } else {
        float varB = *(float *)(a1 + 564);
        float varC = downC[(a2 - 1) % 3];
        float val = (varC - varA) * downA[a2 - 1] + (varB - varC) * 0.5f + downB[a2 - 1] - varB * 0.5f;
        *(u64 *)&result = *(u64 *)&val ^ 0x8000000080000000ui64;
    }
    return result;
}

void
Init () {
    bool fontExistAll = true;
    const char *fontToCheck[]{
        "cn_30.nutexb", "cn_30.xml", "cn_32_EB.nutexb", "cn_32_EB.xml", "cn_32_B.nutexb",
        "cn_32_B.xml", "cn_32_DB.nutexb", "cn_32_DB.xml", "cn_64.nutexb", "cn_64.xml"
    };
    for (int i = 0; i < 6; i++) {
        if (std::filesystem::exists (std::string ("..\\..\\Data\\x64\\font\\") + fontToCheck[i])) continue;
        fontExistAll = false;
    }
    if (fontExistAll == false) return;
    LogMessage (LogLevel::INFO, "Using Font patches (Detected cn_xx files)");
    languageData = calloc (languages.size () * languages.size () * 0x70 * 0x5, sizeof (u8));

    switch (gameVersion) {
    default: break;
    case GameVersion::JPN00: break;
    case GameVersion::JPN08: break;
    case GameVersion::JPN39: {
        taiko_mapping = (Taiko_mapping)ASLR (0x140369B60);
        TestMode::RegisterModify (
            L"/root/menu[@id='OthersMenu']/layout[@type='Center']/select-item[@id='LanguageItem']",
            [](pugi::xml_node &node) {
                node.attribute (L"max").set_value (L"4");
                node.attribute (L"replace-text").set_value (L"0:JPN, 1:ENG, 2:zh-tw, 3:KOR, 4:zh-cn");
            }, [&](){
                // load_setting
                INSTALL_FAST_HOOK_DYNAMIC (initial_load_setting, ASLR (0x14001B010));
                // SceneBoot::LoadFont
                language_patch.push_back (safetyhook::create_mid (ASLR (0x140464235), [](SafetyHookContext &ctx){
                    ctx.rbp = ctx.rbx * languageOffset;
                    ctx.rax = (uintptr_t)languageData;
                    ctx.rip = ASLR (0x140464243);
                }));
                language_patch.push_back (safetyhook::create_mid (ASLR (0x14046426C), [](SafetyHookContext &ctx){
                    ctx.r15 = ctx.rbp + languageOffset;
                    ctx.rip = ASLR (0x140464273);
                }));
                // SceneBoot::SetFontShift
                language_patch.push_back (safetyhook::create_mid (ASLR (0x1404644AF), [](SafetyHookContext &ctx){
                    ctx.rdi = ctx.rcx * languageOffset;
                    ctx.rax = (uintptr_t)languageData;
                    ctx.rdi += ctx.rax;
                    ctx.rsi = ctx.rdi + languageOffset;
                    ctx.rip = ASLR (0x1404644C7);
                }));
                // AppLumenRenderer
                language_patch.push_back (safetyhook::create_mid (ASLR (0x140368961), [](SafetyHookContext &ctx){
                    *(u64 *)(ctx.rsp + 0x20) = 4;
                    void *buf = taiko_mapping ((u64 *)(ctx.r14), (u8 *)(ctx.rsp + 0x20));
                    taiko_assign (buf, "cn", 2);
                }));
                // CorrectionOffsetY
                INSTALL_FAST_HOOK_DYNAMIC (GetCorrectionOffsetY, ASLR (0x14049FAD0));
                // user_name_entry
                language_patch.push_back (safetyhook::create_mid (ASLR (0x14016128D), [](SafetyHookContext &ctx)
                { *(int *)(ctx.rsp + 0xD0) = (int)(ctx.rcx % languages.size ()); ctx.rip = ASLR (0x1401612C7); }));
                // user_name_p
                language_patch.push_back (safetyhook::create_mid (ASLR (0x140160F63), [](SafetyHookContext &ctx)
                { *(int *)(ctx.rsp + 0xD0) = (int)(ctx.rdi % languages.size ()); ctx.rip = ASLR (0x140160F9D); }));
                // user_name_ai
                language_patch.push_back (safetyhook::create_mid (ASLR (0x14016146D), [](SafetyHookContext &ctx)
                { *(int *)(ctx.rsp + 0xD0) = (int)(ctx.rcx % languages.size ()); ctx.rip = ASLR (0x1401614A7); }));
                // user_name_soshina
                language_patch.push_back (safetyhook::create_mid (ASLR (0x14016164D), [](SafetyHookContext &ctx)
                { *(int *)(ctx.rsp + 0xD0) = (int)(ctx.rcx % languages.size ()); ctx.rip = ASLR (0x140161687); }));
                cnFontPatches = true;
            }
        );
    } break;
    case GameVersion::CHN00: break;
    }
}
}

namespace Datatable {
const std::vector<std::string> languageTexts = {"japaneseText", "englishUsText", "chineseTText", "koreanText", "chineseSText"};
const std::vector<std::string> languageFontTypes = {"japaneseFontType", "englishUsFontType", "chineseTFontType", "koreanFontType", "chineseSFontType"};
const std::vector<std::string> languageFlas = {"taikoSelectFla", "taikoSelectFlaENUS", "taikoSelectFlaCNTW", "taikoSelectFlaKOR", "taikoSelectFlaZHCN"};

typedef bool (*JsonHasMember) (u64, const char *);
typedef u64 *(*JsonGetMember) (u64, const char *);
JsonHasMember jsonHasMember = nullptr;
JsonGetMember jsonGetMember = nullptr;

uintptr_t WordlistLanguageTypeRIP = 0;
MID_HOOK_DYNAMIC (WordlistLanguageType, SafetyHookContext &ctx) {
    int language = (int)ctx.r12, offset = (int)ctx.rdi;
    u64 wordlist = ctx.rbx;

    int fallback = -1;
    language = language > 0 && language < languages.size () ? language : 0;
    u64 json = 0xFFFFFFFFFFFFi64 & wordlist + offset;
    if (jsonHasMember (json, languageTexts[language].c_str ())) {
        u64 *data = jsonGetMember (json, languageTexts[language].c_str ());
        if ((*((u16 *)data + 7) & 0x1000) == 0) data = (u64 *)(data[1] & 0xFFFFFFFFFFFF);
        taiko_assign ((void *)(ctx.rsp + 0x60), (char *)data, strlen ((char *)data));
    } else if (language == 4 && jsonHasMember (json, languageTexts[2].c_str ())) {
        fallback = 2;
        u64 *data = jsonGetMember (json, languageTexts[2].c_str ());
        if ((*((u16 *)data + 7) & 0x1000) == 0) data = (u64 *)(data[1] & 0xFFFFFFFFFFFF);
        taiko_assign ((void *)(ctx.rsp + 0x60), (char *)data, strlen ((char *)data));
    } else if (language != 0 && jsonHasMember (json, languageTexts[0].c_str ())) {
        fallback = 0;
        u64 *data = jsonGetMember (json, languageTexts[0].c_str ());
        if ((*((u16 *)data + 7) & 0x1000) == 0) data = (u64 *)(data[1] & 0xFFFFFFFFFFFF);
        taiko_assign ((void *)(ctx.rsp + 0x60), (char *)data, strlen ((char *)data));
    } else taiko_assign ((void *)(ctx.rsp + 0x60), "", 0);

    if (fallback != -1) {
        if (jsonHasMember (json, languageFontTypes[fallback].c_str ())) {
            ctx.rcx = *jsonGetMember (json, languageFontTypes[fallback].c_str ());
        } else ctx.rcx = 0;
    } else if (jsonHasMember (json, languageFontTypes[language].c_str ())) {
        ctx.rcx = *jsonGetMember (json, languageFontTypes[language].c_str ());
    } else ctx.rcx = 0;

    ctx.rip = WordlistLanguageTypeRIP;
}

char *languageFla = nullptr;
uintptr_t QrcodeInfoResourceTypeRIP = 0;
MID_HOOK_DYNAMIC (QrcodeInfoResourceType, SafetyHookContext &ctx) {
    languageFla = (char *)(ctx.rsp + 0x130);
    u64 json = ctx.rbx;
    for (int i = 0; i < 4; i ++) {
        int lang = language == 4 ? (i == 2 ? 4 : i) : i;
        if (jsonHasMember (json, languageFlas[lang].c_str ())) {
            u64 *data = jsonGetMember (json, languageFlas[lang].c_str ());
            if ((*((u16 *)data + 7) & 0x1000) == 0) data = (u64 *)(data[1] & 0xFFFFFFFFFFFF);
            std_assign (languageFla + i * 0x20, (char *)data, strlen ((char *)data));
        } else if (language == 4 && jsonHasMember (json, languageFlas[2].c_str ())) {
            u64 *data = jsonGetMember (json, languageFlas[2].c_str ());
            if ((*((u16 *)data + 7) & 0x1000) == 0) data = (u64 *)(data[1] & 0xFFFFFFFFFFFF);
            std_assign (languageFla + i * 0x20, (char *)data, strlen ((char *)data));
        } else if (language != 0 && jsonHasMember (json, languageFlas[0].c_str ())) {
            u64 *data = jsonGetMember (json, languageFlas[0].c_str ());
            if ((*((u16 *)data + 7) & 0x1000) == 0) data = (u64 *)(data[1] & 0xFFFFFFFFFFFF);
            std_assign (languageFla + i * 0x20, (char *)data, strlen ((char *)data));
        } else std_assign (languageFla + i * 0x20, "", 0);
    }
    ctx.rip = QrcodeInfoResourceTypeRIP;
}

std::string CHSGaidenTitle = "";
MID_HOOK_DYNAMIC (GaidenTitleExtren, SafetyHookContext &ctx) {
    LogMessage (LogLevel::DEBUG, "GaidenTitle: {}", (char *)ctx.rcx);
    std::string gaidenTitle = std::string ((char *)ctx.rcx);
    if (gaidenTitle.find ("[CHS]") != std::string::npos) {
        CHSGaidenTitle = gaidenTitle;
    }
}
uintptr_t JumpInjectGaidenTitleRIP = 0;
MID_HOOK_DYNAMIC (GaidenTitleInject, SafetyHookContext &ctx) {
    // only handle ctx.rdx > 2    2 => lang4 => zh-cn   etc.
    if (ctx.rdx == 2) {
        ctx.rdx = (uintptr_t)CHSGaidenTitle.c_str ();
        ctx.r8 = CHSGaidenTitle.size ();
        LogMessage (LogLevel::DEBUG, "GaidenTitleInjectCHS title={} length={}", CHSGaidenTitle, CHSGaidenTitle.size ());
        ctx.rip = JumpInjectGaidenTitleRIP;
    }
}
MID_HOOK_DYNAMIC (GaidenTitleClean, SafetyHookContext &ctx) {
    CHSGaidenTitle = "";
}
uintptr_t JumpInjectGaidenLanguageRIP = 0;
MID_HOOK_DYNAMIC (GaidenLanguageInject, SafetyHookContext &ctx) {
    if (ctx.rbx == 2) {
        *(int *)(ctx.rsp + 0x2D8) = 4;
        ctx.rip = JumpInjectGaidenLanguageRIP;
    }
}

void
Init () {
    switch (gameVersion) {
    default: case GameVersion::JPN00: case GameVersion::JPN08: case GameVersion::CHN00: break;
    case GameVersion::JPN39: {
        jsonHasMember = (JsonHasMember)ASLR (0x1400B5A30);
        jsonGetMember = (JsonGetMember)ASLR (0x1400B6080);
        // App::DataTableHolder<App::WordInfo>::Serialize
        WordlistLanguageTypeRIP     = ASLR (0x1400B20B8);
        INSTALL_MID_HOOK_DYNAMIC (WordlistLanguageType, ASLR (0x1400B1E47));
        // App::DataTableHolder<App::QRCodeInfo>::Serialize
        QrcodeInfoResourceTypeRIP   = ASLR (0x1400B2959);
        INSTALL_MID_HOOK_DYNAMIC (QrcodeInfoResourceType, ASLR (0x1400B276E));
        INSTALL_MID_HOOK_DYNAMIC (GaidenTitleExtren, ASLR (0x1403E2224));
        JumpInjectGaidenTitleRIP    = ASLR (0x1403E2416);
        INSTALL_MID_HOOK_DYNAMIC (GaidenTitleInject, ASLR (0x1403E238F));
        INSTALL_MID_HOOK_DYNAMIC (GaidenTitleClean, ASLR (0x1403E2980));
        JumpInjectGaidenLanguageRIP = ASLR (0x1403E2721);
        INSTALL_MID_HOOK_DYNAMIC (GaidenLanguageInject, ASLR (0x1403E26E2));
    } break;
    }
}
}

namespace Script {
FAST_HOOK_DYNAMIC (i64, RefPlayDataManager, i64 a1) {
    LogMessage (LogLevel::HOOKS, "RefPlayDataManager was called");
    const auto result = originalRefPlayDataManager.fastcall<i64> (a1);
    language          = *reinterpret_cast<u32 *> (result);
    return result;
}
FAST_HOOK_DYNAMIC (i64, GetLanguage, u64 *a1, i64 a2) {
    LogMessage (LogLevel::HOOKS, "GetLanguage was called");
    lua_settop (a2, 0);
    int langFix = languageFix (language);
    const char* type = languageStr (langFix);
    lua_pushstring (a2, type);
    return 1;
}
FAST_HOOK_DYNAMIC (i64, GetRegionLanguage, i64 a1) {
    LogMessage (LogLevel::HOOKS, "GetRegionLanguage was called");
    lua_settop (a1, 0);
    int langFix = languageFix (language);
    lua_pushstring (a1, languageStr (langFix));
    return 1;
}
std::thread::id *LoadMovieThreadId = nullptr;
FAST_HOOK_DYNAMIC (i64, GetCabinetLanguage, u64 *a1, i64 a2) {
    LogMessage (LogLevel::HOOKS, "GetCabinetLanguage was called");
    i64 result = originalGetCabinetLanguage.fastcall<i64> (a1, a2);
    if (language == 4 && (LoadMovieThreadId == nullptr || *LoadMovieThreadId != std::this_thread::get_id())) {
        std_assign ((void *)result, languageStr (2), 5);
    } else std_assign ((void *)result, languageStr (language), (language == 0 || language == 3) ? 3 : 5);
    return result;
}
FAST_HOOK_DYNAMIC (i64, GetLanguageType, i64 a1) {
    LogMessage (LogLevel::HOOKS, "GetLanguageType was called");
    lua_settop (a1, 0);
    lua_pushnumber (a1, languageFix (language));
    return 1;
}
void
Init () {
    switch (gameVersion) {
    default: case GameVersion::JPN00: case GameVersion::JPN08: case GameVersion::CHN00: break;
    case GameVersion::JPN39: {
        INSTALL_FAST_HOOK_DYNAMIC (RefPlayDataManager, ASLR (0x140024AC0));
        // Export methods for lua invoke
        INSTALL_FAST_HOOK_DYNAMIC (GetLanguage,        ASLR (0x1401D1930));
        INSTALL_FAST_HOOK_DYNAMIC (GetRegionLanguage,  ASLR (0x1401CE9B0));
        INSTALL_FAST_HOOK_DYNAMIC (GetCabinetLanguage, ASLR (0x14014DB80));
        INSTALL_FAST_HOOK_DYNAMIC (GetLanguageType,    ASLR (0x1401CECE0));
    } break;
    }
}
}

namespace Lumen {
FAST_HOOK_DYNAMIC (i64, LoadDemoMovie, i64 a1, i64 a2, i64 a3) {
    std::thread::id this_id = std::this_thread::get_id();
    Script::LoadMovieThreadId = &this_id;
    i64 result = originalLoadDemoMovie.fastcall<i64> (a1, a2, a3);
    Script::LoadMovieThreadId = nullptr;
    return result;
}
std::string onpCn = "textures/onpu_cn/onp_all.nutexb";
MID_HOOK_DYNAMIC (ChangeOnpFile, SafetyHookContext &ctx) {
    if (language != 4) return;
    ctx.rdx = (uintptr_t) onpCn.c_str ();
    ctx.r8 = 0x1F;
}
void
InjectLanguageType (uintptr_t language, uintptr_t &reg) {
    int langFix = languageFix ((int)language);
    reg = (uintptr_t)languageStr (langFix);
}

bool titleExistSeason = true, titleExistMaid = true, titleExistMaidCn = true;
std::string seasonNulm = "title_season.nulm", seasonNutexb = "title_" + season () + ".nutexb";
std::string maidNulmCn = "title_maid.nulm", maidNutexbCn = "title_maid.nutexb";
std::string maidNulm = "title_maid.nulm", maidNutexb = "title_maid.nutexb";
TestMode::Value *theme = TestMode::CreateValue (L"ModTheme");
void
Init () {
    switch (gameVersion) {
    default: break;
    case GameVersion::JPN00: break;
    case GameVersion::JPN08: break;
    case GameVersion::JPN39: {
        bool demoMovieExistAll = true;
        const char *movieToCheck[]{"movie\\attractdemo_cn_cn.wmv", "sound\\attractdemo_cn_cn.nus3bank"};
        for (int i = 0; i < 2; i++) {
            if (std::filesystem::exists (std::string ("..\\..\\Data\\x64\\") + movieToCheck[i])) continue;
            demoMovieExistAll = false;
        }
        if (demoMovieExistAll) {
            LogMessage (LogLevel::INFO, "Using Attractdemo patches (Detected attractdemo_cn files)");
            INSTALL_FAST_HOOK_DYNAMIC (LoadDemoMovie, ASLR (0x1404313F0));
        }

        bool onpCnExist = false;
        if (std::filesystem::exists (std::string ("..\\..\\Data\\x64\\textures\\onpu_cn\\onp_all.nutexb"))) onpCnExist = true;
        if (onpCnExist) {
            LogMessage (LogLevel::INFO, "Using Onp patches (Detected onpu_cn files)");
            INSTALL_MID_HOOK_DYNAMIC (ChangeOnpFile, ASLR (0x140134D16));
        }
        language_patch.push_back (safetyhook::create_mid (ASLR (0x140134E22), [](SafetyHookContext &ctx){
            if (ctx.rcx == 4) *(int *)(ctx.r15 + 0x3100) = 2;
            else *(int *)(ctx.r15 + 0x3100) = (int)ctx.rcx;
            ctx.rip = ASLR (0x140134E29);
        }));

        language_patch.push_back (safetyhook::create_mid (ASLR (0x1400E8791), [](SafetyHookContext &ctx)
        { InjectLanguageType (ctx.rax, ctx.rdx); ctx.rip = ASLR (0x1400E8796); }));     // HitEffectGetLanguageType
        language_patch.push_back (safetyhook::create_mid (ASLR (0x14011F8E4), [](SafetyHookContext &ctx)
        { InjectLanguageType (ctx.rax, ctx.rdx); ctx.rip = ASLR (0x14011F8E9); }));     // KusudamaGetLanguageType
        language_patch.push_back (safetyhook::create_mid (ASLR (0x140122404), [](SafetyHookContext &ctx)
        { InjectLanguageType (ctx.rax, ctx.rdx); ctx.rip = ASLR (0x14012240C); }));     // NoteJumpGetLanguageType
        language_patch.push_back (safetyhook::create_mid (ASLR (0x14012425E), [](SafetyHookContext &ctx)
        { InjectLanguageType (ctx.rax, ctx.rdx); ctx.rip = ASLR (0x140124263); }));     // RendaEffectGetLanguageType
        language_patch.push_back (safetyhook::create_mid (ASLR (0x140129450), [](SafetyHookContext &ctx)
        { InjectLanguageType (ctx.rax, ctx.rdx); ctx.rip = ASLR (0x140129455); }));     // RendaEffectGetLanguageType
        language_patch.push_back (safetyhook::create_mid (ASLR (0x14012B8C8), [](SafetyHookContext &ctx)
        { InjectLanguageType (ctx.rax, ctx.rdx); ctx.rip = ASLR (0x14012B8D0); }));     // ScoreGetLanguageType
        LayeredFs::RegisterBefore ([&] (const std::string &originalFileName, const std::string &currentFileName) -> std::string {
            if (language != 4 || currentFileName.starts_with ("F:\\lumen\\") || currentFileName.find ("\\lumen\\") == std::string::npos) return "";
            std::string fileName = std::string (currentFileName);
            fileName             = replace (fileName, "\\lumen\\", "\\lumen_cn\\");
            if (std::filesystem::exists (fileName)) return fileName;
            return "";
        });

        std::string base[]   = {"..\\..\\Data\\x64\\lumen\\000_default\\attract\\", "..\\..\\Data_mods\\x64\\lumen\\000_default\\attract\\"};
        std::string baseCn[] = {"..\\..\\Data\\x64\\lumen_cn\\000_default\\attract\\", "..\\..\\Data_mods\\x64\\lumen_cn\\000_default\\attract\\"};
        for (std::string prefix : baseCn) {
            if (!seasonNulm.starts_with ("..\\") && std::filesystem::exists (prefix + seasonNulm)) seasonNulm = prefix + seasonNulm;
            if (!seasonNutexb.starts_with ("..\\") && std::filesystem::exists (prefix + seasonNutexb)) seasonNutexb = prefix + seasonNutexb;
        }
        if (!seasonNulm.starts_with ("..\\") || !seasonNutexb.starts_with ("..\\")) titleExistSeason = false;
        for (std::string prefix : baseCn) {
            if (!maidNulmCn.starts_with ("..\\") && std::filesystem::exists (prefix + maidNulmCn)) maidNulmCn = prefix + maidNulmCn;
            if (!maidNutexbCn.starts_with ("..\\") && std::filesystem::exists (prefix + maidNutexbCn)) maidNutexbCn = prefix + maidNutexbCn;
        }
        if (!maidNulmCn.starts_with ("..\\") || !maidNutexbCn.starts_with ("..\\")) titleExistMaidCn = false;
        for (std::string prefix : base) {
            if (!maidNulm.starts_with ("..\\") && std::filesystem::exists (prefix + maidNulm)) maidNulm = prefix + maidNulm;
            if (!maidNutexb.starts_with ("..\\") && std::filesystem::exists (prefix + maidNutexb)) maidNutexb = prefix + maidNutexb;
        }
        if (!maidNulm.starts_with ("..\\") || !maidNutexb.starts_with ("..\\")) titleExistMaid = false;

        if (titleExistSeason || (titleExistMaid && titleExistMaidCn)) {
            TestMode::RegisterItem (
                L"<select-item label=\"THEME\" param-offset-x=\"35\" replace-text=\"0:DEFAULT, 1:SEASON(CHN), "
                L"2:MAID\" group=\"Setting\" id=\"ModTheme\" max=\"2\" min=\"0\" default=\"0\"/>",
                [&](){
                    LayeredFs::RegisterBefore ([&] (const std::string &originalFileName, const std::string &currentFileName) -> std::string {
                        if (currentFileName.starts_with ("F:\\lumen\\") || currentFileName.find ("title") == std::string::npos) return "";
                        if (currentFileName.ends_with ("\\title.nulm")) {
                            if (language == 4 && theme->Read () == 1 && titleExistSeason) return seasonNulm;
                            if (language == 4 && theme->Read () == 2 && titleExistMaidCn) return maidNulmCn;
                            if (theme->Read () == 2 && titleExistMaid) return maidNulm;
                        } else if (currentFileName.ends_with ("\\title.nutexb")) {
                            LogMessage (LogLevel::DEBUG, "nutexb");
                            if (language == 4 && theme->Read () == 1 && titleExistSeason) return seasonNutexb;
                            if (language == 4 && theme->Read () == 2 && titleExistMaidCn) return maidNutexbCn;
                            if (theme->Read () == 2 && titleExistMaid) return maidNutexb;
                        }
                        return "";
                    });
                    LayeredFs::RegisterBefore ([&] (const std::string &originalFileName, const std::string &currentFileName) -> std::string {
                        if (theme->Read () != 2 || currentFileName.starts_with ("F:\\lumen\\") || currentFileName.find ("\\lumen\\") == std::string::npos) return "";
                        std::string fileName = std::string (currentFileName);
                        fileName             = replace (fileName, "\\lumen\\", "\\lumen_maid\\");
                        if (std::filesystem::exists (fileName)) return fileName;
                        return "";
                    });
                });
        }
    } break;
    case GameVersion::CHN00: break;
    }
}
}

namespace Voice {
std::map<std::string, bool> voiceCnExist;
std::map<std::string, int> nus3bankMap;
bool enableSwitchVoice = false;
int nus3bankIdCounter = 0;
std::mutex nus3bankMtx;

int
get_bank_id (const std::string &bankName) {
    if (!nus3bankMap.contains (bankName)) {
        nus3bankMap[bankName] = nus3bankIdCounter;
        nus3bankIdCounter++;
    }
    LogMessage (LogLevel::DEBUG, "LoadBank {} id={}", bankName, nus3bankMap[bankName]);
    return nus3bankMap[bankName];
}

void
check_voice_tail (const std::string &bankName, u8 *pBinfBlock, std::map<std::string, bool> &voiceExist, const std::string &tail) {
    // check if any voice_xxx.nus3bank has xxx_cn audio inside while loading
    if (bankName.starts_with ("voice_")) {
        const int binfLength = *reinterpret_cast<int *> (pBinfBlock + 4);
        u8 *pGrpBlock        = pBinfBlock + 8 + binfLength;
        const int grpLength  = *reinterpret_cast<int *> (pGrpBlock + 4);
        u8 *pDtonBlock       = pGrpBlock + 8 + grpLength;
        const int dtonLength = *reinterpret_cast<int *> (pDtonBlock + 4);
        u8 *pToneBlock       = pDtonBlock + 8 + dtonLength;
        const int toneSize   = *reinterpret_cast<int *> (pToneBlock + 8);
        u8 *pToneBase        = pToneBlock + 12;
        for (int i = 0; i < toneSize; i++) {
            if (*reinterpret_cast<int *> (pToneBase + i * 8 + 4) <= 0x0C) continue; // skip empty space
            u8 *currToneBase = pToneBase + *reinterpret_cast<int *> (pToneBase + i * 8);
            int titleOffset  = -1;
            switch (*currToneBase) {
            case 0xFF: titleOffset = 9; break; // audio mark
            case 0x7F: titleOffset = 5; break; // randomizer mark
            default: continue;                 // unknown mark skip
            }
            if (titleOffset > 0) {
                std::string title (reinterpret_cast<char *> (currToneBase + titleOffset));
                if (title.ends_with (tail)) {
                    if (!voiceExist.contains (bankName) || !voiceExist[bankName]) {
                        voiceExist[bankName] = true;
                        enableSwitchVoice = true;
                    }
                    return;
                }
            }
        }
        if (!voiceExist.contains (bankName) || voiceExist[bankName]) voiceExist[bankName] = false;
    }
}

void
checkVoiceFile (const char *fileName) {
    withFile (fileName, [](u8 *bankData){
        if (bankData[0] == 'N' && bankData[1] == 'U' && bankData[2] == 'S' && bankData[3] == '3') {
            const int tocLength  = *reinterpret_cast<int *> (bankData + 16);
            u8 *pPropBlock       = bankData + 20 + tocLength;
            const int propLength = *reinterpret_cast<int *> (pPropBlock + 4);
            u8 *pBinfBlock       = pPropBlock + 8 + propLength;
            const std::string bankName (reinterpret_cast<char *> (pBinfBlock + 0x11));
            check_voice_tail (bankName, pBinfBlock, voiceCnExist, "_cn");
        }
    });
}

MID_HOOK_DYNAMIC (GenNus3bankId, SafetyHookContext &ctx) {
    LogMessage (LogLevel::HOOKS, "GenNus3bankId was called");
    std::lock_guard lock (nus3bankMtx);
    if (reinterpret_cast<u8 **> (ctx.rcx + 8) != nullptr) {
        u8 *pNus3bankFile = *reinterpret_cast<u8 **> (ctx.rcx + 8);
        if (pNus3bankFile[0] == 'N' && pNus3bankFile[1] == 'U' && pNus3bankFile[2] == 'S' && pNus3bankFile[3] == '3') {
            const int tocLength  = *reinterpret_cast<int *> (pNus3bankFile + 16);
            u8 *pPropBlock       = pNus3bankFile + 20 + tocLength;
            const int propLength = *reinterpret_cast<int *> (pPropBlock + 4);
            u8 *pBinfBlock       = pPropBlock + 8 + propLength;
            const std::string bankName (reinterpret_cast<char *> (pBinfBlock + 0x11));
            ctx.rax = get_bank_id (bankName);
        }
    }
}

std::string
FixToneName (const std::string &bankName, std::string toneName, int voiceLang) {
    if (voiceLang == 1) {
        if (voiceCnExist.contains (bankName) && voiceCnExist[bankName]) return toneName + "_cn";
    }
    return toneName;
}

size_t commonSize = 0;
TestMode::Value *voiceLanguage = TestMode::CreateValue (L"VoiceLanguageItem");
FAST_HOOK_DYNAMIC (i64, PlaySoundMain, i64 a1) {
    LogMessage (LogLevel::HOOKS, "PlaySoundMain was called");
    int lang = voiceLanguage->Read ();
    if (lang > 0) {
        const std::string bankName (lua_tolstring (a1, -3, &commonSize));
        if (bankName[0] == 'v') {
            lua_pushstring (a1, FixToneName (bankName, lua_tolstring (a1, -2, &commonSize), lang).c_str ());
            lua_replace (a1, -3);
        }
    }
    return originalPlaySoundMain.fastcall<i64> (a1);
}

FAST_HOOK_DYNAMIC (i64, PlaySoundMulti, i64 a1) {
    LogMessage (LogLevel::HOOKS, "PlaySoundMulti was called");
    int lang = voiceLanguage->Read ();
    if (lang > 0) {
        const std::string bankName (const_cast<char *> (lua_tolstring (a1, -3, &commonSize)));
        if (bankName[0] == 'v') {
            lua_pushstring (a1, FixToneName (bankName, lua_tolstring (a1, -2, &commonSize), lang).c_str ());
            lua_replace (a1, -3);
        }
    }
    return originalPlaySoundMulti.fastcall<i64> (a1);
}

u64 *
FixToneNameEnso (u64 *Src, const std::string &bankName, int voiceLang) {
    if (voiceLang == 1) {
        if (voiceCnExist.contains (bankName) && voiceCnExist[bankName]) Src = (u64 *)std_append (Src, "_cn", 3);
    }
    return Src;
}

FAST_HOOK_DYNAMIC (bool, PlaySoundEnso, u64 *a1, u64 *a2, i64 a3) {
    LogMessage (LogLevel::HOOKS, "PlaySoundEnso was called");
    int lang = voiceLanguage->Read ();
    if (lang > 0) {
        const std::string bankName = a1[3] > 0x10 ? std::string (*reinterpret_cast<char **> (a1)) : std::string (reinterpret_cast<char *> (a1));
        if (bankName[0] == 'v') a2 = FixToneNameEnso (a2, bankName, lang);
    }
    return originalPlaySoundEnso.fastcall<bool> (a1, a2, a3);
}

FAST_HOOK_DYNAMIC (bool, PlaySoundSpecial, u64 *a1, u64 *a2) {
    LogMessage (LogLevel::HOOKS, "PlaySoundSpecial was called");
    int lang = voiceLanguage->Read ();
    if (lang > 0) {
        const std::string bankName = a1[3] > 0x10 ? std::string (*reinterpret_cast<char **> (a1)) : std::string (reinterpret_cast<char *> (a1));
        if (bankName[0] == 'v') a2 = FixToneNameEnso (a2, bankName, lang);
    }
    return originalPlaySoundSpecial.fastcall<bool> (a1, a2);
}

void
Init () {
    switch (gameVersion) {
    default: case GameVersion::JPN00: case GameVersion::JPN08: case GameVersion::CHN00: break;
    case GameVersion::JPN39: {
        INSTALL_MID_HOOK_DYNAMIC (GenNus3bankId, ASLR (0x1407B97BD));
        std::vector<std::string> voiceFiles = {
            "voice_aienso_v12g.nus3bank", "voice_airesult_v12g.nus3bank", "voice_aistory_v12g.nus3bank",
            "voice_aoharu_v1238.nus3bank", "voice_common_v12a.nus3bank", "voice_daniodai_v12e.nus3bank",
            "voice_daniresult_v12e.nus3bank", "voice_enso_v12a.nus3bank", "voice_entry_v12a.nus3bank",
            "voice_howtoplay_v12a.nus3bank", "voice_result_v12a.nus3bank", "voice_senkuoku_v12a.nus3bank"
        };
        for (std::string voiceFile : voiceFiles) checkVoiceFile (("..\\..\\Data\\x64\\sound\\" + voiceFile).c_str ());
        if (enableSwitchVoice) {
            LogMessage (LogLevel::INFO, "Using Voice patches (Detected voice files)");
            TestMode::RegisterItemAfter(
                L"/root/menu[@id='OthersMenu']/layout[@type='Center']/select-item[@id='LanguageItem']",
                L"<select-item label=\"VOICE\" param-offset-x=\"35\" replace-text=\"0:JPN, 1:CHN\" group=\"Setting\" "
                L"id=\"VoiceLanguageItem\" max=\"1\" min=\"0\" default=\"0\"/>",
                [&](){
                    INSTALL_FAST_HOOK_DYNAMIC (PlaySoundMain, ASLR (0x1404C6DC0));
                    INSTALL_FAST_HOOK_DYNAMIC (PlaySoundMulti, ASLR (0x1404C6D60));
                    INSTALL_FAST_HOOK_DYNAMIC (PlaySoundEnso, ASLR (0x1404ED590));
                    INSTALL_FAST_HOOK_DYNAMIC (PlaySoundSpecial, ASLR (0x1404ED230));
                }
            );
        }
    } break;
    }
}
}

boolean
CnFontPatches () {
    return cnFontPatches;
}

void
Init () {
    switch (gameVersion) {
    default: break;
    case GameVersion::JPN00: break;
    case GameVersion::JPN08: break;
    case GameVersion::JPN39: {
        languageFix  = [](int lang){ return lang == 4 ? 2 : lang; };
        taiko_string = (Taiko_string)ASLR (0x1400203B0);
        taiko_assign = (Taiko_assign)ASLR (0x140020850);
        std_assign   = (Taiko_assign)ASLR (0x1400209E0);
        std_append   = (Taiko_assign)ASLR (0x140028DA0);
        Language::Font::Init ();
        Language::Datatable::Init ();
        Language::Script::Init ();
        Language::Lumen::Init ();
        Language::Voice::Init ();
    } break;
    case GameVersion::CHN00: break;
    }
}
}