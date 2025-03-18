#include "constants.h"
#include "helpers.h"
#include "patches.h"
#include "banner.h"
#include <exception>
#include <format>
#include <queue>
#include <map>

extern GameVersion gameVersion;

bool chsPatch = false;
#define MIN(a, b) ((a)<(b)?(a):(b))

namespace patches::TestMode {
bool hookInstallFinished = false;
std::vector<std::function<void ()>> hooks = {};
std::thread *hookInstall;
std::mutex hooksMutex;
std::condition_variable hooksCV;
std::vector<SafetyHookMid> patches = {};
std::vector<Value *> values = {};
std::map<std::wstring, Value *> valueMap = {};

std::string ver = Banner::Version::instance ().getVer ();

std::wstring
mergeCondition (std::wstring original, std::wstring addition, std::wstring value) {
    size_t pos = original.find (L"/");
    if (pos < 0) return original;
    std::wstring b = original.substr (0, pos - 1);
    if (b == L"False") {
        if (value.starts_with (L"!")) value = value.substr (1, value.size () - 1);
        else value = L"!" + value;
    }

    if (original.ends_with(L")")) return replace (original, L")", std::format (L" or {}:{})", addition, value));
    else return std::format (L"{}/({} or {}:{})", b, original.substr (pos + 1, original.size () - pos - 1), addition, value);
}
void
addConditionValue (pugi::xml_node *node, std::wstring key, std::wstring addition, std::wstring value) {
    pugi::xml_attribute attr = node->attribute (key.c_str ());
    if (attr != nullptr) {
        attr.set_value (mergeCondition(attr.value (), addition, value).c_str ());
    } else node->append_attribute (key.c_str ()) = std::format(L"True/{}:{}", addition, value).c_str ();
}
class RegisteredItem : public Applicable {
public:
    std::wstring selectItem;
    std::function<void ()> registerInit;
    pugi::xml_node temp;
    RegisteredItem (const std::wstring &selectItem, const std::function<void ()> &initMethod) {
        this->selectItem   = selectItem;
        this->registerInit = initMethod;
    }
    pugi::xml_node *
    Apply (pugi::xml_node *doc, pugi::xml_node *node) override {
        pugi::xml_document itemDoc;
        std::wstring itemLine = L"<root>" + this->selectItem + L"</root>";
        bool success = itemDoc.load_string (itemLine.c_str ());
        if (!success) {
            LogMessage (LogLevel::ERROR, L"Failed to parse item: {}\n", this->selectItem);
            itemDoc.load_string (L"<root><text-item label=\"@Color/Red;!!!ERROR APPLYING ITEM!!!@Color/Default;\"/></root>");
        } else {
            LogMessage (LogLevel::DEBUG, L"[TestMode] Add Item: {}", itemDoc.attribute(L"label").as_string ());
        }
        // if (success) AddHookInLoop (this->registerInit);
        std::wstring layoutName = L"layout";
        if (layoutName == node->name ()) {
            temp = node->append_copy (itemDoc.first_child ().first_child ());
            return &temp;
        } else {
            pugi::xml_node breakItem = node->parent ().insert_child_after (L"break-item", *node);
            temp = node->parent ().insert_copy_after (itemDoc.first_child ().first_child (), breakItem);
            return &temp;
        }
    }
};
class RegisteredMenu : public Menu {
public:
    std::wstring menuName;
    std::wstring menuId;
    std::vector<Applicable *> items;
    pugi::xml_node temp;
    RegisteredMenu (const std::wstring &menuName, const std::wstring &menuId, std::vector<Applicable *> items = {}) {
        this->menuName = menuName;
        this->menuId = menuId;
        this->items = items;
    }
    void
    RegisterItem (Applicable *item) override {
        this->items.push_back (item);
    }
    pugi::xml_node *
    Apply (pugi::xml_node *doc, pugi::xml_node *node) override {
        size_t size = items.size ();
        if (size == 0) return node;
        size_t maxPage = size <= 14 ? 1 : (size / 13 + (size % 13 > 0));
        LogMessage (LogLevel::DEBUG, L"[TestMode] Add Menu {} (items={}, page={})", menuName, size, maxPage);
        std::wstring menuName = L"menu";
        pugi::xml_node parentNode = node->parent ();
        while (parentNode.name() != menuName) parentNode = parentNode.parent ();
        std::wstring parentId = parentNode.attribute (L"id").value ();

        pugi::xml_document menuItem;
        menuItem.load_string (std::format (L"<root><menu-item label=\"{}\" menu=\"{}\"/></root>", this->menuName, this->menuId).c_str ());
        std::wstring layoutName = L"layout";
        if (layoutName == node->name ()) {
            temp = node->append_copy (menuItem.first_child ().first_child ());
        } else {
            pugi::xml_node menuBreakItem = node->parent ().insert_child_after (L"break-item", *node);
            temp = node->parent ().insert_copy_after (menuItem.first_child ().first_child (), menuBreakItem);
        }

        pugi::xml_document menu;
        menu.load_string (std::format (
            L"<root><menu id=\"{}\">"
            L"<layout type=\"Header\"><break-item/><text-item label=\"{}\"/><break-item/>"
            L"</layout><layout type=\"Center\" padding-x=\"23\"></layout>"
            L"<layout type=\"Footer\"><data-item data=\"FooterTextData\" param-offset-x=\"-20\"/></layout>"
            L"</menu></root>",
            this->menuId, this->menuName
        ).c_str ());
        pugi::xpath_query query = pugi::xpath_query (L"/root/menu/layout[@type='Center']");
        pugi::xml_node passing = menu.select_node (query).node ();
        std::vector<pugi::xml_node> refs = {};

        for (size_t page = 0; page < maxPage; page ++) {
            // LogMessage (LogLevel::DEBUG, L"[TestMenu] menu page ({}/{})", page + 1, maxPage);
            pugi::xml_node ref = passing;
            size_t maxIndex = MIN(size - page * 13, 13);
            for (size_t index = 0; index < maxIndex; index ++) {
                // LogMessage (LogLevel::DEBUG, "Render testmode menu page ({}/{}) item ({}/{})", page + 1, maxPage, index + 1, maxIndex);
                if (index >= refs.size ()) {
                    passing = *(items[page * 13 + index]->Apply (doc, &passing));
                    if (size > 14) {
                        addConditionValue (&passing, L"invisible", std::format (L"{}Pagenation", this->menuId), std::format(L"!{}", page + 1));
                        addConditionValue (&passing, L"disable", std::format (L"{}Pagenation", this->menuId), std::format(L"!{}", page + 1));
                    }
                    refs.push_back (passing);
                } else {
                    pugi::xml_node ref = refs[index];
                    ref = *(items[page * 13 + index]->Apply (doc, &ref));
                    addConditionValue (&ref, L"invisible", std::format (L"{}Pagenation", this->menuId), std::format(L"!{}", page + 1));
                    addConditionValue (&ref, L"disable", std::format (L"{}Pagenation", this->menuId), std::format(L"!{}", page + 1));
                    ref.parent ().remove_child (ref.previous_sibling ());
                    refs[index] = ref;
                }
            }
            if (size == 14) passing = *(items[13]->Apply (doc, &passing));
        }
        passing = passing.parent ().last_child ();
        // LogMessage (LogLevel::DEBUG, "Begin render testmode footer");
        size_t maxIndex = size == 14 ? 14 : MIN(size, 13);
        for (size_t i = 0; i < 15 - maxIndex; i++) passing = passing.parent ().insert_child_after (L"break-item", passing);
        if (maxPage > 1) {
            pugi::xml_document pagenation;
            pagenation.load_string (std::format (L"<root><select-item label=\"PAGE\" param-offset-x=\"23\" prefix=\"(\" suffix=\"/{})\" param-color=\"Color/Yellow\" id=\"{}Pagenation\" min=\"1\" max=\"{}\" default=\"1\"/></root>", maxPage, this->menuId, maxPage).c_str ());
            passing = passing.parent ().insert_copy_after (pagenation.first_child ().first_child (), passing);
        }

        passing = passing.parent ().insert_child_after (L"break-item", passing);
        pugi::xml_document exit;
        exit.load_string (std::format (L"<root><menu-item label=\"EXIT\" menu=\"{}\"/></root>", parentId).c_str ());
        passing = passing.parent ().insert_copy_after (exit.first_child ().first_child (), passing);

        // LogMessage (LogLevel::DEBUG, L"Insert testmode menu menuId: {}, fromId: {}", this->menuId, parentId);
        pugi::xml_node root = doc->first_child ();
        root.insert_copy_after (menu.first_child ().first_child (), root.first_child ());
        return &temp;
    }
};
class RegisteredSingleItem : Applicable {
public:
    std::wstring query;
    Applicable *item;
    RegisteredSingleItem (const std::wstring query, Applicable *item) {
        this->query = query;
        this->item = item;
    }
    pugi::xml_node *
    Apply (pugi::xml_node *doc, pugi::xml_node *node) override {
        pugi::xpath_query insertQuery = pugi::xpath_query (this->query.c_str ());
        try {
            if (pugi::xml_node insertNode = doc->select_node (insertQuery).node ()) return this->item->Apply (doc, &insertNode);
        } catch ([[maybe_unused]] std::exception &e) { LogMessage (LogLevel::ERROR, L"Failed to find node by xpath: {}\n", this->query); }
        return nullptr;
    }
};
class RegisteredModify : Applicable {
public:
    std::wstring query;
    std::function<void (pugi::xml_node &)> nodeModify;
    std::function<void ()> registerInit;
    RegisteredModify (const std::wstring &query, const std::function<void (pugi::xml_node &)> &nodeModify, const std::function<void ()> &initMethod) {
        this->query        = query;
        this->nodeModify   = nodeModify;
        this->registerInit = initMethod;
    }
    pugi::xml_node *
    Apply (pugi::xml_node *doc, pugi::xml_node *node) override {
        pugi::xpath_query modifyQuery = pugi::xpath_query (this->query.c_str ());
        try {
            if (pugi::xml_node modifyNode = node->select_node (modifyQuery).node ()) {
                this->nodeModify (modifyNode);
                // AddHookInLoop (this->registerInit);
            }
        } catch ([[maybe_unused]] std::exception &e) { LogMessage (LogLevel::ERROR, L"Failed to find node by xpath: {}\n", this->query); }
        return nullptr;
    }
};
class TestModeValue : Value {
public:
    std::wstring key;
    int value;
    boolean reset;
    TestModeValue (std::wstring key) {
        this->key = key;
        this->value = -1;
        this->reset = true;
    }
    int
    Read (int value) override {
        if (reset) {
            this->value = ReadTestModeValue (key.c_str ());
            if (this->value != -1) {
                LogMessage (LogLevel::DEBUG, L"TestMode Value({}) got val: {}", this->key, this->value);
                reset = false;
            }
        }
        if (this->value == -1) return value;
        else return this->value;
    }
    void
    Write (int value) override {
        SetTestModeValue (this->key.c_str (), value);
        this->reset = true;
    }
    void
    Reset () override {
        this->reset = true;
    }
};
Menu *modManager = new RegisteredMenu (L"MOD MANAGER", L"ModManagerMenu");
std::vector<RegisteredSingleItem *> registeredSingleItems = {};
std::vector<RegisteredModify *> registeredModifies        = {};
std::wstring moddedInitial                                = L"";
std::wstring modded                                       = L"";

std::wstring usingDeviceInitialize = L"";
std::wstring moddedDeviceInitialize = L"DeviceInitialize_mod.xml";
std::wstring usingTestMode = L"";
std::wstring moddedTestMode = L"TestMode_mod.xml";

u64 appAccessor             = 0;
RefTestModeMain refTestMode = nullptr;

FAST_HOOK_DYNAMIC (char, SceneTestModeLoading, u64 a1, u64 a2, u64 a3) {
    char result = originalSceneTestModeLoading.fastcall<char> (a1, a2, a3);
    return result;
}

FAST_HOOK_DYNAMIC (char, SceneTestModeFinalize, u64 a1, u64 a2, u64 a3) {
    char result = originalSceneTestModeFinalize.fastcall<char> (a1, a2, a3);
    std::thread ([](){for (auto value : values) { value->Reset (); value->Read (); }}).detach ();
    return result;
}

FAST_HOOK_DYNAMIC (void, SceneFirstInitialize, u64 a1, u64 a2, u64 a3) {
    std::thread ([](){for (auto value : values) { value->Reset (); value->Read (); }}).detach ();
    originalSceneFirstInitialize.fastcall<char *> (a1, a2, a3);
}

std::wstring
replaceFileName (std::wstring fullFileName, std::wstring usingLastFileName) {
    size_t lastPosition = fullFileName.find_last_of (L"/");
    if (lastPosition == -1) return usingLastFileName;
    else return fullFileName.substr(0, lastPosition + 1) + usingLastFileName;
}

FAST_HOOK_DYNAMIC (void, TestModeSetMenuHook, u64 testModeLibrary, const wchar_t *lFileName) {
    auto start = std::chrono::high_resolution_clock::now();
    const auto originalFileName = std::wstring (lFileName);
    LogMessage (LogLevel::DEBUG, L"Begin Loading {}", originalFileName);
    std::wstring fileName       = originalFileName;
    if (fileName.find (L"/DeviceInitialize") != -1) {
        if (moddedInitial == L"") {
            if (usingDeviceInitialize != L"") {
                fileName = replaceFileName (originalFileName, usingDeviceInitialize);
            } else fileName = originalFileName;
            LogMessage (LogLevel::DEBUG, L"final path: {}", fileName);
            if (pugi::xml_document doc; !doc.load_file (fileName.c_str ())) {
                LogMessage (LogLevel::ERROR, L"Loading DeviceInitialize structure failed! path: " + fileName);
                moddedInitial = originalFileName;
                fileName      = originalFileName;
            } else {
                const std::wstring modFileName = replaceFileName (originalFileName, moddedDeviceInitialize);
                const pugi::xpath_query dongleQuery = pugi::xpath_query (L"/root/menu[@id='TopMenu']/layout[@type='Center']/select-item[@id='DongleItem']");
                const pugi::xml_node dongleItem     = doc.select_node (dongleQuery).node ();
                pugi::xml_node talItem        = dongleItem.parent ().append_copy (dongleItem);
                talItem.attribute (L"label").set_value (L"TAIKOARCADELOADER");
                talItem.attribute (L"id").set_value (L"TaikoArcadeLoader");
                talItem.attribute (L"replace-text").set_value ((L"1:@Color/Lime;V" + std::wstring ({ver.begin (), ver.end ()})).c_str ());
                talItem.append_attribute (L"default") = L"1";
                dongleItem.parent ().append_child (L"break-item");

                [[maybe_unused]] auto ignored = doc.save_file (modFileName.c_str ());
                moddedInitial = modFileName;
                fileName      = modFileName;
            }
        } else fileName = moddedInitial;
    } else if (fileName.find (L"/TestMode") != -1) {
        if (modded == L"") {
            if (!((RegisteredMenu*)modManager)->items.empty () || registeredSingleItems.empty () || !registeredModifies.empty ()) {
                if (usingTestMode != L"") {
                    fileName = replaceFileName (originalFileName, usingTestMode);
                } else fileName = originalFileName;
                LogMessage (LogLevel::DEBUG, L"final path: {}", fileName);
                if (pugi::xml_document doc; !doc.load_file (fileName.c_str ())) {
                    LogMessage (LogLevel::ERROR, L"Loading TestMode structure failed! path: " + fileName);
                    modded   = originalFileName;
                    fileName = originalFileName;
                } else {
                    const std::wstring modFileName = replaceFileName (originalFileName, moddedTestMode);
                    if (!((RegisteredMenu*)modManager)->items.empty ()) {
                        LogMessage (LogLevel::DEBUG, "Begin generate Mod Manager menu");
                        pugi::xpath_query menuQuery
                            = pugi::xpath_query (L"/root/menu[@id='TopMenu']/layout[@type='Center']/menu-item[@menu='GameOptionsMenu']");
                        pugi::xml_node menuItem = doc.select_node (menuQuery).node ();
                        modManager->Apply (&doc, &menuItem);
                        LogMessage (LogLevel::DEBUG, "End generate Mod Manager menu");
                    }

                    if (!registeredSingleItems.empty()) {
                        LogMessage (LogLevel::DEBUG, "Begin Apply Single Items");
                        for (RegisteredSingleItem *item : registeredSingleItems) item->Apply (&doc, &doc);
                        LogMessage (LogLevel::DEBUG, "End Apply Single Items");
                    }

                    if (!registeredModifies.empty ()) {
                        LogMessage (LogLevel::DEBUG, "Begin Apply Modify Items");
                        for (RegisteredModify *modify : registeredModifies) modify->Apply (&doc, &doc);
                        LogMessage (LogLevel::DEBUG, "End Apply Modify Items");
                    }

                    [[maybe_unused]] auto _ = doc.save_file (modFileName.c_str ());
                    modded   = modFileName;
                    fileName = modFileName;
                }
            } else modded = fileName;
        } else fileName = modded;
    }

    std::chrono::duration<double> duration = std::chrono::high_resolution_clock::now() - start;
    LogMessage (LogLevel::DEBUG, L"TestModeLibrary load: {}, spend: {:.2f}ms", fileName, duration.count () * 1000);
    originalTestModeSetMenuHook.fastcall<void> (testModeLibrary, fileName.c_str ());
}

void
CommonModify () {
    // Default off Close time
    RegisterModify (
        L"/root/menu[@id='CloseTimeSettingMenu']/layout[@type='Center']/select-item[@id='ScheduleTypeItem']",
        [&] (const pugi::xml_node &node) { node.attribute (L"default").set_value (L"0"); }, [&] {});
}

void
LocalizationCHS () {
    TestMode::RegisterModify (
        L"/root/menu[@id='TopMenu']/layout[@type='Center']/menu-item[@menu='ModManagerMenu']",
        [&] (const pugi::xml_node &node) { node.attribute (L"label").set_value (L"模组管理"); }, [](){});
    TestMode::RegisterModify (
        L"/root/menu[@id='ModManagerMenu']/layout[@type='Header']/text-item",
        [&] (const pugi::xml_node &node) { node.attribute (L"label").set_value (L"模组管理"); }, [](){});
    TestMode::RegisterModify (
        L"/root/menu[@id='ModManagerMenu']/layout[@type='Center']/select-item[@id='ModUnlockSongs']",
        [&] (const pugi::xml_node &node) {
            node.attribute (L"label").set_value (L"解锁歌曲");
            node.attribute (L"replace-text").set_value (L"0:禁用, 1:启用, 2:强制");
        }, [](){});
    TestMode::RegisterModify (
        L"/root/menu[@id='ModManagerMenu']/layout[@type='Center']/select-item[@id='ModFreezeTimer']",
        [&] (const pugi::xml_node &node) {
            node.attribute (L"label").set_value (L"冻结计时器");
            node.attribute (L"replace-text").set_value (L"0:禁用, 1:启用");
        }, [](){});
    TestMode::RegisterModify (
        L"/root/menu[@id='ModManagerMenu']/layout[@type='Center']/select-item[@id='ModModeCollabo024']",
        [&] (const pugi::xml_node &node) {
            node.attribute (L"label").set_value (L"鬼灭之刃模式");
            node.attribute (L"replace-text").set_value (L"0:默认, 1:启用, 2:仅刷卡");
        }, [](){});
    TestMode::RegisterModify (
        L"/root/menu[@id='ModManagerMenu']/layout[@type='Center']/select-item[@id='ModModeCollabo025']",
        [&] (const pugi::xml_node &node) {
            node.attribute (L"label").set_value (L"航海王模式");
            node.attribute (L"replace-text").set_value (L"0:默认, 1:启用, 2:仅刷卡");
        }, [](){});
    TestMode::RegisterModify (
        L"/root/menu[@id='ModManagerMenu']/layout[@type='Center']/select-item[@id='ModModeCollabo026']",
        [&] (const pugi::xml_node &node) {
            node.attribute (L"label").set_value (L"ＡＩ粗品模式");
            node.attribute (L"replace-text").set_value (L"0:默认, 1:启用, 2:仅刷卡");
        }, [](){});
    TestMode::RegisterModify(
        L"/root/menu[@id='ModManagerMenu']/layout[@type='Center']/select-item[@id='ModModeAprilFool001']",
        [&](pugi::xml_node &node) {
            node.attribute(L"label").set_value(L"青春之达人模式");
            node.attribute(L"replace-text").set_value(L"0:默认, 1:启用, 2:仅刷卡");
        }, [](){});
    TestMode::RegisterModify (
        L"/root/menu[@id='ModManagerMenu']/layout[@type='Center']/select-item[@id='ModInstantResult']",
        [&] (const pugi::xml_node &node) {
            node.attribute (L"label").set_value (L"即时上传成绩");
            node.attribute (L"replace-text").set_value (L"0:禁用, 1:启用");
        }, [](){});
    TestMode::RegisterModify (
        L"/root/menu[@id='ModManagerMenu']/layout[@type='Center']/select-item[@id='ModTheme']",
        [&] (const pugi::xml_node &node) {
            node.attribute (L"label").set_value (L"界面主题");
            node.attribute (L"replace-text").set_value (L"0:默认, 1:四季(仅中文), 2:秋叶原女仆");
        }, [](){});
    TestMode::RegisterModify (
        L"/root/menu[@id='ModManagerMenu']/layout[@type='Center']/menu-item[@menu='TopMenu']",
        [&] (const pugi::xml_node &node) { node.attribute (L"label").set_value (L"离开"); }, [](){});
    TestMode::RegisterModify(
        L"/root/menu[@id='OthersMenu']/layout[@type='Center']/select-item[@id='AttractDemoItem']",
        [&](pugi::xml_node &node) {
            node.attribute(L"label").set_value(L"演示游玩影片");
            node.attribute(L"replace-text").set_value(L"0:关闭, 1:开启");
        }, [](){});
    TestMode::RegisterModify(
        L"/root/menu[@id='OthersMenu']/layout[@type='Center']/select-item[@id='LanguageItem']",
        [&](pugi::xml_node &node) {
            node.attribute(L"label").set_value(L"文本语言");
            const wchar_t *vals = node.attribute(L"replace-text").value ();
            std::wstring langaugeReplace = std::wstring (vals);
            langaugeReplace = replace (langaugeReplace, L"JPN", L"日语");
            langaugeReplace = replace (langaugeReplace, L"ENG", L"英语");
            langaugeReplace = replace (langaugeReplace, L"zh-tw", L"繁体中文");
            langaugeReplace = replace (langaugeReplace, L"KOR", L"韩语");
            langaugeReplace = replace (langaugeReplace, L"zh-cn", L"简体中文");
            node.attribute(L"replace-text").set_value(langaugeReplace.c_str ());
        }, [](){});
    TestMode::RegisterModify(
        L"/root/menu[@id='OthersMenu']/layout[@type='Center']/select-item[@id='VoiceLanguageItem']",
        [&](pugi::xml_node &node) {
            node.attribute(L"label").set_value(L"语音语言");
            node.attribute(L"replace-text").set_value(L"0:日语, 1:中文");
        }, [](){});
}

void
Init () {
    LogMessage (LogLevel::DEBUG, "Init TestMode patches");

    const u64 testModeLibrary = (u64)GetModuleHandle ("TestModeLibrary.dll");
    const u64 testModeSetMenu = testModeLibrary + 0x99D0;

    CommonModify ();
    for (auto method : hooks) method ();

    switch (gameVersion) {
    case GameVersion::JPN00: break;
    case GameVersion::JPN08: break;
    case GameVersion::JPN39: {
        INSTALL_FAST_HOOK_DYNAMIC (SceneTestModeLoading, ASLR (0x1404793D0));
        INSTALL_FAST_HOOK_DYNAMIC (SceneTestModeFinalize, ASLR (0x140479600));
        INSTALL_FAST_HOOK_DYNAMIC (SceneFirstInitialize, ASLR (0x1404574B0));
        if (Language::CnFontPatches () && std::filesystem::exists ("..\\..\\Data\\x64\\testmode\\DeviceInitialize_china.xml")) {
            LogMessage (LogLevel::INFO, L"Using DeviceInitialize: DeviceInitialize_china.xml");
            usingDeviceInitialize = L"DeviceInitialize_china.xml";
            patches.push_back (safetyhook::create_mid (ASLR (0x140465549), [](SafetyHookContext &ctx)
            { ctx.r8 = 2; ctx.rdx = (uintptr_t)"cn"; ctx.rip = ASLR (0x140465556); }));     // DeviceCheck = Loading Font
            patches.push_back (safetyhook::create_mid (ASLR (0x1404676E7), [](SafetyHookContext &ctx)
            { ctx.r8 = 2; ctx.rdx = (uintptr_t)"cn"; ctx.rip = ASLR (0x1404676F4); }));     // ??
            WRITE_MEMORY (ASLR (0x140CD1E40), wchar_t, L"加载中\0");
            WRITE_MEMORY (ASLR (0x140CD1E28), wchar_t, L"加载中.\0");
            WRITE_MEMORY (ASLR (0x140CD1E68), wchar_t, L"加载中..\0");
            WRITE_MEMORY (ASLR (0x140CD1E50), wchar_t, L"加载中...\0");
        }
        if (Language::CnFontPatches () && std::filesystem::exists ("..\\..\\Data\\x64\\testmode\\TestMode_china.xml")) {
            LogMessage (LogLevel::INFO, L"Using TestMode: TestMode_china.xml");
            usingTestMode = L"TestMode_china.xml";
            patches.push_back (safetyhook::create_mid (ASLR (0x14047C603), [](SafetyHookContext &ctx)
            { ctx.r8 = 2; ctx.rdx = (uintptr_t)"cn"; ctx.rip = ASLR (0x14047C610); }));     // TestMode = DeviceInitialize
            LocalizationCHS ();
        }
    } break;
    case GameVersion::CHN00: break;
    }

    INSTALL_FAST_HOOK_DYNAMIC (TestModeSetMenuHook, testModeSetMenu);
}

void
SetupAccessor (const u64 appAccessor, const RefTestModeMain refTestMode) {
    if (TestMode::appAccessor != appAccessor) {
        LogMessage (LogLevel::DEBUG, "TestMode setup!");
        TestMode::appAccessor = appAccessor;
        TestMode::refTestMode = refTestMode;
        // std::thread ([](){for (auto value : values) { value->Reset (); value->Read (); }}).detach ();
    }
}

int
ReadTestModeValue (const wchar_t *itemId) {
    if (appAccessor) {
        try {
            if (const u64 testModeMain = refTestMode (appAccessor)) {
                int value   = 0;
                u64 *reader = *reinterpret_cast<u64 **> (testModeMain + 16);
                (*reinterpret_cast<void (__fastcall **) (u64 *, const wchar_t *, int *)> (*reader + 256)) (reader, itemId, &value);
                return value;
            }
        } catch ([[maybe_unused]] std::exception &e) {
            LogMessage (LogLevel::ERROR, L"Reading Value({}) Error!", itemId);
        }
    }
    return -1;
}

void
SetTestModeValue (const wchar_t *itemId, int value) {
    if (appAccessor) {
        if (const u64 testModeMain = refTestMode (appAccessor)) {
            u64 *reader = *reinterpret_cast<u64 **> (testModeMain + 16);
            (*reinterpret_cast<void (__fastcall **) (u64 *, const wchar_t *, int)> (*reader + 176)) (reader, itemId, value);
        }
    }
}

Menu *
CreateMenu (const std::wstring &menuName, const std::wstring &menuId) {
    // LogMessage (LogLevel::DEBUG, L"Create MenuName: {} MenuId: {}", menuName, menuId);
    return new RegisteredMenu (menuName, menuId);
}

Value *
CreateValue (const std::wstring &key) {
    // LogMessage (LogLevel::DEBUG, L"Create TestMode Value key: {}", key);
    // if (valueMap.count (key)) return valueMap[key];
    Value *value = (Value *)(new TestModeValue (key));
    values.push_back ((Value *)value);
    // valueMap[key] = value;
    return value;
}

void
RegisterItem (const std::wstring &item, const std::function<void ()> &initMethod, Menu *menu) {
    // LogMessage (LogLevel::DEBUG, L"Register Item: {}", item);
    hooks.push_back (initMethod);
    menu->RegisterItem (new RegisteredItem (item, [](){}));
}

void
RegisterItem (const std::wstring &item, const std::function<void ()> &initMethod) {
    // LogMessage (LogLevel::DEBUG, L"Register Item: {}", item);
    hooks.push_back (initMethod);
    modManager->RegisterItem (new RegisteredItem (item, [](){}));
}

void
RegisterItem (const std::wstring &item, Menu *menu) {
    // LogMessage (LogLevel::DEBUG, L"Register Item: {}", item);
    menu->RegisterItem (new RegisteredItem (item, [](){}));
}

void
RegisterItem (const std::wstring &item) {
    // LogMessage (LogLevel::DEBUG, L"Register Item: {}", item);
    modManager->RegisterItem (new RegisteredItem (item, [](){}));
}

void
RegisterItem (Applicable *item, Menu *menu) {
    // LogMessage (LogLevel::DEBUG, L"Register Item");
    menu->RegisterItem (item);
}

void
RegisterItem (Applicable *item) {
    // LogMessage (LogLevel::DEBUG, L"Register Item");
    modManager->RegisterItem (item);
}

void
RegisterItemAfter (const std::wstring &query, const std::wstring &item, const std::function<void()> &initMethod) {
    // LogMessage (LogLevel::DEBUG, L"Register Query: {} Item: {}", query, item);
    hooks.push_back (initMethod);
    registeredSingleItems.push_back (new RegisteredSingleItem (query, new RegisteredItem (item, [](){})));
}

void
RegisterItemAfter (const std::wstring &query, const std::wstring &item) {
    // LogMessage (LogLevel::DEBUG, L"Register Query: {} Item: {}", query, item);
    registeredSingleItems.push_back (new RegisteredSingleItem (query, new RegisteredItem (item, [](){})));
}

void
RegisterItemAfter (const std::wstring &query, Applicable *item) {
    // LogMessage (LogLevel::DEBUG, L"Register Query: {} Item: ptr", query);
    registeredSingleItems.push_back (new RegisteredSingleItem (query, item));
}

void
RegisterModify (const std::wstring &query, const std::function<void (pugi::xml_node &)> &nodeModify, const std::function<void ()> &initMethod) {
    // LogMessage (LogLevel::DEBUG, L"Register Modify: {}", query);
    hooks.push_back (initMethod);
    registeredModifies.push_back (new RegisteredModify (query, nodeModify, [](){}));
}

void
RegisterModify (const std::wstring &query, const std::function<void (pugi::xml_node &)> &nodeModify) {
    // LogMessage (LogLevel::DEBUG, L"Register Modify: {}", query);
    registeredModifies.push_back (new RegisteredModify (query, nodeModify, [](){}));
}

void
RegisterHook (const std::function<void()> &initMethod) {
    // LogMessage (LogLevel::DEBUG, L"Register Hook");
    hooks.push_back (initMethod);
}

void
Append (const pugi::xml_node &node, const wchar_t *attr, const std::wstring &append) {
    pugi::xml_attribute attribute = node.attribute (attr);
    const std::wstring attrValue        = std::wstring (attribute.value ()) + append;
    attribute.set_value (attrValue.c_str ());
}
}